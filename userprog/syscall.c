#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include <stdbool.h>
#include "threads/init.h"
#include <devices/input.h>
#include "threads/vaddr.h"
#include "threads/synch.h"
#include <list.h>
#include "lib/user/syscall.h"
#include "threads/malloc.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"






void halt(void) NO_RETURN;
bool create(const char * file, unsigned initial_size);
int open (const char *file);
int write (int fd, const void *buffer, unsigned size);
int read (int fd, void *buffer, unsigned size);
void close (int fd);
void exit(int status) NO_RETURN;
int wait(pid_t pid);
pid_t exec (const char *cmd_line);
int check_string( char *string);
int check_params(int *esp,int bytes);
void seek (int fd, unsigned position);
unsigned tell (int fd);
int filesize (int fd) ;
bool remove (const char *file_name);
void seek (int fd, unsigned position);
unsigned tell (int fd);
bool remove (const char *file_name) ;
static void syscall_handler (struct intr_frame *);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


int check_params(int *esp,int bytes){
   char* esp_t = (char*) esp;
   if(!esp_t ) return 0;
   for(int i=0;i<=bytes;i++){
      if(is_kernel_vaddr(esp_t+i)) 
         return 0;
      if( !pagedir_get_page(thread_current()->pagedir,esp_t+i))
         return 0;
   }
   return 1;
}

int check_string(char *string){
   if(!string ) return 0;
   char* temp = string;
   while(1) {
      if(is_kernel_vaddr(temp) || !pagedir_get_page(thread_current()->pagedir,temp)) 
      return 0;
      else if(*temp == '\0') return 1;
      temp++;
   }
   return 1;
}

