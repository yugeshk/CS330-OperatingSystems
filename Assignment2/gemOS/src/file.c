#include<types.h>
#include<context.h>
#include<file.h>
#include<lib.h>
#include<serial.h>
#include<entry.h>
#include<memory.h>
#include<fs.h>
#include<kbd.h>
#include<pipe.h>


/************************************************************************************/
/***************************Do Not Modify below Functions****************************/
/************************************************************************************/
void free_file_object(struct file *filep)
{
    if(filep)
    {
       os_page_free(OS_DS_REG ,filep);
       stats->file_objects--;
    }
}

struct file *alloc_file()
{
  
  struct file *file = (struct file *) os_page_alloc(OS_DS_REG); 
  file->fops = (struct fileops *) (file + sizeof(struct file)); 
  bzero((char *)file->fops, sizeof(struct fileops));
  stats->file_objects++;
  return file; 
}

static int do_read_kbd(struct file* filep, char * buff, u32 count)
{
  kbd_read(buff);
  return 1;
}

static int do_write_console(struct file* filep, char * buff, u32 count)
{
  struct exec_context *current = get_current_ctx();
  return do_write(current, (u64)buff, (u64)count);
}

struct file *create_standard_IO(int type)
{
  struct file *filep = alloc_file();
  filep->type = type;
  if(type == STDIN)
     filep->mode = O_READ;
  else
      filep->mode = O_WRITE;
  if(type == STDIN){
        filep->fops->read = do_read_kbd;
  }else{
        filep->fops->write = do_write_console;
  }
  filep->fops->close = generic_close;
  filep->ref_count = 1;
  return filep;
}

int open_standard_IO(struct exec_context *ctx, int type)
{
   int fd = type;
   struct file *filep = ctx->files[type];
   if(!filep){
        filep = create_standard_IO(type);
   }else{
         filep->ref_count++;
         fd = 3;
         while(ctx->files[fd])
             fd++; 
   }
   ctx->files[fd] = filep;
   return fd;
}
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/



void do_file_fork(struct exec_context *child)
{
   /*TODO the child fds are a copy of the parent. Adjust the refcount*/
    for(int i=0;i<MAX_OPEN_FILES;i++){
        if(child->files[i]){
            child->files[i]->ref_count++;
        }
    }
 
}

void do_file_exit(struct exec_context *ctx)
{
   /*TODO the process is exiting. Adjust the ref_count
     of files*/
    for(int i=0;i<MAX_OPEN_FILES;i++){
        if(ctx->files[i]){
            generic_close(ctx->files[i]);
            ctx->files[i]=NULL;
        }
    }
}

long generic_close(struct file *filep)
{
  /** TODO Implementation of close (pipe, file) based on the type 
   * Adjust the ref_count, free file object
   * Incase of Error return valid Error code 
   * 
   */
    int ret_fd = -EINVAL; 
    if(filep == NULL){
        return ret_fd;
    }
    if(--(filep->ref_count)){
      return filep->ref_count;
    }
    else{
        if(filep->type==REGULAR){
            free_file_object(filep);
        }
        else if(filep->type==PIPE){
            if(filep->mode == O_READ){
                filep->pipe->is_ropen = 0;
                if(filep->pipe->is_wopen == 0){
                    free_pipe_info(filep->pipe);
                }
            }
            else if(filep->mode == O_WRITE){
                filep->pipe->is_wopen = 0;
                if(filep->pipe->is_ropen == 0){
                    free_pipe_info(filep->pipe);
                }
            }
            free_file_object(filep);
        }
        else{
            free_file_object(filep);
        }
        return 0;
    }
    return ret_fd;
}

static int do_read_regular(struct file *filep, char * buff, u32 count)
{
   /** TODO Implementation of File Read, 
    *  You should be reading the content from File using file system read function call and fill the buf
    *  Validate the permission, file existence, Max length etc
    *  Incase of Error return valid Error code 
    * */
    int ret_fd = -EINVAL; 
    if(filep == NULL || buff == NULL){
        return ret_fd;
    }
    if(filep->mode&0x1){
        int bytes_read = flat_read(filep->inode, buff, count, &(filep->offp));
        filep->offp+=bytes_read;
        return bytes_read;
    }
    else{
        return -EACCES;
    }
    return ret_fd;
}


static int do_write_regular(struct file *filep, char * buff, u32 count)
{
    /** TODO Implementation of File write, 
    *   You should be writing the content from buff to File by using File system write function
    *   Validate the permission, file existence, Max length etc
    *   Incase of Error return valid Error code 
    * */
    int ret_fd = -EINVAL; 
    if(filep == NULL || buff == NULL){
        return ret_fd;
    }
    if(filep->mode&0x2){
        int bytes_written = flat_write(filep->inode, buff, count, &(filep->offp));
        if( bytes_written < 0 ){
            filep->offp+=bytes_written;
        }
        else{
            return -EINVAL;
        }
        return bytes_written;
    }
    else{
        return -EACCES;
    }
    return ret_fd;
}

