// name:
// username of iLab:
// iLab Server:

#include "myallocate.h"
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<signal.h>
#include<sys/time.h>
#include <sys/mman.h>
#include "my_pthread_t.h"

int init = 0;
int malloc_init = 0;
int shalloc_init = 0;

 static void seghandler(int sig, siginfo_t *si, void *unused){
   unsigned long address = *((unsigned long*)(si->si_addr));
   printf("Address: %lu\n", address);
   return;
 }

int initialize(){
  printf("Running init\n");
  int context_size = 64000 + sizeof(tcb) + sizeof(ucontext_t);
  page_directory page_dir;
  posix_memalign((void**)&mem, PAGE_SIZE, MEM_SIZE * sizeof(char));

  int i;
  for(i = 0; i < NUM_PAGES; i++){
    page_entry page;
    page.start_index = i*PAGE_SIZE;
    page.available = 1;
    page.head = NULL;
    page.owner = NULL;
   page_dir.pages[i] = page;
  }
  memcpy(mem, &page_dir, sizeof(page_dir));
  p_dir = (page_directory*)mem;

  //context table
  context_directory context_dir;

  for(i = 0; i < NUM_CONTEXTS;i++){
    context_entry context;
    context.start = &mem[CONTEXT_START* PAGE_SIZE+ sizeof(context_directory)+i*context_size];
    context.available = 1;
    context.owner = NULL;
    context_dir.contexts[i] = context;
  }
//  memcpy(&mem[CONTEXT_START * PAGE_SIZE], &context_dir, sizeof(context_directory));
//  c_dir = (context_directory*)(&mem[CONTEXT_START*PAGE_SIZE]);
  //Segfault handler

  struct sigaction seg;
  seg.sa_sigaction = seghandler;
  sigemptyset(&seg.sa_mask);
  seg.sa_flags = SA_SIGINFO;

  if (sigaction(SIGSEGV, &seg, NULL) == -1){
    printf("Fatal error setting up signal handler\n");
    exit(EXIT_FAILURE);    //explode!
  }

  init = 1;

}

int requestPage(){
  int i;
  for(i = USER_PAGE_START; i < SHALLOC_PAGE_START; i++){
    if(p_dir->pages[i].available == 1){
      p_dir->pages[i].available = 0;
      return i;
    }
  }
  //All pages occupied
  return -1;
}

int getCurrentPage(void* ptr){ //Given a pointer return what page it's located in
  int displacement = (int)ptr - (int)mem;
  return displacement/PAGE_SIZE;
}

void* myallocate(unsigned int size, char* file, unsigned int line, int threadreq){

  if(!init){
    initialize();
  }

  if(threadreq == 0){ //poop
      if(size == sizeof(my_pthread_mutex_t)){

      }
      int i;
      for(i = 0; i < NUM_CONTEXTS; i++){
        if(c_dir->contexts[i].available == 1){
          break;
        }
      }
      if(i == 0){
        if(size == sizeof(ucontext_t)){
          return (c_dir->contexts[i].start);
        }
        if(size == 64000/2){
          c_dir->contexts[i].available = 0;
          return (c_dir->contexts[i].start + sizeof(ucontext_t));
        }
      }
      if(i == 2){
        if(size == sizeof(ucontext_t)){
          return (c_dir->contexts[i].start);
        }
        if(size == sizeof(tcb)){
          return (c_dir->contexts[i].start + sizeof(ucontext_t));
        }
        if(size == sizeof(my_pthread_t)){
          c_dir->contexts[i].available = 0;
          return (c_dir->contexts[i].start + sizeof(ucontext_t) + sizeof(tcb));
        }
      }
      if(size == sizeof(ucontext_t)){
        return (c_dir->contexts[i].start);
      }
      if(size == sizeof(tcb)){
        return (c_dir->contexts[i].start + sizeof(ucontext_t));
      }
      if(size == 64000){
        c_dir->contexts[i].available = 0;
        return (c_dir->contexts[i].start + sizeof(ucontext_t) + sizeof(tcb));
      }

  }

  if(size == 0)
    return NULL;

  static mem_entry* head;
  mem_entry* temp;
  mem_entry* next;

  if(!malloc_init){
    int i = requestPage();
    head = (mem_entry*)&mem[PAGE_SIZE*USER_PAGE_START]; //points to beginning of user space in mem
    head->next = NULL;
    head->prev = NULL;
    head->available = 1;
    head->size = PAGE_SIZE - sizeof(mem_entry);
    malloc_init = 1;
    p_dir->pages[USER_PAGE_START].head = head;
  }

  temp = head;

  while(temp != NULL){
    if(temp->available != 1 || temp->size < size){
      temp = temp->next;
    }
    else if(temp->size < size+sizeof(mem_entry)){    //Last block case; No space left after
      int p_num = requestPage();
      if(p_num == -1){ //No more pages available
        temp->available = 0;
        return NULL;
      }

      int curr_page = getCurrentPage((void*)temp);
      if(curr_page != p_num+1){ //Pages are not next to each other, gotta swap pages
        //SWAP THEM PAGES SO THEY'RE CONTINGUOUS
      } else {
        p_dir->pages[p_num].head = head;
        temp->size += PAGE_SIZE;
        continue;
      }
      //Should add space to page and to mem_entry, make continguous.
      //What if page given not next to curr page?
      return (char*)temp + sizeof(mem_entry); //why?
    }
    else{
      next = (mem_entry*)((char*)temp+sizeof(mem_entry)+size);    //Create header for next block
      next->available = 1;
      next->prev = temp;
      next->next = temp->next;
      next->size = temp->size - sizeof(mem_entry) - size;
      next->available = 1;

      if(temp->next != NULL)
        temp->next->prev = next;

      temp->next = next;
      temp->size = size;
      temp->available = 0;
      return(char*)temp+sizeof(mem_entry);
    }
  }
}

