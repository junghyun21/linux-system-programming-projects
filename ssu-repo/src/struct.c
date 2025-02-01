#include "struct.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

static int filter_all(const struct dirent *entry);
// static int filter_onlyfile(const struct dirent *entry);
// static int filter_onlyDir(const struct dirent *entry);

// struct.h에 전역변수로 정의되어 있음
DirNode *unknownPaths; // 원본 파일들을 리스트로 관리
DirNode *trackedPaths; // 스테이징 구역(.staging.log)을 바탕으로 생성한 추적할 경로들의 연결 리스트
DirNode *untrackedPaths; // 스테이징 구역(.staging.log)을 바탕으로 생성한 추적하지 않을 경로들의 연결 리스트
DirNode *commitList; // 커밋 로그(.commit.log)를 바탕으로 커밋된 파일 중 최근에 커밋된 버전 관리

ChangeNode *changesList; // 변경 사항을 저장한 연결 리스트

FileNode *recoverList; // 원본 경로로 복원을 진행한 파일들의 연결 리스트

// -------------------------------stagingList--------------------------------------

// 변경사항 추적 경로와 관련된 연결 리스트(stagingList) 생성
void initStagingList()
{
    FILE *fp;
    char buf[BUFFER_SIZE];
    char cmd[BUFFER_SIZE];
    char fullPath[BUFFER_SIZE];

    // .staging.log 파일을 바탕으로 변경 사항 추적 경로의 연결 리스트 생성하기 위해 해당 파일 오픈
    // .staging.logg를 오픈
    if((fp = fopen(stagingPATH, "rb")) == NULL){
        fprintf(stderr, "fopen error for %s\n", stagingPATH);
        exit(1);
    }

    // 해당 경로의 하위 파일들에 대한 연결리스트 저장 (unknownPaths);
    initOriginList(&unknownPaths, exePATH); 

    // 오픈한 .staging.log 파일에서 로그 기록 한 줄씩 읽어옴
    // 파일/디렉토리에 대해서만 연결리스트의 삽입 및 삭제 진행 
    // add이면 연결 리스트에 해당 경로 삽입, remove이면 연결 리스트에서 해당 경로 삭제
    // feof(): 파일의 끝이 아닌 경우 0 리턴
    while(fgets(buf, sizeof(buf), fp) != NULL){
        buf[strlen(buf) - 1] = '\0';
        
        // add/remove "path"
        sscanf(buf, "%s \"%[^\"]\"", cmd, fullPath);
        
        // 해당 경로의 파일/디렉토리가 존재하지 않는 경우: 연결리스트의 삽입/삭제 진행하지 않음
        if(access(fullPath, F_OK))
            continue;

        // staging.log 파일에서 읽어온 경로가 현재 작업 경로를 벗어난 경로인 경우: 에러 처리 후 프롬프트 재출력
        if(strncmp(fullPath, exePATH, strlen(exePATH))){
            fprintf(stderr, "ERROR: \"staging.log\" file contains invalid values\n");
            exit(1);
        }

        // printf("cmd: %s\n", cmd);
        // printf("fullPath: %s\n", fullPath);

        // 추적 대상이 된 경로는 unknownPaths에서 삭제
        deleteStagingList(&unknownPaths, fullPath);

        // 해당하는 경로 연결리스트에 삽입/삭제
        // add: 추적할 경로들의 연결리스트(trackedPaths)에 삽입, 추적하지 않을 경로들의 연결리스트(untrackedPaths)에서 삭제
        if(!strcmp(cmd, "add")){
            insertStagingList(&trackedPaths, fullPath);
            deleteStagingList(&untrackedPaths, fullPath);
        }
        // remove: 추적할 경로들의 연결리스트(trackedPaths)에서 삭제, 추적하지 않을 경로들의 연결리스트(untrackedPaths)에 삽입
        else if(!strcmp(cmd, "remove")){
            deleteStagingList(&trackedPaths, fullPath);
            insertStagingList(&untrackedPaths, fullPath);
        }
        else{
            fprintf(stderr, "ERROR: \"staging.log\" file contains invalid command\n");
            exit(1);
        }
    }

    fclose(fp);
}

// fullPath에서 현재 작업 경로를 제외한 경로를 바탕으로 연결 리스트 삽입
// 현재 작업 경로에 해당하는 디렉토리의 노드는 이미 연결리스트에 존재
void insertStagingList(DirNode **listHead, char *fullPath)
{
    DirNode *currDir;
    struct stat statbuf;
    char *path;
    char *tmpPath;
    char *token = NULL;

    // 빈 연결 리스트일 때, 현재 작업 경로의 정보를 담은 디렉토리 노드 생성
    if(*listHead == NULL){
        DirNode *newDirNode = (DirNode *)malloc(sizeof(DirNode));

        strcpy(newDirNode->name, exePATH);
        strcpy(newDirNode->path, exePATH);
        newDirNode->parentDir = NULL;
        newDirNode->subDir = NULL;
        newDirNode->next = NULL;
        newDirNode->prev = NULL;
        newDirNode->fileHead = NULL;
        newDirNode->isTracked = FALSE;

        *listHead = newDirNode;
    }

    currDir = *listHead;

    // 현재 작업 경로 전체가 대상이 아닌 경우에만 연결리스트에 노드 삽입 진행
    // stagingList는 현재 작업 경로의 정보를 담고 있는 노드를 가리킴
    if(strcmp(fullPath, exePATH)){
        // tmpPath: strtok()를 진행할 때 사용, fullPath에서 현재 작업 경로를 제외한 경로
        // path: 해당 경로의 파일/디렉토리의 정보를 확인하고, 노드에 경로를 저장할 때 사용
        path = (char *)malloc(sizeof(char) * (strlen(fullPath) + 1));
        tmpPath = (char *)malloc(sizeof(char) * (strlen(fullPath) + 1));

        strcpy(tmpPath, fullPath + strlen(exePATH) + 1); 
        strcpy(path, exePATH);

        // 해당하는 노드가 연결리스트 내에 존재하는지 확인하고, 없으면 노드를 생성하여 연결 리스트에 삽입
        token = strtok(tmpPath, "/");
        while(token != NULL){
            sprintf(path, "%s/%s", path, token);

            // path에 해당하는 경로의 정보 획득
            if(lstat(path, &statbuf) < 0){
                fprintf(stderr, "lstat error for %s\n", fullPath);
                exit(1);
            }

            // path가 파일인 경우: fileNode 삽입
            if(S_ISREG(statbuf.st_mode)){
                insertFileNode(&currDir, path, token, "");
                break;
            }
            // path가 디렉토리인 경우: dirNode 삽입
            else if(S_ISDIR(statbuf.st_mode)){
                currDir = insertDirNode(&currDir, path, token);
            }
            // 파일 및 디렉토리가 아닌 경우: 에러 출력 후, 프롬프트 재출력
            else{
                fprintf(stderr, "ERROR: \"staging.log\" file contains invalid path\n");
                exit(1);
            }

            token = strtok(NULL, "/");
        }

        free(path);
        free(tmpPath);
    }

    // 추적 경로 대상이 파일인 경우
    //  - 해당 파일의 상위 디렉토리 노드의 멤버변수인 isTracked는 false
    //  - 해당 파일이 삭제되면, 생성된 상위 디렉토리 노드도 삭제되어야 함
    // 추적 경로 대상이 디렉토리인 경우 (token == NULL)
    //  - 해당 디렉토리의 노드의 멤버변수인 isTracked는 true
    //  - 디렉토리 하위의 파일/디렉토리가 모두 삭제되더라도, 해당 디렉토리의 노드는 삭제되지 않음
    //  - 원본 파일 경로를 참고하여 해당 디렉토리의 하위 파일/디렉토리도 연결리스트에 연결
    if(token == NULL){
        struct dirent **namelist;
        int cnt;
        int i;

        //path: 해당 경로의 하위 파일/디렉토리의 경로를 노드에 저장할 때 사용
        path = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
        currDir->isTracked = TRUE;

        // fullPath에 해당하는 디렉토리의 하위 파일 및 디렉토리들 추출
        // 추출한 파일/디렉토리를 연결 리스트에 연결
        if((cnt = scandir(fullPath, &namelist, filter_all, alphasort)) < 0){
            fprintf(stderr, "scandir error for %s\n", fullPath);
            exit(1);
        }

        for(i = 0; i < cnt; i++){
            sprintf(path, "%s/%s", fullPath, namelist[i]->d_name);
            insertStagingList(listHead, path);
        }
    }

    return;

}

