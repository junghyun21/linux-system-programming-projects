#include "filepath.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

char exePATH[MAX_PATH];
char homePATH[MAX_PATH];
char backupPATH[MAX_PATH];
char logPATH[MAX_PATH];

static Dir *insertDirNode(Dir **listHead, char *path);
static void insertBackupDirNode(Dir **dir, BackupDir *backupDir);

// 현재 프로세스가 저장된 디렉토리(exePATH), 홈 디렉토리(homePATH), 백업 디렉토리(backupPATH), 로그 파일(logPATH)의 절대 경로 저장
void initPath()
{ 
    char *tempPATH;
    int idx;
    int fd;

    // 현재 실행되고 있는 프로세스의 상위 디렉토리
    getcwd(exePATH, MAX_PATH);

    // 홈 디렉토리 (/home/user)
    // sudo로 프로그램 실행시키면, getenv("HOME")의 리턴값은 '/root'
    // sprintf(homePATH, "%s", getenv("HOME"));
    tempPATH = (char *)malloc(sizeof(char) * strlen(exePATH));
    strcpy(tempPATH, exePATH);

    for(idx = 6; exePATH[idx] != '/'; idx++); // '/home/' 제외
    tempPATH[idx] = '\0';

    strcpy(homePATH, tempPATH);

    // 백업 디렉토리 (/home/backup)
    strcpy(backupPATH, "/home/backup");
    if(access(backupPATH, F_OK)){
        if(mkdir(backupPATH, 0777) < 0){
            fprintf(stderr, "mkdir error for %s\n", backupPATH);
            exit(1);
        }
    }

    // 로그 파일 (/home/backpup/ssubak.log)
    strcpy(logPATH, "/home/backup/ssubak.log");
    if(access(logPATH, F_OK)){
        if((fd = creat(logPATH, 0666)) < 0){
            fprintf(stderr, "creat error for %s\n", logPATH);
            exit(1);
        }
    }
}

// 입력 받은 경로(path)를 절대 경로(fullPath)로 변환하고, 경로에 해당하는 파일/디렉토리명(name)을 저장
int processPath(char *path, char *fullPath, char *name)
{
    PathList *headPath;
    PathList *currPath;
    PathList *temp;
    char *tmpPath;
    char *token;
    int i;

    // 경로가 입력되지 않은 경우: 에러
    if(path == NULL)
        return -1;

    // 입력 받은 경로(path)를 절대 경로(tmpPath)로 변환
    tmpPath = (char *)malloc(sizeof(char) * MAX_PATH);

    if(path[0] == '~') // 홈 디렉토리인 경우
        sprintf(tmpPath, "%s%s", homePATH, path+1);
    else if(path[0] != '/') // 상대 경로인 경우
        sprintf(tmpPath, "%s/%s", exePATH, path);
    else // 절대 경로인 경우
        sprintf(tmpPath, "%s", path);
    
    // 경로 중간에 .이나 ..이 껴있는 경우, 해당하는 디렉토리 명으로 변경하기 위해 연결리스트(PathList)에 각 경로의 이름 저장
    headPath = (PathList *)malloc(sizeof(PathList));
    currPath = headPath;

    token = strtok(tmpPath, "/"); // 각 디렉토리 및 파일의 이름 추출
    while(token != NULL){
        // 현재 디렉토리인 경우: 아무 작업도 진행하지 않음
        if(!strcmp(token, "."))
            ;
        // 상위 디랙토리인 경우: 상위 디렉토리로 이동 (현재 위치하고 있는 디렉토리 삭제)
        else if(!strcmp(token, "..")){
            temp = currPath;
            currPath = currPath->prev;
            currPath->next = NULL;
            
            free(temp);
        }
        // 디렉토리 혹은 파일의 이름인 경우: 연결리스트의 뒷 부분에 노드 추가
        else{
            temp = (PathList *)malloc(sizeof(PathList));
            strcpy(temp->name, token);
            currPath->next = temp;
            temp->prev = currPath;
            
            currPath =  currPath->next;
        }

        token = strtok(NULL, "/");
    }

    currPath = headPath->next; // 연결 리스트(PathList)의 첫 번째 노드 가리킴

    // 연결 리스트(PathList)에 저장된 값을 바탕으로 경로 가공
    // 기존의 경로: 경로 사이에 .이나 ..이 껴있는 절대 경로
    // 새로운 경로: 경로에는 파일/디렉토리 이름만 있는 절대 경로
    strcpy(tmpPath, "");
    while(currPath != NULL){
        strcat(tmpPath, "/");
        strcat(tmpPath, currPath->name);

        // 연결 리스트(PathList)의 마지막 노드의 이름이 백업 대상이 되는 파일명/디렉토리명
        if(currPath->next == NULL){
            strcpy(name, currPath->name);
            break;
        }
        currPath = currPath->next;
    }
    strcpy(fullPath, tmpPath);

    // 연결 리스트 해제
    currPath = headPath;
    while(currPath != NULL){
        temp = currPath->next;
        free(currPath);
        currPath = temp;
    }

    return 0;
}

