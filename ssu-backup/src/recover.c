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

// 복구 시 BFS를 하기 위해 디렉토리 정보를 따로 저장해두는 연결 리스트
typedef struct DirNode{
  Dir *recoverDirList;
  char *originPath;
  struct DirNode *next;
}DirNode;

typedef struct DirList{
  struct DirNode *head;
  struct DirNode *tail;
}DirList;

// BFS로 파일 및 디렉토리를 복구할 때 사용되는 연결리스트
// 복구를 진행헤야 하는 순서로 디렉토리들의 정보가 연결되어 있음
DirList *dirList;
DirNode *curr;

// 복구할 백업 본이 2개 이상안 경우, 해당 백업본들의 정보를 저장해둘 연결 리스트
typedef struct RecoverNode{
    char originPath[MAX_PATH + 1];
    char backupPath[MAX_PATH + 1];
    char backupDate[LEN_DATE + 1];
    char *fileSize;
    struct RecoverNode *next;
}RecoverNode;

typedef struct RecoverFileList{
    struct RecoverNode *head;
    struct RecoverNode *tail;
    int recoverFileCnt;
}RecoverFileList;

int getOpt(char *opt, char *newPath, int argc, char *argv[]); // 옵션 추출하여 opt에 저장
int checkOpt(int isReg , char opt); // 압력받은 경로에 맞는 올바른 옵션이 입력되었는지 확인

Dir *canRecover(char *originPath, char *name, int *isReg);
void makeDir(char *newPath);
RecoverFileList *searchRecoverable(Dir *recoverDirList, char *name);
int selectRecoverFile(RecoverFileList *recoverFileList);

void recoverFile(RecoverNode *file, char *newPath, char opt, char *date);
void recoverDir(Dir *recoverDirList, char *newPath, int opt, char *date);