static long do_lseek_regular(struct file *filep, long offset, int whence)
{
    /** TODO Implementation of lseek 
    *   Set, Adjust the ofset based on the whence
    *   Incase of Error return valid Error code 
    * */
    int ret_fd = -EINVAL; 
    if(filep == NULL){
        return ret_fd;
    }
    unsigned int offp;
    if(whence==SEEK_SET){
        offp=0; 
    } 
    else if(whence==SEEK_CUR){
        offp=filep->offp;
    }
    else if(whence==SEEK_END){
        offp=filep->inode->file_size;
    }
    else{
        return ret_fd;
    }
    offp+=offset;
    if(offp >= FILE_SIZE || offp < 0){
        return ret_fd;
    }
    filep->offp = offp;
    return filep->offp;
}

extern int do_regular_file_open(struct exec_context *ctx, char* filename, u64 flags, u64 mode)
{ 
  /**  TODO Implementation of file open, 
    *  You should be creating file(use the alloc_file function to creat file), 
    *  To create or Get inode use File system function calls, 
    *  Handle mode and flags 
    *  Validate file existence, Max File count is 32, Max Size is 4KB, etc
    *  Incase of Error return valid Error code 
    * */
    int ret_fd = -EINVAL; 

    struct inode *file_inode;
    if(file_inode=lookup_inode(filename)){
       //Check if user has permissions to open
       if((flags&file_inode->mode&0x7)==(flags&0x7)){
           struct file* file_ptr = alloc_file();
           file_ptr->type = REGULAR;
           file_ptr->mode = flags;
           file_ptr->offp = 0;
           file_ptr->ref_count = 1;
           file_ptr->inode = file_inode;
           file_ptr->fops->read = do_read_regular;
           file_ptr->fops->write = do_write_regular;
           file_ptr->fops->lseek = do_lseek_regular;
           file_ptr->fops->close = generic_close;
           file_ptr->pipe = NULL;
           int fd=3;
           while(ctx->files[fd]){
              fd++; 
           }
           ctx->files[fd]=file_ptr;
           return fd;
       }
       else{
           return -EACCES; 
       }
    }
    else{
        if((flags&O_CREAT)==O_CREAT){
            file_inode = create_inode(filename, mode);
            /* if((flags&file_inode->mode&0x7)==(flags&0x7)){ */
            struct file* file_ptr = alloc_file();
            file_ptr->type = REGULAR;
            file_ptr->mode = flags;
            file_ptr->offp = 0;
            file_ptr->ref_count = 1;
            file_ptr->inode = file_inode;
            file_ptr->fops->read = do_read_regular;
            file_ptr->fops->write = do_write_regular;
            file_ptr->fops->lseek = do_lseek_regular;
            file_ptr->fops->close = generic_close;
            file_ptr->pipe = NULL;
            int fd=3;
            while(ctx->files[fd]){
               fd++; 
               if(fd==MAX_OPEN_FILES){
                 return -EOTHERS;
               }
            }
            ctx->files[fd]=file_ptr;
            return fd;
            /* } */
            /* else{ */
            /*    return -EACCES; */
            /* } */
        }    
        else{
            return -EINVAL;
        }
    }
    return ret_fd;
}

int fd_dup(struct exec_context *current, int oldfd)
{
     /** TODO Implementation of dup 
      *  Read the man page of dup and implement accordingly 
      *  return the file descriptor,
      *  Incase of Error return valid Error code 
      * */
    int ret_fd = -EINVAL; 
    if(oldfd >= MAX_OPEN_FILES){
        return -EOTHERS;
    }
    if(current->files[oldfd]){
        int new_fd=3;
        while(current->files[new_fd]){
            new_fd++;
            //check for maximum open files limit
            if(new_fd == MAX_OPEN_FILES){
                return -EOTHERS;
            }
        }
        current->files[new_fd]=current->files[oldfd];
        current->files[new_fd]->ref_count++;
        return new_fd;
    }
    else{
        return -EINVAL;
    }
    return ret_fd;
}


int fd_dup2(struct exec_context *current, int oldfd, int newfd)
{
  /** TODO Implementation of the dup2 
    *  Read the man page of dup2 and implement accordingly 
    *  return the file descriptor,
    *  Incase of Error return valid Error code 
    * */
    int ret_fd = -EINVAL; 
    if(oldfd >= MAX_OPEN_FILES || newfd >= MAX_OPEN_FILES){
        return -EOTHERS;
    }
    if(current->files[oldfd]){
        if(current->files[newfd]&&oldfd!=newfd){
            current->files[newfd]->fops->close(current->files[newfd]);
        }
        current->files[newfd]=current->files[oldfd];
        current->files[oldfd]->ref_count++;
        return newfd;
    }                                
    else{
        return -EINVAL;
    }
    return ret_fd;
}