void deleteStagingList(DirNode **listHead, char *fullPath)
{
    DirNode *currDir;
    struct stat statbuf;

    // fullPath가 디렉토리인지 파일인지 확인
    if(lstat(fullPath, &statbuf) < 0){
        fprintf(stderr, "lstat error for %s\n", fullPath);
        exit(1);
    }

    // 파일인 경우
    if(S_ISREG(statbuf.st_mode)){
        char *name;
        char *parentPath;
        int i;
        
        // 해당 파일의 부모 디렉토리 및 이름 추출
        parentPath = (char *)malloc(sizeof(char) * (strlen(fullPath) + 1));
        strcpy(parentPath, fullPath);

        for(i = strlen(parentPath) - 1; i >= 0 && parentPath[i] != '/'; i--);
        parentPath[i] = '\0';

        name = (char *)malloc(sizeof(char) * (strlen(parentPath) - i));
        strcpy(name, parentPath + i + 1);
        
        // 해당 파일의 상위 디렉토리의 정보를 담고 있는 디렉토리 탐색하고, name에 해당하는 파일 노드 삭제
        if((currDir = searchDirNode(*listHead, parentPath)) != NULL)
            deleteFileNode(&currDir, name);
        
    }
    // 디렉토리인 경우
    else if(S_ISDIR(statbuf.st_mode)){
        // fullPath의 정보를 담고 있는 디렉토리 탐색하고, 해당 디렉토리 노드와 하위 디렉토리/파일 모두 삭제
        if((currDir = searchDirNode(*listHead, fullPath)) != NULL)
            deleteDirNode(&currDir);
        
    }
    // 파일 또는 디렉토리 모두 아닌 경우: 에러 처리 후, 프롬프트 재출력₩
    else{
        fprintf(stderr, "ERROR: \"staging.log\" file contains invalid path\n");
        exit(1);
    }

    // 추적 대상이 아닌 디렉토리 노드들 중 하위 파일 및 디렉토리가 없는 경우: 해당 디렉토리 노드 삭제
    deleteNotTrackedDirNode(listHead);

    // 전체 경로를 삭제한 경우: 전역변수들도 NULL로 변경
    if(*listHead != NULL && !strcmp(fullPath, exePATH))
        *listHead = NULL;

}

// 가장 하위에 위치한 디렉토리 노드로 이동하여, 상위 디렉토리로 올라오면서 하나씩 삭제
void deleteNotTrackedDirNode(DirNode **listHead)
{
    DirNode *currDir = *listHead;

    if(currDir != NULL){
        // 가장 하위 디렉토리 중, 리스트에서 가장 마지막에 위치한 디렉토리로 이동
        deleteNotTrackedDirNode(&(currDir->subDir));
        deleteNotTrackedDirNode(&(currDir->next));

        // 해당 디렉토리가 추적 대상이 아니고, 하위 파일/디렉토리가 모두 없는 경우: 해당 디렉토리 노드 삭제
        if(currDir->isTracked == FALSE && currDir->fileHead == NULL && currDir->subDir == NULL){
            // 현재 작업 디렉토리 노드인 경우
            if(!strcmp(currDir->path, exePATH)){
                free(currDir);
                *listHead = NULL;
            }
            else{
                // 해당 디렉토리의 형제 노드가 없는 경우: 상위 디렉토리 노드의 하위 디렉토리 리스트는 NULL을 가리킴
                // currDir은 가장 하위에 위치한 디렉토리 노드들 중, 연결 리스트에서 가장 마지막에 저장된 노드
                if(currDir->prev == NULL){
                    currDir->parentDir->subDir = NULL;  
                }
                // 해당 디렉토리의 형제 노드가 있는 경우: 해당 노드만 삭제
                else{
                    currDir->prev->next = currDir->next;
                    if(currDir->next != NULL)
                        currDir->next->prev = currDir->prev;

                }

                free(currDir);
            }
        }
    }

    return;
}

// 기존 원본 경로의 파일들에 대한 연결리스트(unknownPaths) 생성
void initOriginList(DirNode **dir, char *fullPath)
{
    DirNode *currDir;
    DirNode *tmpDir;
    struct stat statbuf;
    struct dirent **namelist;
    char *path;
    int cnt;
    int i;

    // 빈 연결 리스트일 때, 현재 작업 경로의 정보를 담은 디렉토리 노드 생성
    if(*dir == NULL){
        DirNode *newDirNode = (DirNode *)malloc(sizeof(DirNode));

        strcpy(newDirNode->name, fullPath);
        strcpy(newDirNode->path, fullPath);
        newDirNode->parentDir = NULL;
        newDirNode->subDir = NULL;
        newDirNode->next = NULL;
        newDirNode->prev = NULL;
        newDirNode->fileHead = NULL;
        newDirNode->isTracked = FALSE;

        *dir = newDirNode;
    }
        
    currDir = *dir;

    // 해당 경로(fullPath)의 하위 디렉토리 및 파일의 정보 추출
    if((cnt = scandir(fullPath, &namelist, filter_all, alphasort)) < 0){
        fprintf(stderr, "scandir error for %s\n", fullPath);
        exit(1);
    }

    // 하위 디렉토리 및 파일 추가
    path = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    for(i = 0; i < cnt; i++){
        sprintf(path, "%s/%s", fullPath, namelist[i]->d_name);

        if(lstat(path, &statbuf) < 0){
            fprintf(stderr, "lstat error for %s\n", path);
            exit(1);
        }

        // 파일인 경우
        if(S_ISREG(statbuf.st_mode)){
            insertFileNode(&currDir, path, namelist[i]->d_name, "");
        }
        // 디렉토리인 경우
        else if(S_ISDIR(statbuf.st_mode)){
            tmpDir = currDir;

            currDir = insertDirNode(&currDir, path, namelist[i]->d_name);
            initOriginList(&currDir, path);

            currDir = tmpDir;
        }
    }

    free(path);

    return;
}

// 스테이징 구역에 포함되지 않은 경로들의 노드 출력
void printUnknownPaths(DirNode *dir)
{
    DirNode *currDir = dir;
    FileNode *currFile;
    char *path = (char *)malloc(sizeof(char) * (MAX_PATH + 1));

    // 상대경로로 출력
    if(currDir != NULL){
        // 하위 파일 출력
        currFile = currDir->fileHead;
        while(currFile != NULL){
            sprintf(path, ".%s", currFile->path + strlen(exePATH));
            printf("\tnew file: \"%s\"\n", path);

            currFile = currFile->next;
        }

        // 디렉토리 이동
        currDir = currDir->subDir;
        while(currDir != NULL){
            printUnknownPaths(currDir);
            currDir = currDir->next;
        }
    }

    return;
}

