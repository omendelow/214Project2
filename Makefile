CC = gcc
CFLAGS = -g -std=c99 -Wvla -Wall -fsanitize=address,undefined

WFD: WFD.o
	$(CC) $(CFLAGS) -o $@ $^

WFD.o: WFD.c
	$(CC) -c $(CFLAGS) WFD.c

newCompare: newCompare.o
	$(CC) $(CFLAGS) -o $@ $^

newCompare.o: newCompare.c
	$(CC) -c $(CFLAGS) newCompare.c

clean:
	rm -f *.o WFD newCompare
