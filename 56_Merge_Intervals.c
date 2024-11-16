#include<stdio.h>
#include<stdlib.h>

int cmp(const void *a, const void * b)
{
    int* c = *(int **)a;
    int* d = *(int **)b;
    return c[0] - d[0]; // 如果是第二个元素排序就是c[1]与d[1]
}


int** merge(int** intervals, int intervalsSize, int* intervalsColSize, int* returnSize, int** returnColumnSizes) {
	qsort(intervals, intervalsSize, sizeof(int*), cmp);
    int** ans=(int **)malloc(intervalsSize * sizeof(int *));
    *returnSize=0;
    
    int i=0,l,r,tmp;
    while(i<intervalsSize){
    	l=i;
    	tmp=intervals[l][1];
    	while(i<intervalsSize && intervals[i][0]<=tmp){
    		if(intervals[i][1]>tmp) tmp=intervals[i][1];
    		i++;
    	}
    	r=i-1;
    	int* inter=(int *)malloc(2 * sizeof(int));
    	inter[0]=intervals[l][0];
    	if(intervals[r][1]<tmp) inter[1]=tmp;
    	else inter[1]=intervals[r][1];
    	ans[(*returnSize)++]=inter;
    }
    (*returnColumnSizes)=(int*)malloc((*returnSize)*sizeof(int));
    for(i=0;i<(*returnSize);i++){
    	(*returnColumnSizes)[i]=2;
    }
    
    return ans;
}

int main(void){
    int intervals[4][2]={{1,3},{2,6},{8,10},{15,18}};
    int intervalsSize=4, intervalsColSize=2;
    int* p[intervalsSize], i;
    for(i=0;i<intervalsSize;i++){
    	p[i]=intervals[i];
    }
    int returnSize, *returnColumnSizes, **ans;
    ans=merge(p, intervalsSize, &intervalsColSize, &returnSize, &returnColumnSizes);
    for(i=0;i<returnSize;i++){
    	printf("%d %d\n", ans[i][0], ans[i][1]);
    }

    return 0;
}