// ---------------------------------commitList------------------------------------

// 레포 디렉토리에 백업된 파일 리스트(commitList) 생성
// commitList: 커밋된 내역들 중 commitName까지의 상태 관리 (NULL이면 전체 관리)
void initCommitList(char *commitName)
{
    FILE *fp;
    char buf[BUFFER_SIZE];
    char tmpName[BUFFER_SIZE];
    char filePath[BUFFER_SIZE];
    char *token = NULL;
    char action;
    int isSame = FALSE;
    int isExist = FALSE;

    // commit.log 오픈
    if((fp = fopen(commitPATH, "rb")) == NULL){
        fprintf(stderr, "fopen error for %s\n", commitPATH);
        exit(1);
    }

    memset(tmpName, 0 ,sizeof(tmpName));

    // 한 줄씩 읽어서, commitList 초기화
    while(fgets(buf, sizeof(buf), fp) != NULL){
        // new file: commitList에 노드 추가
        if(sscanf(buf, "commit: \"%[^\"]\" - new file: \"%[^\"]\"", tmpName, filePath) == 2){
            action = 'n';
        }
        // modified: commitList에 있던 기존 노드에서 commit 멤버 변수를 tmpName으로 변경
        else if(sscanf(buf, "commit: \"%[^\"]\" - modified: \"%[^\"]\"", tmpName, filePath) == 2){
            action = 'm';
        }
        // removed: commitList에서 노드 삭제
        else if(sscanf(buf, "commit: \"%[^\"]\" - removed: \"%[^\"]\"", tmpName, filePath) == 2){
            action = 'r';
        }
        else{
            fprintf(stderr, "ERROR: \"commit.log\" file contains invalid values\n");
            exit(1);
        }

        // 특정 버전 상태의 연결 리스트를 만들어야하는 경우
        if(commitName != NULL){
            if(!strcmp(tmpName, commitName)){
                isSame = TRUE;
                isExist = TRUE;
            }
            
            else if(isSame)
                break;
        }

        switch(action){
            case 'n':
                insertCommitList(&commitList, filePath, tmpName);
                break;

            case 'm':
                modifyCommitList(commitList, filePath, tmpName);
                break;

            case 'r':
                deleteCommitList(commitList, filePath);
                break;

            default:
                fprintf(stderr, "ERROR: \"commit.log\" file contains invalid values\n");
                exit(1);
        }

    }

    // 특정 버전까지의 커밋 상태의 연결리스트를 생성할 때, 주어진 버전 이름(commitName)으로 커밋한 적이 없는 경우
    if(commitName != NULL && !isExist){
        fprintf(stderr, "\"%s\" is not exist in repo\n", commitName);
        exit(1);
    }

    fclose(fp);
}

void insertCommitList(DirNode **dir, char *fullPath, char *commitName)
{
    DirNode *currDir;
    struct stat statbuf;
    char *tmpPath;
    char *parentPath;
    char *name;
    char *token = NULL;
    int i;

    // 빈 연결 리스트일 때, 현재 작업 경로의 정보를 담은 디렉토리 노드 생성
    if(*dir == NULL){
        DirNode *newDirNode = (DirNode *)malloc(sizeof(DirNode));

        strcpy(newDirNode->name, fullPath);
        strcpy(newDirNode->path, fullPath);
        newDirNode->parentDir = NULL;
        newDirNode->subDir = NULL;
        newDirNode->next = NULL;
        newDirNode->prev = NULL;
        newDirNode->fileHead = NULL;
        newDirNode->isTracked = FALSE;

        *dir = newDirNode;
    }

    currDir = *dir;

    // 해당 파일의 부모 디렉토리 및 이름 추출
    parentPath = (char *)malloc(sizeof(char) * (strlen(fullPath) + 1));
    strcpy(parentPath, fullPath);

    for(i = strlen(parentPath) - 1; i >= 0 && parentPath[i] != '/'; i--);
    parentPath[i] = '\0';

    name = (char *)malloc(sizeof(char) * (strlen(parentPath) - i));
    strcpy(name, parentPath + i + 1);

    // 상위 디렉토리 경로에서 현재 작업 디렉토리 경로 제외
    sprintf(parentPath, "%s", parentPath + strlen(exePATH));

    // fullPath: 파일의 경로
    // fullPath의 상위 디렉토리인 parentPath에 대한 정보를 담고 있는 디렉토리 노드 생성
    tmpPath = (char *)malloc(sizeof(char) * (strlen(fullPath) + 1));
    strcpy(tmpPath, exePATH);

    token = strtok(parentPath, "/");
    while(token != NULL){
        sprintf(tmpPath, "%s/%s", tmpPath, token);
        currDir = insertDirNode(&currDir, tmpPath, token);

        token = strtok(NULL, "/");
    }

    // parentPath에 해당하는 디렉토리 노드에 fullPath에 해당하는 파일 노드 추가
    insertFileNode(&currDir, fullPath, name, commitName);

    free(tmpPath);
}

void modifyCommitList(DirNode *list, char *fullPath, char *commitName)
{
    DirNode *currDir;
    FileNode *currFile;
    char *parentPath;
    char *name;
    int i;

    // 해당 파일의 부모 디렉토리 및 이름 추출
    parentPath = (char *)malloc(sizeof(char) * (strlen(fullPath) + 1));
    strcpy(parentPath, fullPath);

    for(i = strlen(parentPath) - 1; i >= 0 && parentPath[i] != '/'; i--);
    parentPath[i] = '\0';

    name = (char *)malloc(sizeof(char) * (strlen(parentPath) - i));
    strcpy(name, parentPath + i + 1);

    // fullPath(파일)의 상위 디렉토리에 해당하는 디렉토리 노드 반환 받음
    if((currDir = searchDirNode(list, parentPath)) != NULL){
        if((currFile = searchFileNode(currDir, name)) != NULL){
            memset(currFile->commit, 0, MAX_NAME);
            strcpy(currFile->commit, commitName);
        }
    }
}

void deleteCommitList(DirNode *list, char *fullPath)
{
    DirNode *currDir;
    FileNode *currFile;
    char *parentPath;
    char *name;
    int i;

    // 해당 파일의 부모 디렉토리 및 이름 추출
    parentPath = (char *)malloc(sizeof(char) * (strlen(fullPath) + 1));
    strcpy(parentPath, fullPath);

    for(i = strlen(parentPath) - 1; i >= 0 && parentPath[i] != '/'; i--);
    parentPath[i] = '\0';

    name = (char *)malloc(sizeof(char) * (strlen(parentPath) - i));
    strcpy(name, parentPath + i + 1);

    // fullPath(파일)의 상위 디렉토리에 해당하는 디렉토리 노드 반환 받음
    if((currDir = searchDirNode(list, parentPath)) != NULL)
        deleteFileNode(&currDir, name);

    // list에 해당하는 디렉토리 내에 하위 디렉토리 및 파일이 없는 경우: 디렉토리 노드 삭제
    if(currDir->fileHead == NULL && currDir->subDir == NULL){
        // 첫 번째 노드를 삭제하는 경우
        if(currDir->prev == NULL){
            currDir->parentDir->subDir = currDir->next;

            if(currDir->next != NULL)
                currDir->next->prev = currDir->prev;
            else // 마지막 노드인 경우
                currDir->parentDir = NULL;
        }
        // 그 외의 노드를 삭제하는 경우
        else{
            currDir->prev->next = currDir->next;

            if(currDir->next != NULL)
                currDir->next->prev = currDir->prev;
        }
        
        free(currDir);
    }
}

