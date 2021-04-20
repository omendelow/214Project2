CC = gcc
CFLAGS = -g -std=c99 -Wvla -Wall -fsanitize=address,undefined

WFD: WFD.o
	$(CC) $(CFLAGS) -o $@ $^

WFD.o: WFD.c
	$(CC) -c $(CFLAGS) WFD.c

newCompare: newCompare.o
	$(CC) $(CFLAGS) -o $@ $^ -lm -lpthread

newCompare.o: newCompare.c
	$(CC) -c $(CFLAGS) newCompare.c -lm -lpthread

JSD: JSD.o
	$(CC) $(CFLAGS) -o $@ $^ -lm -lpthread

JSD.o: JSD.c
	$(CC) -c $(CFLAGS) JSD.c -lm -lpthread


clean:
	rm -f *.o WFD newCompare JSD
