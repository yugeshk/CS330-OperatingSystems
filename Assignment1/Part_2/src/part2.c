//Name : Yugesh Kothari Roll : 170830
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<sys/types.h>

pid_t pid;
int pipe1[2];
int pipe2[2];

#define BUFF_SIZE 1024

/* methods */
void exec_grep(char **);
void exec_wc(char **);
void exec_tee(char **);
void exec_command(int, char **);

void exec_grep(char *argv[]){
    //input from stdin in argv (taken with command)
    //output goes to pipe1
    dup2(pipe1[1], 1);
    //close file descriptors
    
    close(pipe1[0]);
    close(pipe1[1]);

    //exec grep -rf string path
    execlp("grep", "grep", "-rF", argv[2], argv[3], (char *)NULL);

    //exec failed
    perror("couldnt call grep");
    _exit(1);
}

void exec_wc(char *argv[]){
    //exec_grep has already written to the output side of pipe1
    //we take that input from pipe1
    dup2(pipe1[0], 0);

    //we have to output to stdout so we leave fd 1 as is
    //close fds
    close(pipe1[0]);
    close(pipe1[1]);

    //exec wc -l
    execlp("wc", "wc", "-l", (char *)NULL);

    //exec failed
    perror("couldnt call wc");
    _exit(1);
}

void exec_tee(char *argv[]){
    //exec_grep has already written to the output side of pipe1
    //we take input from pipe1 and output to pipe2
    dup2(pipe1[0], 0);
    dup2(pipe2[1], 1);

    //close fds
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    int o_fd = open(argv[4], O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
    char buff[BUFF_SIZE+1];
    int bytes_read;
    /* write() */
    while((bytes_read = read(0, buff, BUFF_SIZE)) > 0) {
        if(write(o_fd, buff, bytes_read) != bytes_read) {
            perror("Couldn't write all bytes to file");
            _exit(1);
        }

        /* fprintf(stderr, "%s", buff); */
        if(write(1, buff, bytes_read) != bytes_read) {
            perror("Couldn't write all bytes to pipe2");
            _exit(1);
        }

    }

    _exit(1);

}

void exec_command(int argc, char *argv[]){
    //exec_tee has already written to output side of pipe2
    //we take input from pipe2 and output to stdout
    dup2(pipe2[0], 0);

    //we have to output to stdout so we leave fd 1 as is
    //close fds
    close(pipe2[0]);
    close(pipe2[1]);
    /* close(pipe1[1]); */

    char *cmd[argc-4];
    for(int i=0;i<argc-5;i++){
        cmd[i] = argv[i+5];
    }
    cmd[argc-5] = (char *)(NULL);

    //exec command
    execvp(cmd[0], cmd);
    /* execlp("cat", "cat"); */

    //exec failed
    perror("couldnt call command");
    _exit(1);
}

void handle_at(char *argv[]){
    
    /* printf("Looking for %s in %s", argv[2], argv[3]); */
    //create pipe1
    if(pipe(pipe1) == -1){
        perror("piping error pipe1");
        exit(1);
    }

    // grep -rF string path
    if((pid = fork()) == -1){
        perror("forking error");
        exit(1);
    }
    else if(pid == 0){
        //stdin -> grep -> pipe1
        exec_grep(argv);
    }
    //parent

    // wc -l
    if((pid = fork()) == -1){
        perror("forking error");
        exit(1);
    }
    else if(pid == 0){
        //pipe1 -> wc -> stdout
        exec_wc(argv);
    }
    //parent

    //close unused file desciptors
    close(pipe1[0]);
    close(pipe1[1]);

}

void handle_dollar(int argc, char *argv[]){

    /* printf("Looking for %s in %s", argv[2], argv[3]); */
    //create pipe1
    if(pipe(pipe1)==-1){
        perror("piping error pipe1\n");
        exit(1);
    }

    //grep -rF string path
    if((pid = fork()) == -1){
        perror("forking error\n");
        exit(1);
    }
    else if(pid == 0){
        // stdin -> grep -> pipe1
        exec_grep(argv);
    }
    //parent
    
    //create pipe2
    if(pipe(pipe2)==-1){
        perror("piping error pipe2\n");
        exit(1);
    }

    //unused file desciptors
    close(pipe1[1]);
    
    //tee output_file
    if((pid = fork()) == -1){
        perror("forking error");
        exit(1);
    }
    else if(pid == 0){
        //pipe1 -> tee -> pipe2 and output_file
        exec_tee(argv);
    }
    //parent
    
    //unused file desciptors
    close(pipe1[0]);
    close(pipe2[1]);

    //third command
    if((pid = fork()) == -1){
        perror("forking error in third command\n");
        exit(1);
    }

    else if(pid == 0){
        //pipe2 -> command -> stdout
        exec_command(argc, argv);
    }
    //parent
    
    //unused file desciptors
    close(pipe2[0]);


}

int main(int argc, char *argv[]){
    char op;
    char *string, *path;
    if (argc < 4){
        printf("Incorrent usage. Use - ./part2 op string path\n");
        return 0;
    }

    struct stat stt;
    if(stat(argv[3], &stt)==0){
        op = *argv[1];
        switch(op){
            case '@':
                if (argc != 4){
                    printf("Incorrect number of arguements for @\n");
                    return 0;
                }
                handle_at(argv);
                break;
            case '$':
                if (argc < 6){
                    printf("Incorrect number of arguements for $\n");
                    return 0;
                }
                handle_dollar(argc, argv);
                break;
            default:
                printf("Incorrect operator. please use @/$\n");
                exit(1);
        }
        return 1;
    }
    else{
        perror("stat()\n");
        exit(1);
    }

}
