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
#include<pthread.h>
#include<assert.h>
#include<signal.h>
#include<sys/time.h>
#define MEM 64000
ucontext_t create;

tcb* root;

// void initialize(){
// 	struct itimerval it;
//   struct sigaction act, oact;
//   act.sa_handler = sighandler;
//   sigemptyset(&act.sa_mask);
//   act.sa_flags = 0;
//
//   sigaction(SIGPROF, &act, &oact);
//   // Start itimer
//   it.it_interval.tv_sec = 4;
//   it.it_interval.tv_usec = 50000;
//   it.it_value.tv_sec = 1;
//   it.it_value.tv_usec = 100000;
//   setitimer(ITIMER_PROF, &it, NULL);
// }

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	sigset_t a,b;
	sigemptyset(&a);
	sigaddset(&a, SIGPROF);
	sigprocmask(SIG_BLOCK, &a, &b);
	short isFirst = 0;
	if(root == NULL)
		isFirst = 1;
	ucontext_t * nthread = (ucontext_t*) malloc(sizeof(ucontext_t));
	getcontext(nthread);
	void (*texit)(void*);
	texit = &my_pthread_exit;
	ucontext_t * exitCon = (ucontext_t*) malloc(sizeof(ucontext_t));
	getcontext(exitCon);
	exitCon->uc_link=0;
	exitCon->uc_stack.ss_sp=malloc(MEM/2);
	exitCon->uc_stack.ss_size=MEM/2;
	exitCon->uc_stack.ss_flags=0;
	makecontext(exitCon, texit, 1, NULL);

	nthread->uc_link=exitCon;
	nthread->uc_stack.ss_sp=malloc(MEM);
	nthread->uc_stack.ss_size=MEM;
	nthread->uc_stack.ss_flags=0;
	makecontext(nthread, function, 1, *((int*) arg));

	/* Adding new thread to front of LL */
	tcb* new_thread = (tcb*)malloc(sizeof(tcb));
	new_thread->thread = nthread;
	new_thread->tid = *thread;
	new_thread->next = root;
	new_thread->prior = 1;
	new_thread->joinQueue = NULL;
	root = new_thread;

	if(isFirst == 1){
		ucontext_t * first = (ucontext_t*) malloc(sizeof(ucontext_t));
		getcontext(first);
		tcb* first_thread = (tcb*)malloc(sizeof(tcb));
		first_thread->thread = nthread;
		first_thread->tid = *thread;
		first_thread->next = root;
		first_thread->prior = 1;
		updatePrior(first_thread, 1);
		first_thread->joinQueue = NULL;
		root = first_thread;
		startScheduler();
/*
		ucontext_t * scheduler = (ucontext_t*) malloc(sizeof(ucontext_t));
		getcontext(scheduler);
		scheduler->uc_link=0;
		scheduler->uc_stack.ss_sp=malloc(MEM);
		scheduler->uc_stack.ss_size=MEM;
		scheduler->uc_stack.ss_flags=0;
		makecontext(scheduler, &startScheduler, 0);
		setcontext(scheduler);
		*/
	}
	sigprocmask(SIG_SETMASK, &b, NULL);
	return 0;
}

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	updatePrior(root, 1);
	return 0;
}

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
	printf("MY PTHREAD EXIT BEGIN\n");
	sigset_t a,b;
	sigemptyset(&a);
	sigaddset(&a, SIGPROF);
	sigprocmask(SIG_BLOCK, &a, &b);


	free(root->thread->uc_stack.ss_sp);
	tcb* temp = root;
	removeFromQueue(root);
	free(temp->thread);
	free(temp);
