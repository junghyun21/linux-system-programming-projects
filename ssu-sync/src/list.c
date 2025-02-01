#include "monitoring.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    FILE *fp;
    char fullPath[MAX_PATH + 1];
    char buf[BUFFER_SIZE];
    int pid;

    initPath();

    // list만 입력된 경우
    if(argc == 1){
        if((fp = fopen(monitorPATH, "rb")) != NULL){
            while(fgets(buf, BUFFER_SIZE, fp) != NULL){
                fprintf(stdout, "%s", buf);
            }
        }
    }
    // list [DAEMON_PID]
    else if(argc == 2){
        // 압력받은 pid에 해당하는 데몬 프로세스가 있는지 확인
        pid = atoi(argv[1]);
        if(!isExistPid(pid, fullPath)){
            fprintf(stderr, "ERROR: %s is wrong pid\n", argv[1]);
            exit(1);
        }

        ;
    }

    exit(0);
}