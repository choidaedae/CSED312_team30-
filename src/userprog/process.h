#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "userprog/syscall.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

void construct_esp(char *file_name, void **esp);
int fn_to_argument(char **argv, char *file_name);
void argument_stack(char **argv, int argc, void **esp);
struct thread *get_child_process (pid_t pid);

#endif /* userprog/process.h */
