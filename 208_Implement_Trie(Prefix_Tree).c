#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>

#define ALPHABET_SIZE 26

typedef struct Node{
	struct Node* son[ALPHABET_SIZE];
	bool end;
}Node;

typedef struct{
	Node* root;
}Trie;

// 注意这里Node型对象的malloc需要通过calloc进行初始化
// 否则在判断p->son[c]是否为NULL时会报错

Trie* trieCreate(){
	Trie* obj=(Trie*)malloc(sizeof(Trie));
	obj->root=(Node*)calloc(1, sizeof(Node));
	return obj;
}

void trieInsert(Trie* obj, char* word){
	Node* p=obj->root;
	int i;
	for(i=0;word[i]!='\0';i++){
		int c=word[i]-'a';
		if(!p->son[c]){
			p->son[c]=(Node*)calloc(1, sizeof(Node));
		}
		p=p->son[c];
	}
	p->end=true;
}

int trieFind(Trie* obj, char* word){
	Node* p=obj->root;
	int i;
	for(i=0;word[i]!='\0';i++){
		int c=word[i]-'a';
		if(!p->son[c]) return 0;
		p=p->son[c];
	}
	return p->end ? 2:1;
}

bool trieSearch(Trie* obj, char* word){
	return trieFind(obj, word)==2;
}

bool trieStartsWith(Trie* obj, char* prefix){
	return trieFind(obj, prefix)!=0;
}

void trieFreeNode(Node* p){
	if(!p) return;
	int i;
	for(i=0;i<ALPHABET_SIZE;i++){
		trieFreeNode(p->son[i]);
	}
	free(p);
}

void trieFree(Trie* obj){
	trieFreeNode(obj->root);
	free(obj);
}

int main(void){
    Trie* trie=trieCreate();
    char* str="apple";
    
    trieInsert(trie, str);
    
    if(trieSearch(trie, str)) printf("true\n");
    else printf("false\n");
    
    str="app";
    if(trieSearch(trie, str)) printf("true\n");
    else printf("false\n");
    
    if(trieStartsWith(trie, str)) printf("true\n");
    else printf("false\n");
    
    trieInsert(trie, str);
    
    if(trieSearch(trie, str)) printf("true\n");
    else printf("false\n");

    return 0;
}
