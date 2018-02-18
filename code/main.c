#include "my_pthread_t.h"
#include <stdio.h>

void *test2(void *arg){
	printf("MAMANCISCO SUCKS\n");
	return NULL;
}

void *test(void *arg){
	printf("BRANCISCO SUCKS\n");
	my_pthread_t p2;
	//my_pthread_create(&p2, NULL, test2, (void*)"A");
	//printf("got to join\n" );
	//my_pthread_join(p2, NULL);
	return NULL;
}

int main(){
	// initialize();
	my_pthread_t p1;
	printf("Main begin\n");

	my_pthread_create(&p1, NULL, test, (void*)"A");
  printf("Got to join\n");
	my_pthread_join(p1, NULL);

	printf("Main ends\n");

	return 0;
}
