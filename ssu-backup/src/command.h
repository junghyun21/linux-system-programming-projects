// 내장 명령어와 관련된 헤더파일
#ifndef COMMAND_H
#define COMMAND_H

#define NUM_CMD 5

extern char *command[NUM_CMD];

void runCmd(char *cmd, char **arglist);
void help(char *cmd);

#endif