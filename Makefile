all:
	gcc -c -Wall -g -O0 event.c
	gcc -c -Wall -g -O0 event_test.c
	gcc -o test event.o event_test.o

clean:
	rm -f *.o test
