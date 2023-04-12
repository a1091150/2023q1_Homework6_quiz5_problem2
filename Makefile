all:
	gcc main.c xalloc.c rbtree.c

clean:
	rm *.out
	rm *.gch