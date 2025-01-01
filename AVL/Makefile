# $^ 代替 上面的指令
# RM 代替 rm -f
# $@ 代替 目标文件

OBJS=AVLTest.o AVLTree.o
CC=gcc
CFLAGS=-c -Wall -g

avl:$(OBJS)
	$(CC) $^ -o avl
AVLTest.o:AVLTest.c
	$(CC) $^ $(CFLAGS) -o $@
AVLTree.o:AVLTree.c
	$(CC) $^ $(CFLAGS) -o $@

clean:
	rm *.o avl

