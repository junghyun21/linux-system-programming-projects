#include "monitoring.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

static int getOpt(char *opt, int *period, int argc, char *argv[]); // 옵션 추출하여 opt에 저장
static int checkOpt(mode_t mode, char opt); // 압력받은 경로에 맞는 올바른 옵션이 입력되었는지 확인

int main(int argc, char *argv[])
{
    FILE *fp;
    struct stat statbuf;
    char *backupDirPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *backupLogPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char fullPath[MAX_PATH + 1];
    char name[MAX_NAME + 1];
    char opt;
    pid_t pid;
    int period;
    int result;

    // 경로가 입력되지 않은 경우: add 명령어에 대한 에러 처리 및 Usage 출력 후, 프롬프트 재출력
    if(argc < 2){
        fprintf(stderr, "ERROR: <PATH> is not include\nUsage: %s\n", USAGE_ADD);
        exit(1);
    }

    // 홈 디렉토리, 현재 실행 디렉토리 경로 등을 초기화
    initPath();

    // 입력 받은 경로를 절대 경로(fullPath)로 변환하고, 경로에 해당하는 파일/디렉토리명(name)을 저장하고
    // 해당 파일/디렉토리가 올바른 경로인지 확인
    if(processPath(argv[1], fullPath, name) < 0 || checkPath(fullPath, name) < 0){
        fprintf(stderr, "ERROR: %s is wrong path\n", argv[1]);
        exit(1);
    }

    // 입력 받은 경로의 정보 획득
    if(lstat(fullPath, &statbuf) < 0){
        fprintf(stderr, "lstat error for %s\n", argv[1]);
        exit(1);
    }

    // 옵션이 있는 경우: 옵션 추출 및 옵션에 따른 뒤의 인자 추출
    // 옵션이 올바르지 않은 경우: add 사용법 출력 후 비정상 종료
    period = 1; // default 주기: 1초
    if(getOpt(&opt, &period, argc, argv) < 0){
        fprintf(stderr, "ERROR: wrong option\nUsage: %s\n", USAGE_ADD);
        exit(1);
    }

    // 입력받은 경로에 맞는 올바른 옵션이 입력되었는지 확인
    if(checkOpt(statbuf.st_mode, opt) < 0){
        if(S_ISREG(statbuf.st_mode))
            fprintf(stderr, "\"%s\" is file\n", fullPath);
        else if(S_ISDIR(statbuf.st_mode))
            fprintf(stderr, "\"%s\" is directory\n", fullPath);

        exit(1);
    }

    // monitor_list.log을 바탕으로 입력 받은 경로가 현재 추적 경로인지 확인
    // 노드가 존재하는 경우: 에러 처리
    if(isExistDaemon(fullPath)){
        fprintf(stderr, "ERROR: %s is already exist\n", argv[1]);
        exit(1);
    }

    // 현재 프로세스를 데몬 프로세스로 전환
    daemonize();
    
    // monitor_list.log 파일에 경로 및 pid 기록
    if((fp = fopen(monitorPATH, "a+")) != NULL){
        pid = getpid();

        fprintf(fp, "%d : %s\n", pid, fullPath);
        fprintf(stdout, "monitoring started (%s) : %d\n", fullPath, pid);

        fclose(fp);
    }
    else{
        fprintf(stderr, "fopen error for %s\n", monitorPATH);
        exit(1);
    }
    

    // 추적할 경로 내의 파일을 저장할 백업 파일 및 로그 파일 생성
    sprintf(backupDirPath, "%s/%d", backupPATH, pid);
    sprintf(backupLogPath, "%s.log", backupDirPath);

    if(access(backupDirPath, F_OK)){
        int fd;

        mkdir(backupDirPath, 0777);
        if((fd = creat(backupLogPath, 0666)) < 0){
            fprintf(stderr, "creat error for %s\n", backupLogPath);
            exit(1);
        }
        close(fd);
    }

    // 입력 받은 경로에 대해 변경사항 추적
    // 입력받은 경로가 일반 파일인 경우
    if(S_ISREG(statbuf.st_mode))
        trackFile(backupDirPath, backupLogPath, fullPath, period);
    // 입력받은 경로가 디렉토리인 경우
    else if(S_ISDIR(statbuf.st_mode))
        trackDir(backupDirPath, backupLogPath, fullPath, period, opt); 
    
    exit(0);
}

// 옵션 추출하여 opt에 저장
static int getOpt(char *opt, int *period, int argc, char *argv[])
{
    char temp;
    int optCnt = 0; // 옵션의 개수
    int lastind = 2; // 옵션은 argv[2]부터 존재

    optind = 0; // 다음에 처리할 옵션의 인덱스 (default: 1)
    opterr = 0; // getopt() 사용 시, 해당 값이 0이 아니면 에러 로그 출력

    *opt = 0;

    // 성공 시 파싱된 옵션 문자 반환, 파싱이 모두 종료되면 -1 반환되고 optind는 0 
    // 옵션 뒤에 인자는 오지 않음
    while((temp = getopt(argc, argv, "drt:")) != -1){
        // getopt() 내부에서 optind의 값이 갱신되지 않은 경우: 함수 내부에서 에러가 발생한 것으로 간주
        if(lastind == optind){
            fprintf(stderr, "wrong option input\n");
            exit(1);
        }

        if(temp == 'd')
            *opt |= OPT_D;
        else if(temp == 'r')
            *opt |= OPT_R;
        else if(temp == 't'){
            *period = atoi(optarg);
            *opt |= OPT_T;
        }
        else
            return -1;

        optCnt++;
        lastind = optind;
    }

    // 옵션을 제외하면 인자가 3개 남는 경우
    if((*opt) & OPT_T){
        if(argc - optCnt != 3)
            return -1;
    }
    // 옵션을 제외하면 인자가 2개 남는 경우
    else{
        if(argc - optCnt != 2)
            return -1;
    }

    return 0;
}

// 압력받은 경로에 맞는 올바른 옵션이 입력되었는지 확인
static int checkOpt(mode_t mode, char opt)
{
    char useDR = opt & (OPT_D | OPT_R); // 옵션 d 또는 r 모두 사용하지 않은 경우 useDR의 값은 0

    // 백업 대상이 일반 파일인데 옵션 d 또는 r를 사용한 경우: 에러
    if(S_ISREG(mode) && useDR)
        return -1;
    
    // 백업 대상이 디렉토리인데 옵션 d 또는 r을 사용하지 않은 경우: 에러
    if(S_ISDIR(mode) && !useDR)
        return -1;

    return 0;
}