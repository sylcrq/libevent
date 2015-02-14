CFLAGS = -Wall -g -O0

all:
	gcc ${CFLAGS} -I./include -I. -c event.c
	gcc ${CFLAGS} -c select.c
	gcc ${CFLAGS} -c test/event_test.c
	gcc -o evtest event.o event_test.o

clean:
	rm -f *.o evtest
