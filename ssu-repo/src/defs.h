#define OPENSSL_API_COMPAT 0x10100000L

#define FALSE   0
#define TRUE    1

#define USAGE_ADD       "add <PATH> : add files/directories to staging area"
#define USAGE_REMOVE    "remove <PATH> : remove files/directories from staging area"
#define USAGE_STATUS    "status : show staging area status"
#define USAGE_COMMIT    "commit <name> : backup staging area with commit name"
#define USAGE_REVERT    "revert <name> : revert commit directory with commit name"
#define USAGE_LOG       "log : show commands for log"
#define USAGE_HELP      "help : show commands for program"
#define USAGE_EXIT      "exit : exit program"

#define MAX_NAME 255 // 파일 이름의 최대 길이
#define MAX_PATH 4096 // 파일 경로의 최대 길이

#define BUFFER_SIZE (MAX_PATH + MAX_NAME) // 별 뜻 없음

// path.c
extern char *homePATH;
extern char *exePATH;
extern char *repoPATH;
extern char *stagingPATH;
extern char *commitPATH;