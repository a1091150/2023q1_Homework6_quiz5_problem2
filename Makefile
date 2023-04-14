all:
	gcc main.c xalloc.c rbtree.c heap.c

clean:
	rm *.out
	rm *.gch