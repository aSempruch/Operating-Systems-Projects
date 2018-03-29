#include <stdio.h>
#include <stdlib.h>
#include "my_pthread_t.h"
#include "myallocate.h"

void testFunc(int rank);
int num_bytes;
int num_calls;
int verbose;

int main(int argc, char* argv[]){
	if(argc < 4){
		printf("Wrong number of agruments dumbey\n");
		exit(0);
	}
	int num_threads = atoi(argv[1]);
	num_bytes = atoi(argv[2]);
	num_calls = atoi(argv[3]);
	if(argc > 4 && *(argv[4]) == 'v')
		verbose = 1;
	else
		verbose = 0;
	printf("Number of threads: %d\n", num_threads);
	printf("Allocation size: %d\n", num_bytes);

	my_pthread_t* pthreads;

	int k;
	for(k = 0; k < num_threads; k++)
		my_pthread_create(pthreads+k, NULL, testFunc, k);

	for(k = 0; k < num_threads; k++)
		my_pthread_join(*(pthreads+k), NULL);
}

void testFunc(int rank){
	if(verbose == 1)
			printf("Thread %d is mallocing %d bytes\n", rank, num_bytes);
	int k, y;
	char** obj = malloc(num_calls);
	for(k = 0; k < num_calls; k++)
		*(obj+k) = malloc(num_bytes);
	if(verbose == 1)
			printf("Thread %d malloc succesful\n", rank);
	if(verbose == 1)
			printf("Thread %d filling up object\n", rank);
	for(y = 0; y < num_calls; y++)
		for(k = 0; k < num_bytes; k++)
				*(*(obj+y)+k) = 'X';

	for(k = 0; k < num_calls; k++)
		free(*(obj+k));
	if(verbose == 1)
			printf("Thread %d free succesful\n", rank);
}