// 파일의 경로와 이름이 올바른지 확인
int checkPath(char *path, char *name)
{
    // 입력받은 경로(path)가 사용자의 홈 디렉토리를 벗어날 경우: 에러 처리 후 비정상 종료
    if(strncmp(homePATH, path, strlen(homePATH)))
        return -1;

    // 파일 경로의 길이가 4096byte를 넘기거나, 이름의 길이가 255byte를 넘기는 경우: 에러 처리 후 프로그램 비정상 종료
    if(strlen(path) > MAX_PATH || strlen(name) > MAX_NAME)
        return -1;

    return 0;
}

// 특정 디렉토리에 하위 파일이 존재하는지 검사
void removeEmptyDir(char *backupPath)
{
    char *parentPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
    struct dirent **namelist;
    int cnt;
    int i;

    // parentPath: 삭제한 파일의 상위 디렉토리 경로
    strcpy(parentPath, backupPath);
    for(i = strlen(parentPath); parentPath[i] != '/'; i--);
    parentPath[i] = '\0';

    while(strcmp(parentPath, backupPATH)){
        // 해당 디렉토리 내에 파일 또는 디렉토리가 존재하는지 검사
        if((cnt = scandir(parentPath, &namelist, filter_all, alphasort)) == -1){
            fprintf(stderr, "scandir error for %s\n", parentPath);
            exit(1);
        }

        if(cnt > 0)
            break;
        else{
            // 디렉토리 삭제 (성공 시 0 리턴)
            if(rmdir(parentPath)){
                fprintf(stderr, "rmdir error for %s\n", parentPath);
                exit(1);
            }

            for(i = strlen(parentPath); parentPath[i] != '/'; i--);
            parentPath[i] = '\0';
        }
        

    }

}

// 백업하려는 파일이 저장되어 있는 백업 디렉토리 내 백업 디렉토리의 리스트 추출
Dir *searchBackupDir(Dir *listHead, char *parentPath)
{
    Dir *currDir = listHead;
    char *path = (char *)malloc(sizeof(char) * (strlen(parentPath) + 1));
    char *token;

    // parentPath: 백업 대상의 상위 디렉토리 경로
    strcpy(path, parentPath);

    // 백업 디렉토리 리스트 추출
    if(!strcmp(path, homePATH)){ // /home/user 바로 밑에 있는 파일을 백업하려고 할 때
        return currDir;
    }
    
    // /home/user 를 제외한 나머지 디렉토리명을 통해 리스트 추출
    sprintf(path, "%s", path + strlen(homePATH));
    token = strtok(path, "/");
    while(token != NULL){
        currDir = currDir->subDir;

        // 하위 디렉토리를 돌면서 token에 해당하는 디렉토리 찾음
        while(currDir != NULL){
            if(!strcmp(currDir->dirName, token)){
                break;
            }

            currDir = currDir->siblingDir;
        }

        // 하위 디렉토리에 token 디렉토리가 존재하지 않는 경우
        if(currDir == NULL){
            return NULL;
        }

        token = strtok(NULL, "/");
    }

    return currDir;
    
}

