#include<stdio.h>

void travel(char** grid, int i, int j, int gridSize, int* gridColSize){
	if(grid[i][j]=='0' ||grid[i][j]=='2') return;
	else grid[i][j]='2';
	if(j+1<*gridColSize) travel(grid, i, j+1, gridSize, gridColSize);
	if(i+1<gridSize) travel(grid, i+1, j, gridSize, gridColSize);
	if(j-1>=0) travel(grid, i, j-1, gridSize, gridColSize);
    if(i-1>=0) travel(grid, i-1, j, gridSize, gridColSize);
}

int numIslands(char** grid, int gridSize, int* gridColSize) {
	int ans=0, i, j;
	for(i=0;i<gridSize;i++){
		for(j=0;j<*gridColSize;j++){
			if(grid[i][j]=='1'){
				ans++;
				travel(grid, i, j, gridSize, gridColSize);
			}
		}
	}
	
    return ans;
}

int main(void){
    char grid[4][5]={{'1','1','0','0','0'},{'1','1','0','0','0'},{'0','0','1','0','0'},{'0','0','0','1','1'}};
    int gridSize=4, gridColSize=5, i;
    char* p[gridSize];
    for(i=0;i<gridSize;i++){
    	p[i]=grid[i];
    }
    int ans=numIslands(p, gridSize, &gridColSize);
    printf("%d\n", ans);

    return 0;
}
