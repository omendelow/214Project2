CC = gcc
CFLAGS = -g -std=c99 -Wvla -Wall -fsanitize=address,undefined

compare: compare.o
	$(CC) $(CFLAGS) -o $@ $^ -lm -lpthread

compare.o: compare.c
	$(CC) -c $(CFLAGS) compare.c -lm -lpthread

clean:
	rm -f *.o compare
