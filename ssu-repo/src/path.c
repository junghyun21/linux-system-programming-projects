#include "path.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

char *homePATH;
char *exePATH;
char *repoPATH;
char *stagingPATH;
char *commitPATH;

// 전역변수에 있는 path들 초기화
void initPath()
{
    homePATH = getenv("HOME"); // 홈 디렉토리 경로
    exePATH = getcwd(NULL, 0); // 현재 작업 경로
    
    // 레포 디렉토리 경로
    repoPATH = (char *)malloc(sizeof(char) * (strlen(exePATH) + strlen("/.repo") + 1));
    sprintf(repoPATH, "%s/.repo", exePATH);

    // 스테이징 구역 경로
    stagingPATH = (char *)malloc(sizeof(char) * (strlen(repoPATH) + strlen("/.staging.log") + 1));
    sprintf(stagingPATH, "%s/.staging.log", repoPATH);

    // 커밋 로그 경로
    commitPATH = (char *)malloc(sizeof(char) * (strlen(repoPATH) + strlen("/.commit.log") + 1));
    sprintf(commitPATH, "%s/.commit.log", repoPATH);
}

// 입력 받은 경로(path)를 절대 경로(fullPath)로 변환하고, 경로에 해당하는 파일/디렉토리명(name)을 저장
int processPath(char *path, char *fullPath, char *relPath, char *name)
{
    PathList *headPath;
    PathList *currPath;
    PathList *temp = NULL;
    char *tmpPath;
    char *token;
    int i;

    // 입력 받은 경로(path)를 절대 경로(fullPath)로 변환
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
            currPath = currPath->prev;
            currPath->next = NULL;
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

    // 상대 경로 저장
    sprintf(relPath, ".%s", fullPath + strlen(exePATH));

    return 0;
}

// 파일의 경로와 이름, 접근권한 등 올바른지 확인
int checkPath(char *fullPath, char *name)
{
    // 해당 경로가 레포 디렉토리에 포함된 경우: 에러 처리 후 프롬프트 재출력
    if(!strncmp(fullPath, repoPATH, strlen(repoPATH)))
        return -1;

    // 파일 경로의 길이가 4096byte를 넘기거나, 이름의 길이가 255byte를 넘기는 경우: 에러 처리 후 프롬프트 재출력
    if(strlen(fullPath) > MAX_PATH || strlen(name) > MAX_NAME)
        return -1;

    // 입력받은 경로(path)가 사용자의 현재 작업경로를 벗어난 경우: 에러 처리 후 프롬프트 재출력
    if(strncmp(fullPath, exePATH, strlen(exePATH)))
        return -1;

    return 0;
}

int processName(int argc, char *argv[], char *name)
{
    char buf[BUFFER_SIZE];
    int i;

    memset(buf, 0, BUFFER_SIZE);

    for(i = 0; i < argc; i++){
        strcat(buf, argv[i]);
        strcat(buf, " ");
    }
    buf[strlen(buf) - 1] = '\0';

    // 따옴표 제거
    if(buf[0] == '\"' || buf[0] == '\'')
        sprintf(buf, "%s", buf + 1);

    if(buf[strlen(buf) - 1] == '\"' || buf[strlen(buf) - 1] == '\'')
        buf[strlen(buf) - 1] = '\0';

    // 해당 이름이 이름 제한(255 bytes)를 넘기면 에러
    if(strlen(buf) > MAX_NAME)
        return -1;

    strcpy(name, buf);

    return 0;
}