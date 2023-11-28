#include "vm/frame.h"
#include <list.h>
#include "vm/page.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"

void lru_list_init(void)
{
  list_init(&lru_list);
  lock_init(&lru_lock);
  lru_clock = NULL;
}

void add_page_to_lru_list(struct page *page)
{
  lock_acquire(&lru_lock);
  list_push_back(&lru_list, &page->lru_elem);
  lock_release(&lru_lock);
}

void del_page_from_lru_list(struct page *page)
{
  if (lru_clock == &page->lru_elem)
  {
    lru_clock = list_remove(lru_clock);
  }
  else
  {
    list_remove(&page->lru_elem);
  }
}