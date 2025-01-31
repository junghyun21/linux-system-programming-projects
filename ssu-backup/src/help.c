#include "defs.h"
#include "command.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// help [COMMAND]
// argv[0]: help
// argv[1]: COMMAND
// argv[2]: NULL
int main(int argc, char *argv[])
{
    char fname[MAX_NAME];
    char c;
    int fd;
    int length;
    int i;

    // help 뒤에 인자가 없는 경우: 프로그램 내장명령어에 대한 설명 출력
    if(argv[1] == NULL){
        printf("Usage:\n");
        for(i = 0; i < NUM_CMD; i++){
            // 버퍼 안의 데이터 강제로 비우지 않으면 write()가 먼저 출력됨
            // printf()는 종료 문자(\n, \0)를 만났을 때에만 버퍼의 내용 출력하기 때문
            printf("  > ");
            fflush(stdout);

            sprintf(fname, "./usage/%s.txt", command[i]);

            if((fd = open(fname, O_RDONLY)) < 0){
                fprintf(stderr, "open error for %s\n", fname);
                exit(1);
            }

            while(read(fd, &c, 1) > 0){
                write(STDOUT_FILENO, &c, 1);
                if(c == '\n'){
                    printf("  ");
                    fflush(stdout);
                }
            }
            printf("\n");

            close(fd);
        }
    }
    // help 뒤에 인자가 있는 경우: 인자로 들어온 명령어에 대한 설명 출력
    else{
        // 각 명령어의 사용법에 해당하는 파일명(fname) 저장
        if(!strcmp(argv[1], command[0]))
            sprintf(fname, "./usage/%s.txt", command[0]);
        else if(!strcmp(argv[1], command[1]))
            sprintf(fname, "./usage/%s.txt", command[1]);
        else if(!strcmp(argv[1], command[2]))
            sprintf(fname, "./usage/%s.txt", command[2]);
        else if(!strcmp(argv[1], command[3]))
            sprintf(fname, "./usage/%s.txt", command[3]);
        else if(!strcmp(argv[1], command[4]))
            sprintf(fname, "./usage/%s.txt", command[4]);
        else{
            fprintf(stderr, "Usage: help [backup|remove|recover|list|help]\n");
            exit(1);
        }

        if((fd = open(fname, O_RDONLY)) < 0){
            fprintf(stderr, "open error for %s\n", fname);
            exit(1);
        }

        printf("Usage: ");
        fflush(stdout);
        while(read(fd, &c, 1) > 0)
            write(STDOUT_FILENO, &c, 1);
        printf("\n");

        close(fd);
    }

    exit(0);
}