// 해당 이름으로 커밋한 적이 있는지 확인하고, 없으면 해당 이름으로 버전 디렉토리 생성 가능
int isAvailableCommitName(char *commitName)
{
    FILE *fp;
    char buf[BUFFER_SIZE];
    char tmp[BUFFER_SIZE];
    int i;

    if((fp = fopen(commitPATH, "rb")) == NULL){
        fprintf(stderr, "fopen error for %s\n", commitPATH);
        exit(1);
    }

    while(fgets(buf, sizeof(buf), fp) != NULL){
        if(sscanf(buf, "commit: \"%[^\"]\"", tmp) == 1){
            if(!strcmp(commitName, tmp))
                return FALSE;
        }
        
        memset(buf, 0, sizeof(buf));
    }

    fclose(fp);
    
    return TRUE;
}


// ---------------------------------changesList------------------------------------

// trackedPaths와 commitList를 비교하여 변경 사항을 changesList에 저장하고, 세부 변경 내용을 각 인자에 저장
void checkChanges(DirNode *trackedDirNode, DirNode *commitDirNode, ChangeNode **prevNode)
{
    ChangeNode *currNode = *prevNode;
    DirNode *trackedDir = trackedDirNode;
    DirNode *commitDir = commitDirNode;
    DirNode *tmpDir;
    FileNode *trackedFile;
    FileNode *commitFile;
    char *parentPath;
    int i;
    int result;
    int isNotCommit;

    if(trackedDir != NULL && commitDir != NULL){
        // 각 디렉토리 노드의 하위 파일 노드 비교
        trackedFile = trackedDir->fileHead;
        commitFile = commitDir->fileHead;
        while(trackedFile != NULL && commitFile != NULL){
            result = strcmp(trackedFile->name, commitFile->name);
            // commitFile->name이 사전 순으로 더 뒤에 위치한 경우: trackedFile은 백업된 적이 없음
            if(result < 0){
                isNotCommit = TRUE;
                insertChangeNode(trackedFile, NULL, &currNode, isNotCommit);

                trackedFile = trackedFile->next;
            }
            // 각 파일들은 오름차순으로 연결되어 있음
            // 백업 이력이 있는 경우
            else if(result == 0){
                isNotCommit = FALSE;
                insertChangeNode(trackedFile, commitFile, &currNode, isNotCommit);

                trackedFile = trackedFile->next;
                commitFile = commitFile->next;
            }
            // commitFile->name이 사전 순으로 더 잎에 위치한 경우
            // 이전에 백업되었던 commitFile은 현재 추적 대상이 아니거나
            // 원본 경로에서 삭제된 것
            else{
                isNotCommit = FALSE;
                
                // 해당 파일의 상위 디렉토리 경로
                parentPath = (char *)malloc(sizeof(char) * (strlen(commitFile->path) + 1));
                strcpy(parentPath, commitFile->path);

                for(i = strlen(parentPath) - 1; i >= 0 && parentPath[i] != '/'; i--);
                parentPath[i] = '\0';

                // 추적 대상이 아닌지 확인 (untrackedPaths 리스트에 저장되어 있는지 확인)
                // 해당 리스트에 저장되어 있다면 아무런 작업도 하지 않음
                // untrackedPaths 내에 없다면 삭제된 것
                if((tmpDir = searchDirNode(untrackedPaths, parentPath)) == NULL){
                    insertChangeNode(NULL, commitFile, &currNode, isNotCommit);
                }
                else{
                    if(searchFileNode(tmpDir, commitFile->path) == NULL)
                        insertChangeNode(NULL, commitFile, &currNode, isNotCommit);
                    
                }

                free(parentPath);

                commitFile = commitFile->next;
            }
        }

        // trackedFile 내의 노드가 남은 경우: 이전에 백업된 적이 없는 파일들
        while(trackedFile != NULL){
            isNotCommit = TRUE;
            insertChangeNode(trackedFile, NULL, &currNode, isNotCommit);

            trackedFile = trackedFile->next;
        }

        // commitFile 내의 노드가 남은 경우: 추적 대상이 아니거나, 삭제되었거나
        while(commitFile != NULL){
            isNotCommit = FALSE;
            
            // 해당 파일의 상위 디렉토리 경로
            parentPath = (char *)malloc(sizeof(char) * (strlen(commitFile->path) + 1));
            strcpy(parentPath, commitFile->path);

            for(i = strlen(parentPath) - 1; i >= 0 && parentPath[i] != '/'; i--);
            parentPath[i] = '\0';

            // 추적 대상이 아닌지 확인(untrackedPaths 리스트에 저장되어 있는지 확인)하고 노드 추가
            if((tmpDir = searchDirNode(untrackedPaths, parentPath)) == NULL){
                insertChangeNode(NULL, commitFile, &currNode, isNotCommit);
            }
            else{
                if(searchFileNode(tmpDir, commitFile->path) == NULL)
                    insertChangeNode(NULL, commitFile, &currNode, isNotCommit);
            }

            free(parentPath);

            commitFile = commitFile->next;
        }

        // 각 디렉토리 노드의 하위 디렉토리로 이동
        trackedDir = trackedDir->subDir;
        commitDir = commitDir->subDir;

        while(trackedDir != NULL && commitDir != NULL){
            // 디렉토리도 오름차순으로 저장되어 있음
            result = strcmp(trackedDir->name, commitDir->name);

            // 새로 추가해야하는 경우
            if(result < 0){
                isNotCommit = TRUE;

                insertSubFileNode(trackedDir, &currNode, isNotCommit);
                trackedDir = trackedDir->next;
            }
            else if(result == 0){
                checkChanges(trackedDir, commitDir, &currNode);
                trackedDir = trackedDir->next;
                commitDir = commitDir->next;
            }
            // 삭제되었거나 추적 경로가 아니게 된 경우
            else{
                isNotCommit = FALSE;

                insertSubFileNode(commitDir, &currNode, isNotCommit);
                commitDir = commitDir->next;
            }

        }

    }

    // trackedDir 노드가 남은 경우: 이전에 백업된 적이 없는 추적 대상이 되는 파일들의 상위(루트) 디렉토리
    // 해당 디렉토리 노드의 하위 파일들 중 원본 경로에 존재하는 파일들 모두 new file로 changesList에 추가
    if(trackedDir != NULL){
        isNotCommit = TRUE;
        insertSubFileNode(trackedDir, &currNode, isNotCommit);
    }

    // 삭제되었거나 더 이상 추적 경로가 아닌 경우
    if(commitDir != NULL){
        isNotCommit = FALSE;
        insertSubFileNode(commitDir, &currNode, isNotCommit);
    }
    
    return;
}

