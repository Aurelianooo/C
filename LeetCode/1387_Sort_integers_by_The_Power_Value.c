#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int cmp(const void* a, const void* b){
    int* c=*(int**)a;
    int* d=*(int**)b;

    if(c[1]==d[1]){
        return c[0]-d[0];
    }else{
        return c[1]-d[1];
    }
}

int recursive(int* record, int i){
    if(i==1) return 0;
    if(record[i]!=-1) return record[i];
    if(i%2==0) return recursive(record, i/2)+1;
    else return recursive(record, i*3+1)+1;
}

int getKth(int lo, int hi, int k) {
    int* record=(int*)malloc(399999*sizeof(int));
    memset(record, -1, 399999*sizeof(int));
    int i;
    for(i=1;i<1001;i++)
        record[i]=recursive(record, i);
    int len=hi-lo+1;
    int** ans=(int**)malloc(len*sizeof(int*));
    for(i=0;i<len;i++){
        int* t=(int*)malloc(2*sizeof(int));
        t[0]=i+lo;
        t[1]=record[i+lo];
        ans[i]=t;
    }
    qsort(ans, len, sizeof(int*), cmp);
    
    return ans[k-1][0];
}

int main(void){
	int ans=getKth(1,1000,777);
    printf("%d\n", ans);

    return 0;
}
