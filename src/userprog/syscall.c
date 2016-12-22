#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <stdbool.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "devices/input.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "threads/synch.h"
/* helper functions */
int get_call_type(void* esp);
void* get_ptr(void** esp);
int get_int(void** esp);
bool is_valid_stack_ptr(void *esp);
bool address_mapped(void *addr);
//int get_fd(struct file *file);
//struct file_discriptor *file_exists(char *file_name);

/* system call subroutine */
void halt (void) NO_RETURN;
int exit (int status);
pid_t exec (const char *file, bool *error_occured);
int wait (pid_t process_id);
bool create (const char *file, unsigned initial_size, bool *error_occured);
bool remove (const char *file, bool *error_occured);
int open (const char *file, bool *error_occured);
int filesize (int fd, bool *error_occured);
int read (int fd, void *buffer, unsigned length, bool *error_occured);
int write (int fd, const void *buffer, unsigned length, bool *error_occured);
void seek (int fd, unsigned position, bool *error_occured);
unsigned tell (int fd, bool *error_occured);
void close (int fd, bool *error_occured);

static int fd_counter = 2 ; /* as fd 0 reserved for (STDIN_FILENO) and 1 for  (STDOUT_FILENO) */ 

static void syscall_handler (struct intr_frame *);


void
syscall_init (void) 
{
  file_lock	= (struct lock*) malloc(sizeof(struct lock));
 	
  /* lock init */
  lock_init(file_lock) ;
  
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  
  
}