void insertChangeNode(FileNode *trackedFile, FileNode *commitFile, ChangeNode **prevNode, int isNotCommit)
{
    ChangeNode *newNode;
    FileNode *currFile;
    char buf[BUFFER_SIZE];

    // 커밋 이력이 없는 파일일 때
    if(isNotCommit){
        currFile = trackedFile;

        // 원본 경로에 존재하지 않는 경우: 노드 삽입 하지 않음
        if(access(currFile->path, F_OK)){
            return;
        }
        // 원본 경로에 존재하는 경우: new file로 노드 추가
        else{
            FILE *fp;

            // 노드 생성 및 초기화
            newNode = (ChangeNode *)malloc(sizeof(ChangeNode));

            strcpy(newNode->path, currFile->path);
            strcpy(newNode->action, "new file");

            newNode->insertions = 0;
            newNode->deletions = 0;

            newNode->next = NULL;
            newNode->prev = *prevNode;
            if(*prevNode != NULL)
                newNode->prev->next = newNode;
            else
                changesList = newNode;
            
            // 원본 경로에서 해당 파일 오픈
            if((fp = fopen(newNode->path, "rb")) == NULL){
                fprintf(stderr, "fopen error for %s\n", newNode->path);
                exit(1);
            }

            // 추가된 파일의 줄 수 구하기
            while(fgets(buf, sizeof(buf), fp) != NULL)
                newNode->insertions++;

            fclose(fp);
        }

    }
    // 커밋 이력이 있는 파일일 때: 수정되었거나 삭제된 파일
    else{
        FILE *fp;
        char *repoPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));

        currFile = commitFile;
 
        // newNode->Path의 백업 파일의 경로 (레포디렉토리 내부에 있음)
        sprintf(repoPath, "%s/%s%s", repoPATH, currFile->commit, currFile->path + strlen(exePATH));

        // printf("repo: %s\n", repoPath);
        // printf("path: %s\n", currFile->path);

        // 원본 경로에 존재하지 않는 경우: 삭제된 파일
        if(access(currFile->path, F_OK)){
            // 노드 생성 및 초기화
            newNode = (ChangeNode *)malloc(sizeof(ChangeNode));

            strcpy(newNode->path, currFile->path);
            strcpy(newNode->action, "removed");

            newNode->insertions = 0;
            newNode->deletions = 0;

            newNode->next = NULL;
            newNode->prev = *prevNode;
            if(*prevNode != NULL)
                newNode->prev->next = newNode;
            else
                changesList = newNode;

            // 레포 디렉토리 내의 경로에서 해당 파일 오픈
            if((fp = fopen(repoPath, "rb")) == NULL){
                fprintf(stderr, "fopen error for %s\n", repoPath);
                exit(1);
            }

            // 삭제된 파일의 줄 수 구하기
            while(fgets(buf, sizeof(buf), fp) != NULL)
                newNode->deletions++;

            fclose(fp);
        }
        // 원본 경로에 존재하는 경우: 변경 사항이 있는지 확인
        else{
            FILE *fp1;
            FILE *fp2;
            char buf1[BUFFER_SIZE];
            char buf2[BUFFER_SIZE];
            int insertions = 0;
            int deletions = 0;
            int isDiff = FALSE;

            // 파일 오픈
            if((fp1 = fopen(repoPath, "rb")) == NULL || (fp2 = fopen(currFile->path, "rb")) == NULL){
                fprintf(stderr, "fopen error\n");
                exit(1);
            }

            // printf("repo: %s\n", repoPath);
            // printf("path: %s\n", currFile->path);

            // 한 줄씩 비교
            while(!feof(fp1) && !feof(fp2)){
                fgets(buf1, sizeof(buf1), fp1);
                fgets(buf2, sizeof(buf2), fp2);

                if(buf1[strlen(buf1) - 1] == '\n')
                    buf1[strlen(buf1) - 1] = '\0';
                if(buf2[strlen(buf2) - 1] == '\n')
                    buf2[strlen(buf2) - 1] = '\0';
                
                // 한 줄이라도 다르면 isDiff는 true
                if(strcmp(buf1, buf2)){
                    if(!strcmp(buf1, ""))
                        insertions++;
                    if(!strcmp(buf2, ""))
                        deletions++;
                    
                    isDiff = TRUE;
                }

                memset(buf1, 0, sizeof(buf1));
                memset(buf2, 0, sizeof(buf2));
            }

            // fp1이 가리키는 파일(레포 디렉토리 내 파일)은 아직 끝나지 않은 경우
            if(!feof(fp1)){
                isDiff = TRUE;
                while(fgets(buf1, sizeof(buf1), fp1) != NULL)
                    deletions++;
            }

            // fp2가 가리키는 파일(원본 경로 파일)은 아직 끝나지 않은 경우
            if(!feof(fp2)){
                isDiff = TRUE;
                while(fgets(buf2, sizeof(buf2), fp2) != NULL)
                    insertions++;
            }

            fclose(fp1);
            fclose(fp2);

            // 파일이 수정된 경우
            if(isDiff){
                // 노드 생성 및 초기화
                newNode = (ChangeNode *)malloc(sizeof(ChangeNode));

                strcpy(newNode->path, currFile->path);
                strcpy(newNode->action, "modified");

                newNode->insertions = insertions;
                newNode->deletions = deletions;

                newNode->next = NULL;
                newNode->prev = *prevNode;
                if(*prevNode != NULL)
                    newNode->prev->next = newNode;
                else
                    changesList = newNode;

            }
            // 파일이 수정되지 않은 경우
            else{
                return;
            }

        }

        free(repoPath);
    }

    *prevNode = newNode;

}

// trackedDir 노드가 남은 경우: 이전에 백업된 적이 없는 추적 대상이 되는 파일들의 상위(루트) 디렉토리
// 해당 디렉토리 노드의 하위 파일들 중 원본 경로에 존재하는 파일들 모두 new file로 changesList에 추가
void insertSubFileNode(DirNode *dir, ChangeNode **prevNode, int isNotCommit)
{
    ChangeNode *currNode = *prevNode;
    DirNode *currDir = dir;
    FileNode *currFile;

    // new file
    if(isNotCommit){
        if(currDir != NULL){
            // 하위 파일: 파일이 원본 경로에 존재하면 노드 추가
            currFile = currDir->fileHead;
            while(currFile != NULL){
                insertChangeNode(currFile, NULL, &currNode, isNotCommit);
                currFile = currFile->next;
            }
            
            // 하위 디렉토리
            currDir = currDir->subDir;
            while(currDir != NULL){
                insertSubFileNode(currDir, &currNode, isNotCommit);
                currDir = currDir->next;
            }
        }
    }
    // removed or untracked
    else{
        // 추적하지 않는 경로 내에 포함되어 있는지 확인
        // untrackedList에 포함되지 않으면 ChangeList에 삭제된 노드로 추가
        if(searchDirNode(untrackedPaths, currDir->path) == NULL){
            // 해당 디렉토리 내 파일들 모두 저장
            currFile = currDir->fileHead;
            while(currFile != NULL){
                insertChangeNode(NULL, currFile, &currNode, isNotCommit);
                currFile = currFile->next;
            }
        }
        // 해당 디렉토리가 있는 경우: 디렉토리 내에서 추적 대상이 아닌 파일들만 changesList에 추가
        else{
            currFile = currDir->fileHead;
            while(currFile != NULL){
                if(searchFileNode(currDir, currFile->path) == NULL)
                    insertChangeNode(NULL, currFile, &currNode, isNotCommit);
            }
        }

        // 해당 디렉토리의 하위 디렉토리 내 파일들도 저장
        currDir = currDir->subDir;
        while(currDir != NULL){
            insertSubFileNode(currDir, &currNode, isNotCommit);
            currDir = currDir->next;
        }
        
    }

    return;
}

