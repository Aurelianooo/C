#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>

bool recursive(char* s, char** wordDict, int wordDictSize, int length, int index, int* record){
	if(length==index) return true;
    if(record[length-index-1]) return false;

	int i, j;
	bool tmp=false;
	for(i=0;i<wordDictSize;i++){
		for(j=0;j<strlen(wordDict[i]);j++){
			if(s[index+j]!=wordDict[i][j]){
				j=0;break;
			}
		}
		if(!j) continue;
		else tmp=tmp || recursive(s, wordDict, wordDictSize, length, index+j, record);
		if(tmp) return true;
	}
    record[length-index-1]=1;
	return false;
}

bool wordBreak(char* s, char** wordDict, int wordDictSize) {
    int length =strlen(s);
    int* record=(int*)malloc(length*sizeof(int));
    memset(record, 0, length*sizeof(int));

    return recursive(s, wordDict, wordDictSize, length, 0, record);
}

int main(void){
    char* s="catsandog";
    char* wordDict[5]={"cats", "dog", "sand", "and", "cat"};
    int wordDictSize=5;
    if(wordBreak(s, wordDict, wordDictSize)) printf("true\n");
    else printf("false\n");

    return 0;
}
