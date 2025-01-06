#include<stdio.h>
#include<string.h>

void setZeroes(int** matrix, int matrixSize, int* matrixColSize) {
    int i=0,j=0;
    int fi[matrixSize];
    int fj[*matrixColSize];
    memset(fi, 0, sizeof(fi));
    memset(fj, 0, sizeof(fj));
    for(i=0;i<matrixSize;i++){
    	for(j=0;j<*matrixColSize;j++){
    		if(!matrix[i][j]){
    			fi[i]=1;
    			fj[j]=1;
    		}
    	}
    }
    
    for(i=0;i<matrixSize;i++){
    	for(j=0;j<*matrixColSize;j++){
    		if(fi[i]||fj[j]){
    			matrix[i][j]=0;
    		}
    	}
    }
    
}

int main(void){
    int matrix[][4]={{0,1,2,0},{3,4,5,2},{1,3,1,5}};
    int matrixSize=3, matrixColSize=4, i, j;
    int *p[matrixColSize];
    for(i=0;i<matrixSize;i++){
    	p[i]=matrix[i];
    }
    setZeroes(p, matrixSize, &matrixColSize);
    for(i=0;i<matrixSize;i++){
    	for(j=0;j<matrixColSize;j++){
    		printf("%d ", matrix[i][j]);
    	}
    	printf("\n");
    }

    return 0;
}

