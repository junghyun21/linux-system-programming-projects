#ifndef PATH_H
#define PATH_H

#include "defs.h"

// 파일/디렉토리의 이름을 각 노드에 저장하여 연결리스트로 연결함으로써 절대경로 추출
typedef struct PathList{
    struct PathList *prev;
    struct PathList *next;
    char name[MAX_NAME + 1];
}PathList;

void initPath();

int processPath(char *path, char *fullPath, char *name);
int checkPath(char *fullPath, char *name);

#endif