CC = gcc
CFLAGS = -g -std=c99 -Wvla -Wall -fsanitize=address,undefined

WFD: WFD.o
	$(CC) $(CFLAGS) -o $@ $^

WFD.o: WFD.c
	$(CC) -c $(CFLAGS) WFD.c


clean:
	rm -f *.o WFD
