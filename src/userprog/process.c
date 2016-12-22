#include "userprog/process.h"
#include "userprog/syscall.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <list.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "lib/string.h"

#define MAX_SIZE 100

static thread_func start_process NO_RETURN;
bool load (const char *file_name, void (**eip) (void), void **esp, char** tokens, int tokens_count);
static struct list all_processes;

struct list_elem* initialize_process(pid_t pid, char* process_name, bool parent_exited);
struct process* get_process_by_id(pid_t pid);
void inform_parent_exit(struct list* children);
void remove_all(struct list* l);


/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */


void process_init(pid_t main_pid, char* main_name)
{
  list_init(&all_processes);
  struct list_elem* elem = initialize_process(main_pid, main_name, true);
  list_push_front(&all_processes, elem);
}




char **split (char *cmd);
void push_to_stack (void **esp, char **args);



/*
  A method that takes the command line and returns an array
  of the arguments split around spaces.
*/
char **
split (char *cmd)
{
    char *p = cmd, *l;
    char ch = ' ';
    int cnt = 0;
    while (*p == ' ') p++;
    while (*p != '\n' && *p != '\0') {
        if ((*p == ' ' || *p == '\t') && (ch != ' ' && ch != '\t')) cnt++;
        if (*p == '\"') {
            p++;
            while (*p != '\"') p++;
        }
        ch = *p;
        p++;
    }
    if (ch == ' ' || ch == '\t') cnt--;
    char **args = (char **) malloc((cnt + 2) * sizeof(char *));
    int i = 0;
    p = cmd;
    while (*p != '\n' && *p != '\0' && i < cnt + 1) {
        while (*p == ' ' || *p == '\t') p++;
        l = p;
        if (*p == '\"') {
            l++;
            while (*l != '\"' && *l != '\0' && *l != '\n') l++;
            if (*l != '\0' || *l != '\n') l++;
        } else {
            while (*l != ' ' && *l != '\t' && *l != '\0' && *l != '\n') l++;
        }
        args[i] = (char *) malloc(l - p + 1);
        strlcpy(args[i], p, l - p + 1);
        i++;
        p = l;
    }
    args[i] = NULL;
    p = NULL;
    l = NULL;
    return args;
}

void
push_to_stack (void **esp, char **args)
{
  size_t len;
  void **addr;
  char *tmp_str UNUSED;
  uint8_t aword = 0;
  int cnt = 0, tmp_cnt = 0;
  /* Get number of arguments. */
  char **tmp_args = args;
  while (*tmp_args != NULL) {
    cnt++;
    *tmp_args++;
  }
  /* Void array to save addresses at. */
  addr = (void *) malloc(cnt * sizeof(void *));
  tmp_cnt = cnt - 1;
  /* Insert argument and save the address. */
  while (tmp_cnt >= 0) {
    len = strlen(args[tmp_cnt]) + 1;
    *esp -= len;
    addr[tmp_cnt] = *esp;
        strlcpy(*esp, args[tmp_cnt], len);
    tmp_cnt--;
  }
  /* Insert 0s as alignment blocks. */
  while (* (int  *)esp % 4) {
      *esp -= 1;
      memcpy(*esp, &aword, 1);
  }
    if (tmp_cnt) {
  }
  /* Insert NULL terminator. */
  *esp -= sizeof(char *);
  tmp_cnt = 0;
  memcpy(*esp, &tmp_cnt, sizeof(char *));
  /* Insert addresses of arguments. */
  tmp_cnt = cnt - 1;
  while (tmp_cnt >= 0) {
    *esp -= sizeof(char *);
    memcpy(*esp, &addr[tmp_cnt], sizeof(char *));
    tmp_cnt--;
  }
  /* Insert the starting address of the array. */
  addr[0] = *esp;
  *esp -= sizeof(char *);
  memcpy(*esp, &addr[0], sizeof(char *));
  /* Insert number of arguments. */
  *esp -= sizeof(int);
    memcpy(*esp, &cnt, sizeof(int));
  /* Insert fake return address. */
  *esp -= sizeof(void *);
  memcpy(*esp, &addr[0], sizeof(void *));
  /* Free the memory. */
  free(addr);
}







  


