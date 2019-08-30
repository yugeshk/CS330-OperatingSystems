//Name : Yugesh Kothari Roll : 170830

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>
#include<dirent.h>

//Reads a line from the file pointer
char* read_line(FILE *fptr){
    char *buf;
    char *tmp;
    int read_chars = 0;
    //initial buffer size. Will be dynamically increased as per use
    int bufsize = 1024;
    char *line = malloc(bufsize);

    if( !line ){
        return NULL;
    }

    buf = line;


    while( fread(buf, 1, 1, fptr)!=0 ){
        read_chars++;
        buf=buf+1;
        if(line[read_chars-1]=='\n'){
            line[read_chars-1]='\0';
            return line;
        }

        else if (read_chars == bufsize){
            bufsize = 2*bufsize;
            tmp = realloc(line, bufsize);
            if(tmp){
                line = tmp;
                buf = line + read_chars;
            }
            else{
               free(line);
               return NULL;
            }
        }
    }

    return NULL;
}

void pattern_match(char *patt, char *path){
    FILE *fptr;
    char *line;

    fptr = fopen(path, "r+");
    //set end of file to '\n'
    char ch;
    fseek(fptr, -1, SEEK_END);
    ch = fgetc(fptr);
    if(ch!='\n'){
        fputc('\n', fptr);
    }
    fseek(fptr, 0, SEEK_SET);

    if (fptr){
        while( line = read_line(fptr)){
            if(strstr(line, patt)){
                printf("%s:%s\n", path, line);
            }
            free(line);
        }
        
    }
}

void pattern_match_file(char *patt, char *path){
    FILE *fptr;
    char *line;

    fptr = fopen(path, "r+");
    //set end of file to '\n'
    char ch;
    fseek(fptr, -1, SEEK_END);
    ch = fgetc(fptr);
    if(ch!='\n'){
        fputc('\n', fptr);
    }
    fseek(fptr, 0, SEEK_SET);

    if (fptr){
        while( line = read_line(fptr)){
            if(strstr(line, patt)){
                printf("%s\n", line);
            }
            free(line);
        }
        
    }
}

void mygrep(char *patt, char *path){
    char new_path[1000];
    struct dirent *dp;
    DIR *dir = opendir(path);
    struct stat stt;

    if(!dir){
        return;
    }

    while((dp = readdir(dir))!=NULL){
        if(strcmp(dp->d_name, ".")!=0 && strcmp(dp->d_name, "..")!=0){
            int l = strlen(path);
            if(path[l-1]!='/'){
                sprintf(new_path, "%s/%s", path, dp->d_name);
            }
            else{
                sprintf(new_path, "%s%s", path, dp->d_name);
            }
            if(stat(new_path, &stt)==0){
                if(S_ISDIR(stt.st_mode)){
                    mygrep(patt, new_path);
                }
                else if(S_ISREG(stt.st_mode)){
                    pattern_match(patt, new_path);
                }
                else{
                    continue;
                }
            }
            else{
                perror("Unable to read file/directory.\n");
                exit(1);
            }
        }
    }
}

int main(int argc, char *argv[]){
    if(argc != 3){
        perror("Incorrect usage");
        exit(1);
    }

    struct stat stt;
    if(stat(argv[2], &stt)==0){
        char *patt = argv[1];
        char *path = argv[2];
        if(S_ISREG(stt.st_mode)){
            pattern_match_file(patt, path);            
        }
        else{
            mygrep(patt, path);
        }
    }
    else{
        perror("Unable to get stat for file/directory\n");
    }
}
