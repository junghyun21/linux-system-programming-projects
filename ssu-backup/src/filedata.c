#include "filedata.h"
#include "defs.h"
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// 파일의 내용을 바탕으로 해시값으로 변환
void convertHash(char *path, char *hashRes)
{
	FILE *fp;
	unsigned char hash[MD5_DIGEST_LENGTH];
	unsigned char buffer[SHRT_MAX];
	int bytes = 0;
	MD5_CTX md5;

	if((fp = fopen(path, "rb")) == NULL){
		fprintf(stderr, "fopen error for %s\n", path);
		exit(1);
	}

	MD5_Init(&md5);

	while((bytes = fread(buffer, 1, SHRT_MAX, fp)) != 0)
		MD5_Update(&md5, buffer, bytes);
	
	MD5_Final(hash, &md5);

	for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
		sprintf(hashRes + (i * 2), "%02x", hash[i]);
	hashRes[HASH_MD5-1] = 0;

	fclose(fp);
}

// 두 파일의 해시값을 비교
// 같으면 0 리턴
int cmpHash(char *path1, char *path2)
{
	char *hash1 = (char *)malloc(sizeof(char) * HASH_MD5);
	char *hash2 = (char *)malloc(sizeof(char) * HASH_MD5);

	convertHash(path1, hash1);
	convertHash(path2, hash2);

	return strcmp(hash1, hash2);
}

// 정수형의 파일의 크기를 천 단위로 쉼표를 찍어 문자열에 저장
char *cvtSizeComma(int size)
{
	char *str = (char *)malloc(sizeof(char) * BUFFER_SIZE);
	char *ret = (char *)malloc(sizeof(char) * BUFFER_SIZE);
	int i;

	// 일의 자리부터(거꾸로) 파일의 사이즈를 가져옴
	for(i = 0; size > 0; i++){
		str[i] = (size % 10) + '0';
		size /= 10;

		// 천 단위로 쉼표 찍음
		if((i % 4) == 2) {
			i++;
			str[i] = ',';
		}
	}
	str[i] = '\0';

	// str에는 값이 거꾸로 저장되어 있기 때문에, ret에 뒤집어서 값 저장
	for(i = 0; i < strlen(str); i++) {
		ret[i] = str[strlen(str) - i - 1];
	}
	ret[i] = '\0';
	
	return ret;
}