// 모든 백업 파일에 대해 로그 파일 내용과 대조하여 파일들에 대한 백업 상태 정보를 링크드 리스트에 저장
Dir *createBackupList()
{
    // 현재 백업 디렉토리(/home/backup)의 정보를 기준으로 로그 파일에서 필요한 정보 추출
    // backup과 관련된 로그만 확인 (remove, recover 관련 로그는 현재 남아있는 백업 디렉토리 내 백업 파일들과 관련이 없음)
    Dir *listHead;
    FILE *fp;
    char buf[LEN_DATE + 1];
    struct dirent **namelist;
    int backupCnt;
    int canIncrease;
    int idx;
    char c;
    
    // 홈 디렉토리(/home/user)의 정보를 담은 노드 생성
    listHead = (Dir *)malloc(sizeof(Dir));
    strcpy(listHead->dirName, homePATH);
    listHead->subDir = NULL;
    listHead->siblingDir = NULL;
    listHead->head = NULL;
    listHead->tail = NULL;

    // /home/backup 파일 내의 디렉토리/파일의 개수(backupCnt)와 각 이름(namelist) 저장
    if((backupCnt = scandir(backupPATH, &namelist, filter_onlyDir, alphasort)) == -1){
        fprintf(stderr, "scandir error for %s\n", backupPATH);
        exit(1);
    }   

    if(backupCnt == 0)
        return listHead;

    // ssubak.log를 오픈
    if((fp = fopen(logPATH, "r")) == NULL){
        fprintf(stderr, "fopen error for %s\n", logPATH);
        exit(1);
    }

    // 오픈한 ssubak.log 파일에서 로그 기록 한 줄씩 읽어옴
    // 읽는 도중 namelist에 해당하는 파일 관련 로그가 있으면 연결 리스트에 연결
    // 로그 파일과 namelist 모두 최근에 기록/생성된 것일 수록 뒤 쪽에 있음
    canIncrease = FALSE;
    idx = 0;
    while(fgets(buf, LEN_DATE + 1, fp) != NULL){
        // 이전까지 namelist[idx]에 헤당하는 로그 파일 내용이 여러 줄이었을 때
        // 그 다음 줄이 namelise[idx+1]과 일치하는 경우를 찾기 위함
        // 이 과정이 없으면 그 다음 줄은 여전히 namelist[idx]와 비교하게 됨 
        if(idx + 1 < backupCnt && !strncmp(buf, namelist[idx + 1]->d_name, LEN_DATE)){
            ++idx;
            canIncrease = FALSE;
        }

        // 로그 파일은 'YYMMDDHHMMSS: ...' 형식으로 저장
        // 이 때 YYMMDDHHMMSS는 /home/backup 내의 디렉토리 명과 같은 형식
        if(!strncmp(buf, namelist[idx]->d_name, LEN_DATE)){
            Dir *dir;
            BackupDir *backupDir;
            char originPath[MAX_PATH + 1];
            char parentPath[MAX_PATH + 1];
            char backupPath[MAX_PATH + 1];
            int i;

            // YYMMDDHHMMSS : "/home/user/a/b/c.txt" backuped to "/home/backup/YYMMDDHHMMSS/b/c.txt"
            // "/home/user/a/b/c.txt" 이 부분에서 home/user/a/b 부분만 추출
            // 실제 경로의 상위 디렉토리(home/user/a/b)와 백업된 디렉토리(/home/backup/a/b/YYMMDDHHMMSS)는 연동되어 있음
            // 현재 YYMMDDHHMMSS까지 읽어온 상태
            // -> 원본 경로의 상위 디렉토리 경로
            i = 0;
            while((c = fgetc(fp)) != '\"');
            while((c = fgetc(fp)) != '\"')
                originPath[i++] = c;
            
            originPath[i] = '\0'; // /home/user/a/b/c.txt
            if(strncmp(originPath, homePATH, strlen(homePATH))){
                fprintf(stderr, "wrong path is stored in %s\n", logPATH);
                exit(1);
            }

            while(originPath[--i] != '/');
            originPath[i] = '\0'; // /home/user/a/b

            sprintf(parentPath, "%s", originPath + strlen(homePATH)); // /a/b -> listHead가 가리키는 노드는 '/home/user'

            // backup 디렉토리 내의 경로를 알아내기 위함
            // 백업 디렉토리 내에 해당 경로(backupPath)의 파일 및 디렉토리가 존재하지 않으면
            // remove나 recover로 인해 백업 디렉토리 내에서 백업 파일/디렉토리가 삭제된 것
            // 따라서 연결 리스트(listHead)에 해당 경로(parent) 추가하지 않음
            i = 0;
            while((c = fgetc(fp)) != '\"');
            while((c = fgetc(fp)) != '\"')
                backupPath[i++] = c;
            
            backupPath[i] = '\0'; // /home/backup/YYMMDDHHMMSS/b/c.txt

            if(strncmp(backupPath, backupPATH, strlen(backupPATH))){
                fprintf(stderr, "wrong path is stored in %s\n", logPATH);
                exit(1);
            }

            // 존재하면(remove, recover되지 않았으면) 리스트에 연결 
            if(!access(backupPath, F_OK)){ 
                while(backupPath[--i] != '/');
                backupPath[i] = '\0'; // /home/backup/YYMMDDHHMMSS/b

                // /home/backup 내의 백업 디렉토리 노드
                backupDir = (BackupDir *)malloc(sizeof(BackupDir));
                
                backupDir->parentPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
                strcpy(backupDir->parentPath, originPath);
                backupDir->backupPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
                sprintf(backupDir->backupPath, "%s", backupPath);
                strcpy(backupDir->dirName, namelist[idx]->d_name);
                backupDir->next = NULL;

                // backupDir과 연동되어 있는 원본 경로 디렉토리 노드가 listHead 연결리스트 내 존재하는지 확인하고 만약 노드가 존재하지 않는다면 생성
                // backupDir가 연동되어야 할 원본 경로 디렉토리(dir) 리턴
                dir = insertDirNode(&listHead, parentPath);

                // dir에 해당 디렉토리와 연동되어 있는 백업 디렉토리 내 디렉토리(backupDir)들을 연결 리스트로 연결
                // 즉, dir은 backupDir들끼리 연결된 리스트의 헤더(더미) 노드
                insertBackupDirNode(&dir, backupDir);

            }
            
            canIncrease = TRUE;
        }
        // 찾는 파일(namelist[idx])의 로그 기록이 아닌 경우 
        // 현재 idx의 증가가 가능한 상태라면, idx를 증가시키고 idx의 증가가 불가능한 상태로 변경
        // canIncrease: 직전에 로그 파일과 namelist[idx]가 매칭이 되었던 경우에만 TRUE가 됨
        else{
            if(canIncrease){
                if(++idx >= backupCnt)
                    break;
                canIncrease = FALSE;
            }
        }
        
        // 파일에서 한 줄을 다 읽어오지 못한 경우: 다음 개행문자까지 파일 내용 무시
        if(strlen(buf) == LEN_DATE && buf[LEN_DATE - 1] != '\n')
            while((c = fgetc(fp)) != '\n' && c != EOF);

    }

    fclose(fp);

    return listHead;
}

