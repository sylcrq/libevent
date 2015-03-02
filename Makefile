CFLAGS = -Wall -g -O0

all:
	gcc ${CFLAGS} -I./include -c event.c
	gcc ${CFLAGS} -I./include -c select.c
	ar rcs ./lib/libevent.a event.o select.o

event_test:
	gcc ${CFLAGS} -I. -I./include -o evtest test/event_test.c -L./lib -levent

time_test:
	gcc ${CFLAGS} -I. -I./include -o tmtest test/time_test.c -L./lib -levent

clean:
	rm -f *.o *.fifo ./lib/*.a
	rm -f evtest tmtest