/*
tcb* target = root;
tcb* queuePtr = target->joinQueue;
	tcb* ptr = root;
  tcb* temp;
	if(target == root){
		if(root->next != NULL)
			root = root->next;
		else
			root = NULL;
		while(queuePtr != NULL){
			temp = queuePtr->next;
			queuePtr->next = root;
			root = queuePtr;
			updatePrior(root, root->prior);
			*(queuePtr->joinArg) = value_ptr;
			queuePtr = temp;
		}
		free(target);

	}

	while(ptr->next != NULL){
		if(ptr->next == target){
			tcb* temp = ptr->next;
			ptr->next = ptr->next->next;
			while(queuePtr != NULL){
				temp = queuePtr->next;
				queuePtr->next = root;
				root = queuePtr;
				updatePrior(root, root->prior);
				*(queuePtr->joinArg) = value_ptr;
				queuePtr = temp;
			}
			free(temp);
			break;
		}
		ptr = ptr->next;
	}
	*/
	sigprocmask(SIG_SETMASK, &b, NULL);
	printf("MY PTHREAD EXIT ENDS, SETTING CONTEXT TO ROOT");
	setcontext(root->thread);
	return;
}

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	sigset_t a,b;
	sigemptyset(&a);
	sigaddset(&a, SIGPROF);
	sigprocmask(SIG_BLOCK, &a, &b);
	tcb* threadPtr = root;
	threadPtr->joinArg = value_ptr;
	tcb* ptr = root;
	while(root != NULL){
		if((ptr->tid) == thread)
			break;
		ptr = ptr->next;
	}
	if(ptr->joinQueue == NULL){
		ptr->joinQueue = threadPtr;
		removeFromQueue(threadPtr);
	}
	else{
		removeFromQueue(threadPtr);
		tcb* queuePtr = ptr->joinQueue;
		while(queuePtr->next != NULL){
			queuePtr = queuePtr->next;
		}
		queuePtr->next =threadPtr;
	}
	sigprocmask(SIG_SETMASK, &b, NULL);
	setcontext(root->thread);
	return 0;
}

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
}

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	tcb* thread = root;
	if(mutex->state == 1){
		removeFromQueue(thread);
		if(mutex->head == NULL){
			mutex->head = thread;
		}
		tcb* ptr = mutex->head;
		while(ptr->next != NULL)
			ptr = ptr->next;
		ptr->next = thread;
	}
	while(mutex->state == 1);
	return 0;
}

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	// -1 error code; mutex already unlocked
	tcb* ptr = mutex->head;
	if(mutex->state == 0)
		return -1;

	if(mutex->state == 1){
		//if(mutex->head == root)
			mutex->state = 0;
	}
	mutex->head = mutex->head->next;
	ptr->next = root;
	root = ptr;
	updatePrior(root, root->prior);
	return 0;
}

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	my_pthread_mutex_unlock(mutex);
	free(mutex->attr);
	free(mutex);
	return 0;
}

tcb* getParent(tcb* thread){
	tcb* ptr = root;
	if(thread == root){
		return thread;
	}
	while(ptr->next != NULL && ptr->next != thread)
		ptr = ptr->next;
	return ptr;
}

/* Update Priority of thread and reorder LinkedList appropriately */
int updatePrior2(tcb* thread, int prior){
	tcb* ptr = root;
	while(ptr != NULL && thread != ptr) //Loop through queue
		ptr = ptr->next;
	if(ptr == NULL) //Could not find tcb
		return -1;
	ptr->prior = prior;
	while(1){ //Until order is correct
		tcb* parent = getParent(ptr);
		if(ptr->next->prior > prior){ //If tcb behind has higher priority than ptr
			tcb* temp = ptr->next;
			ptr->next = ptr->next->next;
			if(parent == root)
				root = ptr;
			else
				parent->next = temp;
			temp->next = ptr;
		}
		else if(parent->prior < prior){ //If tcb in front has lower priority than ptr
			tcb* temp = ptr->next;
			if(parent == root){
				ptr->next = root;
				root->next = temp;
				root = ptr;
			}
			else{
				getParent(parent)->next = ptr;
				ptr->next = parent;
				parent->next = temp;
			}
		}
		else
			break;
	}
	return 0;
}

