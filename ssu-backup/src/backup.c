#include "filedata.h"
#include "command.h"
#include "filepath.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>

// 백업 시 BFS를 하기 위해 디렉토리 정보를 따로 저장해두는 연결 리스트
typedef struct DirNode{
  char *originPath;
  char *subPath;
  struct DirNode *next;
}DirNode;

typedef struct DirList{
  struct DirNode *head;
  struct DirNode *tail;
}DirList;

// BFS로 파일 및 디렉토리를 백업할 때 사용되는 연결리스트
// 백업을 진행헤야 하는 순서로 디렉토리들의 정보가 연결되어 있음
DirList *dirList;
DirNode *curr;

int getOpt(char *opt, int argc, char *argv[]); // 옵션 추출하여 opt에 저장
int checkOpt(mode_t mode, char opt); // 압력받은 경로에 맞는 올바른 옵션이 입력되었는지 확인

int isBackuped(BackupDir *backupDirNode, char *originPath, char *name);
void backupFile(char *parentPath, char *name, char *subPath, char *date);
void backupDir(char *originPath, char *subPath, char *date, int (*filter)(const struct dirent *), int allowDup);

// sudo ./backup <PATH> [OPTION] ...
// argv[0]: ./backup
// argv[1]: PATH
// argv[2]: OPTION
// ...
int main(int argc, char *argv[])
{
    Dir *dirNode;
    BackupDir *backupDirNode;
    struct stat statbuf;
    char fullPath[MAX_PATH + 1];
    char name[MAX_NAME + 1];
    char opt;
    int allowDup;
    
    // 경로를 입력하지 않은 경우: backup 사용법 출력 후 비정상 종료
    if(argc < 2){
        help("backup");
        exit(1);
    }

    // 홈 디렉토리, 현재 실행 디렉토리 등을 초기화하고, 만약 백업 디렉토리와 로그 파일이 없다면 생성
    initPath();

    // 입력 받은 경로를 절대 경로(fullPath)로 변환하고, 경로에 해당하는 파일/디렉토리명(name)을 저장하고
    // 해당 파일/디렉토리가 올바른 경로인지 확인
    if(processPath(argv[1], fullPath, name) < 0 || checkPath(fullPath, name) < 0){
        fprintf(stderr, "incorrect path: %s\n", argv[1]);
        exit(1);
    }

    // 입력 받은 경로에 대한 파일이 존재하지 않거나, 파일에 대해 접근 권한이 없는 경우
    if(access(fullPath, F_OK | R_OK | W_OK)){
        fprintf(stderr, "incorrect path: %s\n", argv[1]);
        exit(1);
    }

    // 백업할 파일/디렉토리의 정보 추출
    // 성공 시 0, 실패 시 -1 리턴
    if(lstat(fullPath, &statbuf) < 0){
        fprintf(stderr, "lstat error for %s\n", fullPath);
        exit(1);
    }
    // 인자로 입력받은 경로가 일반 파일이나 디렉토리가 아닌 경우: 에러 처리 후 비정상 종료
    // S_ISREG(mode): mode에 해당하는 파일 종류가 일반 파일이 아니라면 0 리턴
    if(!S_ISREG(statbuf.st_mode) && !S_ISDIR(statbuf.st_mode)){
        fprintf(stderr, "%s is not directory or regular file\n", fullPath);
        exit(1);
    }

    // 옵션을 추출하여 opt에 각각의 비트로 저장
    // 옵션이 올바르지 않은 경우: backup 사용법 출력 후 비정상 종료
    if(getOpt(&opt, argc, argv) < 0){
        help("backup");
        exit(1);
    }

    // 압력받은 경로에 맞는 올바른 옵션이 입력되었는지 확인
    if(checkOpt(statbuf.st_mode, opt) < 0){
        fprintf(stderr, "path and option don't match\n");
        exit(1);
    }

    // ----------------------------------------------------------------

    // 백업된 파일들의 정보를 저장한 연결리스트 생성 (listHead가 연결리스트 가리킴)
    listHead = createBackupList();

    // 해당 프로세스에서 /home/backup 내 백업 디랙토리 내의 파일들에 접근하기 위해 파일 마스크 설정
    // 프로세스가 종료되면 해당 마스크 소멸
    umask(0002); 

    // 일반 파일인 경우
    if(S_ISREG(statbuf.st_mode)){
        char *parentPath = (char *)malloc(sizeof(char *) * (MAX_PATH + 1));

        // parentPath: 백업 대상의 상위 디렉토리 경로
        strcpy(parentPath, fullPath);
        parentPath[strlen(fullPath) - strlen(name) - 1] = '\0';

        backupDirNode = NULL;

        // 옵션 y가 적용된 경우
        // (옵션 y: 동일한 백업 파일이 백업 디렉토리 내에 존재해도 백업 진행)
        if(opt & OPT_Y)
            backupFile(parentPath, name, "", getDate());
        else{
            // 백업하려는 파일의 상위 디렉토리와 연동되어 있는 백업 디렉토리의 리스트 추출
            if((dirNode = searchBackupDir(listHead, parentPath)) != NULL)
                backupDirNode = dirNode->head;
            //backupDirNode = searchBackupDir(listHead, parentPath);

            // 해당 파일이 백업되지 않았을 때에만 백업 진행
            if(!isBackuped(backupDirNode, parentPath, name))
                backupFile(parentPath, name, "", getDate());
            
        }
    }
    // 디렉토리인 경우
    else if(S_ISDIR(statbuf.st_mode)){
        // "filepath.h"에 정의되어 있는 전역변수들
        // 옵션 r 사용 시에 BFS를 하기 위해 디렉토리들을 순서대로 저장해두는 연결 리스트
        dirList = (DirList *)malloc(sizeof(*dirList));
        dirList->head = NULL;
        dirList->tail = NULL;
        curr = dirList->head;

        // 옵션 d와 r이 모두 사용되었을 때, r 적용
        if((opt & OPT_D) && (opt & OPT_R))
            opt &= ~OPT_D;
        
        // 옵션 d: 해당 디렉토리(fullPath) 아래 있는 모든 파일들에 대해 백업 진행
        if(opt & OPT_D)
            backupDir(fullPath, "", getDate(), filter_onlyfile, opt & OPT_Y);
        // 옵션 r: 해당 디렉토리(fullPath) 아래의 모든 서브 디렉토리 내 모든 파일애 대해 백업 진행
        else if(opt & OPT_R)
            backupDir(fullPath, "", getDate(), filter_all, opt & OPT_Y);
    }
    
    exit(0);
}