// 변경 사항을 버전 디렉토리에 백업
void backupChanges(char *commitName)
{
    FILE *fp;
    ChangeNode *currNode = changesList;
    char *repoPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *parentPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *tmpPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *path = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *name = (char *)malloc(sizeof(char) * (MAX_NAME + 1));
    char *token = NULL;
    int i;

    // 커밋 로그파일 오픈
    if((fp = fopen(commitPATH, "a")) == NULL){
        fprintf(stderr, "fopen error for %s\n", commitPATH);
        exit(1);
    }

    sprintf(repoPath, "%s/%s", repoPATH, commitName);

    // 파일 백업
    while(currNode != NULL){
        // 삭제된 파일은 백업하지 않음
        if(strcmp(currNode->action, "removed")){
            // 초기화
            token = NULL;
            memset(parentPath, 0, MAX_PATH);
            memset(tmpPath, 0, MAX_PATH);
            memset(path, 0, MAX_PATH);
            memset(name, 0, MAX_NAME);

            // currNode 노드에 해당하는 파일의 레포 디렉토리 내에서의 경로
            sprintf(path, "%s%s", repoPath, currNode->path + strlen(exePATH));

            // 해당 파일의 부모 디렉토리 및 이름 추출
            parentPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
            strcpy(parentPath, path);

            for(i = strlen(parentPath) - 1; i >= 0 && parentPath[i] != '/'; i--);
            parentPath[i] = '\0';

            strcpy(name, parentPath + i + 1);

            // 상위 디렉토리 경로에서 레포 디렉토리 경로 제외
            sprintf(parentPath, "%s", parentPath + strlen(repoPATH));

            // 파일의 상위 디렉토리가 없다면 생성
            strcpy(tmpPath, repoPATH);
            token = strtok(parentPath, "/");
            while(token != NULL){
                sprintf(tmpPath, "%s/%s", tmpPath, token);

                if(access(tmpPath, F_OK))
                    mkdir(tmpPath, 0775);

                token = strtok(NULL, "/");
            }

            // 파일 백업
            sprintf(tmpPath, "%s/%s", tmpPath, name);
            copyFile(currNode->path, tmpPath);
        }

        // 로그 파일 출력
        fprintf(fp, "commit: \"%s\" - %s: \"%s\"\n", commitName, currNode->action, currNode->path);

        currNode = currNode->next;
    }

    fclose(fp);

    free(repoPath);
    free(parentPath);
    free(tmpPath);
    free(path);
    free(name);
}

void copyFile(char *originPath, char *destPath)
{
    struct stat statbuf;
    char buf[BUFFER_SIZE];
    int fd1;
    int fd2;
    int length;

    // 원본 파일 오픈
    if((fd1 = open(originPath, O_RDONLY)) < 0){
        fprintf(stderr, "open error for %s\n", originPath);
        exit(1);
    }

    // 원본 파일의 권한 획득
    if(stat(originPath, &statbuf) < 0){
        fprintf(stderr, "stat error for %s\n", originPath);
        exit(1);
    }

    // 새로 생성할 파일 오픈 -> 접근 권한은 원본 파일과 동일
    if((fd2 = open(destPath, O_WRONLY | O_CREAT | O_TRUNC, statbuf.st_mode)) < 0){
        fprintf(stderr, "open error for %s\n", destPath);
        exit(1);
    }
    
    // 파일 복사
    while((length = read(fd1, buf, sizeof(buf))) > 0)
        write(fd2, buf, length);

    close(fd1);
    close(fd2);
}

void printChangesInfo()
{
    ChangeNode *currNode = changesList;
    int files = 0;
    int insertions = 0;
    int deletions = 0;

    while(currNode != NULL){
        files++;
        insertions += currNode->insertions;
        deletions += currNode->deletions;

        currNode = currNode->next;
    }

    printf("%d files changed, %d insertions(+), %d deletions(-)\n", files, insertions, deletions);
}

void printChangesList()
{
    ChangeNode *currNode = changesList;
    char *path = (char *)malloc(sizeof(char) * (MAX_PATH + 1));

    // 상대경로로 출력
    while(currNode != NULL){
        sprintf(path, ".%s", currNode->path + strlen(exePATH));
        printf("\t%s: \"%s\"\n", currNode->action, path);

        currNode = currNode->next;
    }

    free(path);
}

// ---------------------------------recover------------------------------------

void checkRecover(DirNode *dir, FileNode **list)
{
    DirNode *currDir = dir;
    FileNode *prevRecoverFile = *list;
    FileNode *currFile;

    if(currDir != NULL){
        // 파일 복원
        currFile = currDir->fileHead;
        while(currFile != NULL){
            // 복원된 파일이 하나라도 존재한다면, true 리턴
            recoverFile(&prevRecoverFile, currFile->path, currFile->name, currFile->commit);
            currFile = currFile->next;
        }

        // 디렉토리 내 하위 파일 및 디렉토리 복원
        currDir = currDir->subDir;
        while(currDir != NULL){
            // 복원된 파일이 하나라도 존재한다면, true 리턴
            checkRecover(currDir, &prevRecoverFile);
            currDir = currDir->next;
        }
    }

    return;
}

void recoverFile(FileNode **prevRecoverFile, char *fullPath, char *name, char *commitName)
{
    FILE *fp1;
    FILE *fp2;
    struct stat statbuf;
    FileNode *newFileNode;
    char buf1[BUFFER_SIZE];
    char buf2[BUFFER_SIZE];
    char *repoPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *parentPath;
    char *path;
    char *token = NULL;

    sprintf(repoPath, "%s/%s%s", repoPATH, commitName, fullPath + strlen(exePATH));

    // 복원할 파일이 원본 경로에 있는 경우
    if(!access(fullPath, F_OK)){
        int isDiff = FALSE;

        // 파일명과 동일한 디렉토리 존재하면 에러 출력 후 프롬프트 재출력
        if(stat(fullPath, &statbuf) < 0){
            fprintf(stderr, "stat error for %s\n", fullPath);
            exit(1);
        }

        if(S_ISDIR(statbuf.st_mode)){
            fprintf(stderr, "directory path \"%s\" already exists\n", fullPath);
            exit(1);
        }

        // 레포 디렉토리 내부의 파일 오픈
        if((fp1 = fopen(repoPath, "rb")) < 0){
            fprintf(stderr, "fopen error for %s\n", repoPath);
            exit(1);
        }

        // 원본 경로 내 파일 오픈
        if((fp2 = fopen(fullPath, "rb")) < 0){
            fprintf(stderr, "fopen error for %s\n", fullPath);
            exit(1);
        }

        // 원본 경로 내 파일과 레포 디렉토리 내 파일의 내용이 같은 경우에는 복원 진행하지 않음
        while(!feof(fp1) && !feof(fp2)){
            fgets(buf1, sizeof(buf1), fp1);
            fgets(buf2, sizeof(buf1), fp2);

            if(strcmp(buf1, buf2)){
                isDiff = TRUE;
                break;
            }

            memset(buf1, 0, sizeof(buf1));
            memset(buf2, 0, sizeof(buf2));
        }

        if(!feof(fp1) || !feof(fp2))
            isDiff = TRUE;

        fclose(fp1);
        fclose(fp2);

        if(!isDiff){
            free(repoPath);
            return;
        }

    }

    // 복원가능한 경우 (원본 경로에 파일이 존재하지 않음 or 원본 경로 내 파일과 레포 디렉토리 내 파일이 다름)
    // : 상위 디렉토리 생성 + 복원 및 프롬프트 출력
    parentPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    path = (char *)malloc(sizeof(char) * (MAX_PATH + 1));

    // 백업하려는 파일(fullPath)의 상위 디렉토리에서 현재 작업 경로 제외
    strcpy(parentPath, fullPath + strlen(exePATH));
    parentPath[strlen(parentPath) - strlen(name) - 1] = '\0';

    // 파일을 저장할 상위 디렉토리 생성
    strcpy(path, exePATH);
    token = strtok(parentPath, "/");
    while(token != NULL){
        sprintf(path, "%s/%s", path, token);

        // 상위 디렉토리가 없다면 생성
        if(access(path, F_OK)){
            mkdir(path, 0775);
        }
        // 상위 디렉토리와 동일한 이름의 파일명 존재하면 에러 출력 후 프롬프트 재출력
        else{
            if(stat(path, &statbuf) < 0){
                fprintf(stderr, "stat error for %s\n", path);
                exit(1);
            }

            if(S_ISREG(statbuf.st_mode)){
                fprintf(stderr, "file path \"%s\" already exists\n", path);
                exit(1);
            }
        }
        
        token = strtok(NULL, "/");
    }

    // 레포 디렉토리 내 파일을 원본 경로에 복원
    copyFile(repoPath, fullPath);

    // 복원한 파일들의 연결리스트 노드를 생성 및 초기화하고 앞의 노드(prevRecoverFile)와 연결
    newFileNode = (FileNode *)malloc(sizeof(FileNode));
    strcpy(newFileNode->name, name);
    strcpy(newFileNode->path, fullPath);
    strcpy(newFileNode->commit, commitName);
    newFileNode->parentDir = NULL; // 사용 x
    newFileNode->next = NULL;
    newFileNode->prev = *prevRecoverFile;
    if(*prevRecoverFile != NULL)
        newFileNode->prev->next = newFileNode;
    else
        recoverList = newFileNode;

    *prevRecoverFile = newFileNode;
    
    free(repoPath);
    free(path);

    return;
}

