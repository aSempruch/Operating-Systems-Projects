// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"
#include "myallocate.h"
#include<stdio.h>
#include<stdlib.h>
#include<ucontext.h>
#include<pthread.h>
#include<assert.h>
#include<signal.h>
#include<sys/time.h>
#define MEM 64000
ucontext_t create;

/* create a new thread */
ucontext_t * exitCon;
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	sigset_t a,b;
	sigemptyset(&a);
	sigaddset(&a, SIGALRM);
	sigprocmask(SIG_BLOCK, &a, &b);
	short isFirst = 0;
	if(root == NULL){
		isFirst = 1;
		void (*texit)(void*);
		texit = &my_pthread_exit;
		exitCon = (ucontext_t*) myallocate(sizeof(ucontext_t), __FILE__,__LINE__, 0);
		// exitCon = (ucontext_t*) malloc(sizeof(ucontext_t));
		getcontext(exitCon);
		exitCon->uc_link=0;
		exitCon->uc_stack.ss_sp=myallocate(MEM/2, __FILE__,__LINE__,0);
		// exitCon->uc_stack.ss_sp=malloc(MEM/2);
		exitCon->uc_stack.ss_size=MEM/2;
		exitCon->uc_stack.ss_flags=0;
		makecontext(exitCon, texit, 1, NULL);
	}

	ucontext_t * nthread = (ucontext_t*)myallocate(sizeof(ucontext_t), __FILE__,__LINE__, 0);
	// ucontext_t * nthread = (ucontext_t*)malloc(sizeof(ucontext_t));
	getcontext(nthread);


	nthread->uc_link=exitCon;
	nthread->uc_stack.ss_sp=myallocate(MEM, __FILE__,__LINE__,0);
	// nthread->uc_stack.ss_sp=malloc(MEM);
	nthread->uc_stack.ss_size=MEM;
	nthread->uc_stack.ss_flags=0;
	nthread->uc_sigmask = b;
	makecontext(nthread, function, 1, (int) arg);

	/* Adding new thread to front of LL */
	tcb* new_thread = (tcb*)myallocate(sizeof(tcb), __FILE__,__LINE__,0);
	// tcb* new_thread = (tcb*)malloc(sizeof(tcb));
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
		ucontext_t * first = (ucontext_t*) myallocate(sizeof(ucontext_t), __FILE__,__LINE__,0);
		// ucontext_t * first = (ucontext_t*) malloc(sizeof(ucontext_t));
		//getcontext(first);
		first->uc_stack.ss_flags = 0;
		first->uc_link = 0;
		tcb* first_thread = (tcb*)myallocate(sizeof(tcb), __FILE__,__LINE__,0);
		// tcb* first_thread = (tcb*)malloc(sizeof(tcb));
		my_pthread_t *mainTid = (my_pthread_t*)myallocate(sizeof(my_pthread_t), __FILE__, __LINE__, 0);
		// my_pthread_t *mainTid = (my_pthread_t*)malloc(sizeof(my_pthread_t));
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

	}
	sigemptyset(&b);
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
	sigaddset(&a, SIGALRM);
	sigprocmask(SIG_BLOCK, &a, &b);

	// //make context space available in memory
	// int i;
	// for(i=0; i < NUM_CONTEXTS; i++){
	// 	if(c_dir->contexts[i].owner == root){
	// 		c_dir->contexts[i].available = 1;
	// 		break;
	// 	}
	// }

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

	sigprocmask(SIG_SETMASK, &b, NULL);
	setcontext(root->thread);
	return;
}

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	sigset_t a,b;
	sigemptyset(&a);
	sigaddset(&a, SIGALRM);
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
		sigemptyset(&b);
		sigprocmask(SIG_SETMASK, &b, NULL);
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
	sigemptyset(&b);
	sigprocmask(SIG_SETMASK, &b, NULL);
	// printf("Swapping context in join\n");
	swapMem(threadPtr, root);
	swapcontext(threadPtr->thread, root->thread);
	return 0;
}

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	sigset_t a,b;
	sigemptyset(&a);
	sigaddset(&a, SIGALRM);
	sigprocmask(SIG_BLOCK, &a, &b);
	my_pthread_mutex_t* test = (my_pthread_mutex_t*)myallocate(sizeof(my_pthread_mutex_t), __FILE__, __LINE__,0);
	// my_pthread_mutex_t* test = (my_pthread_mutex_t*)malloc(sizeof(my_pthread_mutex_t));



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
	sigaddset(&a, SIGALRM);
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
	sigprocmask(SIG_SETMASK, &b, NULL);
	while(mutex->state == 1);
	mutex->state = 1;
	return 0;
}

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	sigset_t a,b;
	sigemptyset(&a);
	sigaddset(&a, SIGALRM);
	sigprocmask(SIG_BLOCK, &a, &b);

	// -1 error code; mutex already unlocked
	tcb* ptr = mutex->head;
	if(mutex->state == 0){
		sigprocmask(SIG_SETMASK, &b, NULL);
		return -1;
	}

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
			while(curr != NULL){ //This is stupid -___- fix this
				if(curr->prior < ptr->prior){
					prev->next = ptr;
					ptr->next = curr;
					return 0;
				} else {
					prev = curr;
					curr = curr->next;
				}
			}
			//If I get to end
			prev->next = ptr;
			ptr->next = NULL;
			return 0;
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
 swapMem(temp, root);
 swapcontext(temp->thread, root->thread);
}

void startScheduler(){
	//printf("Using our library\n");
	struct itimerval it;
  struct sigaction act, oact;
  act.sa_sigaction = sighandler;
	// act.sa_handler = sighandler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  sigaction(SIGALRM, &act, &oact);


  // Start itimer
  it.it_interval.tv_sec = 0;
  it.it_interval.tv_usec = 2500;
  it.it_value.tv_sec = 1;
  it.it_value.tv_usec = 100000;
  setitimer(ITIMER_REAL, &it, NULL);
	// it.it_interval.tv_sec = 0;
  // it.it_interval.tv_usec = 50000;
  // it.it_value.tv_sec = 0;
  // it.it_value.tv_usec = 0;
  // setitimer(ITIMER_REAL, &it, NULL);
	//
	// signal(SIGALRM, sighandler);
	//getcontext(root->thread);
	//setcontext(root->next->thread);
}
