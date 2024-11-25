#include<stdio.h>
#include<stdlib.h>

struct ListNode {
    int val;
    struct ListNode *next;
};

int cmp(const void* a, const void* b){
    return(*(struct ListNode**)a)->val > (*(struct ListNode**)b)->val;
}

void sortList(struct ListNode* head){
    int cnt=0;
    struct ListNode* arr[50001]={NULL};
    while(head){
        arr[cnt++]=head;
        head=head->next;
    }
    qsort(arr, cnt, sizeof(struct ListNode*),cmp);
    for(int i=0;i<cnt;i++){
        arr[i]->next=arr[i+1];
    }
}


int main(void){
    struct ListNode* head=(struct ListNode*)malloc(sizeof(struct ListNode));
    head->val=-1;
    struct ListNode* p1=(struct ListNode*)malloc(sizeof(struct ListNode));
    p1->val=5;
    head->next=p1;
    struct ListNode* p2=(struct ListNode*)malloc(sizeof(struct ListNode));
    p2->val=3;
    p1->next=p2;
    struct ListNode* p3=(struct ListNode*)malloc(sizeof(struct ListNode));
    p3->val=4;
    p2->next=p3;
    struct ListNode* p4=(struct ListNode*)malloc(sizeof(struct ListNode));
    p4->val=0;
    p3->next=p4;
    p4->next=NULL;
    
    sortList(head);
    while(head){
    	printf("%d ", head->val);
    	head=head->next;
    }
    printf("\n");

    return 0;
}
