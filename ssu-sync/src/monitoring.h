#ifndef MONITORING_H
#define MONITORING_H

#include "path.h"
#include <time.h>

// 디렉토리 노드
typedef struct DirNode{
    char path[MAX_PATH + 1]; // 해당 디렉토리의 원본 경로

    struct DirNode *parentDir; // 상위 디렉토리
    struct DirNode *subDir; // 하위 디렉토리
    struct FileNode *fileHead; // 하위 파일들의 리스트

    // 형제 디렉토리
    struct DirNode *next; 
    struct DirNode *prev; 
}DirNode;

// 파일 노드
typedef struct FileNode{
    char path[MAX_PATH + 1]; // 해당 파일의 경로
    time_t mtime;

    // 해당 파일의 상위 디렉토리 노드
    struct DirNode *parentDir;

    // 형제 파일
    struct FileNode *prev;
    struct FileNode *next;
}FileNode;

extern DirNode *trackList;

int isExistDaemon(char *inputPath);
void daemonize();

void trackFile(char *dirPath, char *logPath, char *trackedPath, int period);
void trackDir(char *dirPath, char *logPath, char *trackedPath, int period, char opt);
void initTrackList(char *fullPath);
void checkChanges(DirNode **dir, char *backupDirPath, char *logPath, char *fullPath, char opt, struct tm *time);
void checkRemove(DirNode **dir, char *dirPath, char *logPath, struct tm *time);
void backupModifyFile(char *backupDirPath, char *filePath, struct tm *time);
void writeLog(char *change, char *logPath, char *filePath, struct tm *time);
char *getName(char *fullPath);

FileNode *searchFileNode(DirNode *parentDir, char *filePath);
void insertDirNode(DirNode **parentDir, char *dirPath);
void insertFileNode(DirNode **parentDir, char *filePath, time_t mtime);
void deleteFileNode(FileNode **file, char *filePath);

int isExistPid(int pid, char *fullPath);
void removeDaemon(int pid);
void removeBackup(char *backupPath);

#endif