static void
syscall_handler(struct intr_frame * f UNUSED) {
	
  char *name;
  unsigned *length;
  int *const ptr = f->esp;
  int* fd ;
  int* status;
  void* buffer;
  const char* charbuff;
  
  if(!check_params(ptr,4)) exit(-1);
  
  switch (*ptr) {

    case SYS_HALT:
     if(!check_params(ptr,4)) exit(-1);
      halt();
      break;
  
    case SYS_CREATE:
      if(!check_params(ptr,12)) exit(-1);
      length = (unsigned*)(ptr + 2);
      name = (char*)*(ptr+1);
      
      if (! is_user_vaddr (name))
         f->eax = -1;
      else 
         f->eax = (uint32_t)create(name, *length);
      break;
      
        
    case SYS_OPEN:
      if(!check_params(ptr,8)) exit(-1);
      name = (char*)*(ptr+1);
      f->eax = (uint32_t) open(name);
      break;
      
    case SYS_CLOSE:
      if(!check_params(ptr,8)) exit(-1);
      fd = ptr + 1;
      close(*fd);
      break;
        
    case SYS_READ:
      if(!check_params(ptr,16)) exit(-1);
      fd = ptr + 1;

      buffer = (void*)*(ptr+2);
      length = (unsigned*)ptr+3;
      f->eax = (uint32_t) read(*fd, buffer, *length);
      break;
      
    case SYS_WRITE:
      if(!check_params(ptr,16)) exit(-1);
      fd = ptr + 1;
      buffer = (void*)*(ptr+2);
      length = (unsigned*)ptr+3;
      f->eax =(uint32_t) write(*fd,buffer,*length);
      break;
      
    case SYS_EXIT:
      if(!check_params(ptr,8)) exit(-1);
      status = ptr+1;
      exit(*status);
      break;
      
    case SYS_FILESIZE: 
       if(!check_params(ptr,8)) exit(-1);
      fd = ptr + 1;
      f->eax = filesize(*fd);
      break;
   
    case SYS_SEEK:
     if(!check_params(ptr,12)) exit(-1);
     fd = ptr + 1;
     length = (unsigned*)ptr+2;
     seek(*fd,*length);
    break;
    
    case SYS_TELL:
    if(!check_params(ptr,8)) exit(-1);
      fd = ptr + 1;
    f->eax = tell(*fd);
    break;
    
    case SYS_REMOVE:
     if(!check_params(ptr,8)) exit(-1);
      charbuff = (const char*)*(ptr+1);
     f->eax = remove(charbuff);
    break;
    
    case SYS_WAIT:
    
      if(!check_params(ptr,8)) exit(-1);
      f->eax = (uint32_t)wait(*((pid_t*)(ptr+1)));
      break;
    
    case SYS_EXEC:
      if(!check_params(ptr,8)) exit(-1);
      charbuff = (const char*)*(ptr+1);
      f->eax = (uint32_t)exec(charbuff);
      break;
    
    default:
      break;
  }
}

  void halt(void) {
    power_off();
  }

    
  bool create(const char * file, unsigned initial_size) {
   if(!check_string((char *)file)) exit(-1);
   return filesys_create(file,initial_size);
  }
  
    
  int open (const char *file){
    if(!check_string((char *)file))
        exit(-1);
     
    struct thread* current = thread_current();
    int actualfd = pop(&(current->fdDisp));
    if(actualfd == -1)
      return -1;

    struct file * openedFile = filesys_open(file);
    if(openedFile == NULL){
      push(&(current->fdDisp), actualfd);
      return -1;
    }
    current->fileDescriptors[actualfd-2] = openedFile;
    return actualfd;
  }


  int read (int fd, void *buffer, unsigned size) {
    int bytes_read = -1;
    if(!check_params(buffer,size)) {
       exit(-1);
    }
    if (fd == 0) {
      uint8_t* index = buffer;
      for (unsigned i = 0; i < size; i ++) {
        *index = input_getc ();
        index ++;
      }
      return size;
    }
    
    if(fd >= 2 && fd <= 129){
      struct thread* current = thread_current();
      struct file *read_file = current->fileDescriptors[fd-2];
      if (read_file != NULL)
        bytes_read = file_read (read_file, buffer, size);
    }
		return bytes_read;
	}
	
  
	void close (int fd){
		if(fd >= 2 && fd <= 129){
			struct thread* current = thread_current();
			struct file* closed = current->fileDescriptors[fd-2];
			if(closed != NULL){
				file_close(closed);
				current->fileDescriptors[fd-2] = NULL;
				push(&(current->fdDisp), fd);
			}			
		}
	}
  
	
	int write (int fd, const void *buffer, unsigned size) {
		struct thread *t = thread_current ();
      if(!check_params((int*)buffer,size)) exit(-1);

      
		if (fd > 129 || fd < 1)
			return -1;
      
		if (fd == 1) {
         //print to stdout with multiple putbuf if buffer is too big
         int i = 0;
         for (; size > 200; i ++) {
            putbuf (buffer + i * 200, 200);
            size -= 200;
         }
         putbuf (buffer + i * 200, size);
			return size;
		} 
    
		if(!(t->fileDescriptors[fd-2]))
			return -1;
		return file_write (t->fileDescriptors[fd-2], buffer, size);
	}


  void exit(int status) { 
     struct thread *t = thread_current();
     if (t->parent ) {
        lock_acquire(&(t->shared_parent->family_count_lock));
        
        //check if parent has died
        bool parent_died = t->shared_parent->p_died;
        
        if(!parent_died){
         struct list_elem *e;
         lock_acquire(&(t->parent->child_status_lock));

          for (e = list_begin (&(t->parent->child_status_list)); e != list_end(&(t->parent->child_status_list));
              e = list_next (e)) {
                 
             struct child_status *f = list_entry (e, struct child_status, elem);
             
             if (f->c_tid == thread_current()->tid){   
                f->status = status;
                sema_up(&(f->exit_sema));
                break;
             }
          }
          lock_release(&(t->parent->child_status_lock));
       }
      lock_release(&(t->shared_parent->family_count_lock));
    }
    printf("%s: exit(%d)\n", t->name, status);
    thread_exit();
  }
  
  int wait(pid_t pid) {
    return process_wait((tid_t)pid);;
  }
  
  
  pid_t exec (const char *cmd_line) {
    if(!check_string((char *)cmd_line)) exit(-1);     
    return process_execute(cmd_line);
  }
  
  int filesize(int fd){
     if (fd > 129 || fd <= 1)
			return -1;
      struct thread* current = thread_current();
      struct file *read_file = current->fileDescriptors[fd-2];
      if (read_file == NULL) return -1;
        return (int) file_length(read_file);
  }
  
  void seek (int fd, unsigned position) {
       if (fd > 129 || fd <= 1)
			return;
      struct thread* current = thread_current();
      struct file *read_file = current->fileDescriptors[fd-2];
      if (read_file == NULL) return;
      return file_seek(read_file,(off_t) position);
  }
  
  unsigned tell (int fd){
      if (fd > 129 || fd <= 1)
			return -1;
      struct thread* current = thread_current();
      struct file *read_file = current->fileDescriptors[fd-2];
      if (read_file == NULL) return -1;
      return file_tell(read_file);
  } 
  
  bool remove (const char *file_name) {
     if(!check_string((char *)file_name)) exit(-1);  
     return filesys_remove(file_name);
  }



