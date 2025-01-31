#include "command.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

char *command[NUM_CMD] = {
    "backup",
    "remove",
    "recover",
    "list",
    "help"
};

void runCmd(char *cmd, char **arglist)
{
    pid_t pid;
    int status;

    // 프로세스 생성 실패
    if((pid = fork()) < 0){
        fprintf(stderr, "fork error for %s command\n", cmd);
        exit(1);
    }
    // 자식 프로세스: cmd에 해당하는 프로그램 실행
    // /home/backup 디렉토리에 접근해야 하므로 root 권한으로 실행
    // sudo ./command [path] [option] ...
    else if(pid == 0){
        char *temp = (char *)malloc(sizeof(char *) * (strlen(arglist[1]) + 3));
        
        // arglist[0]을 ./ssu_backup에서 sudo로 변경해줌
        strcpy(arglist[0], "sudo");
        
        // 내장 명령어에 해당하는 파일의 경로를 저장해줌
        strcpy(temp, "./"); 
        strcat(temp, arglist[1]);
        strcpy(arglist[1], temp);

        execvp("sudo", arglist);
        exit(0);
    }
    // 부모 프로세스
    else{
        wait(&status);
        
        // 자식 프로세스가 정상적으로 종료되었으면 true 리턴
        // 만약 자식 프로세스가 비정상적으로 종료되었다면 부모 프로세스도 비정상 종료 처리
        if(!WIFEXITED(status))
            exit(1);
    }
}

void help(char *cmd)
{
    pid_t pid;
    int status;

    // 프로세스 생성 실패
    if((pid = fork()) < 0){
        fprintf(stderr, "fork error for help\n");
        exit(1);
    }
    // 자식 프로세스: help 프로그램 실행
    else if(pid == 0){
        execl("help", "help", cmd, NULL);
        exit(0);
    }
    // 부모 프로세스
    else{
        wait(&status);
        
        // 자식 프로세스가 정상적으로 종료되었으면 true 리턴
        // 만약 자식 프로세스가 비정상적으로 종료되었다면 부모 프로세스도 비정상 종료 처리
        if(!WIFEXITED(status))
            exit(1);
    }
}