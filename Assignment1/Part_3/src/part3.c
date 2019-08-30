//Name : Yugesh Kothari Roll : 170830
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<dirent.h>
#include<libgen.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<sys/types.h>

pid_t pid;
struct stat sttt;

/* methods */
int process_count(char *);
void calulate_size_recursively(char *, int);

//Redundant method - was implemented for multi-threading.
//Counts the number of immediate sub-directories for a given directory
int process_count(char *path){
    char new_path[100];
    struct dirent *dp;
    DIR *dir = opendir(path);
    DIR *tmpdir;

    if(!dir){
        return 0;
    }

    int n=0;

    while((dp = readdir(dir))!=NULL){
        if(strcmp(dp->d_name, ".")!=0 && strcmp(dp->d_name, "..")!=0){
            sprintf(new_path, "%s/%s", path, dp->d_name);
            if((tmpdir = opendir(new_path)) != NULL){
                n++;
            }    
        }
    }

    return n;
}

//Goes through a directory recursively and computes the total size
int calculate_recursively_subdirectory(char *path){
    char new_path[1000]; //Assume path size does not cross 1000
    struct dirent *dp;
    DIR *dir = opendir(path);
    DIR *tmpdir;
    long long size=0;
    
    if(!dir){
        return 0;
    }

    while((dp = readdir(dir))!=NULL){
        if(strcmp(dp->d_name, ".")!=0 && strcmp(dp->d_name, "..")!=0){
            sprintf(new_path, "%s/%s", path, dp->d_name);
            if(stat(new_path, &sttt)==0){
                if(S_ISDIR(sttt.st_mode)){
                    size+=calculate_recursively_subdirectory(new_path);
                }
                else if(S_ISREG(sttt.st_mode)){
                    size+=sttt.st_size;
                }
                else{
                    perror("Not a directory or regular file. Unrecognized type\n");
                    return 0;
                }
            }
            else{
                perror("Unable to open file/directory\n");
                return 0;
            }
        }
    }

    return size;
}

//This is the call for the root directory
void calulate_size_recursively(char *path, int n_processes){
    char new_path[1000];
    struct dirent *dp;
    DIR *dir = opendir(path);
    DIR *tmpdir;
    long long root_size=0;
    int pipe1[2];

    if(!dir){
        return;
    }

    while((dp = readdir(dir))!=NULL){
        if(strcmp(dp->d_name, ".")!=0 && strcmp(dp->d_name, "..")!=0){
            sprintf(new_path, "%s/%s", path, dp->d_name);
            if(stat(new_path, &sttt)==0){
                if(S_ISDIR(sttt.st_mode)){
                    //create a pipe
                    if(pipe(pipe1)==-1){
                        perror("Cound not create pipe1\n");
                        exit(1);
                    }
                    // stdin should be closed so that we can read from pipe
                    dup2(pipe1[0], 0);
                    char buf[32];

                    if((pid = fork()) == -1){
                        perror("forking error\n");
                        exit(1);
                    }
                    else if(pid == 0){
                        //input will be passed. output has to go to output of pipe1

                        dup2(pipe1[1], 1);
                        //close other fds
                        close(pipe1[0]);
                        close(pipe1[1]);

                        int p = calculate_recursively_subdirectory(new_path);
                        sprintf(buf, "%d", p);
                        write(1, buf, sizeof(int));
                        _exit(1);
                    }

                    //parent
                    wait(NULL);
                    int p;
                    read(0, buf, sizeof(int));
                    sscanf(buf, "%d", &p);
                    printf("%s %d\n", basename(new_path), p);
                    root_size+=p;
                }
                else if(S_ISREG(sttt.st_mode)){
                    root_size+=sttt.st_size;
                }
            }
            else{
                perror("Unable to open file/directory\n");
                exit(1);
            }
        }
    }

    printf("%s %d\n", basename(path), root_size);
}


int main(int argc, char *argv[]){

    if (argc != 2){
        printf("Incorrect usage. Use ./a.out path_to_dir\n");
        exit(1);
    }
    
    struct stat stt;
    if(stat(argv[1], &stt)==0){
        char *file = argv[1];
        if(stt.st_mode & S_IFMT & S_IFREG){
            printf("%s %d\n", argv[1], stt.st_size);
        }
        else{
            int n_processes = process_count(argv[1]);
            calulate_size_recursively(argv[1], n_processes);
        }
    }
    else{
        perror("Unable to open directory/file. Please check\n");
        exit(1);
    }
}
