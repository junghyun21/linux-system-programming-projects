#include "command.h"

char *commandList[NUM_CMD] = {
    "add",
    "remove",
    "status",
    "commit",
    "revert",
    "log",
    "help",
    "exit"
};

// 공백을 기준으로 프롬프트 상에서 입력받은 문자열을 토큰화하는 함수
char **getArglist(char *input, int *argCnt)
{
    char **argList;
    char *token = NULL;
    char *del = " ";
    *argCnt = 0;

    token = strtok(input, del);

    // 빈 문자열이 입력된 경우
    if(token == NULL)
        return NULL;

    // 문자열이 입력된 경우: 토큰의 개수는 최대 2개 (명령어, 인자)
    argList = (char **)malloc(sizeof(char *) * MAX_ARGC);
    while(token != NULL){
        argList[*argCnt] = (char *)malloc(sizeof(char) * (strlen(token) + 1));
        strcpy(argList[(*argCnt)++], token);

        token = strtok(NULL, " ");
    }

    return argList;
}

// 내장 명령어를 실행하는 함수
void builtin_command(char **argList, int argc)
{
    char **argv = (char **)malloc(sizeof(char *) * (argc + 1));
    int i;

    // execv()를 실행할 때, 실행할 프로그램에 전달할 인자들을 나타내는 배열의 마지막 원소는 NULL 포인터여야함
    for(i = 0; i < argc; i++)
        argv[i] = argList[i];
    argv[i] = NULL;

    // help 명령어인 경우: help 다음에 온 인자 또는 NULL 전달
    if(!strcmp(argv[0], "help"))
        builtin_help(argv[1]);
    // 그 외의 다른 명령어인 경우: 해당 명령어 프로세스 실행
    else{
        pid_t pid;

        // 내장 명령어 실행 -> 뒤의 인자만 전달 (명령어 자체는 전달하지 않음)
        if((pid = fork()) < 0){
            fprintf(stderr, "fork error for %s\n", argv[0]);
            exit(1);
        }
        else if(pid == 0){
            execv(argv[0], argv);
            exit(0);
        }
        else{
            wait(NULL);
        }

    }

    free(argv);
}

// 내장 명령어 중 help 명령어를 실행하는 함수
void builtin_help(char *cmd)
{
    char **argv;
    pid_t pid;

    // 모든 내장 명령어에 대한 설명을 출력하는 경우
    if(cmd == NULL){
        argv = (char **)malloc(sizeof(char *) * 2);
        argv[0] = "help";
        argv[1] = NULL;
    }
    // 특정 명령어(cmd)에 대한 설명만 출력하는 경우 
    else{
        argv = (char **)malloc(sizeof(char *) * 3);
        argv[0] = "help";
        argv[1] = cmd;
        argv[2] = NULL;
    }

    // help() 내장 명령어 실행
    if((pid = fork()) < 0){
        fprintf(stderr, "fork error for help\n");
        exit(1);
    }
    else if(pid == 0){
        execv("help", argv);
        exit(0);
    }
    else{
        wait(NULL);
    }

    free(argv);

    return;
}
