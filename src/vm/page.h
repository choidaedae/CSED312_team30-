#ifndef VM_PAGE_H
#define VM_PAGE_H

#define VM_BIN 0
#define VM_FILE 1
#define VM_ANON 2

#include <hash.h>
#include "userprog/syscall.h"
#include "threads/palloc.h"
#include "filesys/off_t.h"
#include "userprog/syscall.h"
extern struct lock lru_lock;


struct vm_entry {
    uint8_t type;
    void *vaddr;
    bool writable;
    bool is_loaded;
    bool _pin;
    struct file* file;
    size_t offset;
    size_t read_bytes;
    size_t zero_bytes;

    struct hash_elem elem;
    struct list_elem mmap_elem;
    size_t swap_slot;
};

struct mmap_file {
    mapid_t mapid;
    struct file* file;
    struct list vme_list;
    struct list_elem elem;
};

struct page {
    void *kaddr;
    struct vm_entry *vme;
    struct thread *thread;
    struct list_elem lru_elem;
};

static struct list_elem *get_next_lru_clock();

void vm_init(struct hash *vm);
void vm_destroy(struct hash *vm);
struct vm_entry *find_vme(void *vaddr);
bool insert_vme(struct hash *vm, struct vm_entry *vme);
bool delete_vme(struct hash *vm, struct vm_entry *vme);
bool load_file(void *kaddr, struct vm_entry *vme);

void* try_to_free_pages(enum palloc_flags flags);
struct page* alloc_page(enum palloc_flags flags);
void free_page(void *kaddr);

#endif