static void
syscall_handler (struct intr_frame *f) 
{
	void *temp_esp = f->esp;

	//hex_dump(0, temp_esp, 20, true);
	//printf("++++%lld, %lld\n", temp_esp, PHYS_BASE);
	if (!is_valid_stack_ptr(temp_esp + sizeof(int)) 
		|| !address_mapped(temp_esp) 
		|| !address_mapped(temp_esp + sizeof(int) - 1)) {//wrong address
		f->eax = exit(-1);
		return;
	}
	int call_type = get_call_type(temp_esp);

	//esp points to args
	temp_esp += sizeof(int);

	int int_size = sizeof(int);
	int ptr_size = sizeof(void*);
	int arg_size = 0;

	switch (call_type) {
		case SYS_HALT:
		{
			break;
		}
	    case SYS_EXIT:
	    {
	    	arg_size = int_size;
			break;
	    }
	    case SYS_EXEC:
	    {
	    	arg_size = ptr_size;
			break;
	    }
	    case SYS_WAIT:
	    {
	    	arg_size = int_size;
			break;
	    }
	    case SYS_CREATE:
	    {
	    	arg_size = int_size + ptr_size;
			break;
	    }
	    case SYS_REMOVE:
	    {
	    	arg_size = ptr_size;
			break;
	    }
	    case SYS_OPEN:
	    {
	    	arg_size = ptr_size;
			break;
	    }
	    case SYS_FILESIZE:
	    {
	    	arg_size = int_size;
			break;
	    }
	    case SYS_READ:
	    {
	    	arg_size = 2 * int_size + ptr_size;
			break;
	    }
	    case SYS_WRITE:
	    {
	   		arg_size = 2 * int_size + ptr_size;
			break;
	    }
	    case SYS_SEEK:
	    {
	    	arg_size = 2 * int_size;
			break;
	    }
	    case SYS_TELL:
	    {
	    	arg_size = int_size;
			break;
	    }
	    case SYS_CLOSE:
	    {
	    	arg_size = int_size;
			break;
	    }
	}

	if (!is_valid_stack_ptr(temp_esp + arg_size) 
		|| !address_mapped(temp_esp) 
		|| !address_mapped(temp_esp + arg_size - 1)) {//error
		f->eax = exit(-1);
		return;
	}


	switch (call_type) {
		case SYS_HALT:
		{
			halt();
			return;
		}
	    case SYS_EXIT:
	    {
	    	int status = get_int(&temp_esp);
			f->eax = status;	
	    	f->eax = exit(status);
	    	return;
	    }
	    case SYS_EXEC:
	    {
	    	const char *file = get_ptr(&temp_esp);
	    	bool error_occured = false;
	    	f->eax = exec(file, &error_occured);
	    	if (error_occured) {
	    		f->eax = exit(-1);
				return;
	    	}
	    	return;
	    }
	    case SYS_WAIT:
	    {
	    	pid_t arg = (pid_t) get_int(&temp_esp);
	    	f->eax = wait(arg);
	    	return;
	    }
	    case SYS_CREATE:
	    {
	    	const char *file = (const char *) get_ptr(&temp_esp);
	    	unsigned initial_size = (unsigned) get_int(&temp_esp);
	    	bool error_occured = false;
	    	f->eax = create(file, initial_size, &error_occured);
	    	if (error_occured) {
	    		f->eax = exit(-1);
				return;
	    	}
	    	return;
	    }
	    case SYS_REMOVE:
	    {
	    	const char *file = (const char *) get_ptr(&temp_esp);
	    	bool error_occured = false;
	    	f->eax = remove(file, &error_occured);
	    	if (error_occured) {
	    		f->eax = exit(-1);
				return;
	    	}
	    	return;
	    }
	    case SYS_OPEN:
	    {
	    	const char *file = (const char *) get_ptr(&temp_esp);
	    	bool error_occured = false;
	    	f->eax = open(file, &error_occured);
	    	if (error_occured) {
	    		f->eax = exit(-1);
				return;
	    	}
	    	return;
	    }
	    case SYS_FILESIZE:
	    {
	    	int fd = get_int(&temp_esp);
	    	bool error_occured = false;
	    	f->eax = filesize(fd, &error_occured);
	    	if (error_occured) {
	    		f->eax = exit(-1);
				return;
	    	}
	    	return;
	    }
	    case SYS_READ:
	    {
	    	int fd = get_int(&temp_esp);
	    	void* buffer = get_ptr(&temp_esp);
	    	unsigned length = (unsigned) get_int(&temp_esp);
	    	bool error_occured = false;
	    	f->eax = read(fd, buffer, length, &error_occured);
	    	if (error_occured) {
	    		f->eax = exit(-1);
				return;
	    	}
	    	return;
	    }
	    case SYS_WRITE:
	    {
	   		int fd = get_int(&temp_esp);
			void* buffer = get_ptr(&temp_esp);
	    	unsigned length = (unsigned) get_int(&temp_esp);
	    	bool error_occured = false;
	    	f->eax = write(fd, buffer, length, &error_occured);
	    	if (error_occured) {
	    		f->eax = exit(-1);
				return;
	    	}
	    	return;
	    }
	    case SYS_SEEK:
	    {
	    	int fd = get_int(&temp_esp);
	    	unsigned position = (unsigned) get_int(&temp_esp);
	    	bool error_occured = false;
	    	seek(fd, position, &error_occured);
	    	if (error_occured) {
	    		f->eax = exit(-1);
				return;
	    	}
	    	return;
	    }
	    case SYS_TELL:
	    {
	    	int fd = get_int(&temp_esp);
	    	bool error_occured = false;
	    	f->eax = tell(fd, &error_occured);
	    	if (error_occured) {
	    		f->eax = exit(-1);
				return;
	    	}
	    	return;
	    }
	    case SYS_CLOSE:
	    {
	    	int fd = get_int(&temp_esp);
	    	bool error_occured = false;
	    	close(fd, &error_occured);
	    	if (error_occured) {
	    		f->eax = exit(-1);
				return;
	    	}
	    	return;
	    }
	}
}


void halt (void)
{
	shutdown_power_off();
}

int exit (int status)
{
	//get process pointer
	struct process* current_process = get_process_by_id(thread_current()->tid);
	if (current_process == NULL 
			|| !current_process->process_alive) {//no process found
		printf ("%s: exit(%d)\n", current_process->name, -1);
		return -1;
	}

	//set process return status
	current_process->return_val = status;

	//if not waiting
	//process_exit();
	
	//return 0

	//else return status
	printf ("%s: exit(%d)\n", current_process->name, status);	

	//notify the parent, incase it is waiting
  	
  	thread_exit();
  	
    finish_file() ; /* free file */ 
  	
	return status;
}

