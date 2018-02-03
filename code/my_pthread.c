// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"
#include<stdio.h>
#include<stdlib.h>
#include<ucontext.h>
#define MEM 64000
ucontext_t main;
ucontext_t create;

tcb* root;

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
  ucontext_t * nthread = (ucontext_t*)(thread);
	getcontext(&main);
	getcontext(nthread);
  nthread->uc_link=0;
  nthread->uc_stack.ss_sp=malloc(MEM);
  nthread->uc_stack.ss_size=MEM;
  nthread->uc_stack.ss_flags=0;
  makecontext(nthread, function, 1, arg);

	/* Adding new thread to front of LL */
	tcb* new_thread = (tcb*)malloc(sizeof(tcb));
	new_thread->thread = thread;
	new_thread->next = root;
	new_thread->prior = 1;
	root->prev = new_thread;
	root = new_thread;

	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	return 0;
};

/* Update Priority of thread and reorder linklist appropriatley */
int updatePrior(tcb* thread, int prior){
	return 0;
}
