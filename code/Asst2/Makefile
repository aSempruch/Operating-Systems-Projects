HEADERS = my_pthread_t.h myallocate.h

default: test

test: my_pthread.c myallocate.c
	gcc -g -w -pthread -o test test.c -lm my_pthread.c myallocate.c

extreme: my_pthread.c myallocate.c
	gcc -g -w -pthread -o extremetest extremetest.c -lm my_pthread.c myallocate.c
