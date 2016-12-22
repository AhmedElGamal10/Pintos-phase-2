#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include <debug.h>
#include "filesys/file.h"

/* Process identifier. */
typedef int pid_t;

/* lock for using file sys */ 
static struct lock *file_lock ;

void syscall_init (void);

struct file*get_file(int) ;

void finish_file(void) ; 
#endif /* userprog/syscall.h */