int updatePrior(tcb* thread, int prior){
	tcb* ptr = root;
	tcb* parent;
	while(ptr != NULL && thread != ptr){ //Loop through queue
		parent = ptr;
		ptr = ptr->next;
	}
	if(ptr == NULL) //Could not find tcb
		return -1;
	ptr->prior = prior;

	tcb* curr = root;
	tcb* prev = curr;

	if(root == ptr){ //If root prior is changed
		if(root->next != NULL && root->next->prior < root->prior){ //If root priority is increased do nothing
			return 0;
		} else if(root->next != NULL && root->next->prior > root->prior){ //If root priority is decreased move to proper location and change root pointer
			root = root->next;
			curr = root;
			prev = ptr;
			while(curr != NULL){
				if(curr->prior < ptr->prior){
					prev->next = ptr;
					ptr->next = curr;
				} else {
					prev = curr;
					curr = curr->next;
				}
			}
		}
	}

	if(root->prior < ptr->prior){ //If ptr has highest prior now
		parent->next = ptr->next;
		ptr->next = root;
		root = ptr;
	}

	while(curr != NULL){
		if(curr != ptr && curr->prior < ptr->prior){
			parent->next = ptr->next;
			prev->next = ptr;
			ptr->next = curr;
		} else {
			prev = curr;
			curr = curr->next;
		}
	}

	return 0;
}
/*
tcb* gettcb{
	ucontext_t* curr = (ucontext_t*) malloc(sizeof(ucontext_t));
	printf("Get context return value: %d\n", getcontext(curr));
	tcb* ptr = root;

	while(ptr != NULL){
		if((ptr->thread->uc_stack.ss_sp) == (curr->uc_stack.ss_sp))
			return ptr;
		ptr = ptr->next;
	}
	printf("Returning NULL in root\n");
	return NULL;
}
*/
int removeFromQueue(tcb* thread){
	if(thread == root){
		root = root->next;
		return 0;
	}
	tcb* ptr = root;
	while(ptr->next != NULL){
		if(ptr->next == thread){
			ptr->next = ptr->next->next;
			return 0;
		}
		ptr = ptr->next;
	}
	return -1;
}

// void *test2(void *arg){
// 	printf("MAMANCISCO SUCKS\n");
// 	return NULL;
// }
//
// void *test(void *arg){
// 	printf("BRANCISCO SUCKS\n");
// 	my_pthread_t p2;
// 	my_pthread_create(&p2, NULL, test2, (void*)"A");
// 	my_pthread_join(p2, NULL);
// 	return NULL;
// }

int count = 0;
void sighandler(int sig, siginfo_t *si, void *old_context){
 printf("signal occurred %d times\n",sig, ++count);
 //getcontext(root->thread);
 //printf("Looping\n");
 root->thread = (ucontext_t*) old_context;
 updatePrior(root, root->prior + 10);
 //lastRan = lastRan->next;
 //if(lastRan == NULL)
 //	lastRan = root;

 setcontext(root->thread);
}

void startScheduler(){

	struct itimerval it;
  struct sigaction act, oact;
  act.sa_sigaction = sighandler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  sigaction(SIGPROF, &act, &oact);
  // Start itimer
  it.it_interval.tv_sec = 4;
  it.it_interval.tv_usec = 50000;
  it.it_value.tv_sec = 1;
  it.it_value.tv_usec = 100000;
  setitimer(ITIMER_PROF, &it, NULL);
	setcontext(root->thread);
}

// int main(){
// 	// initialize();
// 	my_pthread_t p1;
// 	printf("Main begin\n");
//
// 	my_pthread_create(&p1, NULL, test, (void*)"A");
// 	my_pthread_join(p1, NULL);
//
// 	printf("Main ends\n");
//
// 	return 0;
// }
