#include "path.h"
#include "struct.h"
#include <stdio.h>
#include <stdlib.h>

// 커밋 이름을 입력받아 해당 이름의 버전으로 되돌아감
// argv[0]: log
// argv[1]: NAME
// argv[2]: NAME
// ...
int main(int argc, char *argv[])
{
    char name[MAX_NAME + 1];

    // 이름 입력되지 않은 경우: commit 명령어에 대한 에러 처리 및 Usage 출력 후, 프롬프트 재출력
    if(argc < 2){
        fprintf(stderr, "ERROR: <PATH> is not include\nUsage: %s\n", USAGE_COMMIT);
        exit(1);
    }

    // 홈 디렉토리, 현재 실행 디렉토리 등을 초기화
    initPath();

    // 입력 받은 이름은 띄어쓰기도 허용하므로, argv[1]~argv[n]까지 변수를 하나의 이름으로 가공
    if(processName(argc - 1, argv + 1, name) < 0){
        fprintf(stderr, "ERROR: name cannot exceed 255 bytes\n");
        exit(1);
    }

    initStagingList();
    initCommitList(name); // name 버전 디렉토리를 포함한 백업 상태 연결 리스트 생성

    // name 버전 디렉토리로 복원
    // 복원한 파일은 recoverList 연결리스트에 저장
    checkRecover(commitList, &recoverList);

    if(recoverList == NULL){
        printf("nothing changed with \"%s\"\n", name);
    }
    else{
        printf("revert to \"%s\"\n", name);
        printRecoverList();
    }

    return 0;
}