// backupDir과 연동되어 있는 원본 경로 디렉토리 노드가 listHead 연결리스트 내 존재하는지 확인하고
// 존재하지 않는다면 노드를 생성 및 연결하고, 연동되어야 할 원본 경로 디렉토리 리턴
static Dir *insertDirNode(Dir **listHead, char *path)
{
    Dir *currDir = *listHead;
    char *temp = (char *)malloc(sizeof(char) * (strlen(path) + 1));
    char *token;

    strcpy(temp, path);

    // 노드가 이미 연결리스트에 있는지 확인하고, 없으면 노드를 생성하여 연결 리스트에 삽입
    token = strtok(temp, "/"); // 각 디렉토리 및 파일의 이름 추출
    while(token != NULL){
        // 현재 디렉토리(currDir)와 연결된 하위 디렉토리가 하나도 없는 경우
        // : 하위 디렉토리(newDir) 노드 생성 후, 현재 디렉토리와 연결 (subDir)
        if(currDir->subDir == NULL){
            // 새로운 노드 생성 및 초기화
            Dir *newDir = (Dir *)malloc(sizeof(Dir));
            strcpy(newDir->dirName, token);
            newDir->subDir = NULL;
            newDir->siblingDir = NULL;
            newDir->head = NULL;
            newDir->tail = NULL;
            
            // 상위 디렉토리와 연결
            currDir->subDir = newDir;
            currDir = currDir->subDir;
        }
        // 현재 디렉토리(currDir)의 하위 디렉토리가 연결 리스트 내에 삽입되어 있는 경우
        // subDir: 하위 디렉토리들을 오름차순으로 정렬했을 때, 맨 첫 번째 원소
        // siblingDir: 하위 디렉토리들끼리 오름차순으로 가리키는 다음 디렉토리
        else{
            Dir *prev;
            int result;
            int isFirst;
            
            // token과 상위 디렉토리가 동일한 디렉토리들끼리 연결 리스트로 연결
            isFirst = TRUE;
            prev = currDir;
            currDir = currDir->subDir;
            while(currDir != NULL){
                // token에 해당하는 디렉토리가 이미 연결 리스트에 삽입되어 있는 경우
                if((result = strcmp(currDir->dirName, token)) == 0){
                    break;
                }
                // currDir의 디렉토리가 token 디렉토리보다 앞에 삽입되어 있어야 하는 경우
                else if(result < 0){
                    prev = currDir;
                    currDir = currDir->siblingDir;
                }
                // currDir 바로 앞에 token 디렉토리가 삽입되어야 하는 경우
                else{
                    // 새로운 노드 생성 및 초기화
                    Dir *newDir = (Dir *)malloc(sizeof(Dir));
                    strcpy(newDir->dirName, token);
                    newDir->subDir = NULL;
                    newDir->siblingDir = NULL;
                    newDir->head = NULL;
                    newDir->tail = NULL;

                    // token에 해당하는 디렉토리가 가장 맨 앞에 위치하는 경우
                    // 상위 디렉토리의 subDir 포인터가 newDir(token)를 가리켜야 함
                    if(isFirst){
                        newDir->siblingDir = prev->subDir;
                        prev->subDir = newDir;
                    }
                    // token에 해당하는 디렉토리가 중간에 위치하는 경우
                    else{
                        // 'prev - newDir(token) - currDir' 순으로 연결
                        newDir->siblingDir = prev->siblingDir;
                        prev->siblingDir = newDir;
                    }

                    currDir = newDir;

                    break;
                }

                isFirst = FALSE;
            }

            // 하위 디렉토리들을 오름차순으로 정렬했을 때 token에 해당하는 디렉토리가 가장 맨 끝에 위치하는 경우 
            if(currDir == NULL){
                // 새로운 노드 생성 및 초기화
                Dir *newDir = (Dir *)malloc(sizeof(Dir));
                strcpy(newDir->dirName, token);
                newDir->subDir = NULL;
                newDir->siblingDir = NULL;
                newDir->head = NULL;
                newDir->tail = NULL;

                // 'prev - newDir(token)' 순으로 연결 (가장 마지막 노드에 삽입)
                prev->siblingDir = newDir;

                currDir = newDir;
            }
        }

        token = strtok(NULL, "/");
    }

    free(temp);

    return currDir;
}

