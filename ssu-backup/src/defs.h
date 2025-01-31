// 프로그램을 실행하는 데 필요한 매크로 상수체를 선언한 헤더 파일
#ifndef DEFS_H
#define DEFS_H

#define OPENSSL_API_COMPAT 0x10100000L

#define TRUE 1
#define FALSE 0

#define MAX_PATH 4096
#define MAX_NAME 255
#define LEN_DATE 12

#define BUFFER_SIZE 4096

#define OPT_A 0b00000001
#define OPT_D 0b00000010
#define OPT_L 0b00000100
#define OPT_N 0b00001000
#define OPT_R 0b00010000
#define OPT_Y 0b00100000

#endif