#include "path.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 커밋 로그 출력
// argv[0]: log
// argv[1]: [NAME]
// ...
int main(int argc, char *argv[])
{
    FILE *fp;
    char buf[BUFFER_SIZE];
    char commitName[BUFFER_SIZE];
    char prevCommitName[BUFFER_SIZE];
    char action[BUFFER_SIZE];
    char path[BUFFER_SIZE];
    char *token = NULL;
    int isEmpty = TRUE;

    initPath();

    // commit.log 오픈
    if((fp = fopen(commitPATH, "rb")) == NULL){
        fprintf(stderr, "fopen error for %s\n", commitPATH);
        exit(1);
    }

    strcpy(prevCommitName, "");

    // 모든 커밋 로그 출력
    if(argc == 1){
        while(fgets(buf, sizeof(buf), fp) != NULL){
            isEmpty = FALSE;

            buf[strlen(buf) - 1] = '\0';

            if(sscanf(buf, "commit: \"%[^\"]\" - new file: \"%[^\"]\"", commitName, path) == 2)
                strcpy(action, "new file");
            else if(sscanf(buf, "commit: \"%[^\"]\" - modified: \"%[^\"]\"", commitName, path) == 2)
                strcpy(action, "modified");
            else if(sscanf(buf, "commit: \"%[^\"]\" - removed: \"%[^\"]\"", commitName, path) == 2)
                strcpy(action, "removed");
            
            else{
                fprintf(stderr, "ERROR: \"commit.log\" file contains invalid values\n");
                exit(1);
            }

            // 해당 문구는 각 커밋마다 처음에만 출력
            if(strcmp(prevCommitName, commitName))
                printf("commit: \"%s\"\n", commitName);
            
            printf(" - %s: \"%s\"\n", action, path);

            strcpy(prevCommitName, commitName);
        }
    }
    // NAME 으로 입력된 커밋 로그 출력
    else if(argc >= 2){
        char name[MAX_NAME + 1];
        int isExist = FALSE;

        // 입력 받은 이름은 띄어쓰기도 허용하므로, argv[1]~argv[n]까지 변수를 하나의 이름으로 가공
        if(processName(argc - 1, argv + 1, name) < 0){
            fprintf(stderr, "ERROR: name cannot exceed 255 bytes\n");
            exit(1);
        }

        // .commit.log 파일 한 줄씩 읽기
        // NAME으로 commit 명령어를 실행한 적이 없다면 에러 처리 후 프롬프트 재출력
        while(fgets(buf, sizeof(buf), fp) != NULL){
            isEmpty = FALSE;

            buf[strlen(buf) - 1] = '\0';

            if(sscanf(buf, "commit: \"%[^\"]\" - new file: \"%[^\"]\"", commitName, path) == 2)
                strcpy(action, "new file");
            else if(sscanf(buf, "commit: \"%[^\"]\" - modified: \"%[^\"]\"", commitName, path) == 2)
                strcpy(action, "modified");
            else if(sscanf(buf, "commit: \"%[^\"]\" - removed: \"%[^\"]\"", commitName, path) == 2)
                strcpy(action, "removed");

            else{
                fprintf(stderr, "ERROR: \"commit.log\" file contains invalid values\n");
                exit(1);
            }

            if(!strcmp(name, commitName)){
                isExist = TRUE;

                // 해당 문구는 처음에만 출력
                if(strcmp(prevCommitName, commitName))
                    printf("commit: \"%s\"\n", commitName);

                printf(" - %s: \"%s\"\n", action, path);
            }

            strcpy(prevCommitName, commitName);
        }

        // 커밋 기록이 있지만 해당 이름으로 커밋한 적이 없는 경우
        if(!isEmpty && !isExist){
            fprintf(stderr, "No committed to that \"%s\"\n", name);
            exit(1);
        }
    
    }

    // 커밋 기록이 없다면 에러 처리 후 프롬프트 재출력
    if(isEmpty){
        fprintf(stderr, "No commit history\n");
        exit(1);
    }

    fclose(fp);

    return 0;
}