// 옵션 추출하여 opt에 저장
int getOpt(char *opt, int argc, char *argv[])
{
    char temp;
    int optCnt = 0; // 옵션의 개수
    int lastind = 2; // 옵션은 argv[2]부터 존재

    optind = 0; // 다음에 처리할 옵션의 인덱스 (default: 1)
    opterr = 0; // getopt() 사용 시, 해당 값이 0이 아니면 에러 로그 출력

    *opt = 0;

    // 성공 시 파싱된 옵션 문자 반환, 파싱이 모두 종료되면 -1 반환되고 optind는 0 
    // 옵션 뒤에 인자는 오지 않음
    while((temp = getopt(argc, argv, "dry")) != -1){
        // getopt() 내부에서 optind의 값이 갱신되지 않은 경우: 함수 내부에서 에러가 발생한 것으로 간주
        if(lastind == optind){
            fprintf(stderr, "wrong option input\n");
            exit(1);
        }

        if(temp == 'd')
            *opt |= OPT_D;
        else if(temp == 'r')
            *opt |= OPT_R;
        else if(temp == 'y')
            *opt |= OPT_Y;
        else
            return -1;

        optCnt++;
        lastind = optind;
    }

    // argv[]의 인자에서 옵션에 해당하는 인자를 빼면 2개의 인자만 남아야 함 (내장 명령어, 경로)
    if(argc - optCnt != 2)
        return -1;

    return 0;
}

// 압력받은 경로에 맞는 올바른 옵션이 입력되었는지 확인
int checkOpt(mode_t mode, char opt)
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

