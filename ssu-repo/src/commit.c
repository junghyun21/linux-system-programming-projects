#include "path.h"
#include "struct.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

void testStagingList(DirNode *stagingList){
    DirNode *currDir = stagingList;
    int i;
    while (currDir != NULL) {
        i = 0;

        printf("Directory Name: %s\n", currDir->name);
        printf("Directory Path: %s\n", currDir->path);
        printf("Is Tracked: %d\n", currDir->isTracked);

        // 파일 출력
        FileNode *currFile = currDir->fileHead;
        while (currFile != NULL) {
            printf("\t[%d]\n", ++i);
            printf("\tFile Name: %s\n", currFile->name);
            printf("\tFile Path: %s\n", currFile->path);
            printf("\tCommit Name: %s\n", currFile->commit);
            currFile = currFile->next;
        }

        // 하위 디렉토리 검사
        if (currDir->subDir != NULL) {
            printf("> Sub Directories of \"%s\"\n", currDir->path);
            testStagingList(currDir->subDir);
        }

        currDir = currDir->next;
    }
}

void testchanges(ChangeNode *list){
    ChangeNode *currNode = list;
    
    while(currNode != NULL){
        printf("path: %s\n", currNode->path);
        printf("action: %s\n", currNode->action);
        printf("insertions: %d\n", currNode->insertions);
        printf("deletions: %d\n", currNode->deletions);

        currNode = currNode->next;
    }
}


// 커밋 이름을 입력받아 해당 이름으로 버전 디렉토리 생성하여, 스테이징 구역 내 경로들에 대하여 변경 사항에 따라 백업 진행
// argv[0]: commit
// argv[1]: NAME
// argv[2]: NAME
// ...
int main(int argc, char *argv[])
{
    char path[MAX_PATH + 1];
    char name[MAX_NAME + 1];
    int files = 0;
    int insertions = 0;
    int deletions = 0;
    
    // 이름 입력되지 않은 경우: commit 명령어에 대한 에러 처리 및 Usage 출력 후, 프롬프트 재출력
    if(argc < 2){
        fprintf(stderr, "ERROR: <PATH> is not include\nUsage: %s\n", USAGE_COMMIT);
        exit(1);
    }

    // 홈 디렉토리, 현재 실행 디렉토리 등을 초기화
    initPath();

    // 입력 받은 이름은 띄어쓰기도 허용하므로, argv[1]~argv[n]까지 변수를 하나의 이름으로 가공
    if(processName(argc - 1, argv + 1, name) < 0){
        fprintf(stderr, "ERROR: name cannot exceed 255 bytes\n");
        exit(1);
    }

    initStagingList();
    initCommitList(NULL);

    // printf("----------------------------------trackedPaths-----------------------------\n");
    // testStagingList(trackedPaths);
    // printf("----------------------------------untrackedPaths-----------------------------\n");
    // testStagingList(untrackedPaths);
    // printf("-----------------------------------unknownPaths----------------------------\n");
    // testStagingList(unknownPaths);
    // printf("-----------------------------------commitList----------------------------\n");
    // testStagingList(commitList);
 
    sprintf(path, "%s/%s", repoPATH, name);

    // 입력받은 이름으로 커밋된 적이 없는 경우: 버전 디렉토리 생성
    if(isAvailableCommitName(name)){
        if(mkdir(path, 0775) < 0){
            fprintf(stderr, "mkdir error for %s\n", path);
            exit(1);
        }
    }
    // 입력받은 이름과 동일한 버전 디렉토리가 존재하는 경우: 에러 처리 후, 프롬프트 재출력
    else{
        fprintf(stderr, "\"%s\" is already exist in repo\n", name);
        exit(1);
    }

    // 변경 사항을 changesList에 저장하고, 세부 변경 내용을 각 인자에 저장
    // 변경 사항을 버전 디렉토리에 백업
    checkChanges(trackedPaths, commitList, &changesList); 
    backupChanges(name);

    // printf("-----------------------------------changesList----------------------------\n");
    // testchanges(changesList);

    // 변경된 파일이 없는 경우: 해당 버전 디렉토리 삭제
    if(changesList == NULL){
        rmdir(path);
        printf("Nothing to commit\n");
    }
    else{
        struct dirent **namelist;

        printf("commit to \"%s\"\n", name);

        printChangesInfo();
        printChangesList();

        // 변경 내용 중 removed만 존재하는 경우, 백업디렉토리에는 파일들이 백업되지 않음
        // 즉, 로그 파일에만 커밋 내역 기록되고, 해당 커밋 이름의 디렉토리는 생성되면 안됨
        if(scandir(path, &namelist, NULL, alphasort) == 2)
            rmdir(path);
    }

    return 0;
}