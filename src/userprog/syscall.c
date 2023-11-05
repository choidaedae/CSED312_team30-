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

static void syscall_handler(struct intr_frame *);
struct lock lock_file;

static void isAddressValid(void *addr)
{
  if(addr < (void *)0x08048000 || addr >= (void *)0xc0000000)
  {
    sys_exit(-1);
  }
}
static void check_string(char *str, unsigned size)
{
  while(size--)
  {
    isAddressValid((void*)str++);
  }
}

void get_argument(void *esp, int *arg, int count)
{
  int i;
  for (i = 0; i < count; i++)
  {
    isAddressValid(esp + 4 * i);
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
  if (file==NULL)
  {
    sys_exit(-1);
  }
  return filesys_create(file, initial_size);
}

bool sys_remove(const char *file)
{
  return filesys_remove(file);
}

int sys_open(const char *file)
{
  struct file *f;
    
  lock_acquire(&lock_file);
  f = filesys_open(file);

  if (!strcmp(thread_current()->name, file)) file_deny_write(f);

  if (f == NULL)
  {
    lock_release(&lock_file);
    return -1;
  }
  int fd = thread_current()->fd_nxt++;
  thread_current()->fd_table[fd] = f;
  lock_release(&lock_file);
  return fd;
}

int sys_filesize(int fd)
{
  struct file *f;

  if(fd < thread_current()->fd_nxt)
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
}

int sys_read(int fd, void *buffer, unsigned size)
{
  int read_size = 0;
  struct file *f;
  int current_fd=thread_current()->fd_nxt;

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
    f = thread_current()->fd_table[fd];

    if (f==NULL)
    {
      sys_exit(-1);
    }
    read_size = file_read(f, buffer, size);
  }

  lock_release(&lock_file);

  return read_size;
}

int sys_write(int fd, const void *buffer, unsigned size)
{
  int write_size = 0;
  struct file *f;

  int current_fd=thread_current()->fd_nxt;

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
    f = thread_current()->fd_table[fd];

    if (f==NULL)
    {
      sys_exit(-1);
    }
    write_size = file_write(f, (const void *)buffer, size);
  }

  lock_release(&lock_file);

  return write_size;
}

void sys_seek(int fd, unsigned position)
{
  struct file *f;

  if(fd < thread_current()->fd_nxt)
  {
		f = thread_current()->fd_table[fd];
	}
  else
  {
    f=NULL;
  }
  
  if (f != NULL)
  {
    file_seek(f, position);
  }
}

unsigned sys_tell(int fd)
{
  struct file *f;

  if(fd < thread_current()->fd_nxt)
  {
		f = thread_current()->fd_table[fd];
	}
  else
  {
    f=NULL;
  }

  if (f != NULL)
  {
    return file_tell(f);
  }
  return 0;
}

void sys_close(int fd)
{
  struct file *f;
  if(fd < thread_current()->fd_nxt)
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


static void
syscall_handler(struct intr_frame *f)
{
  isAddressValid(f->esp);

  int argv[3];
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
    isAddressValid((void *)argv[0]);
    f->eax = sys_exec((const char *)argv[0]);
    break;
  case SYS_WAIT:
    get_argument(f->esp + 4, &argv[0], 1);
    f->eax = sys_wait((pid_t)argv[0]);
    break;
  case SYS_CREATE:
    get_argument(f->esp + 4, &argv[0], 2);
    isAddressValid((void *)argv[0]);
    f->eax = sys_create((const char *)argv[0], (unsigned)argv[1]);
    break;
  case SYS_REMOVE:
    get_argument(f->esp + 4, &argv[0], 1);
    isAddressValid((void *)argv[0]);
    f->eax = sys_remove((const char *)argv[0]);
    break;
  case SYS_OPEN:
    get_argument(f->esp + 4, &argv[0], 1);
    isAddressValid((void *)argv[0]);
    f->eax = sys_open((const char *)argv[0]);
    break;
  case SYS_FILESIZE:
    get_argument(f->esp + 4, &argv[0], 1);
    f->eax = sys_filesize(argv[0]);
    break;
  case SYS_READ:
    get_argument(f->esp + 4, &argv[0], 3);
    check_string((char*)argv[1],(unsigned)argv[2]);
    f->eax = sys_read((int)argv[0], (void *)argv[1], (unsigned)argv[2]);
    break;
  case SYS_WRITE:
    get_argument(f->esp + 4, &argv[0], 3);
    check_string((char*)argv[1],(unsigned)argv[2]);
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
    sys_close(argv[0]);
    break;
  default:
    sys_exit(-1);
  }
}