// 백업하려는 파일이 백업 디렉토리 내에 백업되어 있는 상태인지 검사
int isBackuped(BackupDir *backupDirNode, char *parentPath, char *name)
{
    char *backupPath = (char *)malloc(sizeof(char *) * (MAX_PATH + 1));
    char *originPath = (char *)malloc(sizeof(char *) * (MAX_PATH + 1));

    sprintf(originPath, "%s/%s", parentPath, name);

    // backupDirNode: 백업하려는 파일의 상위 디렉토리와 연동되어 있는 백업 디렉토리들의 리스트
    while(backupDirNode != NULL){
        // 특정 백업 디렉토리(/home/backup/YYMMDDHHMMSS) 내에 해당 파일(name)이 존재하는지 확인
        // 존재한다면 백업 디렉토리 내의 백업 파일과 백업하려는 파일이 동일한 파일인지 검사
        sprintf(backupPath, "%s/%s", backupDirNode->backupPath, name);

        // 백업 디렉토리 내에 해당 파일이 존재할 때
        if(!access(backupPath, F_OK)){
            // 내용이 같은 파일이 존재한다면 백업 진행하지 않음
            if(!cmpHash(originPath, backupPath)){
                printf("\"%s\" already backuped to \"%s\"\n", originPath, backupPath);
                return TRUE;
            }
        }
        
        backupDirNode = backupDirNode->next;
    }

    return FALSE;
}

// 파일 백업
void backupFile(char *parentPath, char *name, char *subPath, char *date)
{
    char *originPath = (char *)malloc(sizeof(char *) * (MAX_PATH + 1));
    char *backupPath = (char *)malloc(sizeof(char *) * (MAX_PATH + 1));
    char *upperPath = (char *)malloc(sizeof(char *) * (MAX_PATH + 1));
    char *newPath = (char *)malloc(sizeof(char *) * (MAX_PATH + 1));
    char buf[BUFFER_SIZE];
    int fd1, fd2, fd3;
    int length;

    // 백업한 파일을 저장할 백업 디렉토리(/home/backup) 내 디렉토리(/home/backup/YYMMDDHHMMSS)
    // 해당 디렉토리(backupPath)가 존재하지 않으면 생성
    sprintf(backupPath, "%s/%s", backupPATH, date);
    if(access(backupPath, F_OK))
        mkdir(backupPath, 0777);

    // 백업 디렉토리 내 저장될 경로의 상위 디렉토리
    if(!strcmp(subPath, ""))
        sprintf(upperPath, "%s", backupPath);
    else
        sprintf(upperPath, "%s/%s", backupPath, subPath);

    if(access(upperPath, F_OK))
        mkdir(upperPath, 0777);
    
    sprintf(newPath, "%s/%s", upperPath, name); // 백업 디렉토리 내 저장될 경로
    sprintf(originPath, "%s/%s", parentPath, name); // 백업될 대상의 실제 경로

    // 백업 디렉토리에 파일 저장
    if((fd1 = open(originPath, O_RDONLY)) < 0){
        fprintf(stderr, "open error for %s\n", originPath);
        exit(1);
    }

    if((fd2 = open(newPath, O_CREAT | O_TRUNC | O_WRONLY, 0666)) < 0){
        fprintf(stderr, "open error for %s\n", newPath);
        exit(1);
    }

    while((length = read(fd1, buf, BUFFER_SIZE)) > 0)
        write(fd2, buf, length);

    // 로그 파일에 백업 기록 저장
    sprintf(buf, "%s : \"%s\" backuped to \"%s\"\n", date, originPath, newPath);
    if((fd3 = open(logPATH, O_RDWR)) < 0){
        fprintf(stderr, "open error for %s\n", logPATH);
        exit(1);
    }
    lseek(fd3, (long)0, SEEK_END);
    
    if(write(fd3, buf, strlen(buf)) != strlen(buf)){
        fprintf(stderr, "write error for %s\n", logPATH);
        exit(1);
    }

    printf("\"%s\" backuped to \"%s\"\n", originPath, newPath);

    close(fd1);
    close(fd2);
    close(fd3);

    return;
}

