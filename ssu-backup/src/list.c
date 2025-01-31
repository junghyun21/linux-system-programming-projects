// #include "command.h"
// #include "filepath.h"

// Dir *canList(char *originPath, char *name, int *isReg);

// // sudo ./list [PATH]
// // argv[0]: ./list
// // argv[1]: PATH
// int main(int argc, char *argv[])
// {
//     Dir *list;
//     char *fullPath = (char *)malloc(sizeof(char) * (MAX_PATH + 1));
//     char *name = (char *)malloc(sizeof(char) * (MAX_PATH + 1));

//     // 홈 디렉토리, 현재 실행 디렉토리 등을 초기화하고, 만약 백업 디렉토리와 로그 파일이 없다면 생성
//     initPath();
    
//     // 경로가 들어오지 않은 경우
//     if(argc == 1){
//         // 백업된 파일들의 정보를 저장한 연결리스트 생성 (listHead가 연결리스트 가리킴)
//         listHead = createBackupList();

        
//     }
//     // 경로가 들어온 경우
//     else if(argc == 2){
//         // 입력 받은 경로를 절대 경로(fullPath)로 변환하고, 경로에 해당하는 파일/디렉토리명(name)을 저장하고
//         // 해당 파일/디렉토리가 올바른 경로인지 확인
//         if(processPath(argv[1], fullPath, name) < 0 || checkPath(fullPath, name) < 0){
//             fprintf(stderr, "incorrect path: %s\n", argv[1]);
//             exit(1);
//         }

//         // 백업된 파일들의 정보를 저장한 연결리스트 생성 (listHead가 연결리스트 가리킴)
//         listHead = createBackupList();

//         // 입력 받은 경로와 일치하는 백업 파일 및 디렉토리가 없는 경우: 에러 처리 후 비정상 종료
//         // 그렇지 않다면 해당 경로는 파일인지 디렉토리인지 isReg에 저장하고
//         // 대상이 되는 경로의 Dir 노드 반환
//         if((list = canList(fullPath, name, &isReg)) == NULL){
//             fprintf(stderr, "%s that do not exist in backup directory\n", argv[1]);
//             exit(1);
//         }

    


//     }
//     else{
//         help("list");
//         exit(1);
//     }

//     exit(0);
// }

// Dir *canList(char *originPath, char *name, int *isReg)
// {   
//     Dir *dirNode;
//     BackupDir *backupDirNode;
//     char *parentPath = (char *)malloc(sizeof(char *) * (MAX_PATH + 1));
//     char *backupPath = (char *)malloc(sizeof(char *) * (MAX_PATH + 1));

//     sprintf(backupPath, "%s%s", backupPATH, originPath + strlen(homePATH));

//     // 대상의 경로에 대해 백업 파일 리스트를 탐색
//     // - 디렉토리: 디렉토리의 모든 경로에 해당하는 디렉토리들이 연결 리스트에 저장되어 있음
//     // - 파일: 연결 리스트 노드 내에는 상위 디렉토리의 정보만 있고 파일 자체의 정보는 없음, 따라서 무조건 NULL 반환
//     if((dirNode = searchBackupDir(listHead, originPath)) == NULL){
//         // 파일은 무조건 코드 진행
//         // 디렉토리가 이전 조건문에서 NULL을 반환했다면, 다음 코드도 무조건 NULL을 반환
//         *isReg = TRUE;

//         strcpy(parentPath, originPath);
//         parentPath[strlen(parentPath) - strlen(name) - 1] = '\0';

//         if((dirNode = searchBackupDir(listHead, parentPath)) == NULL)
//             return dirNode;

//         backupDirNode = dirNode->head;

//         while(backupDirNode != NULL){
//             // 백업 디렉토리(/home/backup/YYMMDDHHMMSS/...) 내에 해당 파일(name)이 존재하는지 확인
//             // 존재한다면 백업 디렉토리 내의 백업 파일과 백업하려는 파일이 동일한 파일인지 검사
//             sprintf(backupPath, "%s/%s", backupDirNode->backupPath, name);

//             // 백업 디렉토리 내에 해당 파일이 존재할 때
//             if(!access(backupPath, F_OK))
//                 break;
            
//             backupDirNode = backupDirNode->next;
//         }

//         // 상위 디렉토리는 있지만, 해당 파일은 존재하지 않는 경우
//         if(backupDirNode == NULL)
//             return NULL;

//     }
//     // 해당 디렉토리가 백업 디렉토리 내에 존재하는 경우
//     else{
//         *isReg = FALSE;
//     }

//     return dirNode;
// }