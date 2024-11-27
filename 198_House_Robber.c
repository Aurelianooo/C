#include<stdio.h>
#include<stdlib.h>
#include<math.h>

// 包含math.h时使用gcc编译器需要 -lm
// 因为数学函数位于libm.so库文件中（这些库文件通常位于/lib目录下），
// -lm选项告诉编译器，我们程序中用到的数学函数要到这个库文件里找。

int rob(int* nums, int numsSize) {
    if(numsSize==1) return nums[0];
    else if(numsSize==2) return nums[0]>nums[1] ? nums[0]:nums[1];
    else if(numsSize==3) return nums[0]+nums[2]>nums[1]? nums[0]+nums[2]:nums[1];
    int* ans=(int*)malloc(numsSize*sizeof(int));
    int i;
    for(i=0;i<numsSize;i++) ans[i]=0;
    ans[0]=nums[0];
    ans[1]=nums[0]>nums[1] ? nums[0]:nums[1];
    ans[2]=nums[0]+nums[2]>nums[1]? nums[0]+nums[2]:nums[1];
    for(i=3;i<numsSize;i++){
        int tmp=fmax(ans[i-3]+nums[i-1], ans[i-2]+nums[i]);
        tmp=fmax(tmp, ans[i-3]+nums[i]);
        ans[i]=tmp;
    }

    return ans[numsSize-1];
}

int main(void){
    int nums[5]={2,7,9,3,1};
    int numsSize=5;
    printf("%d\n", rob(nums, numsSize));

    return 0;
}
