#include "monitoring.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>

DirNode *trackList;

// monitor_list.log을 바탕으로 입력 받은 경로가 현재 추적 경로인지 확인
int isExistDaemon(char *inputPath)
{
    FILE *fp;
    struct stat statbuf;
    char buf[BUFFER_SIZE];
    char pid[BUFFER_SIZE];
    char fullPath[MAX_PATH + 1];
    int isExist;

    // monitor_list.log를 오픈
    if((fp = fopen(monitorPATH, "rb")) == NULL){
        fprintf(stderr, "fopen error for %s\n", monitorPATH);
        exit(1);
    }

    // pid.log 파일을 한 줄씩 읽어옴
    isExist = 0;
    while(fgets(buf, sizeof(buf), fp) != NULL){
        buf[strlen(buf) - 1] = '\0'; // 개행문자 제거
        sscanf(buf, "%s : %s", pid, fullPath); // pid : fullPath

        // 입력 받은 경로에 대한 데몬 프로세스가 존재하는지 확인
        if(!strcmp(inputPath, fullPath)){
            isExist = 1;
            break;
        }
    }
    fclose(fp);

    return isExist;
}

// 해당 프로세스를 데몬 프로세스로 전환
void daemonize()
{
    pid_t pid;
    int fd;

    if((pid = fork()) < 0){
        fprintf(stderr, "fork error\n");
        exit(1);
    }
    // 부모 프로세스 종료
    // 자식 프로세스의 부모 프로세스는 /etc/init이 됨
    else if(pid != 0){
        exit(0);
    }

    // 새로 생성된 프로세스(자식 프로세스)인 경우에만 실행
    // 새로운 세션 생성하고 자식 프ㅗㄹ세느는 생성한 세션의 리더 및 그룹 리더가 됨
    setsid();

    // 터미널 입출력 무시
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    // 파일 모드 생성 마스크 해제 -> 데몬 프로레스가 생성할 파일의 접근 허가 모드를 모두 허용
    umask(0);

    // 현재 디렉토리를 루트 디렉토리로 설정 -> 파일 시스템 언마운트에 영향을 주지 않기 위함
    chdir("/");

    // 표준 입출력과 에러를 /dev/null로 재지정
    fd = open("/dev/null", O_RDWR);
    dup(0);
    dup(0);

    return;
}

// trackedPath에 해당하는 파일의 변경 사항 추적
void trackFile(char *dirPath, char *logPath, char *trackedPath, int period)
{
    struct stat statbuf;
    struct tm *localTime;
    time_t currTime;
    time_t modifyTime;

    // 현재 시간을 얻어 로컬 시간으로 변환
    time(&currTime);
    localTime = localtime(&currTime);

    if(stat(trackedPath, &statbuf) < 0){
        fprintf(stderr, "stat error for %s\n", trackedPath);
        exit(1);
    }
    modifyTime = statbuf.st_mtime;

    backupModifyFile(dirPath, trackedPath, localTime);
    writeLog("create", logPath, trackedPath, localTime);

    while(1){
        // 현재 시간을 얻어 로컬 시간으로 변환
        time(&currTime);
        localTime = localtime(&currTime);

        // 파일이 존재하지 않는 경우
        if(stat(trackedPath, &statbuf) < 0){
            writeLog("remove", logPath, trackedPath, localTime);
            
            // 해당 파일이 새로 생성될 때까지 대기
            while(1){
                if(stat(trackedPath, &statbuf) >= 0){
                    time(&currTime);
                    localTime = localtime(&currTime);

                    modifyTime = statbuf.st_mtime;

                    backupModifyFile(dirPath, trackedPath, localTime);
                    writeLog("create", logPath, trackedPath, localTime);

                    break;
                }

                sleep(period);
            }
        }
        // 파일이 존재하는 경우
        else{
            // 파일의 최종 수정 시간이 변경되었는지 확인
            if(modifyTime != statbuf.st_mtime){
                backupModifyFile(dirPath, trackedPath, localTime);
                writeLog("modify", logPath, trackedPath, localTime);

                modifyTime = statbuf.st_mtime;
            }
        }

        sleep(period);
    }

}

// trackedPath에 해당하는 디렉토리 내의 변경 사항 추적
void trackDir(char *dirPath, char *logPath, char *trackedPath, int period, char opt)
{
    struct tm *localTime;
    time_t currTime;

    // trackList가 가리키는 노드 생성 및 초기화 -> trackedPath에 해당하는 노드
    initTrackList(trackedPath);

    while(1){
        // 현재 시간을 얻어 로컬 시간으로 변환
        time(&currTime);
        localTime = localtime(&currTime);

        // trackList와 경로 내 파일을 비교하며 변경사항 확인 및 기록
        checkChanges(&trackList, dirPath, logPath, trackedPath, opt, localTime);
        checkRemove(&trackList, dirPath, logPath, localTime);
        sleep(period);
    }
}