// sudo ./recover <PATH> [OPTION] ...
// argv[0]: ./recover
// argv[1]: PATH
// argv[2]: OPTION
// ...
int main(int argc, char *argv[])
{
    Dir *recoverDirList; // 복구하려는 파일의 상위 디렉토리 노드 또는 삭제하려는 디렉토리의 노드
    struct dirent **namelist, **tmplist;
    char *fullPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *newPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *parentPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *backupPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *tmpPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *name = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char opt;
    int isReg;
    int cnt;
    int tmpCnt;
    int i;

    // 경로를 입력하지 않은 경우: recover 사용법 출력 후 비정상 종료
    if(argc < 2){
        help("recover");
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

    // 백업된 파일들의 정보를 저장한 연결리스트 생성 (listHead가 연결리스트 가리킴)
    listHead = createBackupList();

    // 입력 받은 경로와 일치하는 백업 파일 및 디렉토리가 없는 경우: 에러 처리 후 비정상 종료
    // 그렇지 않다면 해당 경로는 파일인지 디렉토리인지 isReg에 저장하고,
    // 복구 대상이 되는 경로의 Dir 노드 반환
    if((recoverDirList = canRecover(fullPath, name, &isReg)) == NULL){
        fprintf(stderr, "%s that do not exist in backup directory\n", argv[1]);
        exit(1);
    }

    // 옵션을 추출하여 opt에 각각의 비트로 저장하고, 새로운 경로가 입력된 경우에는 newPath에 저장
    // 옵션이 올바르지 않은 경우: recover 사용법 출력 후 비정상 종료
    strcpy(newPath, fullPath);
    if(getOpt(&opt, newPath, argc, argv) < 0){
        help("recover");
        exit(1);
    }

    // 압력받은 경로에 맞는 올바른 옵션이 입력되었는지 확인
    if(checkOpt(isReg, opt) < 0){
        fprintf(stderr, "path and option don't match\n");
        exit(1);
    }

    // ----------------------------------------------------------------

    umask(0002); 

    // 복구하려는 대상이 일반 파일인 경우
    if(isReg){
        RecoverFileList *recoverFileList;
        RecoverNode *temp;
        int recoverFileCnt;
        int num;
        int i;

        // parentPath: 복구할 파일의 상위 디렉토리 경로
        strcpy(parentPath, backupPath);
        parentPath[strlen(parentPath) - strlen(name) - 1] = '\0';

        // 복구가 가능한 백업 디렉토리 내부의 파일들 목록 가져옴
        recoverFileList = searchRecoverable(recoverDirList, name);

        // 복구 진행
        if(recoverFileList->recoverFileCnt > 0){
            // 복구 가능한 파일이 하나일 때
            if(recoverFileList->recoverFileCnt == 1){
                temp = recoverFileList->head;
            }
            // 두 개 이상일 때
            else{
                // l 옵션이 적용된 경우 최근에 백업된 파일 복구
                if(opt & OPT_L){
                    // 리스트는 백업 날짜 오름차순으로 저장되어 있음
                    temp = recoverFileList->head;
                    while(temp->next != NULL)
                        temp = temp->next;
                }
                // 삭제할 번호 입력 받음
                else{
                    num = selectRecoverFile(recoverFileList);

                    // num에 해당하는 파일의 정보를 담고 있는 노드 찾아감
                    if(num > 0)
                        for(i = 1, temp = recoverFileList->head; i < num; temp = temp->next, i++);
                }
            }
            // 상위 디렉토리 생성
            sprintf(parentPath, "%s/%s", newPath, name);
            for(i = strlen(parentPath); parentPath[i] != '/'; i--);
            parentPath[i] = '\0';

            makeDir(parentPath);

            recoverFile(temp, newPath, opt, getDate());
        }

    }
    // 복구하려는 대상이 디렉토리인 경우
    else{
        // 옵션 r 사용 시에 BFS를 하기 위해 디렉토리들을 순서대로 저장해두는 연결 리스트
        dirList = (DirList *)malloc(sizeof(*dirList));
        dirList->head = NULL;
        dirList->tail = NULL;
        curr = dirList->head;

        // 옵션 d와 r이 모두 사용되었을 때, r 적용
        if((opt & OPT_D) && (opt & OPT_R))
            opt &= ~OPT_D;

        recoverDir(recoverDirList, newPath, opt, getDate());
    }

    // 삭제가 안된 빈 폴더 모두 삭제
    if((cnt = scandir(backupPATH, &namelist, filter_onlyDir, alphasort)) == -1){
        fprintf(stderr, "scandir error for %s\n", backupPATH);
		exit(1);
    }

    for(i = 0; i < cnt; i++){
        sprintf(tmpPath, "%s/%s", backupPATH, namelist[i]->d_name);

        if((tmpCnt = scandir(tmpPath, &tmplist, filter_all, alphasort)) == -1){
            fprintf(stderr, "scandir error for %s\n", tmpPath);
            exit(1);
        }

        if(tmpCnt == 0){
            // 디렉토리 삭제 (성공 시 0 리턴)
            if(rmdir(tmpPath)){
                fprintf(stderr, "rmdir error for %s\n", tmpPath);
                exit(1);
            }
        }
    }

    exit(0);
}

// 옵션 추출하여 opt에 저장
int getOpt(char *opt, char *newPath, int argc, char *argv[])
{
    char name[MAX_NAME + 1];
    char temp;
    int optCnt = 0; // 옵션의 개수
    int lastind = 2; // 옵션은 argv[2]부터 존재

    optind = 0; // 다음에 처리할 옵션의 인덱스 (default: 1)
    opterr = 0; // getopt() 사용 시, 해당 값이 0이 아니면 에러 로그 출력

    *opt = 0;

    // 성공 시 파싱된 옵션 문자 반환, 파싱이 모두 종료되면 -1 반환되고 optind는 0 
    // 옵션 뒤에 인자는 오지 않음
    while((temp = getopt(argc, argv, "drln:")) != -1){
        // getopt() 내부에서 optind의 값이 갱신되지 않은 경우: 함수 내부에서 에러가 발생한 것으로 간주
        if(lastind == optind){
            fprintf(stderr, "wrong option input\n");
            exit(1);
        }

        if(temp == 'd'){
            *opt |= OPT_D;
        }
        else if(temp == 'r'){
            *opt |= OPT_R;
        }
        else if(temp == 'l'){
            *opt |= OPT_L;
        }
        else if(temp == 'n'){
            *opt |= OPT_N;

            // n 옵션 뒤에 인자가 오지 않은 경우
            if(optarg == NULL){
                fprintf(stderr, "enter new path with '-n'\n");
                exit(1);  
            }

            // 입력 받은 경로를 절대 경로(fullPath)로 변환하고, 경로에 해당하는 파일/디렉토리명(name)을 저장하고
            // 해당 파일/디렉토리가 올바른 경로인지 확인
            if(processPath(optarg, newPath, name) < 0 || checkPath(newPath, name) < 0){
                fprintf(stderr, "incorrect new path: %s\n", optarg);
                exit(1);    
            }
        }
        else{
            return -1;
        }

        optCnt++;
        lastind = optind;
    }

    // argv[]의 인자에서 옵션에 해당하는 인자와 새로운 경로를 빼면 3개의 인자만 남아야 함 (내장 명령어, 경로, 새로운 경로)
    if(*opt & OPT_N && argc - optCnt != 3)
        return -1;

    // argv[]의 인자에서 옵션에 해당하는 인자와 새로운 경로를 빼면 2개의 인자만 남아야 함 (내장 명령어, 경로)
    if(!(*opt & OPT_N) && argc - optCnt != 2)
        return -1;

    return 0;
}

// 압력받은 경로에 맞는 올바른 옵션이 입력되었는지 확인
int checkOpt(int isReg , char opt)
{
    char useDR = opt & (OPT_D | OPT_R); // 옵션 d 또는 r 모두 사용하지 않은 경우 useDR의 값은 0

    // 백업 대상이 일반 파일인데 옵션 d 또는 r를 사용한 경우: 에러
    if(isReg && useDR)
        return -1;
    
    // 백업 대상이 디렉토리인데 옵션 d 또는 r을 사용하지 않은 경우: 에러
    if(!isReg && !useDR)
        return -1;

    return 0;
}

// 입력 받은 경로와 일치하는 백업 파일 및 디렉토리가 있는지 확인
// 만약 존재하지 않는다면 복구 불가능
Dir *canRecover(char *originPath, char *name, int *isReg)
{   
    Dir *dirNode;
    BackupDir *backupDirNode;
    char *parentPath = (char *)malloc(sizeof(char *) * (MAX_PATH + 1));
    char *backupPath = (char *)malloc(sizeof(char *) * (MAX_PATH + 1));

    sprintf(backupPath, "%s%s", backupPATH, originPath + strlen(homePATH));

    // 대상의 경로에 대해 백업 파일 리스트를 탐색
    // - 디렉토리: 디렉토리의 모든 경로에 해당하는 디렉토리들이 연결 리스트에 저장되어 있음
    // - 파일: 연결 리스트 노드 내에는 상위 디렉토리의 정보만 있고 파일 자체의 정보는 없음, 따라서 무조건 NULL 반환
    if((dirNode = searchBackupDir(listHead, originPath)) == NULL){
        // 파일은 무조건 코드 진행
        // 디렉토리가 이전 조건문에서 NULL을 반환했다면, 다음 코드도 무조건 NULL을 반환
        *isReg = TRUE;

        // parentPath: 복구 대상의 상위 디렉토리 경로
        strcpy(parentPath, originPath);
        parentPath[strlen(parentPath) - strlen(name) - 1] = '\0';

        // 복구하려는 파일의 상위 디렉토리가 리스트 내에 없다면, 해당 파일은 백업 디렉토리에 없음을 의미
        if((dirNode = searchBackupDir(listHead, parentPath)) == NULL)
            return dirNode;

        // backupDirNode: 복구하려는 파일의 상위 디렉토리와 연동되어 있는 백업 디렉토리들의 리스트
        backupDirNode = dirNode->head;

        while(backupDirNode != NULL){
            // 백업 디렉토리(/home/backup/YYMMDDHHMMSS/...) 내에 해당 파일(name)이 존재하는지 확인
            // 존재한다면 백업 디렉토리 내의 백업 파일과 백업하려는 파일이 동일한 파일인지 검사
            sprintf(backupPath, "%s/%s", backupDirNode->backupPath, name);

            // 백업 디렉토리 내에 해당 파일이 존재할 때
            if(!access(backupPath, F_OK))
                break;
            
            backupDirNode = backupDirNode->next;
        }

        // 복구 대상의 상위 디렉토리는 있지만, 해당 파일은 존재하지 않는 경우
        if(backupDirNode == NULL)
            return NULL;

    }
    // 해당 디렉토리가 백업 디렉토리 내에 존재하는 경우
    else{
        *isReg = FALSE;
    }

    return dirNode;
}

// 복구한 파일을 저장할 디렉토리 생성
void makeDir(char *newPath)
{
    char *path = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *tmpPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *token;
    int fd;
    int i;

    // 사용자의 홈 디렉토리를 벗어난 경우, 에러
    if(strncmp(newPath, homePATH, strlen(homePATH))){
        fprintf(stderr, "incorrect path\n");
        exit(1);
    }

    sprintf(path, "%s", newPath + strlen(homePATH));

    // 디렉토리 생성
    strcpy(tmpPath, homePATH);
    token = strtok(path, "/");
    while(token != NULL){
        strcat(tmpPath, "/");
        strcat(tmpPath, token);

        if(access(tmpPath, F_OK))
            mkdir(tmpPath, 0777);

        token = strtok(NULL, "/");
    }
    
}

// 복구 가능한 파일 리스트 뽑기
RecoverFileList *searchRecoverable(Dir *recoverDirList, char *name)
{
    struct stat statbuf;
    BackupDir *recoverDirNode = recoverDirList->head;
    RecoverFileList *listHead = (RecoverFileList *)malloc(sizeof(RecoverFileList));
    char *backupPath = (char *)malloc(sizeof(char *) * (MAX_PATH + 1));

    // 복구 대상이 되는 백업 본들을 저장할 리스트 초기화
    listHead->head = NULL;
    listHead->tail = NULL;
    listHead->recoverFileCnt = 0;

    // 복구 대상이 될 수 있는 백업본들을 리스트(listHead)에 저장
    while(recoverDirNode != NULL){
        sprintf(backupPath, "%s/%s", recoverDirNode->backupPath, name);

        // 백업 디렉토리 내에 해당 파일이 존재할 때
        if(!access(backupPath, F_OK)){
            // 복구 대상이 되는 파일의 정보 가져옴
            if(stat(backupPath, &statbuf) < 0){
                fprintf(stderr, "stat error for %s\n", backupPath);
                exit(1);
            }

            // 복구 대상이 되는 파일의 정보를 담을 노드 생성 및 초기화
            RecoverNode *newRecoverNode = (RecoverNode *)malloc(sizeof(RecoverNode));
            
            sprintf(newRecoverNode->originPath, "%s/%s", recoverDirNode->parentPath, name);
            strcpy(newRecoverNode->backupPath, backupPath);
            strcpy(newRecoverNode->backupDate, recoverDirNode->dirName);
            newRecoverNode->fileSize = cvtSizeComma(statbuf.st_size);
            newRecoverNode->next = NULL;

            // 복구 대상이 되는 백업본들을 가리키는 연결 리스트(listHead)에 저장
            if(listHead->head == NULL){
                listHead->head = listHead->tail = newRecoverNode;
            }
            else{
                listHead->tail->next = newRecoverNode;
                listHead->tail = listHead->tail->next;
            }

            // 복구 대상이 되는 백업본의 개수 증가
            listHead->recoverFileCnt++;
        }

        recoverDirNode = recoverDirNode->next;
    }

    return listHead;
}

// 복구 가능한 파일들 중 복구할 파일 하나 고르기
int selectRecoverFile(RecoverFileList *recoverFileList)
{
    RecoverNode *temp= recoverFileList->head;
    int i;
    int num = 1;

    // 복구가 가능한 백업본 출력하고 삭제 대상 입력 받음
    printf("backup files of \"%s\"\n", recoverFileList->head->originPath);
    printf("0. exit\n");
    while(temp != NULL){
        printf("%d. %s\t\t%sbytes\n", num, temp->backupDate, temp->fileSize);

        temp = temp->next;
        ++num;
    }
    printf(">> ");

    scanf("%d", &num);

    return num;
}

// 파일 복구
void recoverFile(RecoverNode *file, char *newPath, char opt, char *date)
{
    //printf("newPath: %s\n", newPath);
    
    FILE *fp;
    char buf[BUFFER_SIZE];
    int fd1, fd2;
    int length;

    // 로그 파일 오픈
    if((fp = fopen(logPATH, "a")) < 0){
        fprintf(stderr, "open error for %s\n", logPATH);
        exit(1);
    }

    if(!access(newPath, F_OK)){
        // 백업 파일이 원본 파일과 내용이 같은 경우: 복원 진행하지 않음
        if(!cmpHash(newPath, file->backupPath)){
            printf("\"%s\" is not changed with \"%s\"\n", file->backupPath, newPath);
            return;
        }
    }

    // 파일 복구
    if((fd1 = open(file->backupPath, O_RDONLY)) < 0){
        fprintf(stderr, "open error for %s\n", file->backupPath);
        exit(1);
    }
    if((fd2 = open(newPath, O_CREAT | O_TRUNC | O_WRONLY, 0666)) < 0){
        fprintf(stderr, "open error for %s\n", newPath);
        exit(1);
    }
    while((length = read(fd1, buf, BUFFER_SIZE)) > 0)
        write(fd2, buf, length);

    // 복구한 파일은 백업 디렉토리 내에서 삭제
    if(remove(file->backupPath)){
        fprintf(stderr, "remove error for %s\n", file->backupPath);
        exit(1);
    }
    removeEmptyDir(file->backupPath);

    // 로그 파일에 삭제 기록 저장하고 출력
    fprintf(fp, "%s : \"%s\" recoverd to \"%s\"\n", date, file->backupPath, file->originPath);                
    printf("\"%s\" recoverd to \"%s\"\n", file->backupPath, file->originPath);

    close(fd1);
    close(fd2);
    fclose(fp);
}

// 디렉토리 복구
void recoverDir(Dir *recoverDirList, char *newPath, int opt, char *date)
{
    PathList *listHead = NULL;
    PathList *currFile;
    PathList *temp;

    struct dirent **namelist;
    Dir *recoverDirNode;
    Dir *tmpDir;
    BackupDir *backupDir;
    BackupDir *tmpBackupDir;

    RecoverFileList *recoverFileList;
    RecoverNode *tmpRecoverNode;
    int recoverFileCnt;
    int num;

    char *tmpPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *name = (char *)malloc(sizeof(char) * (MAX_NAME + 1));
    int result;
    int cnt;
    int i;

    backupDir = recoverDirList->head;

    // recoverDirList 가리키는 디렉토리 경로 내에 존재하는 모든 파일의 이름을 연결리스트(listHead)에 저장
    while(backupDir != NULL){
        // 특정 디렉토리 아래에 존재하는 모든 파일 추출
        if((cnt = scandir(backupDir->backupPath, &namelist, filter_onlyfile, alphasort)) == -1){
            fprintf(stderr, "scandir error for %s\n", backupDir->backupPath);
            exit(1);
        }

        // 해당 디렉토리 내에 있는 모든 파일의 이름을 오름차순으로 리스트에 저장
        for(i = 0; i < cnt; i++){
            // 빈 리스트인 경우: 바로 노드 추가
            if(listHead == NULL){
                // 노드 생성 및 초기화
                PathList *newFile = (PathList *)malloc(sizeof(PathList));
                newFile->prev = NULL;
                newFile->next = NULL;
                strcpy(newFile->name, namelist[i]->d_name);

                // 노드 연결
                listHead = newFile;
            }
            // 연결 리스트 내에 다른 이름들이 있는 경우
            // namelist[i]->d_name에 해당하는 파일명이 이미 연결 리스트에 삽입되어 있는 경우
            else{
                currFile = listHead;

                while(currFile != NULL){
                    // namelist[i]->d_name에 해당하는 파일명이 이미 연결 리스트에 삽입되어 있는 경우
                    if((result = strcmp(currFile->name, namelist[i]->d_name)) == 0){
                        break;
                    }
                    // 해당하는 파일명이 currFile이 가리키는 파일명보다 뒤에 저장되어야 하는 경우
                    else if(result < 0){
                        temp = currFile;
                        currFile = currFile->next;
                    }
                    // currFile이 가리키는 파일 바로 앞에 해당하는 파일이 삽입되어야 하는 경우
                    else{
                        // 노드 생성 및 초기화
                        PathList *newFile = (PathList *)malloc(sizeof(PathList));
                        newFile->prev = NULL;
                        newFile->next = NULL;
                        strcpy(newFile->name, namelist[i]->d_name);

                        // 가장 맨 앞에 저장되어야 하는 경우
                        if(currFile->prev == NULL){
                            newFile->prev = currFile->prev;
                            newFile->next = currFile;
                            currFile->prev = newFile;

                            listHead = newFile;

                            break;
                        }
                        else{
                            newFile->prev = currFile->prev;
                            newFile->prev = currFile;
                            currFile->prev->next = newFile;
                            currFile->prev = newFile;
                        }

                        break;
                    }

                }

                // 해당 파일이 가장 뒤에 저장되어야 하는 경우
                if(currFile == NULL){
                    // 노드 생성 및 초기화
                    PathList *newFile = (PathList *)malloc(sizeof(PathList));
                    newFile->prev = NULL;
                    newFile->next = NULL;
                    strcpy(newFile->name, namelist[i]->d_name);

                    temp->next = newFile;
                    newFile->prev = temp;
                }

            }
        }

        backupDir = backupDir->next;
    }

    // ------------------------------------------------------

    // 파일 복구 진행
    currFile = listHead;
    while(currFile != NULL){
        // 복구가 가능한 백업 디렉토리 내부의 파일들 목록 가져옴
        recoverFileList = searchRecoverable(recoverDirList, currFile->name);
        
        // 복구 진행
        if(recoverFileList->recoverFileCnt > 0){
            // 복구 가능한 파일이 하나일 때
            if(recoverFileList->recoverFileCnt == 1){
                tmpRecoverNode = recoverFileList->head;
            }
            // 두 개 이상일 때
            else{
                // l 옵션이 적용된 경우 최근에 백업된 파일 복구
                if(opt & OPT_L){
                    // 리스트는 백업 날짜 오름차순으로 저장되어 있음
                    tmpRecoverNode = recoverFileList->head;
                    while(tmpRecoverNode->next != NULL)
                        tmpRecoverNode = tmpRecoverNode->next;
                }
                // 삭제할 번호 입력 받음
                else{
                    num = selectRecoverFile(recoverFileList);

                    // num에 해당하는 파일의 정보를 담고 있는 노드 찾아감
                    if(num > 0)
                        for(i = 1, tmpRecoverNode = recoverFileList->head; i < num; tmpRecoverNode = tmpRecoverNode->next, i++);
                }
            }
            makeDir(newPath);
            sprintf(tmpPath, "%s/%s", newPath, currFile->name);

            recoverFile(tmpRecoverNode, tmpPath, opt, date);
        }
        
        currFile = currFile->next;

    }

    // 옵션 r이 들어온 경우: 디렉토리 내의 모든 하위 디렉토리 및 파일 삭제
    if(opt & OPT_R){
        recoverDirNode = recoverDirList->subDir;

        while(recoverDirNode != NULL){
            // 디렉토리 정보를 저장할 노드 생성 및 초기화
            DirNode *newDirNode = (DirNode *)malloc(sizeof(DirNode));
            newDirNode->recoverDirList = recoverDirNode;
            newDirNode->originPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
            sprintf(newDirNode->originPath, "%s/%s", newPath, recoverDirNode->dirName);
            newDirNode->next = NULL;

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

            recoverDirNode = recoverDirNode->siblingDir;
        }

        // 백업토리 내부의 파일 삭제
        while(curr != NULL){
            tmpDir = curr->recoverDirList;
            strcpy(name, tmpDir->dirName);
            strcpy(tmpPath, curr->originPath);
            
            curr = curr->next;

            recoverDir(tmpDir, tmpPath, opt, date);
        }
    }

    return;

}