#include "path.h"
#include "struct.h"
#include <stdio.h>
#include <stdlib.h>

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

// 스테이징 구역에 추가된 경로의 파일들에 대해 변경 내역을 추적하여 정보 출력
// argv[0]: status
int main(int argc, char *argv[])
{
    struct dirent **namelist;
    int cnt;

    initPath(); // 홈 디렉토리, 현재 실행 디렉토리 등을 초기화

    initStagingList(); // 스테이징 구역 내의 경로와 구역 외의 경로를 연결리스트에 저장(trackedPaths, untrackedPaths, unknownPaths)
    initCommitList(NULL); // 커밋 구역의 경로를 연결리스트에 저장 (commitList)

    checkChanges(trackedPaths, commitList, &changesList); // 변경 사항을 changesList에 저장하고, 세부 변경 내용을 각 인자에 저장

    // printf("----------------------------------trackedPaths-----------------------------\n");
    // testStagingList(trackedPaths);
    // printf("----------------------------------untrackedPaths-----------------------------\n");
    // testStagingList(untrackedPaths);
    // printf("-----------------------------------unknownPaths----------------------------\n");
    // testStagingList(unknownPaths);
    // printf("-----------------------------------commitList----------------------------\n");
    // testStagingList(commitList);
    // printf("-----------------------------------changesList----------------------------\n");
    // testchanges(changesList);

    // 스테이징 구역에 포함되어 있는 경로 내 파일들에 대하여 출력
    if(changesList != NULL){
        printf("Changes to be commited:\n");
        printChangesList();
    }

    // 스테이징 구역에 없는 경로 내 파일들에 대하여 출력
    if(unknownPaths != NULL){
        printf("Untracked files: \n");
        printUnknownPaths(unknownPaths);
    }

    // 마지막 백업 파일에서 변경되거나 삭제, 새롭게 추가된 파일이 없는 경우
    if(changesList == NULL && unknownPaths == NULL){
        printf("Nothing to commit\n");
    }

    return 0;
}