CFLAGS = -Wall -g -O0

all:
	gcc ${CFLAGS} -I./include -c event.c
	gcc ${CFLAGS} -I./include -c select.c
	gcc ${CFLAGS} -I./include -I. -c test/event_test.c
	gcc -o evtest event.o select.o event_test.o

clean:
	rm -f *.o evtest *.fifo