pid_t exec (const char *file, bool *error_occured)
{
	//check validity of provided file
	if (file == NULL || !is_valid_stack_ptr(file + sizeof(const char *))
		|| !address_mapped(file) 
		|| !address_mapped(file + sizeof(const char *) - 1)) {
		*error_occured = true;
		return -1;
	}

	//start the process
	tid_t tid = process_execute(file);
	if (tid == TID_ERROR) {//thread is not created.
		return -1;
	}

	//give the process the same id as that of its thread
	pid_t pid = (pid_t) tid;

	return pid;
}

int wait (pid_t process_id)
{
	return process_wait(process_id);
}

bool create (const char *file, unsigned initial_size, bool *error_occured)
{
	//check validity of provided file
	if (file == NULL || !is_valid_stack_ptr(file + sizeof(const char *))
		|| !address_mapped(file) 
		|| !address_mapped(file + sizeof(const char *) - 1)) {

		*error_occured = true;
		return false;
	}
	lock_acquire(file_lock) ;	

	bool success = filesys_create (file, initial_size);

	lock_release(file_lock) ;

	return success;
}

bool remove (const char *file, bool *error_occured)
{
	//check validity of provided file
	if (file == NULL || !is_valid_stack_ptr(file + sizeof(const char *))
		|| !address_mapped(file) 
		|| !address_mapped(file + sizeof(const char *) - 1)) {
		*error_occured = true;
		return false;
	}
	lock_acquire(file_lock) ;	
	
	bool success =  remove (file, error_occured);
	
	lock_release(file_lock) ;
	
	return success;
}
int open (const char *file, bool *error_occured)
{
	//check validity of provided file
	if (file == NULL || !is_valid_stack_ptr(file + sizeof(const char *))
		|| !address_mapped(file) 
		|| !address_mapped(file + sizeof(const char *) - 1)) {
		*error_occured = true;
		return -1;
	}

	lock_acquire(file_lock) ;
	struct file * opened_file = filesys_open (file);


	if (opened_file == NULL)
	{
		lock_release(file_lock) ;
		return -1 ; /* unvalid */
	}

	
	/* add for that process the new fd */
	struct thread *cur = thread_current (); /* current process */ 
	
	/* assign new value of fd & file in file_discriptor element */ 
	struct file_discriptor *f_d = (struct file_discriptor*) malloc(sizeof(struct file_discriptor));
	f_d->fd = fd_counter ;
	f_d->file = opened_file ;
	//strlcpy(f_d->name, file, 100);
	
	list_push_back(&cur->opened_files, &f_d->elem);	
	
	
	lock_release(file_lock) ;
	
	return fd_counter++ ;
}

int filesize (int fd, bool *error_occured)
{
	int ret = 0 ;
	
	lock_acquire(file_lock) ;	
	
	struct file*mapped_file = get_file(fd) ;
	
	if (mapped_file != NULL) 
	{
		ret = file_length (mapped_file) ;
	} 
	
	lock_release(file_lock) ;
	
	return ret;
}

int read (int fd, void *buffer, unsigned length, bool *error_occured)
{
	//check validity of provided pointer
	if (buffer == NULL || !is_valid_stack_ptr(buffer + sizeof(void *))
		|| !address_mapped(buffer) 
		|| !address_mapped(buffer + sizeof(void *) - 1)) {
		*error_occured = true;
		return -1;
	}

	int ret = -1 ;
	
	if (fd == 0) /* reads from the keyboard */ 
	{
		unsigned i;
		uint8_t* internal_buffer = (uint8_t *) buffer;
		for (i = 0; i < length; i++)
		{
			internal_buffer[i] = input_getc();
		}
		return length;
	}
	
	lock_acquire(file_lock) ;	
	
	struct file*mapped_file = get_file(fd) ;
	
	
	if (mapped_file != NULL) 
	{
		ret = file_read (mapped_file,  buffer, length) ;
	}
	
	lock_release(file_lock) ;
	
	
	return ret;
}


