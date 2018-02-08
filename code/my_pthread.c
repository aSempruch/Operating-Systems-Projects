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
ucontext_t create;

tcb* root;

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	ucontext_t * nthread = (ucontext_t*)(thread);
	getcontext(nthread);
	nthread->uc_link=0;
	nthread->uc_stack.ss_sp=malloc(MEM);
	nthread->uc_stack.ss_size=MEM;
	nthread->uc_stack.ss_flags=0;
	makecontext(nthread, function, 1, arg);

	/* Adding new thread to front of LL */
	tcb* new_thread = (tcb*)malloc(sizeof(tcb));
	new_thread->thread = nthread;
	new_thread->next = root;
	new_thread->prior = 1;
	root = new_thread;

	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	updatePrior(gettcb(), 1);
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
	ucontext_t * nthread = (ucontext_t*)(thread);
	getcontext(nthread);
	tcb* target = gettcb();
	free(nthread->uc_stack);
	if(value_ptr != NULL)
		(int)(*value_ptr) = 1;

	tcb* ptr = root;

	if(target == root){
		if(root->next != NULL)
			root = root->next;
		else
			root = NULL;
		free(target);
		return;
	}

	while(ptr->next != NULL){
		if(ptr->next == target){
			tcb* temp = ptr->next;
			ptr->next = ptr->next->next;
			free(temp);
			break;
		}
		ptr = ptr->next;
	}
	my_pthread_mutex_unlock(jmutex);
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	updatePrior(gettcb(), 1);
	my_pthread_mutex_t* jmutex;
	my_pthread_mutex_init(jmutex, NULL);
	jmutex->state = 1;
	tcb* ptr = root;
	while(root != NULL){
		if(*(ptr->thread) == thread)
			break;
		ptr = ptr->next;
	}
	jmutex->head = ptr;
	return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	//-1 means invalid pointer
	if(mutex == NULL)
		return -1;

	/*if(*mutex != NULL)
		return -2;*/

	mutex = (my_pthread_mutex_t*)malloc(sizeof(my_pthread_mutex_t));
	mutex->state = 0;
	mutex->attr = mutexattr;
	mutex->head = NULL;
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	if(mutex->state == 1){
		if(mutex->head == NULL){
			mutex->head = gettcb();
		}
		tcb* ptr = mutex->head;
		while(ptr->next != NULL)
			ptr = ptr->next;
		ptr->next = gettcb();
	}
	while(mutex->state == 1);
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	// -1 error code; mutex already unlocked
	if(mutex->state == 0)
		return -1;

	if(mutex->state == 1){
		if(mutex->head == gettcb())
			mutex->state = 0;
	}
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	free(mutex->attr);
	free(mutex);
	return 0;
};

/* Update Priority of thread and reorder linklist appropriately */
int updatePrior(tcb* thread, int prior){
	return 0;
}

tcb* gettcb(){
	ucontext_t* curr = (ucontext_t*) sizeof(ucontext_t);
	getcontext(curr);
	tcb* ptr = root;
	while(ptr != NULL){
		if(*(ptr->thread) == curr)
			return ptr;
		ptr = ptr->next;
	}
	return NULL;
}
