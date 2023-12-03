#include "userprog/syscall.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/malloc.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

#include "vm/page.h"
#include "vm/frame.h"

extern struct lock lru_lock;

#include "userprog/process.h"

static void syscall_handler(struct intr_frame *);
struct lock lock_file;

static void isAddressValid(void *addr, void *esp)
{
  // if(addr >= (void *)0x08048000 && addr < (void *)0xc0000000)
  // {
  //   return true;
  // }
  // else
  // {
  //   return false;
  // }
  if(addr < (void *)0x08048000 || addr >= (void *)0xc0000000) sys_exit(-1);
  if(!find_vme(addr))
  {
    if(!verify_stack((int32_t) addr, esp)) sys_exit(-1);
    if(!expand_stack(addr)) sys_exit(-1);
  }
}
static void check_string(char *str, unsigned size, void *esp)
{
  while(size--) isAddressValid((void*)str++,esp);
}

void get_argument(void *esp, int *arg, int count)
{
  int i;
  for (i = 0; i < count; i++)
  {
    isAddressValid(esp + 4 * i, esp);
    arg[i] = *(int *)(esp + 4 * i);
  }
}

void syscall_init(void)
{
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&lock_file);
}

void sys_halt(void)
{
  shutdown_power_off();
}

void sys_exit(int status)
{
  thread_current()->exit_status = status;
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_exit();
}

pid_t sys_exec(const char *file)
{
  struct thread *child;
  // if(!isAddressValid(file))
  // {
  //   sys_exit(-1);
  // }
  pid_t pid = process_execute(file);
  if (pid == -1)
  {
    return -1;
  }
  child = get_child_process(pid);
  sema_down(&(child->sema_exec));
  if (!child->isLoad)
  {
    return -1;
  }
  return pid;
}

int sys_wait(pid_t pid)
{
  return process_wait(pid);
}

bool sys_create(const char *file, unsigned initial_size)
{
  // if(!isAddressValid(file)||file==NULL)
  // {
  //   sys_exit(-1);
  // }
  if (file == NULL) sys_exit(-1);
  lock_acquire(&lock_file);
  bool success = filesys_create(file, initial_size);
  lock_release(&lock_file);
  return success; 
}

bool sys_remove(const char *file)
{
  // if(!isAddressValid(file))
  // {
  //   sys_exit(-1);
  // }
  if (file == NULL) sys_exit(-1);
  lock_acquire(&lock_file);
  bool success = filesys_remove(file);
  lock_release(&lock_file);
  return success; 
}

int sys_open(const char *file)
{
  // if(!isAddressValid(file))
  // {
  //   sys_exit(-1);
  // }
  struct file *f;
    
  lock_acquire(&lock_file);
  f = filesys_open(file);

  if (f == NULL)
  {
    lock_release(&lock_file);
    return -1;
  }

  if (!strcmp(thread_current()->name, file))
  {
    file_deny_write(f);
  }

  int fd = thread_current()->fd_count++;
  thread_current()->fd_table[fd] = f;
  lock_release(&lock_file);
  return fd;
}

int sys_filesize(int fd)
{
  struct file *f;
  int size;
  lock_acquire(&lock_file);
  if (f = process_get_file(fd)) size = file_length(f);
  else size = -1;

  /*

  if(fd < thread_current()->fd_count)
  {
		f = thread_current()->fd_table[fd];
	}
  else
  {
    f=NULL;
  }


  if (f==NULL)
  {
    return -1;
  }
  else
  {
    return file_length(f);
  }

  */
 lock_release(&lock_file);
 return size;
}

int sys_read(int fd, void *buffer, unsigned size)
{
  // int i;
  // for(int i=0;i<size;i++)
  // {
  //   if(!isAddressValid(buffer+i))
  //   {
  //     sys_exit(-1);
  //   }
  // }
  int read_size = 0;
  struct file *f;
  int current_fd=thread_current()->fd_count;

  if (fd < 0 || fd > current_fd)
  {
    sys_exit (-1);
  }

  lock_acquire(&lock_file);

  if (fd == 0)
  { 
    unsigned int i;
    for (i = 0; i < size; i++)
    {
      if (((char *)buffer)[i] == '\0')
        break;
    }
    read_size = i;
  }
  else
  {
    //^^^
    /*
    f = thread_current()->fd_table[fd];

    if (f==NULL)
    {
      sys_exit(-1);
    }
    read_size = file_read(f, buffer, size);
    */
      //^^^
    if ((f = process_get_file(fd)))
      read_size = file_read(f, buffer, size);
  }

  lock_release(&lock_file); 

  return read_size;
}

