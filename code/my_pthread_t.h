// File:	my_pthread_t.h
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server:
#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

typedef uint my_pthread_t;

typedef struct my_pthread_mutex_t my_pthread_mutex_t;

typedef struct threadControlBlock {
  ucontext_t* thread;
  my_pthread_t tid;
  struct threadControlBlock* next;
  int prior; //Priority, the higher the number the higher the priority
  struct threadControlBlock* joinQueue;
  void** joinArg;
} tcb;

/* mutex struct definition */
struct my_pthread_mutex_t {
  int state; //0 is unlocked, 1 is locked
  tcb* head;
  pthread_mutexattr_t *attr;
};

/* define your data structures here: */

// Feel free to add your own auxiliary data structures


/* Function Declarations: */

tcb* gettcb();
int updatePrior(tcb* thread, int prior);
int removeFromQueue(tcb* thread);
int updatePrior2(tcb* thread, int prior);
tcb* getParent(tcb* thread);
void startScheduler();

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield();

/* terminate a thread */
void my_pthread_exit(void *value_ptr);

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr);

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

#endif

/*my_pthread_t.h:85:24: error: conflicting types for ‘my_pthread_create’
 #define pthread_create my_pthread_create
                        ^
my_pthread_t.h:56:5: note: previous declaration of ‘my_pthread_create’ was here
 int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);*/
