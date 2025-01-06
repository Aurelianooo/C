#include<stdio.h>
#include<stdlib.h>

typedef struct MinStack{
	int data;
	struct MinStack *next;
}MinStack;

MinStack* minStackCreate(){
	MinStack* newStack=(MinStack *)malloc(sizeof(MinStack));
	newStack->data=0;
	newStack->next=NULL;
	return newStack;
}

void minStackPush(MinStack* obj, int val){
	MinStack* newStack=(MinStack *)malloc(sizeof(MinStack));
	newStack->data=val;
	newStack->next=obj->next;
	obj->next=newStack;
}

// pop、top 和 getMin 操作总是在 非空栈 上调用
// 因此代码没有空栈判断
void minStackPop(MinStack* obj){
	if(obj->next!=NULL)
		obj->next=obj->next->next;
}

int minStackTop(MinStack* obj){
	return obj->next->data;
}

int minStackGetMin(MinStack* obj){
	MinStack* p=obj->next;
	int minVal=p->data;
	while(p!=NULL){
		if(p->data<minVal)	minVal=p->data;
		p=p->next;
	}
	return minVal;
}

void minStackFree(MinStack* obj){
	MinStack* p=obj->next;
	MinStack* q=p;
	if(p!=NULL){
		p=p->next;
		free(q);
		q=p;
	}
}


int main(void){
    MinStack* minStack = minStackCreate();
    int res;
	minStackPush(minStack, -2);
	minStackPush(minStack, 0);
	minStackPush(minStack, -3);
	res = minStackGetMin(minStack); // return -3
	printf("%d\n", res);
	minStackPop(minStack);
	res = minStackTop(minStack);    // return 0
	printf("%d\n", res);
	res = minStackGetMin(minStack); // return -2
	printf("%d\n", res);
	minStackFree(minStack);

    return 0;
}