int sys_write(int fd, const void *buffer, unsigned size)
{
  // int i;
  // for(int i=0;i<size;i++)
  // {
  //   if(!isAddressValid(buffer+i))
  //   {
  //     sys_exit(-1);
  //   }
  // }
  int write_size = 0;
  struct file *f;

  int current_fd=thread_current()->fd_count;

  if (fd < 1 || fd > current_fd)
  {
    sys_exit (-1);
  }

  lock_acquire(&lock_file);

  if (fd == 1)
  { 
    putbuf(buffer, size);
    write_size = size;
  }
  else
  {
    
    //^^^
    /*
    f = thread_current()->fd_table[fd];
    if (f==NULL)
    {
      sys_exit(-1);
    }
    */
    //^^^
    if ((f = process_get_file(fd)))
      write_size = file_write(f, (const void *)buffer, size);
  }

  lock_release(&lock_file);

  return write_size;
}

void sys_seek(int fd, unsigned position)
{
  //^^^
  //struct file *f;
  //^^^
  lock_acquire(&lock_file);
  struct file *f = process_get_file(fd); 
  //^^^
  /*
  if(fd < thread_current()->fd_count)
  {
		f = thread_current()->fd_table[fd];
	}
  else
  {
    f=NULL;
  }
  //^^^
  */
  if (f != NULL)
  {
    file_seek(f, position);
  }
  lock_release(&lock_file);
}

unsigned sys_tell(int fd)
{
  //^^^
  //struct file *f;
  //int pos;
  //^^^

  lock_acquire(&lock_file);
  //^^^
  /*
  if(fd < thread_current()->fd_count)
  {
		f = thread_current()->fd_table[fd];
	}
  else
  {
    f=NULL;
  }
  */
  //^^^
  struct file *f = process_get_file(fd); /* file descriptor를 이용하여 파일 객체 검색 */
  unsigned pos;
  if (f != NULL)
  {
    pos = file_tell(f);
  }
  else pos = 0;

  lock_release(&lock_file);

  return pos;
}

void sys_close(int fd)
{
  struct file *f;
  if(fd < thread_current()->fd_count)
  {
		f = thread_current()->fd_table[fd];
	}
  else
  {
    f=NULL;
  }

  if (f==NULL)
  {
		return;
	}
  file_close(f);
	thread_current()->fd_table[fd] = NULL;
}

//&&&
void close(int fd)
{
  close_file(fd);
}
//&&&

mapid_t
sys_mmap(int fd, void *addr)
{
  if (pg_ofs (addr) != 0 || !addr) return -1;
  if (!is_user_vaddr (addr)) return -1;

  struct mmap_file *mfe;
  size_t ofs = 0;

  //mmap_file 생성 및 초기화
  mfe = (struct mmap_file *)malloc(sizeof(struct mmap_file));
  if (!mfe) return -1;
  
  memset (mfe, 0, sizeof(struct mmap_file));
  mfe->mapid = thread_current()->mmap_nxt++;
  lock_acquire(&lock_file);
  mfe->file = file_reopen(process_get_file(fd));
  lock_release(&lock_file);

  list_init(&mfe->vme_list);
  list_push_back(&thread_current()->mmap_list, &mfe->elem);

  //vm_entry 생성 및 초기화
  int file_len = file_length(mfe->file);
  while (file_len > 0)
    {
      if (find_vme (addr)) return -1;

      size_t page_read_bytes = file_len < PGSIZE ? file_len : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      struct vm_entry *vme = (struct vm_entry *)malloc(sizeof(struct vm_entry));
      if (!vme)
    {
      return false;
    }

      memset(vme, 0, sizeof(struct vm_entry));
      vme->type = VM_FILE;
      vme->vaddr = addr;
      vme->writable = true;
      vme->is_loaded = false;
      //%%%%%
      
      vme->_pin=false;
      //or
      //vme->_pin=false;
      //*/
      //%%%%%

      vme->file = mfe->file;
      vme->offset = ofs;
      vme->read_bytes = page_read_bytes;
      vme->zero_bytes = page_zero_bytes;

      list_push_back(&mfe->vme_list, &vme->mmap_elem);
      insert_vme(&thread_current()->vm, vme);
      addr += PGSIZE;
      ofs += PGSIZE;
      file_len -= PGSIZE;
    }
  return mfe->mapid;
}

