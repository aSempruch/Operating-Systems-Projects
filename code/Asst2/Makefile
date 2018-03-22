HEADERS = my_pthread_t.h myallocate.h

default: test

test: my_pthread.c myallocate.c
	gcc -g -w -pthread -o test test.c -lm my_pthread.c myallocate.c
