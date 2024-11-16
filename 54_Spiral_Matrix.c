#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int direct[4][2]={{0,1},{1,0},{0,-1},{-1,0}};

int* spiralOrder(int** matrix, int matrixSize, int* matrixColSize, int* returnSize) {
	int i,total=matrixSize * (*matrixColSize);
    int* ans=(int *)malloc(total * sizeof(int));
    memset(ans, 0, total * sizeof(int));
    if(matrixSize==0||matrixColSize[0]==2){
    	*returnSize=0;
    	return NULL;
    }
    int visited[matrixSize][matrixColSize[0]];
    memset(visited, 0, sizeof(visited));
    int j=0, k=0, di=0, nextj, nextk;
    for(i=0;i<total;i++){
    	ans[i]=matrix[j][k];
    	visited[j][k]=1;
    	nextj=j+direct[di][0];
    	nextk=k+direct[di][1];
    	if(nextj<0 || nextj>=matrixSize || nextk<0 || nextk>=matrixColSize[0] || visited[nextj][nextk]){
    		di=(di+1)%4;
    	}
    	j+=direct[di][0];
    	k+=direct[di][1];
    }
    *returnSize=total;
    
    return ans;
}

int main(void){
    int nums[][4]={{1,2,3,4},{5,6,7,8},{9,10,11,12}}, i;
    int matrixSize=3, matrixColSize=4;
    int returnSize=matrixSize*matrixColSize;
    int *matrix[matrixSize];
    for(i=0;i<matrixSize;i++){
    	matrix[i]=nums[i];
    }
    
    int* ans = spiralOrder(matrix, matrixSize, &matrixColSize, &returnSize);
    for(i=0;i<returnSize;i++){
    	printf("%d ", ans[i]);
    }
    printf("\n");
    free(ans);
    ans=NULL;

    return 0;
}
