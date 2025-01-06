#include<stdio.h>
#include<stdlib.h>
#include<math.h>

// 包含math.h时使用gcc编译器需要 -lm
// 因为数学函数位于libm.so库文件中（这些库文件通常位于/lib目录下），
// -lm选项告诉编译器，我们程序中用到的数学函数要到这个库文件里找。


int rob(int* nums, int numsSize) {
    int ans[101]={0};
    ans[0]=nums[0];
    if(numsSize==1) return ans[0];
    ans[1]=nums[0]>nums[1] ? nums[0]:nums[1];
    if(numsSize==2) return ans[1];
    for(int i=2;i<numsSize;i++){
        ans[i]=fmax(ans[i-1], ans[i-2]+nums[i]);
    }
    return ans[numsSize-1];
}

int main(void){
    int nums[5]={2,7,9,3,1};
    int numsSize=5;
    printf("%d\n", rob(nums, numsSize));

    return 0;
}
