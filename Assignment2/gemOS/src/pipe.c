#include<pipe.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>
/***********************************************************************
 * Use this function to allocate pipe info && Don't Modify below function
 ***********************************************************************/
struct pipe_info* alloc_pipe_info()
{
    struct pipe_info *pipe = (struct pipe_info*)os_page_alloc(OS_DS_REG);
    char* buffer = (char*) os_page_alloc(OS_DS_REG);
    pipe ->pipe_buff = buffer;
    return pipe;
}


void free_pipe_info(struct pipe_info *p_info)
{
    if(p_info)
    {
        os_page_free(OS_DS_REG ,p_info->pipe_buff);
        os_page_free(OS_DS_REG ,p_info);
    }
}
/*************************************************************************/
/*************************************************************************/


int pipe_read(struct file *filep, char *buff, u32 count)
{
    /**
    *  TODO:: Implementation of Pipe Read
    *  Read the contect from buff (pipe_info -> pipe_buff) and write to the buff(argument 2);
    *  Validate size of buff, the mode of pipe (pipe_info->mode),etc
    *  Incase of Error return valid Error code 
    */
    int ret_fd = -EINVAL; 
    if (filep == NULL || count < 0 || buff == NULL){
        return ret_fd;
    }
    if((filep->mode&0x1)&&(filep->pipe->is_ropen)){
        int s_pos = filep->pipe->read_pos;
        int len = count>filep->pipe->buffer_offset?filep->pipe->buffer_offset:count;
        if(len>4096){
            return -EOTHERS;
        }
        if(len <= 0){
            return 0;
        }
        for(int i=0;i<len;i++){
            buff[i]=filep->pipe->pipe_buff[(i+s_pos)%4096];
        }
        filep->pipe->read_pos=(s_pos+len)%4096;
        filep->pipe->buffer_offset-=len;
        return len;
    }
    
    return ret_fd;
}


int pipe_write(struct file *filep, char *buff, u32 count)
{
    /**
    *  TODO:: Implementation of Pipe Read
    *  Write the contect from   the buff(argument 2);  and write to buff(pipe_info -> pipe_buff)
    *  Validate size of buff, the mode of pipe (pipe_info->mode),etc
    *  Incase of Error return valid Error code 
    */
    int ret_fd = -EINVAL; 
    if (filep == NULL || count < 0 || buff == NULL){
        return ret_fd;
    }
    if((filep->mode&0x2)&&(filep->pipe->is_wopen)){
        int s_pos = filep->pipe->write_pos;
        if(filep->pipe->buffer_offset+count > 4096){
            return -EOTHERS;
        }

        for(int i=0;i<count;i++){
            filep->pipe->pipe_buff[(i+s_pos)%4096]=buff[i];
        }
        
        filep->pipe->buffer_offset+=count;
        filep->pipe->write_pos=(count+s_pos)%4096;
        return count;
    }
    return ret_fd;
}

int create_pipe(struct exec_context *current, int *fd)
{
    /**
    *  TODO:: Implementation of Pipe Create
    *  Create file struct by invoking the alloc_file() function, 
    *  Create pipe_info struct by invoking the alloc_pipe_info() function
    *  fill the valid file descriptor in *fd param
    *  Incase of Error return valid Error code 
    */
    int ret_fd = -EINVAL; 

    if (current == NULL || fd == NULL){
        return ret_fd;
    }

    struct file* file_0 = alloc_file();
    struct file* file_1 = alloc_file();
    struct pipe_info *mypipe = alloc_pipe_info();
    file_0->type = PIPE;
    file_0->mode = O_READ;
    file_0->offp=0;
    file_0->ref_count=1;
    file_0->inode = NULL;
    file_0->fops->read = pipe_read;
    file_0->fops->close = generic_close;
    file_0->pipe=mypipe;
    file_0->pipe->read_pos=0;
    file_0->pipe->write_pos=0;
    file_0->pipe->is_ropen=1;
    file_1->type = PIPE;
    file_1->mode = O_WRITE;
    file_1->offp=0;
    file_1->ref_count=1;
    file_1->inode = NULL;
    file_1->fops->write = pipe_write;
    file_1->fops->close = generic_close;
    file_1->pipe=mypipe;
    file_1->pipe->read_pos=0;
    file_1->pipe->write_pos=0;
    file_1->pipe->is_wopen=1;

    fd[0]=0;
    while(current->files[fd[0]]){
        fd[0]++;
    }
    current->files[fd[0]]=file_0;
    fd[1]=fd[0];
    while(current->files[fd[1]]){
        fd[1]++;
    }
    current->files[fd[1]]=file_1;
    if(fd[0]>=16||fd[1]>=16){
        return -EINVAL;
    }

    return 1;
    return ret_fd;
}

