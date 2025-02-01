#ifndef STRUCT_H
#define STRUCT_H

#include "defs.h"

// 디렉토리 노드
typedef struct DirNode{
    char name[MAX_NAME + 1]; // 해당 디렉토리의 이름
    char path[MAX_PATH + 1]; // 해당 디렉토리의 원본 경로

    int isTracked; // 해당 디렉토리 자체가 추적 대상인지 확인하는 플래그 -> staging.log 관련한 변수

    struct DirNode *parentDir; // 상위 디렉토리
    struct DirNode *subDir; // 하위 디렉토리
    struct FileNode *fileHead; // 하위 파일들의 리스트

    // 형제 디렉토리
    struct DirNode *next; 
    struct DirNode *prev; 
}DirNode;

// 파일 노드
typedef struct FileNode{
    char name[MAX_NAME + 1]; // 해당 파일의 이름
    char path[MAX_PATH + 1]; // 해당 파일의 경로
    char commit[MAX_NAME + 1]; // 버전 디렉토리 이름 -> commit.log 관련한 변수

    // 해당 파일의 상위 디렉토리 노드
    struct DirNode *parentDir;

    // 형제 파일
    struct FileNode *prev;
    struct FileNode *next;
}FileNode;

// 파일의 변경 내역을 저장하는 노드
typedef struct ChangeNode{
    char path[MAX_PATH + 1];
    char action[MAX_NAME + 1];
    
    int insertions;
    int deletions;
    
    struct ChangeNode *prev;
    struct ChangeNode *next;
}ChangeNode;

extern DirNode *unknownPaths; // 스테이징 구역에 없는 경로들의 연결 리스트
extern DirNode *trackedPaths; // 스테이징 구역(.staging.log)을 바탕으로 생성한 추적할 경로들의 연결 리스트
extern DirNode *untrackedPaths; // 스테이징 구역(.staging.log)을 바탕으로 생성한 추적하지 않을 경로들의 연결 리스트

extern DirNode *commitList; // 커밋 로그(.commit.log)를 바탕으로 커밋된 파일 중 최근에 커밋된 버전 관리

extern ChangeNode *changesList; // 변경 사항을 저장한 연결 리스트

extern FileNode *recoverList; // 원본 경로로 복원을 진행한 파일들의 연결 리스트

// only for staging.log
void initStagingList();
void insertStagingList(DirNode **listHead, char *fullPath);
void deleteStagingList(DirNode **listHead, char *fullPath);
void deleteNotTrackedDirNode(DirNode **listHead);
void initOriginList(DirNode **dir, char *fullPath);
void printUnknownPaths(DirNode *dir);

// only for commit.log
void initCommitList(char *commitName);
void insertCommitList(DirNode **dir, char *fullPath, char *commitName);
void modifyCommitList(DirNode *list, char *fullPath, char *commitName);
void deleteCommitList(DirNode *list, char *fullPath);
int isAvailableCommitName(char *commitName);

void checkChanges(DirNode *trackedDirNode, DirNode *commitDirNode, ChangeNode **prevNode);
void insertChangeNode(FileNode *trackedFile, FileNode *commitFile, ChangeNode **prevNode, int isNotCommit);
void insertSubFileNode(DirNode *dir, ChangeNode **prevNode, int isNotCommit);
void backupChanges(char *commitName);
void copyFile(char *originPath, char *destPath);
void printChangesInfo();
void printChangesList();

void checkRecover(DirNode *dir, FileNode **list);
void recoverFile(FileNode **prevRecoverFile, char *fullPath, char *name, char *commitName);
void printRecoverList();

DirNode *insertDirNode(DirNode **parentDir, char *dirPath, char *dirName);
void insertFileNode(DirNode **dir, char *filePath, char *fileName, char *commitName);

void deleteDirNode(DirNode **dir);
void deleteFileNode(DirNode **parentDir, char *fileName);

int isNotExistAnyNode(DirNode *listHead, DirNode *list, char *fullPath, char *name); 
DirNode *searchDirNode(DirNode *list, char *dirFullPath);
FileNode *searchFileNode(DirNode *parentDir, char *fileName);

#endif