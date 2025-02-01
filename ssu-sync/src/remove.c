#include "monitoring.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int main(int argc, char *argv[])
{
    int pid;
    char fullPath[MAX_PATH + 1];

    // pid가 입력되지 않은 경우
    if(argc != 2){
        fprintf(stderr, "ERROR: <DAEMON_PID> is not include\nUsage: %s\n", USAGE_REMOVE);
        exit(1);
    }

    // 홈 디렉토리, 현재 실행 디렉토리 경로 등을 초기화
    initPath();

    // 압력받은 pid에 해당하는 데몬 프로세스가 있는지 확인
    pid = atoi(argv[1]);
    if(!isExistPid(pid, fullPath)){
        fprintf(stderr, "ERROR: %s is wrong pid\n", argv[1]);
        exit(1);
    }

    // pid에 해당하는 데몬 프로세스 삭제
    kill(pid, SIGUSR1);
    fprintf(stdout, "monitoring ended (%s) : %d\n", fullPath, pid);
    
    // pid에 해당하는 데몬 프로세스와 관련된 로그 파일 및 디렉토리 삭제
    removeDaemon(pid);

    exit(0);
}