#include <stdio.h>

// 在C99标准以上，数组为局部变量的话，大小是可以定义为变量的，否则，大小不能定义为变量。
// 例如，在函数中定义int b[a]是可以的。
// 但是要是定义成全局数组，或者在函数内将其定义成static int b[a]，都无法编译通过。

void merge(int *nums1, int m, int *nums2, int n)
{
	int p1=0;
	int p2=0;
	int tmp[m+n];
	int t,i;
	
	while(p1<m || p2<n){
		if(p1==m){
			t = nums2[p2++];
		}else if(p2==n){
			t = nums1[p1++];
		}else if(nums1[p1]<=nums2[p2]){
			t = nums1[p1++];
		}else{
			t = nums2[p2++];
		}
		
		tmp[p1+p2-1] = t;
	}
	for(i=0;i<m+n;i++){
		nums1[i] = tmp[i];
	}
}

int main(void)
{
    int nums1[6] = {1, 2, 3, 0, 0, 0};
    int nums2[3] = {2, 5, 6};
    merge(nums1, 3, nums2, 3);
    for (int i = 0; i < 6; i++)
    {
        printf("%d ", nums1[i]);
    }
    printf("\n");
    return 0;
}