void printRecoverList()
{
    FileNode *currFile = recoverList;
    char *path = (char *)malloc(sizeof(char) * (MAX_PATH + 1));

    // 상대경로로 출력
    while(currFile != NULL){
        sprintf(path, ".%s", currFile->path + strlen(exePATH));
        printf("recover \"%s\" from \"%s\"\n", currFile->path, currFile->commit);

        currFile = currFile->next;
    }

    free(path);
}

// ---------------------------------insert------------------------------------

// parnetDir의 하위 디렉토리를 subDir에 연결
DirNode *insertDirNode(DirNode **parentDir, char *dirPath, char *dirName)
{
    DirNode *newDirNode = (DirNode *)malloc(sizeof(DirNode));
    DirNode *currDir = (*parentDir)->subDir;
    int result;

    // 삽입할 디렉토리 초기화
    strcpy(newDirNode->name, dirName);
    strcpy(newDirNode->path, dirPath);
    newDirNode->parentDir = *parentDir;
    newDirNode->subDir = NULL;
    newDirNode->fileHead = NULL;
    newDirNode->next = NULL;
    newDirNode->prev = NULL;
    newDirNode->isTracked = FALSE; // commit.log에서는 사용 x

    // 상위 디렉토리(parentDir)의 하위 디렉토리 노드가 존재하지 않는 경우
    if(currDir == NULL){
        (*parentDir)->subDir = newDirNode;
        currDir = newDirNode;
    }
    // 상위 디렉토리(parentDir)의 하위 디렉토리 노드가 존재하는 경우
    else{
        while(currDir != NULL){
            // dirName에 해당하는 파일이 이미 연결 리스트에 삽입되어 있는 경우
            if((result = strcmp(currDir->name, dirName)) == 0){
                free(newDirNode);
                break;
            }
            // currDir에 해당하는 노드보다 dirName 디렉토리 노드가 뒤에 있어야 하는 경우
            else if(result < 0){
                // 현재 디렉토리 노드가 마지막 노드인 경우
                if(currDir->next == NULL){
                    newDirNode->prev = currDir;
                    newDirNode->prev->next = newDirNode;

                    currDir = newDirNode;

                    break;
                }

                currDir = currDir->next;
            }
            // currDir 바로 앞에 dirName이 삽입되어야 하는 경우
            else{
                // 새로 삽입할 노드의 위치가 연결 리스트의 맨 앞인 경우
                if(currDir->prev == NULL){
                    newDirNode->next = currDir;
                    newDirNode->next->prev = newDirNode;
                    
                    (*parentDir)->subDir = newDirNode;
                }
                // 새로 삽입할 노드의 위치가 연결 리스트의 중간인 경우
                else{
                    newDirNode->next = currDir;
                    newDirNode->prev = currDir->prev;
                    newDirNode->prev->next = newDirNode;
                    newDirNode->next->prev = newDirNode;
                }

                currDir = newDirNode;

                break;
            }
        }
    }

    return currDir;
}

// dir의 하위 파일을 dirNode의 멤버변수인 fileHead 연결 리스트에 삽입
void insertFileNode(DirNode **dir, char *filePath, char *fileName, char *commitName)
{
    FileNode *currFile = (*dir)->fileHead;
    FileNode *newFileNode = (FileNode *)malloc(sizeof(FileNode));
    int result;

    // 파일 노드 초기화
    strcpy(newFileNode->name, fileName);
    strcpy(newFileNode->path, filePath);
    strcpy(newFileNode->commit, commitName); // staging.log에서는 사용 x
    newFileNode->parentDir = *dir;
    newFileNode->prev = NULL;
    newFileNode->next = NULL;

    // dir 디렉토리 하위에 추적할 파일이 존재하지 않는 경우: 노드 생성 후, 현재 파일(currFile)과 연결
    if(currFile == NULL){
        (*dir)->fileHead = newFileNode;
    }
    // dir 디렉토리 하위에 추적할 파일이 존재하는 경우: 오름차순으로 파일 연결
    else{
        while(currFile != NULL){
            // fileName에 해당하는 파일이 이미 연결 리스트에 삽입되어 있는 경우
            if((result = strcmp(currFile->name, fileName)) == 0){
                free(newFileNode);
                break;
            }
            // currFile에 해당하는 파일보다 fileName 파일보다 뒤에 삽입되어 있어야 하는 경우
            else if(result < 0){
                // 현재 파일 노드가 마지막 노드인 경우
                if(currFile->next == NULL){
                    newFileNode->prev = currFile;
                    newFileNode->prev->next = newFileNode;

                    break;
                }

                currFile = currFile->next;
            }
            // currFile 바로 앞에 fileName이 삽입되어야 하는 경우
            else{
                // 새로 삽입할 노드의 위치가 연결 리스트의 맨 앞인 경우
                // : 해당 파일의 상위 디렉토리의 fileHead는 새로 삽입하는 노드를 가리켜야 함
                if(currFile->prev == NULL){
                    newFileNode->next = currFile;
                    newFileNode->next->prev = newFileNode;
                    
                    (*dir)->fileHead = newFileNode;
                }
                // 새로 삽입할 노드의 위치가 연결 리스트의 중간인 경우
                else{
                    newFileNode->next = currFile;
                    newFileNode->prev = currFile->prev;
                    newFileNode->prev->next = newFileNode;
                    newFileNode->next->prev = newFileNode;
                }

                break;
            }
        }
    }

    return;
}

// ----------------------------------delete-----------------------------------

