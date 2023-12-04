#include <string.h>
#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "filesys/file.h"

#include "userprog/syscall.h"
extern struct lock lock_file;

static unsigned vm_hash_func(const struct hash_elem *, void *UNUSED);
static bool vm_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED);
static void vm_destroy_func(struct hash_elem *, void *UNUSED);

static struct list_elem *get_next_lru_clock()
{
    if (list_empty(&lru_list))
    {
        return NULL;
    }

    if (lru_clock && lru_clock != list_end(&lru_list))
    {   
        lru_clock = list_next(lru_clock);   
    } 

    if (!lru_clock || lru_clock == list_end(&lru_list))
    {   
        return (lru_clock = list_begin(&lru_list));   
    } 
    else
    {
        return lru_clock;
    }

}

void vm_init(struct hash *vm)
{
    hash_init(vm, vm_hash_func, vm_less_func, NULL);
}

void vm_destroy(struct hash *vm)
{
    hash_destroy(vm, vm_destroy_func);
}

struct vm_entry *find_vme(void *vaddr)
{
    struct vm_entry vme;
    struct hash *vm = &thread_current()->vm;
    struct hash_elem *elem;
    vme.vaddr = pg_round_down(vaddr);
    if ((elem = hash_find(vm, &vme.elem))) 
    {
        return hash_entry(elem, struct vm_entry, elem);
    }
    else
    {
        return NULL;
    }
}

bool insert_vme(struct hash *vm, struct vm_entry *vme)
{
    if (!hash_insert(vm, &vme->elem))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool delete_vme(struct hash *vm, struct vm_entry *vme)
{
    if (!hash_delete(vm, &vme->elem))
    {
        return false;
    }
    else
    {
        lock_acquire(&lru_lock);
        free_page(pagedir_get_page(thread_current()->pagedir, vme->vaddr));
        lock_release(&lru_lock);
        swap_free(vme->swap_slot);
        free(vme);
        return true;
    }
}

static unsigned
vm_hash_func(const struct hash_elem *e, void *aux UNUSED)
{
    struct vm_entry *vme = hash_entry(e, struct vm_entry, elem);
    return hash_int((int)vme->vaddr);
}

static bool
vm_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
    void *vaddr_a = hash_entry(a, struct vm_entry, elem)->vaddr;
    void *vaddr_b = hash_entry(b, struct vm_entry, elem)->vaddr;
    if(vaddr_a < vaddr_b)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void
vm_destroy_func(struct hash_elem *e, void *aux UNUSED)
{
    struct vm_entry *vme = hash_entry(e, struct vm_entry, elem);
    if(vme->is_loaded)
    {
        lock_acquire(&lru_lock);
        free_page(pagedir_get_page(thread_current()->pagedir, vme->vaddr));
        lock_release(&lru_lock);
    }
    else
    {
        if(vme->_pin)
        {
            swap_free(vme->swap_slot);
        }
    }
    free(vme);
}

bool load_file(void *kaddr, struct vm_entry *vme)
{
    int read_byte = file_read_at(vme->file, kaddr, vme->read_bytes, vme->offset);

    if (read_byte != (int)vme->read_bytes)
        return false;
    memset(kaddr + vme->read_bytes, 0, vme->zero_bytes);

    return true;
}

void* try_to_free_pages(enum palloc_flags flags)
{
    struct page *page;
    struct list_elem *element;
    while (true)
    {
        element = get_next_lru_clock();
        page = list_entry(element, struct page, lru_elem);
        if(page->vme->_pin)
        {
            continue;
        }
        if(pagedir_is_accessed(page->thread->pagedir, page->vme->vaddr))
        {
            pagedir_set_accessed(page->thread->pagedir, page->vme->vaddr, false);
            continue;
        }
        else
        {
            break;
        }
    }

    bool dirty = pagedir_is_dirty(page->thread->pagedir, page->vme->vaddr);
    
    if (page->vme->type == VM_FILE&&dirty)
    {
        file_write_at(page->vme->file, page->kaddr, page->vme->read_bytes, page->vme->offset);
    }
    else if (page->vme->type == VM_ANON||(page->vme->type == VM_BIN&&dirty))
    { 
        page->vme->swap_slot = swap_out(page->kaddr);
        page->vme->type = VM_ANON;
    }

    page->vme->is_loaded = false;
    pagedir_clear_page(page->thread->pagedir, page->vme->vaddr);
    del_page_from_lru_list(page);
    palloc_free_page(page->kaddr);
    free(page);

    return palloc_get_page(flags);
}

struct page *alloc_page(enum palloc_flags flags)
{
    if((flags & PAL_USER) == 0)
    {
        return NULL;
    }

    struct page *page;
    void *page_kaddr;

    page_kaddr=palloc_get_page(flags);
    while (!page_kaddr)
    {
        page_kaddr=try_to_free_pages(flags);
    }

    page = (struct page *)malloc(sizeof(struct page));
    if (!page)
    {
        palloc_free_page(page_kaddr);
        return NULL;
    }

    memset(page, 0, sizeof(struct page));
    page->thread = thread_current();
    page->kaddr = page_kaddr;
    
    add_page_to_lru_list(page);
    return page;
}

void free_page(void *kaddr)
{
    struct page *lru_page;
    struct list_elem *element;
    for (element = list_begin(&lru_list); element != list_end(&lru_list); element = list_next(element))
    {
        lru_page = list_entry(element, struct page, lru_elem);
        if (lru_page->kaddr == kaddr)
        {
            if (lru_page != NULL)
            {
                pagedir_clear_page(lru_page->thread->pagedir, lru_page->vme->vaddr);
                del_page_from_lru_list(lru_page);
                palloc_free_page(lru_page->kaddr);
                free(lru_page);
            }
            break;
        }
    }
}