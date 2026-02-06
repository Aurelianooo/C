#include<stdio.h>

int countAlive(int** board,int i,int j, int boardSize, int* boardColSize){
	int c=0;
	if(i-1>=0 && j-1>=0 && (board[i-1][j-1]==1 || board[i-1][j-1]==-1)) c++;
	if(i-1>=0 && (board[i-1][j]==1 || board[i-1][j]==-1)) c++;
	if(i-1>=0 && j+1<*boardColSize && (board[i-1][j+1]==1 || board[i-1][j+1]==-1)) c++;
	if(j-1>=0 && (board[i][j-1]==1 || board[i][j-1]==-1)) c++;
	if(j+1<*boardColSize && (board[i][j+1]==1 || board[i][j+1]==-1)) c++;
	if(i+1<boardSize && j-1>=0 && (board[i+1][j-1]==1||board[i+1][j-1]==-1)) c++;
	if(i+1<boardSize && (board[i+1][j]==1||board[i+1][j]==-1)) c++;
	if(i+1<boardSize && j+1<*boardColSize && (board[i+1][j+1]==1||board[i+1][j+1]==-1)) c++;
	return c;
}

void gameOfLife(int** board, int boardSize, int* boardColSize) {
    int i,j,c;
    for(i=0;i<boardSize;i++){
    	for(j=0;j<*boardColSize;j++){
    		c=countAlive(board, i,j,boardSize,boardColSize);
    		if(board[i][j]==1 && (c<2 || c>3)) board[i][j]=-1;
    		if(board[i][j]==0 && c==3) board[i][j]=-2;
    	}
    }
    
    for(i=0;i<boardSize;i++){
    	for(j=0;j<*boardColSize;j++){
    		if(board[i][j]==-1) board[i][j]=0;
    		if(board[i][j]==-2) board[i][j]=1;
    	}
    }
}

int main(void){
    int board[][3]={{0,1,0},{0,0,1},{1,1,1},{0,0,0}};
    int boardSize=4, boardColSize=3,i,j;
    int *p[boardColSize];
    for(i=0;i<boardSize;i++){
    	p[i]=board[i];
    }
    gameOfLife(p, boardSize, &boardColSize);
    for(i=0;i<boardSize;i++){
    	for(j=0;j<boardColSize;j++){
    		printf("%d ", board[i][j]);
    	}
    	printf("\n");
    }

    return 0;
}
