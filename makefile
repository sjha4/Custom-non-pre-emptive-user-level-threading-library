all:
	gcc -c -o mythread.o mythread.c
	ar rcs mythread.a mythread.o
clean:
	rm -f ./*


