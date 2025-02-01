#define OPENSSL_API_COMPAT 0x10100000L

#define USAGE_ADD       "add <PATH> [OPTION] : add new daemon process of <PATH> if <PATH> is file\n \
                -d : add new daemon process of <PATH> if <PATH> is directory\n \
                -r : add new daemon process of <PATH> recursive if <PATH> is directory\n \
                -t <PERIOD> : set daemon process time to <PERIOD> sec (default : 1 sec)"
#define USAGE_REMOVE    "remove <DAEMON_PID> : delete deamon porcess with <DAEMON_PID>"
#define USAGE_LIST      "list [DAEMON_PID] : show daemon process list of dir tree"
#define USAGE_HELP      "help [COMMAND]: show commands for program"
#define USAGE_EXIT      "exit : exit program"

#define OPT_D 0b0001
#define OPT_R 0b0010
#define OPT_T 0b0100

#define MAX_NAME 255 // 파일 이름의 최대 길이
#define MAX_PATH 4096 // 파일 경로의 최대 길이
#define LEN_DATE 10 // 날짜 길이 (YYYY-MM-DD)
#define LEN_TIME 8 // 시간 길이 (HH:MM:SS)

#define BUFFER_SIZE (MAX_PATH + MAX_NAME) // 별 뜻 없음

// path.c
extern char *homePATH;
extern char *exePATH;
extern char *backupPATH;
extern char *monitorPATH;