// 디렉토리 백업
void backupDir(char *originPath, char *subPath, char *date, int (*filter)(const struct dirent *), int allowDup)
{
    char *filePath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *tmpSubPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    Dir *dirNode;
    BackupDir *backupDirNode;
    struct dirent **namelist;
    struct stat statbuf;
    int cnt;
    int i;

    // 성공 시 namelist에 저장된 파일/디렉토리의 개수(cnt), 실패 시 -1 리턴
    // filter
    // - 옵션 d: filter_onlyfile() -> originPath의 하위 파일들만 추출
    // - 옵션 r: filter_all() -> originPath의 하위 파일 및 서브 디렉토리 내 파일들 모두 추출
    if((cnt = scandir(originPath, &namelist, filter, alphasort)) == -1){
        fprintf(stderr, "scandir error for %s\n", originPath);
		exit(1);
    }

    // 추출한 파일 및 디렉토리를 바탕으로 백업 진행
    for(i = 0; i < cnt; i++){
        // originPath: 백업하려고 하는 디렉토리의 경로
        // filePath: 백업하려고 하는 디렉토리의 하위 파일 및 서브 디렉토리 원본 경로
        sprintf(filePath, "%s/%s", originPath, namelist[i]->d_name);
        if(lstat(filePath, &statbuf) < 0){
            fprintf(stderr, "lstat error for %s\n", filePath);
            exit(1);
        }

        backupDirNode = NULL;

        // 일반 파일인 경우: 백업 진행
        if(S_ISREG(statbuf.st_mode)){
            // y 옵션이 적용된 경우: 동일한 백업 파일도 모두 백업
            if(allowDup)
                backupFile(originPath, namelist[i]->d_name, subPath, date);
            else{
                if((dirNode = searchBackupDir(listHead, originPath)) != NULL)
                    backupDirNode = dirNode->head;

                if(!isBackuped(backupDirNode, originPath, namelist[i]->d_name))
                    backupFile(originPath, namelist[i]->d_name, subPath, date);
            }
        }
        // 디렉토리인 경우: 파일의 백업이 끝난후 백업 진행
        else if(S_ISDIR(statbuf.st_mode)){
            // subPath: 백업 디렉토리(/home/backup/YYMMDDHHMMSS)를 기준으로 백업할 파일/디렉토리의 상대 경로
            if(!strcmp(subPath, ""))
                sprintf(tmpSubPath, "%s", namelist[i]->d_name);
            else
                sprintf(tmpSubPath, "%s/%s", subPath, namelist[i]->d_name);

            // 디렉토리의 정보를 저장할 노드 생성 및 초기화
            DirNode *newDirNode = (DirNode *)malloc(sizeof(DirNode));
            newDirNode->originPath = (char *)malloc(sizeof(char) * (strlen(filePath) + 1));
            newDirNode->subPath = (char *)malloc(sizeof(char) * (strlen(tmpSubPath) + 1));
            newDirNode->next = NULL;
            strcpy(newDirNode->originPath, filePath);
            strcpy(newDirNode->subPath, tmpSubPath);

            // dirList 내에 DirNode가 하나도 없는 경우 (빈 연결 리스트인 경우)
            if(curr == NULL){
                dirList->head = dirList->tail = newDirNode;
                curr = dirList->head;
            }
            // 빈 리스트가 아닌 경우, 연결 리스트의 가장 뒤에 노드 연결
            else{
                dirList->tail->next = newDirNode;
                dirList->tail = dirList->tail->next;
            }

        }
        // 일반 파일 또는 디렉토리가 아닌 경우: 에러
        else{
            fprintf(stderr, "%s is not directory or regular file\n", filePath);
            exit(1);    
        }
    }

    // 일반 파일을 모두 백업 완료한 상태 -> 디렉토리들을 백업할 차례
    while(curr != NULL){
        strcpy(filePath, curr->originPath);
        strcpy(tmpSubPath, curr->subPath);
        
        curr = curr->next;

        backupDir(filePath, tmpSubPath, date, filter, allowDup);

    }

    return;
}