void deleteDirNode(DirNode **dir)
{
    DirNode *currDir = *dir;
    DirNode *subDir;
    DirNode *tmpDir;
    FileNode *currFile;
    FileNode *tmpFile;

    if(currDir != NULL){
        // 해당 디렉토리(currDir)의 하위 디렉토리로 이동
        subDir = currDir->subDir;
        while(subDir != NULL){
            tmpDir = subDir;
            subDir = subDir->next;

            deleteDirNode(&tmpDir);
        }

        // 해당 디렉토리(currDir)의 하위 파일 모두 삭제 
        currFile = currDir->fileHead;
        while(currFile != NULL){
            tmpFile = currFile;
            currFile = currFile->next;
            
            free(tmpFile);
        }

        // currDir이 현재 작업 경로인 경우
        if(!strcmp(currDir->path, exePATH)){
            currDir->subDir = NULL;
            currDir->fileHead = NULL;

            *dir = NULL;
        }
        // currDir의 형제노드가 더 이상 없는 경우, 상위 디렉토리는 NULL 가리킴
        else if(currDir->parentDir->subDir == currDir){
            currDir->parentDir->subDir = NULL;

            currDir->subDir = NULL;
            currDir->fileHead = NULL;

            free(currDir);
        }

    }
    
    return;
}

void deleteFileNode(DirNode **parentDir, char *fileName)
{
    FileNode *currFile = (*parentDir)->fileHead;

    // 해당하는 파일 노드 삭제
    while(currFile != NULL){
        if(!strcmp(fileName, currFile->name)){
            // 첫 번째 노드를 삭제하는 경우
            if(currFile->prev == NULL){
                (*parentDir)->fileHead = currFile->next;

                if(currFile->next != NULL)
                    currFile->next->prev = currFile->prev;
                else // 마지막 노드인 경우
                    (*parentDir)->fileHead = NULL;
            }
            // 그 외의 노드를 삭제하는 경우
            else{
                currFile->prev->next = currFile->next;

                if(currFile->next != NULL)
                    currFile->next->prev = currFile->prev;
            }

            free(currFile);
            break;
        }

        currFile = currFile->next;
    }
}

// ----------------------------------search-----------------------------------

// 연결리스트 전체 탐색하여, 해당 경로의 정보를 담고있는 노드가 하나라도 존재하지 않으면 true 리턴
int isNotExistAnyNode(DirNode *listHead, DirNode *list, char *fullPath, char *name)
{
    struct stat statbuf;
    DirNode *dirNode;
    FileNode *currFile;
    FileNode *fileNode;
    int result;
    int error = -1;

    // fullPath에 해당하는 파일 또는 디렉토리의 정보 받아옴
    if(lstat(fullPath, &statbuf) < 0)
        return error;

    // fullPath가 파일인 경우
    if(S_ISREG(statbuf.st_mode)){
        // 해당 파일의 상위 디렉토리의 노드 찾기
        char parentPath[MAX_PATH];
        strncpy(parentPath, fullPath, strlen(fullPath) - strlen(name) - 1);

        // 상위 디렉토리(parentPath)의 노드가 존재하지 않은 경우
        if((dirNode = searchDirNode(listHead, parentPath)) == NULL)
            result = TRUE;
        // 디렉토리 노드가 존재하는 경우: 파일 리스트 탐색
        else{
            // 해당하는 파일 노드가 존재하는 않은 경우
            if((fileNode = searchFileNode(dirNode, name)) == NULL)
                result = TRUE;
            // 파일 노드가 존재하는 경우
            else
                result = FALSE;
        }

    }
    // 디렉토리인 경우
    else if(S_ISDIR(statbuf.st_mode)){
        // 자신의 정보를 담고 있는 노드가 존재하지 않는 경우
        if((dirNode = searchDirNode(listHead, fullPath)) == NULL)
            result = TRUE;
        // 해당 디렉토리의 노드가 존재하는 경우
        else{
            char *path = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
            struct dirent **namelist;
            int cnt;
            int i;

            // 해당 디렉토리(fullPath)의 하위 파일 및 디렉토리들이 연결 리스트 내에 존재하는지 검사
            if((cnt = scandir(fullPath, &namelist, filter_all, alphasort)) < 0){
                fprintf(stderr, "scandir error for %s\n", fullPath);
                exit(1);
            }
            
            for(i = 0; i < cnt; i++){
                sprintf(path, "%s/%s", fullPath, namelist[i]->d_name);
                result = isNotExistAnyNode(listHead, dirNode, path, namelist[i]->d_name);

                // 해당 디렉토리의 하위 파일 및 디렉토리 중 하나라도 존재하지 않는 것이 있으면 staging.log에 기록
                if(result == TRUE)
                    break;
            }

            free(path);
        }

    }
    //파일 및 디렉토리 모두 아닌 경우
    else{
        return error;
    }

    return result;
}

// path에 해당하는 경로의 정보를 담고 있는 노드 탐색
DirNode *searchDirNode(DirNode *list, char *dirFullPath)
{   
    DirNode *currDir = list;
    char *tmpPath = (char *)malloc(sizeof(char) * (strlen(dirFullPath) + 1));
    char *token = NULL;

    // 빈 리스트인 경우, NULL 리턴
    if(currDir == NULL)
        return NULL;

    // dirFullPath가 현재 작업 경로(exePATH)일 때
    if(!strcmp(dirFullPath, exePATH))
        return currDir;

    // tmpPath: dirFullPath에서 현재 작업 경로를 제외한 경로
    strcpy(tmpPath, dirFullPath + strlen(exePATH) + 1);
    
    token = strtok(tmpPath, "/");
    while(token != NULL){
        // 하위 디렉토리 노드로 이동
        currDir = currDir->subDir;
        // 하위 디렉토리들 중 해당하는 노드가 있는지 확인
        while(currDir != NULL){
            // 해당하는 노드가 있다면, 이동
            if(!strcmp(token, currDir->name))
                break;

            currDir = currDir->next;
        }

        // 해당하는 경로의 노드가 없는 경우 (즉, 해당 경로는 추적 대상이 아닌 경우)
        if(currDir == NULL)
            return NULL;

        token = strtok(NULL, "/");
    }
    

    return currDir;
}

FileNode *searchFileNode(DirNode *parentDir, char *fileName)
{
    FileNode *currFile = parentDir->fileHead;

    // fileName을 가지고 있는 파일 노드 탐색
    while(currFile != NULL){
        if(!strcmp(fileName, currFile->name))
            break;

        currFile = currFile->next;
    }
        
    return currFile;
}

// --------------------------------filter-------------------------------------

// scandir 해줄 때, 파일과 디렉토리를 모두 추출하는 필터 함수(.과 .. 제외)
// 0을 반환하면 namelist에 해당 entry는 추가되지 않음
static int filter_all(const struct dirent *entry)
{
    if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        return 0;

    // 레포 디렉토리도 제외
    if(entry->d_type == DT_DIR && !strcmp(entry->d_name, ".repo"))
        return 0;

    return 1;
}

// // scandir을 해줄 때 ., .., 파일을 제외한 디렉토리만 추출하는 필터 함수
// static int filter_onlyDir(const struct dirent *entry)
// {
//     // .와 .. 디렉토리는 제외
//     if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
//         return 0;

//     // DR_DIR: 디렉토리를 나타내는 상수
//     // 레포 디렉토리(.repo) 제외
//     // 디렉토리인 경우 1, 그 외는 0 리턴
//     return (entry->d_type == DT_DIR && strcmp(entry->d_name, ".repo"));
// }


// // scandir 해줄 때, 파일만 추출하는 필터 함수
// static int filter_onlyfile(const struct dirent *entry)
// {
//     // DT_REG: 일반 파일을 나타내는 상수
//     // 파일인 경우 1, 그 외는 0 리턴
//     return (entry->d_type == DT_REG);
// }