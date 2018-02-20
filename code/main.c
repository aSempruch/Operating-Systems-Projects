#include "my_pthread_t.h"
#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>

void *test2(void *arg){
	printf("MAMANCISCO SUCKS\n");
	return NULL;
}

my_pthread_mutex_t mutex;

void *test(void *arg){
	//ucontext_t * nthread = (ucontext_t*) malloc(sizeof(ucontext_t));
	//getcontext(nthread);
	//printf("Calling from stack %d\n", &nthread->uc_stack);

	printf("%s\n", (char*)arg);
	printf("BRANCISCO SUCKS\n");
	my_pthread_t p2;
	my_pthread_create(&p2, NULL, test2, (void*)"A");
	printf("got to join\n" );
	my_pthread_join(p2, NULL);
	printf("End of Brancisco");
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
	my_pthread_mutex_init(&mutex, NULL);
	// initialize();
	my_pthread_t p1, p3;
	printf("Main begin\n");

	my_pthread_create(&p1, NULL, wait, NULL);
	my_pthread_create(&p3, NULL, wait, NULL);
  printf("Got to join\n");
	my_pthread_join(p1, NULL);
	my_pthread_join(p3, NULL);

	printf("Main ends\n");

	return 0;
}
