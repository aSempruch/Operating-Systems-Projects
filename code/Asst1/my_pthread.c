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
ucontext_t * exitCon;
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	sigset_t a,b;
	sigemptyset(&a);
	sigaddset(&a, SIGPROF);
	sigprocmask(SIG_BLOCK, &a, &b);
	short isFirst = 0;
	if(root == NULL){
		isFirst = 1;
		void (*texit)(void*);
		texit = &my_pthread_exit;
		exitCon = (ucontext_t*) malloc(sizeof(ucontext_t));
		getcontext(exitCon);
		exitCon->uc_link=0;
		exitCon->uc_stack.ss_sp=malloc(MEM/2);
		exitCon->uc_stack.ss_size=MEM/2;
		exitCon->uc_stack.ss_flags=0;
		makecontext(exitCon, texit, 1, NULL);
	}
	ucontext_t * nthread = (ucontext_t*) malloc(sizeof(ucontext_t));
	getcontext(nthread);


	nthread->uc_link=exitCon;
	nthread->uc_stack.ss_sp=malloc(MEM);
	nthread->uc_stack.ss_size=MEM;
	nthread->uc_stack.ss_flags=0;
	makecontext(nthread, function, 1, (int) arg);

	/* Adding new thread to front of LL */
	tcb* new_thread = (tcb*)malloc(sizeof(tcb));
	new_thread->thread = nthread;
	new_thread->tid = (my_pthread_t)thread;
	*thread = (my_pthread_t)thread;
	new_thread->next = root;
	new_thread->prior = 1;
	new_thread->joinQueue = NULL;
	new_thread->time = 0;
	root = new_thread;
	if(root->next != NULL){
		tcb* swap = root->next;
		new_thread->next = swap->next;
		swap->next = new_thread;
		root = swap;
	}
	if(isFirst == 1){
		ucontext_t * first = (ucontext_t*) malloc(sizeof(ucontext_t));
		//getcontext(first);
		first->uc_stack.ss_flags = 0;
		first->uc_link = 0;
		tcb* first_thread = (tcb*)malloc(sizeof(tcb));
		my_pthread_t *mainTid = (my_pthread_t*)malloc(sizeof(my_pthread_t));
		first_thread->thread = first;
		first_thread->tid = (my_pthread_t)mainTid;
		*mainTid = (my_pthread_t)mainTid;
		first_thread->next = root;
		first_thread->prior = 1;
		first_thread->time = 0;
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
	sigset_t a,b;
	sigemptyset(&a);
	sigaddset(&a, SIGPROF);
	sigprocmask(SIG_BLOCK, &a, &b);

	tcb* exitThread = root;
	tcb* joinQueuePtr = root->joinQueue;
	tcb* tempQueue;
	while(joinQueuePtr != NULL){
		tempQueue = joinQueuePtr->next;
		if(joinQueuePtr->joinArg != NULL)
			*(joinQueuePtr->joinArg) = value_ptr;
		joinQueuePtr->next = root;
		root = joinQueuePtr;
		updatePrior(root, root->prior);
		joinQueuePtr = tempQueue;
	}


	free(exitThread->thread->uc_stack.ss_sp);
	tcb* temp = exitThread;
	removeFromQueue(exitThread);
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
	*/
/*
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
	while(ptr != NULL){
		if((ptr->tid) == thread)
			break;
		ptr = ptr->next;
	}
	if(ptr == NULL){
		return;
	}
	if(ptr->joinQueue == 0){
		ptr->joinQueue = threadPtr;
		removeFromQueue(threadPtr);
		threadPtr->next = NULL;
	}
	else{
		removeFromQueue(threadPtr);
		tcb* queuePtr = ptr->joinQueue;
		while(queuePtr->next != NULL){
			queuePtr = queuePtr->next;
		}
		queuePtr->next =threadPtr;
		threadPtr->next = NULL;
	}
	sigprocmask(SIG_SETMASK, &b, NULL);
	// printf("Swapping context in join\n");
	swapcontext(threadPtr->thread, root->thread);
	return 0;
}

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	sigset_t a,b;
	sigemptyset(&a);
	sigaddset(&a, SIGPROF);
	sigprocmask(SIG_BLOCK, &a, &b);
	char* mal = (char*) malloc(sizeof(char) * 5);
	my_pthread_mutex_t* test = (my_pthread_mutex_t*)malloc(sizeof(my_pthread_mutex_t));

	//-1 means invalid pointer
	//if(mutex == NULL)
	//	return -1;

	/*if(*mutex != NULL)
		return -2;*/

	mutex = test;
	mutex->state = 0;
	mutex->attr = mutexattr;
	mutex->head = NULL;
	sigprocmask(SIG_SETMASK, &b, NULL);
	return 0;
}

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	sigset_t a,b;
	sigemptyset(&a);
	sigaddset(&a, SIGPROF);
	sigprocmask(SIG_BLOCK, &a, &b);

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
	sigprocmask(SIG_SETMASK, &b, NULL);
	return 0;
}

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	sigset_t a,b;
	sigemptyset(&a);
	sigaddset(&a, SIGPROF);
	sigprocmask(SIG_BLOCK, &a, &b);

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
	sigprocmask(SIG_SETMASK, &b, NULL);
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
 // printf("signal occurred %d times\n",sig, ++count);
 root->time++;
 tcb* temp = root;
 //getcontext(root->thread);
 //printf("Looping\n");
 //root->thread = (ucontext_t*) old_context;
 updatePrior(root, root->prior - 10);
 //lastRan = lastRan->next;
 //if(lastRan == NULL)
 //	lastRan = root;
 //printf("Swapping context in scheduler\n");
 swapcontext(temp->thread, root->thread);
}

void startScheduler(){
	//printf("Using our library\n");
	struct itimerval it;
  struct sigaction act, oact;
  act.sa_sigaction = sighandler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  sigaction(SIGPROF, &act, &oact);
  // Start itimer
  it.it_interval.tv_sec = 0;
  it.it_interval.tv_usec = 2500;
  it.it_value.tv_sec = 1;
  it.it_value.tv_usec = 100000;
  setitimer(ITIMER_PROF, &it, NULL);
	//getcontext(root->thread);
	//setcontext(root->next->thread);
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
