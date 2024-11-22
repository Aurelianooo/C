#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>

// stack的查找太慢了，会超时。

/*bool check(char** board, int i,int j, int boardSize, int* boardColSize, int** p, int* t){
	//printf("%d %d\n", i,j);
	if(i<0 || j<0 || i>=boardSize || j>=*boardColSize) return false;
	if(board[i][j]=='O'){
		int k;
		for(k=(*t)-1;k>=0;k--){
			if(p[k][0]==i && p[k][1]==j) return true;
		}
		p[*t][0]=i;
		p[*t][1]=j;
		(*t)++;
		bool up=check(board, i-1, j, boardSize, boardColSize, p, t);
		bool right=check(board, i, j+1, boardSize, boardColSize, p, t);
		bool down=check(board, i+1, j, boardSize, boardColSize, p, t);
		bool left=check(board, i, j-1, boardSize, boardColSize, p, t);
		return up&&right&&down&&left;
	}else{
		return true;
	}
	
}

void solve(char** board, int boardSize, int* boardColSize) {
    int i, j, t=0;
    int stack[40001][2];
    int *p[40001];
    for(i=0;i<40001;i++) p[i]=stack[i];
    for(i=0;i<boardSize;i++){
    	for(j=0;j<*boardColSize;j++){
    		if(board[i][j]=='O'){
    			if(check(board, i, j, boardSize, boardColSize, p, &t)){
					while(t>0){
						t--;
						board[stack[t][0]][stack[t][1]]='X';
					}
				}else{
					while(t>0){
						t--;
						board[stack[t][0]][stack[t][1]]='L';
					}
				}
    		}
    	}
    }
    
    for(i=0;i<boardSize;i++){
    	for(j=0;j<*boardColSize;j++){
    		if(board[i][j]=='L') board[i][j]='O';
    	}
    }
    
}*/

void deal_area(char** board, int boardSize, int* boardColSize, int i, int j, bool **flag_arr)
{
    if(board[i][j] != 'O' || flag_arr[i][j]) // 如果该部分不需要处理
    {
        return;
    }
    flag_arr[i][j] = true; // 置起处理标志位
    if(i > 0) // 向上蔓延
    {
        deal_area(board, boardSize, boardColSize, i - 1, j, flag_arr);
    }
    if(i + 1 < boardSize) // 向下蔓延
    {
        deal_area(board, boardSize, boardColSize, i + 1, j, flag_arr);
    }
    if(j > 0) // 向左蔓延
    {
        deal_area(board, boardSize, boardColSize, i, j - 1, flag_arr);
    }
    if(j + 1 < boardColSize[0]) // 向右蔓延
    {
        deal_area(board, boardSize, boardColSize, i, j + 1, flag_arr);
    }
}

void solve(char** board, int boardSize, int* boardColSize) {
    bool **flag_arr = (bool **)malloc(boardSize * sizeof(bool *)); // 开辟一片空间用于记录该区域是否被处理过
    for(int i = 0; i < boardSize; i++)
    {
        flag_arr[i] = (bool *)calloc(boardColSize[0], sizeof(bool));
    }

    for(int j = 0; j < boardColSize[0]; j++) // 寻找上边无法被捕获元素
    {
        if('O' == board[0][j])
        {
            deal_area(board, boardSize, boardColSize, 0, j, flag_arr);
        }
    }

    for(int j = 0; j < boardColSize[0]; j++) // 寻找下边无法被捕获元素
    {
        if('O' == board[boardSize - 1][j])
        {
            deal_area(board, boardSize, boardColSize, boardSize - 1, j, flag_arr);
        }
    }

    for(int i = 1; i < boardSize - 1; i++) // 寻找左边无法被捕获元素
    {
        if('O' == board[i][0])
        {
            deal_area(board, boardSize, boardColSize, i, 0, flag_arr);
        }
    }

    for(int i = 1; i < boardSize - 1; i++) // 寻找左边无法被捕获元素
    {
        if('O' == board[i][boardColSize[0] - 1])
        {
            deal_area(board, boardSize, boardColSize, i, boardColSize[0] - 1, flag_arr);
        }
    }

    for(int i = 1; i < boardSize - 1; i++) // 内部未处理过的全为捕获区域
    {
        for(int j = 1; j < boardColSize[0] - 1; j++)
        {
            if(!flag_arr[i][j]) // 如果该区域被处理过无需继续处理
            {
                board[i][j] = 'X';
            }
        }
    }
}


int main(void){
    char board[4][4]={{'X','X','X','X'},{'X','O','O','X'},{'X','X','O','X'},{'X','O','X','X'}};
    int boardSize=4, boardColSize=4, i, j;
    char* p[boardSize];
    for(i=0;i<boardSize;i++) p[i]=board[i];
    solve(p, boardSize, &boardColSize);
    for(i=0;i<boardSize;i++){
    	for(j=0;j<boardColSize;j++)
    		printf("%c ", board[i][j]);
    	printf("\n");
    }

    return 0;
}
