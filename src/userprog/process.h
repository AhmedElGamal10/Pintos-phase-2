#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include <hash.h>
#include <list.h>
#include "threads/synch.h"

typedef int pid_t;


static struct semaphore *sema ;

struct process
{
	pid_t pid;                       /* process id */
	struct list_elem children_elem;  /* list element. To use in the parent's children list */
	struct list children;            /* list of child processes */
	struct list_elem all_elem;       /* hash table element for the all processes table */
	bool parent_exited;              /* true if parent finished execution */
	int return_val;              	 /* current thread_return value */
	struct semaphore *waiting_forme; /* semaphore for parent, if wating for this process */
	bool process_alive;              /* process exited ? */
	char *name;					 	 /* process name */
	bool parent_waited;
	int fd;
};

void erase_process(struct process* proc);
void process_init(pid_t main_pid, char* main_name);
struct process* get_process_by_id(pid_t pid);
tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

#endif /* userprog/process.h */
