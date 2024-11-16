#include<stdio.h>
#include<math.h>

int jump(int* nums, int numsSize) {
	int i, j=0, tmp=0, res=0;
	for(i=0;i<numsSize-1;i++){
		tmp = fmax(i+nums[i], tmp);
		if(i==j){
			j=tmp;
			res++;
		}
	}
	return res;
}

int main(void){
	int nums[5]={2,3,1,1,4}, res;
	res = jump(nums, 5);
	printf("%d\n", res);

    return 0;
}