// 추적할 경로의 노드를 연결리스트에 삽입
void initTrackList(char *fullPath)
{
    DirNode *newDirNode = (DirNode *)malloc(sizeof(DirNode));

    strcpy(newDirNode->path, fullPath);

    newDirNode->parentDir = NULL;
    newDirNode->subDir = NULL;
    newDirNode->fileHead = NULL;
    
    newDirNode->next = NULL;
    newDirNode->prev = NULL;

    // 연결리스트의 시작을 newDirNode로 설정
    trackList = newDirNode;
}

// 변경 사항(생성, 수정) 확인 및 기록
void checkChanges(DirNode **dir, char *backupDirPath, char *logPath, char *fullPath, char opt, struct tm *time)
{
    struct dirent **namelist;
    struct stat statbuf;
    DirNode *currDir;
    FileNode *fileNode;
    char backupPath[MAX_PATH + 1];
    char path[MAX_PATH + 1];
    int cnt;
    int i;

    // fullPath에 해당하는 디렉토리 내의 하위 파일 및 디렉토리의 정보 추출
    if((cnt = scandir(fullPath, &namelist, NULL, alphasort)) < 0){
        fprintf(stderr, "scandir error for %s\n", fullPath);
        exit(1);
    }

    // 추출한 파일 및 디렉토리를 바탕으로 연결리스트에 노드 저장
    for(i = 0; i < cnt; i++){
        if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
            continue;

        // 하위 파일 및 디렉토리의 경로
        sprintf(path, "%s/%s", fullPath, namelist[i]->d_name);
        if(lstat(path, &statbuf) < 0){
            fprintf(stderr, "lstat error for %s\n", path);
            exit(1);
        }

        // 파일인 경우
        if(S_ISREG(statbuf.st_mode)){
            // 해당 파일이 연결리스트 내에 삽입되어 있지 않은 경우
            if((fileNode = searchFileNode(*dir, path)) == NULL){
                // 연결리스트 내에 노드 삽입
                insertFileNode(dir, path, statbuf.st_mtime);

                // 생성된 파일을 백업 디렉토리에 추가 및 로그 파일에 기록
                backupModifyFile(backupDirPath, path, time);
                writeLog("create", logPath, path, time);
            }
            // 해당 파일이 연결리스트 내에 삽입되어 있는 경우
            else{
                // 파일의 최종 수정 시간이 변경되었는지 확인
                if(fileNode->mtime != statbuf.st_mtime){
                    // 연결리스트 내 노드 수정
                    fileNode->mtime = statbuf.st_mtime;

                    // 변경된 파일을 백업 디렉토리에 추가 및 로그 파일에 기록
                    backupModifyFile(backupDirPath, path, time);
                    writeLog("modify", logPath, path, time);
                }
            }
        }
        // 디렉토리인 경우
        else if(S_ISDIR(statbuf.st_mode)){
            // 옵션 r인 경우에만 하위 디렉토리 추적
            if(opt & OPT_R){
                // 연결리스트 내에 노드 삽입
                insertDirNode(dir, path);
            }
        }
    }

    // 하위 디렉토리도 확인해야하는 경우
    currDir = (*dir)->subDir;
    while(currDir != NULL){
        sprintf(backupPath, "%s/%s", backupDirPath, getName(currDir->path));
        checkChanges(&currDir, backupPath, logPath, currDir->path, opt, time);
        currDir = currDir->next;
    }

    return;
}

// 삭제 여부 확인 및 기록
void checkRemove(DirNode **dir, char *dirPath, char *logPath, struct tm *time)
{
    DirNode *currDir;
    FileNode *currFile;

    // 파일의 삭제 여부 확인
    currFile = (*dir)->fileHead;
    while(currFile != NULL){
        // 해당 노드(currFile)의 실제 파일이 존재하지 않는 경우
        if(access(currFile->path, F_OK)){
            // 연결리스트 내에서 삭제
            deleteFileNode(&currFile, currFile->path);

            // 로그파일에 기록
            writeLog("remove", logPath, currFile->path, time);
        }

        currFile = currFile->next;
    }

    // 디렉토리의 삭제 여부 확인 (옵션 r인 경우에만 진행됨)
    currDir = (*dir)->subDir;
    while(currDir != NULL){
        checkRemove(&currDir, dirPath, logPath, time);
        currDir = currDir->next;
    }

    return;
}

