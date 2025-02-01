#ifndef COMMAND_H
#define COMMAND_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_CMD 8 // 내장 명령어의 개수
#define MAX_ARGC 1024 // 최대 받을 수 있는 명령행의 개수 (임시로 정해둠) 

extern char *commandList[];

char **getArglist(char *input, int *argCnt);

void builtin_command(char **argList, int argc);
void builtin_help(char *cmd);

#endif