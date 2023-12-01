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
    //#####bool pinned 가 없다.
    uint8_t type; /* VM_BIN, VM_FILE, VM_ANON의 타입 */
    void *vaddr; /* virtual page number */
    bool writable; /* 해당 주소에 write 가능 여부 */
    bool is_loaded; /* physical memory의 load 여부를 알려주는 flag */
    struct file* file; /* mapping된 파일 */
    size_t offset; /* read 할 파일 offset */
    size_t read_bytes; /* virtual page에 쓰여져 있는 데이터 byte 수 */
    size_t zero_bytes; /* 0으로 채울 남은 페이지의 byte 수 */

    struct hash_elem elem; /* Hash Table Element */
    struct list_elem mmap_elem; /* mmap 리스트 element */
    size_t swap_slot;
};

struct mmap_file {
    mapid_t mapid;
    struct file* file;
    struct list_elem elem;
    struct list vme_list;
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