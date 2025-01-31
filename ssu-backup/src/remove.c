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

// 삭제 시 BFS를 하기 위해 디렉토리 정보를 따로 저장해두는 연결 리스트
typedef struct DirNode{
  Dir *removeDirList;
  char *originPath;
  struct DirNode *next;
}DirNode;

typedef struct DirList{
  struct DirNode *head;
  struct DirNode *tail;
}DirList;

// BFS로 파일 및 디렉토리를 삭제할 때 사용되는 연결리스트
// 삭제를 진행헤야 하는 순서로 디렉토리들의 정보가 연결되어 있음
DirList *dirList;
DirNode *curr;

// 삭제할 백업 본이 2개 이상안 경우, 해당 백업본들의 정보를 저장해둘 연결 리스트
typedef struct RemoveNode{
    char originPath[MAX_PATH + 1];
    char backupPath[MAX_PATH + 1];
    char backupDate[LEN_DATE + 1];
    char *fileSize;
    struct RemoveNode *next;
}RemoveNode;

typedef struct RemoveFileList{
    struct RemoveNode *head;
    struct RemoveNode *tail;
    int removeFileCnt;
}RemoveFileList;

int getOpt(char *opt, int argc, char *argv[]); // 옵션 추출하여 opt에 저장
int checkOpt(int isReg , char opt); // 압력받은 경로에 맞는 올바른 옵션이 입력되었는지 확인

Dir *canRemove(char *originPath, char *name, int *isReg);
void removeFile(Dir *removeDirList, char *parentPath, char *name, int removeAll, char *date);
//void removeDir(Dir *removeDirList, char *parentPath, int (*filter)(const struct dirent *), int removeAll, char *date);
void removeDir(Dir *removeDirList, char *originPath, int opt, char *date);

