#include<stdio.h>
#include<stdlib.h>

// 输入一些区间，把这些区间画在数轴上。在数轴上最少要放置多少个点，使得每个区间都包含至少一个点？
// 把区间按照右端点从小到大排序，这样第一个点就放在第一个区间的右端点处。
// 去掉包含第一个点的区间后，第二个点就放在剩余区间的第一个区间的右端点处。依此类推。


int cmp(const void* a, const void* b){
	int* c=*(int **)a;
	int* d=*(int **)b;
	return (c[1]>d[1])-(c[1]<d[1]); // 防止数值过大越界int
}

int findMinArrowShots(int** points, int pointsSize, int* pointsColSize) {
    qsort(points, pointsSize, sizeof(int *), cmp);
    int ans=1;
    int i, pre=points[0][1];
    for(i=1;i<pointsSize;i++){
        if(pre<points[i][0]){
            ans++;
            pre=points[i][1];
        }
    }
    
    return ans;
}

int main(void){
    int points[4][2]={{10,16},{2,8},{1,6},{7,12}};
    int pointsSize=4, pointsColSize=2;
    int *p[pointsSize], i, ans;
    for(i=0;i<pointsSize;i++){
    	p[i]=points[i];
    }
    ans=findMinArrowShots(p, pointsSize, &pointsColSize);
    printf("%d\n", ans);

    return 0;
}
