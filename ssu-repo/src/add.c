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

// 변경 내용을 추적할 경로를 입력받아 스테이징 구역 중 추적할 경로를 담은 리스트에 추가
// argv[0]: add
// argv[1]: PATH
int main(int argc, char *argv[])
{
    char fullPath[MAX_PATH + 1];
    char relPath[MAX_PATH + 1];
    char name[MAX_NAME + 1];
    int result;

    // 경로가 입력되지 않은 경우: add 명령어에 대한 에러 처리 및 Usage 출력 후, 프롬프트 재출력
    if(argc != 2){
        fprintf(stderr, "ERROR: <PATH> is not include\nUsage: %s\n", USAGE_ADD);
        exit(1);
    }

    // 홈 디렉토리, 현재 실행 디렉토리 등을 초기화
    initPath();

    // 입력 받은 경로를 절대 경로(fullPath)로 변환하고, 경로에 해당하는 파일/디렉토리명(name)을 저장하고
    // 해당 파일/디렉토리가 올바른 경로인지 확인
    if(processPath(argv[1], fullPath, relPath, name) < 0 || checkPath(fullPath, name) < 0){
        fprintf(stderr, "ERROR: %s is wrong path\n", argv[1]);
        exit(1);
    }
        
    // 변경사항을 추적할 경로 리스트(trackedPaths)와 추적하지 않을 경로 리스트(untrackedPaths) 초기화
    // 해당 연결리스트는 struct.h에 전역 변수로 선언되어 있음
    initStagingList();

    // printf("----------------------[추적할 경로 리스트]-----------------------\n");
    // testStagingList(trackedPaths);
    // printf("----------------------[추적하지 않을 경로 리스트]-----------------------\n");
    // testStagingList(untrackedPaths);

    // 추적할 경로들의 리스트(trackedPaths) 탐색
    if((result = isNotExistAnyNode(trackedPaths, trackedPaths, fullPath, name)) < 0){
        fprintf(stderr, "ERROR: %s is wrong path\n", argv[1]);
        exit(1);
    }
    // 해당 경로의 정보를 담고있는 노드가 하나라도 존재하지 않는 경우
    else if(result){
        FILE *fp;

        if((fp = fopen(stagingPATH, "a+")) == NULL){
            fprintf(stderr, "fopen error for %s\n", stagingPATH);
            exit(1);
        }
        fprintf(fp, "add \"%s\"\n", fullPath);
        fprintf(stdout, "add \"%s\"\n", relPath);

        fclose(fp);
    }
    // 해당 경로의 정보를 담고있는 노드가 모두 존재하는 경우
    else{
        fprintf(stdout, "\"%s\" already exist in staging area\n", relPath);
    }

    return 0;
}