tid_t
process_execute (const char *file_name) 
{
  char *fn_copy;
  tid_t tid;

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);

  char* save_ptr = NULL;
  char* file_name_ = file_name;
  char *name_ = split(file_name)[0];

  /* Create a new thread to execute FILE_NAME. */
  tid = thread_create (name_ , PRI_DEFAULT, start_process, fn_copy);
  if (tid == TID_ERROR) {
    palloc_free_page (fn_copy); 
  }



  if (tid == TID_ERROR) {//thread is not created.
    return tid;
  }

  struct thread* process_thread = get_thread_by_id (tid);
  if (process_thread == NULL) { //error
    return TID_ERROR;
  }
  //else, we have a valid thread

  //get a pointer to its status
  bool *success = process_thread->process_success;

  //wait for the child process to be loaded
  sema_down(&process_thread->process_loaded);

  
 
  if (!*success) {//error loading process
    return TID_ERROR;
  }

  //else, child process is loaded

  /* put it in hash table */
  struct process* child = get_process_by_id(tid);
  struct process* current = get_process_by_id(thread_current()->tid);
  list_push_front(&current->children, &child->children_elem);
	
 
	
  return tid;
}

/* A thread function that loads a user process and starts it
   running. */
static void
start_process (void *file_name_)
{
  char *file_name = file_name_;
  struct intr_frame if_;
  bool success;
  int tokens_count;
  char** tokens = split((char*)file_name);

  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;

  char *process_name = (char*) malloc(MAX_SIZE * sizeof(char));
  strlcpy(process_name, tokens[0], MAX_SIZE);

  const char * filename = tokens[0];
  success = load (filename, &if_.eip, &if_.esp, tokens, tokens_count);


  //inform the parent process
  *thread_current()->process_success = success;
  

  /* If load failed, quit. */
  palloc_free_page (file_name);
  if (!success) {
    sema_up(&thread_current()->process_loaded);
  
    thread_exit ();
  } else {
    //add the process to the list
    struct list_elem* elem = initialize_process(thread_current()->tid, process_name, false);
    
    list_push_front(&all_processes, elem);

    sema_up(&thread_current()->process_loaded);
  
  }
  

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid) 
{

  struct process* child_process = get_process_by_id(child_tid);
  if (child_process == NULL) {//no process found
    return -1;
  }

  if (child_process->parent_waited) {
    list_remove(&child_process->children_elem);
    erase_process(child_process);
    return -1;
  }

  if (!child_process->process_alive) {//process already exited
    int exit_status = child_process->return_val;
    list_remove(&child_process->children_elem);
    erase_process(child_process);
    return exit_status;
  }
  //process did not exit
  child_process->parent_waited = true;
  sema_down(child_process->waiting_forme);
  return child_process->return_val;
}

/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *cur = thread_current ();
  //get supposed pid
  pid_t pid = cur->tid;
  //get corresponding process
  struct process* current_process = get_process_by_id(pid);
  if (current_process == NULL 
        || !current_process->process_alive) {//no process found or already exited
    return;//error
  }
  //tell children that their parent exited
  inform_parent_exit(&current_process->children);

  //process dies
  current_process->process_alive = false;
  
  //notify the parent, incase it is waiting
  struct semaphore* waiter = current_process->waiting_forme;

  //deallocate the process if necessary
  if (current_process->parent_exited) {//no need to keep it
                                    //we donot need its return value  
    erase_process(current_process);
  }

  finish_file();
  
  sema_up(waiter);
  free(waiter);
  


  uint32_t *pd;

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  pd = cur->pagedir;
  if (pd != NULL) 
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