void sys_munmap(mapid_t mapid)
{
  struct mmap_file *mfe = NULL;
  struct list_elem *ele;
  for (ele = list_begin(&thread_current()->mmap_list); ele != list_end(&thread_current()->mmap_list); ele = list_next (ele))
  {
    mfe = list_entry (ele, struct mmap_file, elem);
    if (mfe->mapid == mapid) break;
  }

  if (!mfe) return;
  
  for (ele = list_begin(&mfe->vme_list); ele != list_end(&mfe->vme_list);)
  {
    struct vm_entry *vme = list_entry(ele, struct vm_entry, mmap_elem);
    if (vme->is_loaded && pagedir_is_dirty(thread_current()->pagedir, vme->vaddr))
    {
      lock_acquire(&lock_file);
      if (file_write_at(vme->file, vme->vaddr, vme->read_bytes, vme->offset) != (int)vme->read_bytes) NOT_REACHED();
      lock_release(&lock_file);
      
      lock_acquire(&lru_lock);
      free_page(pagedir_get_page(thread_current()->pagedir,vme->vaddr));
      lock_release(&lru_lock);
    }
    vme->is_loaded = false;
    ele = list_remove(ele);
    delete_vme(&thread_current()->vm, vme);
  }
  list_remove(&mfe->elem);
  free(mfe);
}


static void
syscall_handler(struct intr_frame *f)
{
  // if(!isAddressValid(f->esp))
  // {
  //   sys_exit(-1);
  // }
  isAddressValid(f->esp, f->esp);

  int argv[4];
  switch (*(uint32_t *)(f->esp))
  {
  case SYS_HALT:
    sys_halt();
    break;
  case SYS_EXIT:
    get_argument(f->esp + 4, &argv[0], 1);
    sys_exit((int)argv[0]);
    break;
  case SYS_EXEC:
    get_argument(f->esp + 4, &argv[0], 1);
    isAddressValid((void *)argv[0], f->esp);
    f->eax = sys_exec((const char *)argv[0]);
    break;
  case SYS_WAIT:
    get_argument(f->esp + 4, &argv[0], 1);
    f->eax = sys_wait((pid_t)argv[0]);
    break;
  case SYS_CREATE:
    get_argument(f->esp + 4, &argv[0], 2);
    isAddressValid((void *)argv[0], f->esp);
    f->eax = sys_create((const char *)argv[0], (unsigned)argv[1]);
    break;
  case SYS_REMOVE:
    get_argument(f->esp + 4, &argv[0], 1);
    isAddressValid((void *)argv[0], f->esp);
    f->eax = sys_remove((const char *)argv[0]);
    break;
  case SYS_OPEN:
    get_argument(f->esp + 4, &argv[0], 1);
    isAddressValid((void *)argv[0], f->esp);
    f->eax = sys_open((const char *)argv[0]);
    break;
  case SYS_FILESIZE:
    get_argument(f->esp + 4, &argv[0], 1);
    f->eax = sys_filesize(argv[0]);
    break;
  case SYS_READ:
    get_argument(f->esp + 4, &argv[0], 3);
    check_string((char*)argv[1],(unsigned)argv[2],f->esp);
    f->eax = sys_read((int)argv[0], (void *)argv[1], (unsigned)argv[2]);
    break;
  case SYS_WRITE:
    get_argument(f->esp + 4, &argv[0], 3);
    check_string((char*)argv[1],(unsigned)argv[2],f->esp);
    f->eax = sys_write((int)argv[0], (const void *)argv[1], (unsigned)argv[2]);
    break;
  case SYS_SEEK:
    get_argument(f->esp + 4, &argv[0], 2);
    sys_seek(argv[0], (unsigned)argv[1]);
    break;
  case SYS_TELL:
    get_argument(f->esp + 4, &argv[0], 1);
    f->eax = sys_tell(argv[0]);
    break;
  case SYS_CLOSE:
    get_argument(f->esp + 4, &argv[0], 1);
    //&&&
    //close(argv[0]);
    //&&&
    sys_close(argv[0]);
    break;
  case SYS_MMAP:
    get_argument(f->esp + 4, &argv[0], 2);
    f->eax = sys_mmap(argv[0], (void *)argv[1]);
    break;
  case SYS_MUNMAP:
    get_argument(f->esp + 4, &argv[0], 1);
    sys_munmap(argv[0]);
    break;
  default:
    sys_exit(-1);
  }
}