// 백업 시간을 YYMMDDHHMMSS 형식으로 가공
char *getDate()
{
	char *date = (char *)malloc(sizeof(char) * (LEN_DATE + 1));
	time_t timer;
	struct tm *t;

	timer = time(NULL);	
	t = localtime(&timer);

    sprintf(date, "%02d%02d%02d%02d%02d%02d",t->tm_year %100, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
  
    return date;
}


// 해당 디렉토리(dir)와 연동되어 있는 백업 디렉토리(/home/backup) 내 디렉토리(backupDir)를 dir의 연결리스트에 연결
// 즉, dir은 해당 디렉토리와 연결되어 있는 backupDir 노드들 간의 연결리스트의 헤더(더미) 노드 
static void insertBackupDirNode(Dir **dir, BackupDir *backupDir)
{
    // 해당 디렉토리(dir)에 연동된 백업 디렉토리(/home/backup) 내 디렉토리가 하나도 없는 경우
    if((*dir)->head == NULL){
        (*dir)->head = (*dir)->tail = backupDir;
    }
    // 백업 디렉토리의 노드가 존재하는 경우: 만약 이미 노드가 존재하면 삽입하지 않음
    else{
        // backupDir의 상위 디렉토리를 기준으로 오름차순으로 노드 삽입됨
        // 즉, 노드는 연결 리스트의 가장 마지막에 저장
        // if(strcmp((*dir)->tail->dirName, backupDir->dirName)){
        if(strcmp((*dir)->tail->backupPath, backupDir->backupPath)){
            (*dir)->tail->next = backupDir;
            (*dir)->tail = (*dir)->tail->next;
        }
        // 이미 존재하는 노드인 경우, 새로 생성한 노드 삭제
        else{
            free(backupDir);
        }
    }

}

// scandir을 해줄 때 ., .., 파일을 제외한 디렉토리만 추출하는 필터 함수
// 0을 반환하면 namelist에 해당 entry는 추가되지 않음
int filter_onlyDir(const struct dirent *entry)
{
    // .와 .. 디렉토리는 제외
    if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        return 0;

    // DR_DIR: 디렉토리를 나타내는 상수
    // 디렉토리인 경우 1, 그 외는 0 리턴
    return (entry->d_type == DT_DIR);
}


// scandir 해줄 때, 파일만 추출하는 필터 함수
int filter_onlyfile(const struct dirent *entry)
{
    // .와 .. 디렉토리는 제외
    if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        return 0;

    // DT_REG: 일반 파일을 나타내는 상수
    // 파일인 경우 1, 그 외는 0 리턴
    return (entry->d_type == DT_REG);
}

// scandir 해줄 때, 파일과 디렉토리를 모두 추출하는 필터 함수(.과 .. 제외)
int filter_all(const struct dirent *entry)
{
    if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        return 0;
}