// 변경 사항이 있는 파일(생성, 변경)을 백업 디렉토리에 백업
void backupModifyFile(char *backupDirPath, char *filePath, struct tm *time)
{
    FILE *originFile, *backupFile;
    char *fileName;
    char *backupFilePath;
    char buf;

    // 백업할 파일의 상위 디렉토리가 없다면 생성
    if(access(backupDirPath, F_OK))
        mkdir(backupDirPath ,0777);

    // 파일의 경로에서 파일의 이름 추출
    fileName = getName(filePath);

    // 원본 파일을 백업 디렉토리에 저장할 백업 파일의 경로
    backupFilePath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    sprintf(backupFilePath, "%s/%s_%04d%02d%02d%02d%02d%02d", backupDirPath, fileName, time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);

    // 기존의 파일을 백업 디렉토리로 백업
    if((originFile = fopen(filePath, "rb")) == NULL){
        fprintf(stderr, "fopen error for %s\n", filePath);
        exit(1);
    }
    if ((backupFile = fopen(backupFilePath, "w+")) == NULL){
        fprintf(stderr, "fopen(%s) error\n", backupFilePath);
        exit(0);
    }
    while (!feof(originFile)){
        fread(&buf, sizeof(char), 1, originFile);
        fwrite(&buf, sizeof(char), 1, backupFile);
    }

    fclose(originFile);
    fclose(backupFile);

    free(backupFilePath);
    free(fileName);
}

// 변경 사항(생성, 변경, 삭제)을 로그에 기록
void writeLog(char *change, char *logPath, char *filePath, struct tm *time)
{
    FILE *fp;
    
    if ((fp = fopen(logPath, "a+")) == NULL){
        fprintf(stderr, "fopen(%s) error\n", logPath);
        exit(0);
    }
    fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d][%s][%s]\n", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec, change, filePath);
    fclose(fp);
}

// 해당 파일의 이름 추출
char *getName(char *fullPath)
{
    char *name = (char *)malloc(sizeof(char) * (MAX_NAME + 1));
    int idx;
    int i;

    for(i = strlen(fullPath); i >= 0 && fullPath[i] != '/'; i--);
    strcpy(name, fullPath + i + 1);

    return name;
}

// filePath에 해당하는 파일이 존재하는지 확인
FileNode *searchFileNode(DirNode *parentDir, char *filePath)
{
    FileNode *currFile = parentDir->fileHead;

    // filePath와 동일한 파일 노드 탐색
    while(currFile != NULL){
        if(!strcmp(filePath, currFile->path))
            break;

        currFile = currFile->next;
    }

    return currFile;
}

