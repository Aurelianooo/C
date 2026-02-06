#include<stdio.h>

void rotate(int** matrix, int matrixSize, int* matrixColSize) {
    int l=0,r=matrixSize-1;
    int i,j, tmp;
    while(l<r){
    	for(i=0;i<matrixSize;i++){
    		tmp=matrix[i][l];
    		matrix[i][l]=matrix[i][r];
    		matrix[i][r]=tmp;
    	}
    	l++;r--;
    }
    for(i=0;i<matrixSize-1;i++){
    	for(j=0;j<matrixSize-1-i;j++){
    		tmp=matrix[i][j];
    		matrix[i][j]=matrix[matrixSize-1-j][matrixSize-1-i];
    		matrix[matrixSize-1-j][matrixSize-1-i]=tmp;
    	}
    }
}

int main(void){
    int matrix[4][4]={{5,1,9,11},{2,4,8,10},{13,3,6,7},{15,14,12,16}};
    int matrixSize=4, matrixColSize=4;
    int *p[matrixSize], i,j;
    for(i=0;i<matrixSize;i++){
    	p[i]=matrix[i];
    }
    rotate(p, matrixSize, &matrixColSize);
    for(i=0;i<matrixSize;i++){
    	for(j=0;j<matrixColSize;j++){
    		printf("%d ", matrix[i][j]);
    	}
    	printf("\n");
    }

    return 0;
}
