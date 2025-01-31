// md5와 관련된 헤더파일
#ifndef FILEDATA_H
#define FILEDATA_H

#define HASH_MD5  33

void ConvertHash(char *path, char *hashRes);
int cmpHash(char *path1, char *path2);
char *cvtSizeComma(int size);

#endif