int mydeallocate(void* item, char* file, unsigned int line, int threadreq){
  if(item == NULL){
    return -1; //Free failed; cannot free null pointer
  }

  mem_entry* head = p_dir->pages[1001].head;
  mem_entry* curr = head;
  mem_entry* temp;
  mem_entry* next;
  mem_entry* prev;
  temp = (mem_entry*)((char*)item - sizeof(mem_entry));
  if(temp->available == 1){
    return -1; //Free failed; pointer has not been malloc'ed or has already been freed
  }

  int i;
  int isvalid = 0;

  while(curr != NULL){
    if(curr == temp && temp->available == 0){
      isvalid = 1;
    }
    curr = curr->next;
  }

  if(!isvalid){
    return -1; //Free failed; Pointer is not allocated
  }

  prev = temp->prev;
  next = temp->next;

  if(prev != NULL && prev->available == 1){
    prev->size = prev->size + sizeof(mem_entry) + temp->size;    // Current block is freed; prev is available, previous block becomes bigger
    prev->next = temp->next;
    if(temp->next != NULL){
      temp->next->prev = prev;
    }
  }
  else{
    temp->available = 1;
    prev = temp;
  }

  if(next != NULL && next->available == 1){ //Current block is freed, next is available, previous block becomes bigger.
    prev->size = prev->size + sizeof(mem_entry) + next->size;
    prev->next = next->next;
    if(next->next != NULL){
      next->next->prev = prev;
    }
  }

  return 0;
}

void* shalloc(size_t size){
  if(!init){
    initialize();
  }

  if(size == 0)
    return NULL;

  static mem_entry* shalloc_head;
  mem_entry* temp;
  mem_entry* next;

  if(!shalloc_init){
    int i;
    for(i = SHALLOC_PAGE_START; i < NUM_PAGES+1; i++){
      p_dir->pages[i].available = 0;
    }

    shalloc_head = (mem_entry*)&mem[PAGE_SIZE*SHALLOC_PAGE_START]; //points to beginning of shalloc space in mem
    shalloc_head->next = NULL;
    shalloc_head->prev = NULL;
    shalloc_head->available = 1;
    shalloc_head->size = PAGE_SIZE*4 - sizeof(mem_entry);
    shalloc_init = 1;
    p_dir->pages[SHALLOC_PAGE_START].head = shalloc_head;
  }

  temp = shalloc_head;

  while(temp != NULL){
    if(temp->available != 1 || temp->size < size){
      temp = temp->next;
    }
    else if(temp->size < size+sizeof(mem_entry)){    //Last block case; No space left after
      temp->available = 0;
      return NULL;
    }
    else{
      next = (mem_entry*)((char*)temp+sizeof(mem_entry)+size);    //Create header for next block
      next->available = 1;
      next->prev = temp;
      next->next = temp->next;
      next->size = temp->size - sizeof(mem_entry) - size;
      next->available = 1;

      if(temp->next != NULL)
        temp->next->prev = next;

      temp->next = next;
      temp->size = size;
      temp->available = 0;
      return(char*)temp+sizeof(mem_entry);
    }
  }
}

//Unprotect previous context, protect next context
void swapMem(tcb* prev, tcb* next){
  int i;
  for(i = 0; i < NUM_PAGES; i++){
    if(p_dir->pages[i].owner == prev){
      mprotect(&mem[i*PAGE_SIZE], PAGE_SIZE, PROT_NONE);
    }
    else if(p_dir->pages[i].owner == next){
      mprotect(&mem[i*PAGE_SIZE], PAGE_SIZE,  PROT_READ | PROT_WRITE);
    }
  }
}

int main(){
  //Check if malloc works.
  printf("If malloc works, should print 12\n");
  int* y = (int*) shalloc (sizeof(int));
  *y = 12;
  printf("%d\n",*y);

  //Check if malloc works
  printf("If malloc works, should print 1002\n");
  int* z = (int*) shalloc (sizeof(int));
  *z = 1002;
  printf("%d\n",*z);

  //Check if malloc works; Try to malloc(0)
  printf("Attempting to malloc 0 bytes\n");
  malloc(0);


  //free of pointer offset
  printf("Attempting to free an offset of a pointer; offset is not allocated\n");
  char* c;
  c = (char *)shalloc( 200 );
  free(c+10);

  //free of something not malloced.
  printf("Attempting to free a pointer that was not malloced.\n");
  int x = 100;
  free(&x);


  //attempt to free the same pointer twice
  printf("Attempting to free same pointer twice.\n");
  free(y);
  free(y);


  //Below tests should work
  printf("Subsequent actions allowed. No errors returned until saturation test.\n");
  c = (char *)shalloc( 100 );
  free( c );
  c = (char *)shalloc( 100 );
  free( c );

  //Freeing everything
  free(z);

  //Attempt to saturate the memory for small blocks
  printf("Attempting to saturate memory.\n");
  int increment;
  for(increment = 0; increment < 5000; increment = increment + sizeof(int)){
    if(increment == 548){
      printf("Reached point where should be saturated\n");
    }

    int* s = (int*) shalloc(sizeof(int));

    if(s == NULL){
      printf("Memory saturated, returned null pointer!\n");
      break;
    }
  }

  //Tests complete
  printf("Tests complete.\n");
  return 0;
}

/*
In file included from myallocate.c:5:0:
myallocate.h:64:1: error: unknown type name ‘mutex_directory’
 mutex_directory *m_dir;
 ^
*/
