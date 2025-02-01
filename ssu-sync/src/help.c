#include "defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// help [command]
// argv[0]: help
// argv[1]: command
int main(int argc, char *argv[])
{    
    // 모든 내장 명령어에 대한 설명을 출력하는 경우
    if(argc == 1){
        printf("Usage: \n");
        printf("    > %s\n", USAGE_ADD);
        printf("    > %s\n", USAGE_REMOVE);
        printf("    > %s\n", USAGE_LIST);
        printf("    > %s\n", USAGE_HELP);
        printf("    > %s\n", USAGE_EXIT);
    }
    // 특정 명령어에 대한 설명을 출력하는 경우
    else{
        if(!strcmp(argv[1], "add")) printf("Usage: %s\n", USAGE_ADD);
        else if(!strcmp(argv[1], "remove")) printf("Usage: %s\n", USAGE_REMOVE);
        else if(!strcmp(argv[1], "list")) printf("Usage: %s\n", USAGE_LIST);
        else if(!strcmp(argv[1], "help")) printf("Usage: %s\n", USAGE_HELP);
        else if(!strcmp(argv[1], "exit")) printf("Usage: %s\n", USAGE_EXIT);
        else printf("ERROR: help [add | remove | list | help | exit]\n");
    }

    exit(0);
}