//static bool setup_stack (void **esp, char ** tokens, int tokens_count);
static bool setup_stack (void **esp, char** tokens);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (const char *file_name, void (**eip) (void), void **esp, char** tokens, int tokens_count) 
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  int i;

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL) 
    goto done;
  process_activate ();

  /* Open executable file. */
  file = filesys_open (file_name);


  if (file == NULL) 
    {
      printf ("load: %s: open failed\n", file_name);
      goto done; 
    }

  //if file was loaded successfully, then deny writing to it
  file_deny_write(file);
  thread_current()->associated_file = file;


  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024) 
    {
      printf ("load: %s: error loading executable\n", file_name);
      goto done; 
    }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++) 
    {
      struct Elf32_Phdr phdr;

      if (file_ofs < 0 || file_ofs > file_length (file))
        goto done;
      file_seek (file, file_ofs);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofs += sizeof phdr;
      switch (phdr.p_type) 
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file)) 
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else 
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
            }
          else
            goto done;
          break;
        }
    }

  /* Set up stack. */
  if (!setup_stack (esp, tokens))
    goto done;

  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;

 success = true;
  

 done:
  /* We arrive here whether the load is successful or not. */
 
  return success;
}

/* load() helpers. */

static bool install_page (void *upage, void *kpage, bool writable);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file) 
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK)) 
    return false; 

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file)) 
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz) 
    return false; 

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;
  
  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) 
{
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0) 
    {
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      /* Get a page of memory. */
      uint8_t *kpage = palloc_get_page (PAL_USER);
      if (kpage == NULL)
        return false;

      /* Load this page. */
      if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
        {
          palloc_free_page (kpage);
          return false; 
        }
      memset (kpage + page_read_bytes, 0, page_zero_bytes);

      /* Add the page to the process's address space. */
      if (!install_page (upage, kpage, writable)) 
        {
          palloc_free_page (kpage);
          return false; 
        }

      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
    }
  return true;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack (void **esp, char** tokens) 
{
  uint8_t *kpage;
  bool success = false;

  kpage = palloc_get_page (PAL_USER | PAL_ZERO);
  if (kpage != NULL) 
    {
      success = install_page (((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true);
      if (success) {
        *esp = PHYS_BASE;
        push_to_stack(esp, tokens);
      }
      else
        palloc_free_page (kpage);
    }

  
  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}


struct list_elem* initialize_process(pid_t pid, char* process_name, bool parent_exited)
{
  struct process* created_process = (struct process*) malloc(sizeof(struct process));
  created_process->pid = pid;
  list_init(&created_process->children);
  created_process->parent_exited = parent_exited;
  created_process->return_val = 0;
  created_process->process_alive = true;
  created_process->name = process_name;
  created_process->parent_waited = false;
  created_process->fd = -1;

  created_process->waiting_forme = (struct semaphore*)malloc(sizeof(struct semaphore));
  sema_init(created_process->waiting_forme, 0);
  return &created_process->all_elem;
}

struct process* get_process_by_id(pid_t pid)
{
  struct list_elem *e;

  for (e = list_begin (&all_processes); e != list_end (&all_processes);
       e = list_next (e))
    {
      struct process *p = list_entry (e, struct process, all_elem);
      if (p->pid == pid) {
        return p;
      }
    }
    return NULL;
}

//sets parent_exited for all list elements to true
void inform_parent_exit(struct list* children)
{
  struct list_elem *e;

  for (e = list_begin (children); e != list_end (children);
       e = list_next (e))
    {
      struct process *p = list_entry (e, struct process, children_elem);
      p->parent_exited = true;
    }
}

//removes all elements form the list
void remove_all(struct list* l)
{
  struct list_elem *e;

  //remove all
  for (e = list_begin (l); e != list_end (l); e = list_remove (e));
}

void erase_process(struct process* proc)
{
  list_remove(&proc->all_elem);
  //remove children from list as part of its deallocation
  remove_all(&proc->children);
  free(proc->name);
  free(proc);
}
