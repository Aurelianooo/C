#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>

bool recursive(char** board, int boardSize, int* boardColSize, char* word, int index, int n, int i, int j){
	if(index==n) return true;
	if(board[i][j]!=word[index]) return false;
	bool left=false, up=false, right=false, down=false;
    board[i][j]=0;
	if(j-1>=0) left=recursive(board, boardSize, boardColSize, word, index+1, n, i, j-1);
	if(i-1>=0) up=recursive(board, boardSize, boardColSize, word, index+1, n, i-1, j);
	if(j+1<*boardColSize) right=recursive(board, boardSize, boardColSize, word, index+1, n, i, j+1);
	if(i+1<boardSize) down=recursive(board, boardSize, boardColSize, word, index+1, n, i+1, j);
	if(left||up||right||down) return true;
    board[i][j]=word[index];
    return false;
}

bool exist(char** board, int boardSize, int* boardColSize, char* word) {
    int i, j;
    int n=strlen(word);
    for(i=0;i<boardSize;i++){
    	for(j=0;j<*boardColSize;j++){
    		if(recursive(board, boardSize, boardColSize, word, 0, n, i, j))
    			return true;
    	}
    }
    return false;
}

int main(void){
    char board[3][4]={{'A','B','C','E'},{'S','F','C','S'},{'A','D','E','E'}};
    char* word="ABCCED";
    int boardSize=3, boardColSize=4, i;
    char* p[boardSize];
    for(i=0;i<boardSize;i++) p[i]=board[i];
    if(exist(p, boardSize, &boardColSize, word)) printf("true\n");
    else printf("false\n");

    return 0;
}
