#include "my_pthread_t.h"
#include "myallocate.h"
#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>

int* y;

void *test2(void *arg){
	//int x = 12;
	// int * x2 = malloc(sizeof(int));
	// *x2 = 12;
	// y = x2;
	printf("2\n");
	int *x = malloc(sizeof(int));
	*x = 12;
	while(1){}
}

void *test3(void *arg){
	printf("3\n");
  int *z = malloc(sizeof(int));
	*z = 18;
	while(1){}
}

my_pthread_mutex_t mutex;

void *test(void *arg){
	//ucontext_t * nthread = (ucontext_t*) malloc(sizeof(ucontext_t));
	//getcontext(nthread);
	//printf("Calling from stack %d\n", &nthread->uc_stack);

	printf("%s\n", (char*)arg);
	printf("FRANCISCO\n");
	my_pthread_t p2;
	my_pthread_create(&p2, NULL, test2, (void*)"A");
	printf("got to join\n" );
	my_pthread_join(p2, NULL);
	printf("End of Francisco");
	return NULL;
}

void *wait(void* arg){
	//my_pthread_mutex_t* mal;
	//my_pthread_mutex_init(mal, NULL);

	my_pthread_mutex_lock(&mutex);
	sleep(10);
	my_pthread_mutex_unlock(&mutex);
	return NULL;
}

int main(){
	//my_pthread_mutex_init(&mutex, NULL);
	// initialize();
	my_pthread_t p1, p3;
	printf("Main begin\n");

	my_pthread_create(&p1, NULL, test2, NULL);
	my_pthread_create(&p3, NULL, test3, NULL);
  printf("Got to join\n");
	my_pthread_join(p1, NULL);
	my_pthread_join(p3, NULL);
	printf("Value of y: %d\n", *y);
	printf("Main ends\n");

	return 0;
}
