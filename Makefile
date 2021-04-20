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

JSD2: JSD2.o
	$(CC) $(CFLAGS) -o $@ $^ -lm -lpthread

JSD2.o: JSD2.c
	$(CC) -c $(CFLAGS) JSD2.c -lm -lpthread

clean:
	rm -f *.o WFD newCompare JSD JSD2