// dirPath에 해당하는 파일의 정보를 담은 노드를 연결리스트에 삽입
void insertDirNode(DirNode **parentDir, char *dirPath)
{
    DirNode *currDir = (*parentDir)->subDir;
    DirNode *newDirNode;
    int result;

    // 디렉토리 노드 생성 및 초기화
    newDirNode = (DirNode *)malloc(sizeof(DirNode));
    strcpy(newDirNode->path, dirPath);
    newDirNode->parentDir = *parentDir;
    newDirNode->subDir = NULL;
    newDirNode->fileHead = NULL;
    newDirNode->next = NULL;
    newDirNode->prev = NULL;

    // 상위 디렉토리(parentDir)의 하위 디렉토리 노드가 존재하지 않는 경우: 바로 삽입
    if(currDir == NULL){
        (*parentDir)->subDir = newDirNode;
    }
    // 상위 디렉토리(parentDir)의 하위 디렉토리 노드가 존재하는 경우: 순서대로 삽입
    else{
        while(currDir != NULL){
            // dirName에 해당하는 파일이 이미 연결 리스트에 삽입되어 있는 경우
            if((result = strcmp(currDir->path, dirPath)) == 0){
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
}

// filePath에 해당하는 파일의 정보를 담은 노드를 연결리스트에 삽입
void insertFileNode(DirNode **parentDir, char *filePath, time_t mtime)
{
    FileNode *currFile = (*parentDir)->fileHead;
    FileNode *newFileNode;
    int result;

    // 파일 노드 생성 및 초기화
    newFileNode = (FileNode *)malloc(sizeof(FileNode));
    strcpy(newFileNode->path, filePath);
    newFileNode->mtime = mtime;
    newFileNode->parentDir = *parentDir;
    newFileNode->next = NULL;
    newFileNode->prev = NULL;

    // parentDir 디렉토리 하위에 추적할 파일이 존재하지 않는 경우: 노드 생성 후, 현재 파일(currFile)과 연결
    if(currFile == NULL){
        (*parentDir)->fileHead = newFileNode;
    }
    // parentDir 디렉토리 하위에 추적할 파일이 존재하는 경우: 오름차순으로 파일 연결
    else{
        while(currFile != NULL){
            // fileName에 해당하는 파일이 이미 연결 리스트에 삽입되어 있는 경우
            if((result = strcmp(currFile->path, filePath)) == 0){
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
                    
                    (*parentDir)->fileHead = newFileNode;
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

// filePath에 해당하는 파일의 정보를 담은 노드를 연결리스트에서 삭제
void deleteFileNode(FileNode **file, char *filePath)
{
    FileNode *currFile = *file;

    // 삭제하려는 노드가 첫 번째 노드인 경우
    if(currFile->prev == NULL){
        currFile->parentDir->fileHead = currFile->next;

        if(currFile->next != NULL)
            currFile->next->prev = currFile->prev;
    }
    // 그 외의 노드가 삭제되는 경우
    else{
        currFile->prev->next = currFile->next;

        if(currFile->next != NULL)
            currFile->next->prev = currFile->prev;
    }

    free(currFile);
}

// pid에 해당하는 데몬 프로세스가 존재하는지 확인
int isExistPid(int pid, char *fullPath)
{
    FILE *fp;
    char buf[BUFFER_SIZE];
    char *tmpPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    int logPid;
    int isExist = 0;

    if((fp = fopen(monitorPATH, "rb")) != NULL){
        while(fgets(buf, BUFFER_SIZE, fp)){
            buf[strlen(buf) - 1] = '\0';
            sscanf(buf, "%d : %s", &logPid, tmpPath);

            if(pid == logPid){
                isExist = 1;
                strcpy(fullPath, tmpPath);

                break;
            }
        }
    }

    fclose(fp);

    free(tmpPath);

    return isExist;
}

// pid에 해당하는 데몬 프로세스 삭제
void removeDaemon(int pid)
{
    FILE *originFile, *newFile;
    char *backupPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char *logPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    char buf[BUFFER_SIZE];
    char fullPath[MAX_PATH + 1];
    char newFileName[MAX_NAME + 1];
    int logPid;

    sprintf(backupPath, "%s/%d", backupPATH, pid);
    sprintf(logPath, "%s.log", backupPath);

    strcpy(newFileName, "temp");

    // monitor_list.log 파일에서 해당 데몬프로세스의 기록 삭제
    if((originFile = fopen(monitorPATH, "rb")) == NULL){
        fprintf(stderr, "fopen error for %s\n", monitorPATH);
        exit(1);
    }
    if((newFile = fopen("temp", "w+")) == NULL){
        fprintf(stderr, "fopen error for %s\n", "temp");
        exit(1);
    }
    while(fgets(buf, BUFFER_SIZE, originFile) != NULL){
        sscanf(buf, "%d : %s\n", &logPid, fullPath);
        
        if(pid == logPid){
            continue;
        }

        fprintf(newFile, "%s", buf);
    }

    fclose(originFile);
    fclose(newFile);

    remove(monitorPATH);
    rename(newFileName, monitorPATH);

    // 백업 파일 삭제
    removeBackup(backupPath);
    rmdir(backupPath);

    // 로그 파일 삭제
    remove(logPath);
}

// 백업한 파일들 모두 삭제
void removeBackup(char *backupPath)
{
    struct dirent **namelist;
    struct stat statbuf;
    char *fullPath;
    int cnt;
    int i;

    if((cnt = scandir(backupPath, &namelist, NULL, alphasort)) < 0){
        fprintf(stderr, "scandir error for %s\n", backupPath);
        exit(1);
    }

    fullPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    strcpy(fullPath, backupPath);
    for(i = 0; i < cnt; i++){
        if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
            continue;

        sprintf(fullPath, "%s/%s", backupPath, namelist[i]->d_name);
        if(lstat(fullPath, &statbuf) < 0){
            fprintf(stderr, "lstat error for %s\n", fullPath);
            exit(1);
        }

        if(S_ISREG(statbuf.st_mode))
            remove(fullPath);
        else if(S_ISDIR(statbuf.st_mode)){
            removeBackup(fullPath);
            rmdir(fullPath);
        }
    }
}