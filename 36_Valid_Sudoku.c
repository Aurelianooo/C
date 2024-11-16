#include<stdio.h>
#include<stdbool.h>
#include<string.h>

bool isValidSudoku(char** board, int boardSize, int* boardColSize) {
    int hash[10], i, j, k, tmp;
    memset(hash, 0, sizeof(hash));
    for(i=0;i<boardSize;i++){
    	for(j=0;j<boardSize;j++){
    		if(board[i][j]=='.') continue;
    		tmp=board[i][j]-'0';
    		if(hash[tmp]) return false;
    		else hash[tmp]++;
    	}
    	memset(hash, 0, sizeof(hash));
    }
    
    for(i=0;i<boardSize;i++){
    	for(j=0;j<boardSize;j++){
	    	if(board[j][i]=='.') continue;
    		tmp=board[j][i]-'0';
    		if(hash[tmp]) return false;
    		else hash[tmp]++;
    	}
    	memset(hash, 0, sizeof(hash));
    }
    for(k=0;k<3;k++){
		for(i=0+k*3;i<3+k*3;i++){
			for(j=0;j<3;j++){
				if(board[i][j]=='.') continue;
				tmp=board[i][j]-'0';
				if(hash[tmp]) return false;
				else hash[tmp]++;
			}
		}
		memset(hash, 0, sizeof(hash));
    }
    
    for(k=0;k<3;k++){
		for(i=0+k*3;i<3+k*3;i++){
			for(j=3;j<6;j++){
				if(board[i][j]=='.') continue;
				tmp=board[i][j]-'0';
				if(hash[tmp]) return false;
				else hash[tmp]++;
			}
		}
		memset(hash, 0, sizeof(hash));
    }
    
    for(k=0;k<3;k++){
		for(i=0+k*3;i<3+k*3;i++){
			for(j=6;j<9;j++){
				if(board[i][j]=='.') continue;
				tmp=board[i][j]-'0';
				if(hash[tmp]) return false;
				else hash[tmp]++;
			}
		}
		memset(hash, 0, sizeof(hash));
    }
    
    return true;
}

int main(void){
    /*char *board[9]={"53..7....",
					"6..195...",
					".98....6.",
					"8...6...3",
					"4..8.3..1",
					"7...2...6",
					".6....28.",
					"...419..5",
					"....8..79"};*/
	
	char board[][9]={{'5','3','.','.','7','.','.','.','.'}
					,{'6','.','.','1','9','5','.','.','.'}
					,{'.','9','8','.','.','.','.','6','.'}
					,{'8','.','.','.','6','.','.','.','3'}
					,{'4','.','.','8','.','3','.','.','1'}
					,{'7','.','.','.','2','.','.','.','6'}
					,{'.','6','.','.','.','.','2','8','.'}
					,{'.','.','.','4','1','9','.','.','5'}
					,{'.','.','.','.','8','.','.','7','9'}};
	int i;
	char *p[9];
	for(i=0;i<9;i++){
		p[i]=board[i];
	}
	int boardColSize =9;
	bool ans=isValidSudoku(p, 9, &boardColSize);
	if(ans) printf("true\n");
	else printf("false\n");

    return 0;
}
