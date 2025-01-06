#include<stdio.h>
#include<stdbool.h>

bool canFinish(int numCourses, int** prerequisites, int prerequisitesSize, int* prerequisitesColSize) {
	if(!prerequisitesSize) return true;
	int map[numCourses], i, j, cnt=0;
	for(i=0;i<numCourses;i++) map[i]=0;
	for(i=0;i<prerequisitesSize;i++)
		map[prerequisites[i][0]]++; //初始化每个点的入度（拓扑图）
	i=0;
	while(i<numCourses){
        if(map[i]==0){
			cnt++;
			if(cnt==numCourses) return true; //拓扑可以结束
			// 更新拓扑图
			for(j=0;j<prerequisitesSize;j++){
				if(prerequisites[j][1]==i)
					map[prerequisites[j][0]]--;
			}
            map[i]=-1;
            i=0;
		}else{
            i++;
        }
	}
	return false;
}

int main(void){
    int prerequisites[2][2]={{1,0},{0,1}};
    int numCourses=2, prerequisitesSize=2, prerequisitesColSize=2, i;
    int *p[prerequisitesSize];
    for(i=0;i<prerequisitesSize;i++) p[i]=prerequisites[i];
    if(canFinish(numCourses, p, prerequisitesSize, &prerequisitesColSize))
    	printf("true\n");
    else printf("false\n");

    return 0;
}
