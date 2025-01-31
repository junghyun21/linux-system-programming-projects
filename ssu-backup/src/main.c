#include "command.h"
#include "filepath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    char **arglist;
    int i;

    // 인자를 입력하지 않은 경우: 에러 처리 후 프로그램 비정상 종료
    if(argc < 2){
        fprintf(stderr, "Usage: %s <command> [option] ...\n", argv[0]);
        exit(1);
    }

    // 홈 디렉토리, 현재 실행 디렉토리 등을 초기화하고, 만약 백업 디렉토리와 로그 파일이 없다면 생성
    initPath();

    // 배열의 마지막 요소에 NULL을 저장하기 위해 원소를 하나 더 생성
    arglist = (char **)malloc(sizeof(char *) * (argc + 1));
    for(i = 0; i < argc; i++){
        arglist[i] = (char *)malloc(sizeof(char) * MAX_PATH);
        strcpy(arglist[i], argv[i]);
    }
    arglist[i] = NULL;

    // 내장 명령어에 따라 작업 수행하고, 만약 잘못된 명령어가 입력된 경우에는 에러 처리 후 프로그램 비정상 종료
    if(!strcmp(argv[1], command[0]))
        runCmd(command[0], arglist);
    else if(!strcmp(argv[1], command[1]))
        runCmd(command[1], arglist);
    else if(!strcmp(argv[1], command[2]))
        runCmd(command[2], arglist);
    else if(!strcmp(argv[1], command[3]))
        runCmd(command[3], arglist);
    else if(!strcmp(argv[1], command[4]))
        help(arglist[2]);
    else{
        fprintf(stderr, "Usage: %s <backup|remove|recover|list|help> [option] ...\n", argv[0]);
        exit(1);
    }

    exit(0);
}