// sudo ./remove <PATH> [OPTION] ...
// argv[0]: ./remove
// argv[1]: PATH
// argv[2]: OPTION
// ...
int main(int argc, char *argv[])
{
    Dir *removeDirList; // 삭제하려는 파일의 상위 디렉토리 노드 또는 삭제하려는 다릭토리의 노드
    struct dirent **namelist, **tmplist;
    char *fullPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *parentPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *backupPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *tmpPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *name = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char opt;
    int isReg;
    int cnt;
    int tmpCnt;
    int i;

    // 경로를 입력하지 않은 경우: remove 사용법 출력 후 비정상 종료
    if(argc < 2){
        help("remove");
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
    // 그렇지 않다면 해당 경로는 파일인지 디렉토리인지 isReg에 저장하고
    // 삭제 대상이 되는 경로의 Dir 노드 반환
    if((removeDirList = canRemove(fullPath, name, &isReg)) == NULL){
        fprintf(stderr, "%s that do not exist in backup directory\n", argv[1]);
        exit(1);
    }

    // 옵션을 추출하여 opt에 각각의 비트로 저장
    // 옵션이 올바르지 않은 경우: remove 사용법 출력 후 비정상 종료
    if(getOpt(&opt, argc, argv) < 0){
        help("remove");
        exit(1);
    }

    // 압력받은 경로에 맞는 올바른 옵션이 입력되었는지 확인
    if(checkOpt(isReg, opt) < 0){
        fprintf(stderr, "path and option don't match\n");
        exit(1);
    }

    // ----------------------------------------------------------------

    // 삭제하려는 대상이 일반 파일인 경우
    if(isReg){
        // parentPath: 삭제할 파일의 상위 디렉토리 경로
        strcpy(parentPath, backupPath);
        parentPath[strlen(parentPath) - strlen(name) - 1] = '\0';

        // 옵션 a가 적용된 경우: 백업본이 2개 이상인 파일들에 대해 목록을 출력하지 않고 모든 백업 파일 삭제
        removeFile(removeDirList, parentPath, name, opt & OPT_A, getDate());
    }
    // 삭제하려는 대상이 디렉토리인 경우
    else{
        // "filepath.h"에 정의되어 있는 전역변수들
        // 옵션 r 사용 시에 BFS를 하기 위해 디렉토리들을 순서대로 저장해두는 연결 리스트
        dirList = (DirList *)malloc(sizeof(*dirList));
        dirList->head = NULL;
        dirList->tail = NULL;
        curr = dirList->head;

        // 옵션 d와 r이 모두 사용되었을 때, r 적용
        if((opt & OPT_D) && (opt & OPT_R))
            opt &= ~OPT_D;

        removeDir(removeDirList, fullPath, opt, getDate());

        // // 옵션 d: 해당 디렉토리(fullPath) 아래 있는 모든 파일들에 대해 삭제 진행
        // if(opt & OPT_D)
        //     removeDir(removeDirList, fullPath, filter_onlyfile, opt & OPT_A, getDate());
        // // 옵션 r: 해당 디렉토리(fullPath) 아래의 모든 서브 디렉토리 내 모든 파일애 대해 삭제 진행
        // else if(opt & OPT_R)
        //     removeDir(removeDirList, fullPath, filter_all, opt & OPT_A, getDate());

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
    while((temp = getopt(argc, argv, "dra")) != -1){
        // getopt() 내부에서 optind의 값이 갱신되지 않은 경우: 함수 내부에서 에러가 발생한 것으로 간주
        if(lastind == optind){
            fprintf(stderr, "wrong option input\n");
            exit(1);
        }

        if(temp == 'd')
            *opt |= OPT_D;
        else if(temp == 'r')
            *opt |= OPT_R;
        else if(temp == 'a')
            *opt |= OPT_A;
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
int checkOpt(int isReg, char opt)
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
// 만약 존재하지 않는다면 삭제 불가능
Dir *canRemove(char *originPath, char *name, int *isReg)
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

        // parentPath: 삭제 대상의 상위 디렉토리 경로
        strcpy(parentPath, originPath);
        parentPath[strlen(parentPath) - strlen(name) - 1] = '\0';

        // 삭제하려는 파일의 상위 디렉토리가 리스트 내에 없다면, 해당 파일은 백업 디렉토리에 없음을 의미
        if((dirNode = searchBackupDir(listHead, parentPath)) == NULL)
            return dirNode;

        // backupDirNode: 삭제하려는 파일의 상위 디렉토리와 연동되어 있는 백업 디렉토리들의 리스트
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

        // 삭제 대상의 상위 디렉토리는 있지만, 해당 파일은 존재하지 않는 경우
        if(backupDirNode == NULL)
            return NULL;

    }
    // 해당 디렉토리가 백업 디렉토리 내에 존재하는 경우
    else{
        *isReg = FALSE;
    }

    return dirNode;
}

// 파일 삭제
void removeFile(Dir *removeDirList, char *parentPath, char *name, int removeAll, char *date)
{
    struct stat statbuf;
    RemoveFileList listHead;
    BackupDir *removeDirNode = removeDirList->head;
    char *backupPath = (char *)malloc(sizeof(char *) * (MAX_PATH + 1));
    char *originPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *tmpPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char buf[BUFFER_SIZE];
    FILE *fp;

    // 삭제 대상이 되는 백업 본들을 저장할 리스트 초기화
    listHead.head = NULL;
    listHead.head = NULL;
    listHead.removeFileCnt = 0;

    // 로그 파일 오픈
    if((fp = fopen(logPATH, "a")) < 0){
        fprintf(stderr, "open error for %s\n", logPATH);
        exit(1);
    }

    // 삭제 진행
    while(removeDirNode != NULL){
        sprintf(backupPath, "%s/%s", removeDirNode->backupPath, name);

        // 백업 디렉토리 내에 해당 파일이 존재할 때
        if(!access(backupPath, F_OK)){
            // a 옵션이 적용되었다면 바로 삭제
            if(removeAll){
                // backupPath 경로에 해당하는 파일 삭제
                // 성공 시 0 리턴
                if(remove(backupPath)){
                    fprintf(stderr, "remove error for %s\n", backupPath);
                    exit(1);
                }

                // 로그 파일에 삭제 기록 저장하고 출력
                //sprintf(tmpPath, "%s", backupPath + strlen(backupPATH) + LEN_DATE + 1);
                //sprintf(originPath, "%s%s", homePATH, tmpPath);

                fprintf(fp, "%s : \"%s\" removed by \"%s/%s\"\n", date, backupPath, removeDirNode->parentPath, name);                
                printf("\"%s\" removed by \"%s/%s\"\n", backupPath, removeDirNode->parentPath, name);

                removeEmptyDir(backupPath);
                // // 삭제한 파일의 상위 디렉토리에 더 이상 파일 또는 디렉토리가 존재하지 않는 경우: 해당 디렉토리 삭제
                // // 디렉토리의 하위 파일 및 디렉토리의 개수 리턴
                // if(!isExistSubFile(backupPath, name)){ 
                //     backupPath[strlen(backupPath) - strlen(name) - 1] = '\0';
                    
                //     // 디렉토리 삭제 (성공 시 0 리턴)
                //     if(rmdir(backupPath)){
                //         fprintf(stderr, "rmdir error for %s\n", backupPath);
                //         exit(1);
                //     }
                // }
            }
            // 적용되지 않았다면 listHead가 가리키는 연결리스트에 해당 파일 저장
            else{
                // 삭제 대상이 되는 파일의 정보 가져옴
                if(stat(backupPath, &statbuf) < 0){
                    fprintf(stderr, "stat error for %s\n", backupPath);
                    exit(1);
                }

                // 삭제 대상이 되는 파일의 정보를 담을 노드 생성 및 초기화
                RemoveNode *newRemoveNode = (RemoveNode *)malloc(sizeof(RemoveNode));
                
                sprintf(newRemoveNode->originPath, "%s/%s", removeDirNode->parentPath, name);
                strcpy(newRemoveNode->backupPath, backupPath);
                strcpy(newRemoveNode->backupDate, removeDirNode->dirName);
                newRemoveNode->fileSize = cvtSizeComma(statbuf.st_size);
                newRemoveNode->next = NULL;

                // 삭제 대상이 되는 백업본들을 가리키는 연결 리스트(listHead)에 저장
                if(listHead.head == NULL){
                    listHead.head = listHead.tail = newRemoveNode;
                }
                else{
                    listHead.tail->next = newRemoveNode;
                    listHead.tail = listHead.tail->next;
                }
    
                // 삭제 대상이 되는 백업본의 개수 증가
                listHead.removeFileCnt++;
            }
        }

        removeDirNode = removeDirNode->next;
    }

    // 옵션 a가 적용되지 않은 경우: listHead가 가리키는 파일들 중 하나 선택해서 삭제
    if(!removeAll && listHead.removeFileCnt > 0){
        // 삭제할 백업본이 하나일 때
        if(listHead.removeFileCnt == 1){
            if(remove(listHead.head->backupPath)){
                fprintf(stderr, "remove error for %s\n", listHead.head->backupPath);
                exit(1);
            }

            // 로그 파일에 삭제 기록 저장하고 출력
            // sprintf(tmpPath, "%s", listHead.head->parenPath + strlen(backupPATH) + LEN_DATE + 1);
            // sprintf(originPath, "%s/%s", listHead.head->parentPath, name);

            fprintf(fp, "%s : \"%s\" removed by \"%s\"\n", date, listHead.head->backupPath, listHead.head->originPath);                
            printf("\"%s\" removed by \"%s\"\n", listHead.head->backupPath, listHead.head->originPath);

            removeEmptyDir(backupPath);
            // // 삭제한 파일의 상위 디렉토리에 더 이상 파일 또는 디렉토리가 존재하지 않는 경우: 해당 디렉토리 삭제
            // // 디렉토리의 하위 파일 및 디렉토리의 개수 리턴
            // if(!isExistSubFile(listHead.head->backupPath, name)){ 
            //     listHead.head->backupPath[strlen(listHead.head->backupPath) - strlen(name) - 1] = '\0';

            //     // 디렉토리 삭제 (성공 시 0 리턴)
            //     if(rmdir(listHead.head->backupPath)){
            //         fprintf(stderr, "rmdir error for %s\n", listHead.head->backupPath);
            //         exit(1);
            //     }
            // }            
        }
        // 삭제할 백업본이 2개 이상일 때
        else{
            RemoveNode *temp = listHead.head;
            int num;
            int i;

            num = 1;

            // 삭제가 가능한 백업본 출력하고 삭제 대상 입력 받음
            printf("backup files of \"%s/%s\"\n", parentPath, name);
            printf("0. exit\n");
            while(temp != NULL){
                printf("%d. %s\t\t%sbytes\n", num, temp->backupDate, temp->fileSize);

                temp = temp->next;
                ++num;
            }
            printf(">> ");

            scanf("%d", &num);
            
            // 입력 받은 번호(num)에 해당하는 파일 삭제
            // 0 입력 시, 해당 파일에 대한 삭제 작업을 진행하지 않음
            if(num != 0){
                // num에 해당하는 파일의 정보를 담고 있는 노드 찾아감
                for(i = 1, temp = listHead.head; i < num; temp = temp->next, i++);
                
                if(remove(temp->backupPath)){
                    fprintf(stderr, "remove error for %s\n", temp->backupPath);
                    exit(1);
                }

                // 로그 파일에 삭제 기록 저장하고 출력
                //sprintf(tmpPath, "%s", temp->backupPath + strlen(backupPATH) + LEN_DATE + 1);
                //sprintf(originPath, "%s%s", homePATH, tmpPath);

                //sprintf(originPath, "%s/%s", listHead.head->parentPath, name);

                fprintf(fp, "%s : \"%s\" removed by \"%s\"\n", date, temp->backupPath, temp->originPath);                
                printf("\"%s\" removed by \"%s\"\n", temp->backupPath, temp->originPath);
            
                removeEmptyDir(backupPath);
                // // 삭제한 파일의 상위 디렉토리에 더 이상 파일 또는 디렉토리가 존재하지 않는 경우: 해당 디렉토리 삭제
                // // 디렉토리의 하위 파일 및 디렉토리의 개수 리턴
                // if(!isExistSubFile(temp->backupPath, name)){ 
                //     temp->backupPath[strlen(temp->backupPath) - strlen(name) - 1] = '\0';
                    
                //     // 디렉토리 삭제 (성공 시 0 리턴)
                //     if(rmdir(temp->backupPath)){
                //         fprintf(stderr, "rmdir error for %s\n", temp->backupPath);
                //         exit(1);
                //     }
                // }
            }

        }
    }

    fclose(fp);
}

// 디렉토리 삭제
void removeDir(Dir *removeDirList, char *originPath, int opt, char *date)
{
    PathList *listHead = NULL;
    PathList *currFile;
    PathList *temp;
    struct dirent **namelist;
    Dir *removeDirNode;
    Dir *tmpDir;
    BackupDir *backupDir;
    BackupDir *tmpBackupDir;
    char *tmpPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *name = (char *)malloc(sizeof(char) * (MAX_NAME + 1));
    int result;
    int cnt;
    int i;

    backupDir = removeDirList->head;

    // removeDirList가 가리키는 디렉토리 경로 내에 존재하는 모든 파일의 이름을 연결리스트(listHead)에 저장
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

    // 파일 삭제
    currFile = listHead;
    while(currFile != NULL){
        removeFile(removeDirList, originPath, currFile->name, opt & OPT_A, date);
        currFile = currFile->next;
    }

    // 옵션 r이 들어온 경우: 디렉토리 내의 모든 하위 디렉토리 및 파일 삭제
    if(opt & OPT_R){
        removeDirNode = removeDirList->subDir;

        while(removeDirNode != NULL){
            // 디렉토리 정보를 저장할 노드 생성 및 초기화
            DirNode *newDirNode = (DirNode *)malloc(sizeof(DirNode));
            newDirNode->removeDirList = removeDirNode;
            newDirNode->originPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
            sprintf(newDirNode->originPath, "%s/%s", originPath, removeDirNode->dirName);
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

            removeDirNode = removeDirNode->siblingDir;
        }

        // 백업토리 내부의 파일 삭제
        while(curr != NULL){
            tmpDir = curr->removeDirList;
            strcpy(name, tmpDir->dirName);
            strcpy(tmpPath, curr->originPath);
            
            curr = curr->next;

            removeDir(tmpDir, tmpPath, opt, date);
        }
    }

    return;
}

// // 디렉토리 삭제
// void removeDir(Dir *removeDirList, char *originPath, int (*filter)(const struct dirent *), int removeAll, char *date)
// {
//     char *filePath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
//     char *backupPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
//     Dir *tmpDirList;
//     BackupDir *backupDir;
//     struct dirent **namelist;
//     struct stat statbuf;
//     int cnt;
//     int i;

//     // 성공 시 namelist에 저장된 파일/디렉토리의 개수(cnt), 실패 시 -1 리턴
//     // filter
//     // - 옵션 d: filter_onlyfile() -> originPath의 하위 파일들만 추출
//     // - 옵션 r: filter_all() -> originPath의 하위 파일 및 서브 디렉토리 내 파일들 모두 추출
//     printf("origin: %s\n", originPath);
//     if((cnt = scandir(originPath, &namelist, filter, alphasort)) == -1){
//         fprintf(stderr, "scandir error for %s\n", originPath);
// 		exit(1);
//     }

//     // 추출한 파일 및 디렉토리를 바탕으로 삭제 진행
//     for(i = 0; i < cnt; i++){
//         // originPath: 삭제하려고 하는 디렉토리의 경로
//         // filePath: 삭제하려고 하는 디렉토리의 하위 파일 및 서브 디렉토리 원본 경로
//         sprintf(filePath, "%s/%s", originPath, namelist[i]->d_name);
//         if(lstat(filePath, &statbuf) < 0){
//             fprintf(stderr, "lstat error for %s\n", filePath);
//             exit(1);
//         }

//         // 일반 파일인 경우: 삭제 진행
//         if(S_ISREG(statbuf.st_mode)){
//             removeFile(removeDirList, originPath, namelist[i]->d_name, removeAll, date);
//         }
//         // 디렉토리인 경우: 파일의 백업이 끝난후 백업 진행
//         else if(S_ISDIR(statbuf.st_mode)){
//             // 해당 디렉토리의 정보를 저장할 노드 생성 및 초기화
//             DirNode *newDirNode = (DirNode *)malloc(sizeof(DirNode));
//             newDirNode->removeDirList = searchBackupDir(removeDirList, namelist[i]->d_name); // namelist[i]->d_name에 해당하는 디렉토리의 정보를 담은 노드 리턴
//             newDirNode->originPath = (char *)malloc(sizeof(char) * (strlen(filePath) + 1));
//             strcpy(newDirNode->originPath, filePath);
//             newDirNode->next = NULL;

//             // dirList 내에 DirNode가 하나도 없는 경우 (빈 연결 리스트인 경우)
//             if(dirList->head == NULL){
//                 dirList->head = dirList->tail = newDirNode;
//                 curr = dirList->head;
//                 printf(">> %s\n", curr->originPath);
//             }
//             // 빈 리스트가 아닌 경우, 연결 리스트의 가장 뒤에 노드 연결
//             else{
//                 dirList->tail->next = newDirNode;
//                 dirList->tail = dirList->tail->next;
//             }
//         }
//         // 일반 파일 또는 디렉토리가 아닌 경우: 에러
//         else{
//             fprintf(stderr, "%s is not directory or regular file\n", filePath);
//             exit(1);    
//         }
//     }

//     // 일반 파일을 모두 삭제 완료한 상태 -> 디렉토리 내부의 파일들을 삭제할 차례
//     while(curr != NULL){
//         tmpDirList = curr->removeDirList;
//         strcpy(filePath, curr->originPath);

//         //printf("tmpDirList: %s\n", tmpDirList->);
//         printf("filePath: %s\n", filePath);
//         curr = curr->next;


//         removeDir(tmpDirList, filePath, filter, removeAll, date);
//     }
    
//     return;
// }