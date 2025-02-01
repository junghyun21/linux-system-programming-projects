#include "command.h"

void init();

int main()
{
    char input[10000]; // 우선 넉넉하게 잡고, 에러 처리는 나중에 진행
    char **argList;
    int argCnt;
    int i;

    // 현재 작업 경로에 레포 디렉토리(.repo)를 생성하고
    // 하위에 커밋 로그(.commit.log) 및 스테이징 구역(.staging.log) 생성
    init();

    // 사용자 입력을 기다리는 프롬프트 출력
    // exit 입력 시, 프로그램 종료
    while(1){
        printf("20201662 > ");
        fgets(input, sizeof(input), stdin);
        input[strlen(input) - 1] = '\0';

        // argList: 공백을 기준으로 자른 토큰들을 가리키는 포인터들를 저장한 리스트
        // argCnt: 토큰의 개수
        // 엔터만 입력한 경우: argList는 NULL, argCnt는 0
        if((argList = getArglist(input, &argCnt)) != NULL){
            if(!strcmp(argList[0], "exit")) break;
            
            // 해당하는 내장 명령어 실행 
            for(i = 0; i < NUM_CMD; i++){
                if(!strcmp(argList[0], commandList[i])){
                    builtin_command(argList, argCnt);
                    break;
                }
            }

            // 프롬프트에서 지정한 내장명령어 외 기타 명령어를 입력한 경우: help 명령어 실행
            if(i == NUM_CMD)
                builtin_help(NULL);

            // 내장명령어, 경로 등을 저장했던 메모리 해제
            for(i = 0; i < argCnt; i++)
                free(argList[i]);
            free(argList);
        }

        memset(input, 0, sizeof(input));
        
        printf("\n");
    }

    return 0;
}

void init()
{
    char *curDir, *repoPath;

    curDir = getenv("PWD");
    repoPath = (char *)malloc(sizeof(char) * (strlen(curDir) + strlen("/.repo") + 1));
    sprintf(repoPath, "%s/.repo", curDir);

    // repoPath(.repo 디렉토리)가 존재하면 0 리턴
    // 존재하지 않는 경우에만 레포 디렉토리와 하위에 커밋 로그(.commit.log) 및 스테이징 구역(.staging.log) 생성
    if(access(repoPath, F_OK)){
        char *fname1, *fname2;
        int fd1, fd2;

        // .repo 디렉토리 생성
        mkdir(repoPath, 0777);

        // 커밋 로그 생성
        fname1 = (char *)malloc(sizeof(char) * (strlen(repoPath) + strlen("/.commit.log") + 1));
        sprintf(fname1, "%s/.commit.log", repoPath);
        if((fd1 = creat(fname1, 0666)) < 0){
            fprintf(stderr, "creat error for %s\n", fname1);
            exit(1);
        }

        // 스테이징 구역 생성
        fname2 = (char *)malloc(sizeof(char) * (strlen(repoPath) + strlen("/.staging.log") + 1));
        sprintf(fname2, "%s/.staging.log", repoPath);
        if((fd2 = creat(fname2, 0666)) < 0){
            fprintf(stderr, "creat error for %s\n", fname2);
            exit(1);
        }

        close(fd1);
        close(fd2);

        free(fname1);
        free(fname2);
    }

    free(repoPath);
}