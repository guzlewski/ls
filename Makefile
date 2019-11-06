ls.out: ls.c
	gcc -Wall -pedantic ls.c -o ls.out

clean:
	rm ls.out
