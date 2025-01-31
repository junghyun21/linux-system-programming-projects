// 파일과 파일 경로와 관련된 변수 및 함수 선언
#ifndef FILEPATH_H
#define FILEPATH_H

#include "defs.h"
#include <dirent.h>

extern char exePATH[];
extern char homePATH[];
extern char backupPATH[];
extern char logPATH[];

void initPath();

int checkPath(char *path, char *name);
int processPath(char *path, char *fullPath, char *name);

void removeEmptyDir(char *backupPath);

char *getDate();

int filter_onlyDir(const struct dirent *entry); //  scandir 해줄 때, 디렉토리만 추출하는 필터 함수
int filter_onlyfile(const struct dirent *entry); // scandir 해줄 때, 파일만 추출하는 필터 함수
int filter_all(const struct dirent *entry); // scandir 해줄 때, 파일과 디렉토리를 추출하는 필터 함수(.과 .. 제외)


// /home/backup 내부에 저장된 파일
typedef struct BackupDir{
    char dirName[LEN_DATE + 1]; // 백업 디렉토리명 (백업 시간)
    char *parentPath; // 원본 상위 경로
    char *backupPath; // 백업 디렉토리 내에서 파일이 저장된 상위 경로
    struct BackupDir *next; // 바로 이전에 백업된 디렉토리로, dirName과 연동된 파일 경로가 동일함
}BackupDir;

// /home/user 내부에 저장된 파일
typedef struct Dir{
    char dirName[MAX_PATH + 1]; // 홈 디렉토리 내에 있는 실제 이름
    struct Dir *subDir; // 하위 디렉토리 중 첫 번째 디렉토리 (오름차순) 
    struct Dir *siblingDir; // 동일한 위치의 디렉토리 중 다음 디렉토리 (오름차순)
    struct BackupDir *head; // originPath와 연동된 가장 최신의 백업 디렉토리
    struct BackupDir *tail; // originPath와 연동된 가장 과거의 백업 디렉토리
}Dir;

Dir *createBackupList();
Dir *searchBackupDir(Dir *listHead, char *parentPath);

Dir *listHead; // 모든 백업 파일에 대해 로그 파일 내용과 대조하여 파일들에 대한 백업 상태 정보를 저장한 링크드 리스트


// 파일/디렉토리의 이름을 각 노드에 저장하여 연결리스트애 연결
typedef struct PathList{
  struct PathList *prev;
  struct PathList *next;
  char name[MAX_NAME + 1];
}PathList;

#endif