int write (int fd, const void *buffer, unsigned length, bool *error_occured)
{
	//check validity of provided pointer
	if (buffer == NULL || !is_valid_stack_ptr(buffer + sizeof(void *))
		|| !address_mapped(buffer) 
		|| !address_mapped(buffer + sizeof(void *) - 1)) {
		*error_occured = true;
		
		return -1;
	}

	if (fd == 1) /* writes to the console */
	{
		putbuf(buffer, length);
		return length;
	}
	
	int ret = -1 ;
	lock_acquire(file_lock) ;	
	
	
	struct file*mapped_file = get_file(fd) ;
	
	if (mapped_file != NULL) 
	{
		ret = file_write (mapped_file, buffer, length) ; 
	} 
	
	lock_release(file_lock) ;
	
	return ret;
}

void seek (int fd, unsigned position, bool *error_occured)
{
	lock_acquire(file_lock) ;	
	
	struct file*mapped_file = get_file(fd) ;
	
	if (mapped_file != NULL) 
	{
		file_seek(mapped_file, position);
	} 
	
	lock_release(file_lock) ;
	
	return ;
}

unsigned tell (int fd, bool *error_occured)
{
	off_t ret ;

	lock_acquire(file_lock) ;	
	
	struct file*mapped_file = get_file(fd) ;
	
	if (mapped_file != NULL) 
	{
		ret = file_tell(mapped_file);		
	}
	
	lock_release(file_lock) ;
	
	return ret;
}

void close (int fd, bool *error_occured)
{
	lock_acquire(file_lock) ;

	bool valid_fd = false;
	struct thread *cur = thread_current() ;
	
	struct list_elem *e;
	for (e = list_begin (&cur->opened_files); e != list_end (&cur->opened_files);
       e = list_next (e))
    {
		struct file_discriptor *f_d = list_entry (e, struct file_discriptor, elem);

		if (f_d == NULL || !is_valid_stack_ptr(f_d + sizeof(struct file_discriptor *))
		|| !address_mapped(f_d) 
		|| !address_mapped(f_d + sizeof(void *) - 1)) {
			continue;
		}

		if(f_d->fd == fd || fd == -1){
			file_close(f_d->file);
			list_remove(&f_d->elem);
			free(f_d);
			valid_fd = true;
		}
	}
	
	if (!valid_fd) {
		error_occured = true;
	}
	lock_release(file_lock) ;
}

//====================== helper function implementation =============================

int get_int(void** esp)
{
	int temp = *(int*)*esp;
	*esp += sizeof(int);
	return temp;
}
void* get_ptr(void** esp)
{
	void* temp = *(void**)*esp;
	*esp += sizeof(void*);
	return temp;
}


int get_call_type(void* esp)
{
	return *((int*) esp);
}

struct file * 
get_file(int fd) 
{
	struct thread *cur = thread_current() ;
	
	struct list_elem *e;
	for (e = list_begin (&cur->opened_files); e != list_end (&cur->opened_files);
       e = list_next (e))
    {
		struct file_discriptor *f_d = list_entry (e, struct file_discriptor, elem);

    	if (f_d == NULL) {
			continue;
		}
		
		if(f_d->fd == fd){
			return f_d->file ;
		}
    }
    return NULL;
}

bool is_valid_stack_ptr(void *esp)
{
	if (esp == NULL || (void *)PHYS_BASE < esp) {
		return false;
	}
	return true;
}

bool address_mapped(void *addr)
{
	return pagedir_get_page(thread_current()->pagedir, addr) != NULL;
}

void 
finish_file()
{
	struct file * file = thread_current()->associated_file ;
	
	file_close(file) ;
	
	bool *error ;
	
	close(-1 , &error) ;
}
