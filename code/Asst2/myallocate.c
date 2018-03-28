// name:
// username of iLab:
// iLab Server:

#include "myallocate.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/mman.h>
#include "my_pthread_t.h"
#include <fcntl.h>
#include <string.h>

int init = 0;
int malloc_init = 0;
int shalloc_init = 0;
int did_segfault = 0;
int swap;

 static void seghandler(int sig, siginfo_t *si, void *unused){
   printf("Got SIGSEGV at address: 0x%lx\n",(void*)si->si_addr);
   void* page_ptr_cause;
   int page_fault_num = getCurrentPage((void*)si->si_addr);
   printf("Page that it segaulted %d\n", page_fault_num);
   page_ptr_cause = (void*)&mem[page_fault_num*PAGE_SIZE];
   movePagesToFront(root, page_fault_num);
   return;
 }

/* Creates swap file, initailizes page directories, and enables our signal handler */
int initialize(){
  // Initializes the virtual memory data structure
  printf("Running init\n");
  createSwap();
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
    page.place = 0;
    page_dir.pages[i] = page;
  }
  memcpy(mem, &page_dir, sizeof(page_dir));
  p_dir = (page_directory*)mem;

  //context table
  context_directory context_dir;

  for(i = 0; i < NUM_CONTEXTS;i++){
    context_entry context;
    context.start = &mem[CONTEXT_START* PAGE_SIZE+sizeof(context_directory)+i*context_size];
    context.available = 1;
    context.owner = NULL;
    context_dir.contexts[i] = context;
  }
  memcpy(&mem[CONTEXT_START * PAGE_SIZE], &context_dir, sizeof(context_directory));
  c_dir = (context_directory*)(&mem[CONTEXT_START*PAGE_SIZE]);

  //mutex table
  mutex_directory mutex_dir;

  for(i = 0; i < NUM_MUTEXES;i++){
    mutex_entry mutex;
    mutex.available = 1;
    mutex.owner = NULL;
    mutex_dir.mutexes[i] = mutex;
  }
  memcpy(&mem[MUTEX_START * PAGE_SIZE], &mutex_dir, sizeof(mutex_directory));
  m_dir = (mutex_directory*)(&mem[MUTEX_START*PAGE_SIZE]);
  //Segfault handler

  struct sigaction seg;
  seg.sa_flags = SA_SIGINFO;
  sigemptyset(&seg.sa_mask);
  seg.sa_sigaction = seghandler;

  if (sigaction(SIGSEGV, &seg, NULL) == -1){
    printf("Fatal error setting up signal handler\n");
    exit(EXIT_FAILURE);    //explode!
  }

  init = 1;
}

/* Creates empty swap file for later use */
void createSwap(){
  swap = open("swapfile", O_RDWR | O_CREAT);
  lseek(swap, (MEM_SIZE * 2 - 1), SEEK_SET);
  write(swap, "", 1);
  lseek(swap, 0, SEEK_SET);
}

/* Returns index of the first available page in our page directory */
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

/* Returns index of the first available page in our swap file */
int requestSwap(){
  int i;
  for(i = REAL_PAGES; i< NUM_PAGES; i++){
    if(p_dir->pages[i].available == 1){
      p_dir->pages[i].available = 0;
      break;
    }
  }
  if(i = NUM_PAGES){
    return -1;
  }

  return i;
}

/* Returns index of page in which ptr points to */
int getCurrentPage(void* ptr){ //Given a pointer return what page it's located in
  long displacement = (char*)ptr - (char*)mem;
  return displacement/PAGE_SIZE;
}

/* Sets mem entry pointers in new page*/
void setMemEntryPtrs(int old_page, int new_page){
  if(old_page < USER_PAGE_START || old_page > NUM_PAGES || new_page < USER_PAGE_START || new_page > NUM_PAGES){
    return;
  }

  if(p_dir->pages[old_page].head == NULL){
    return;
  }

  p_dir->pages[new_page].head = (mem_entry*)&mem[new_page*PAGE_SIZE];
  mem_entry* curr = p_dir->pages[new_page].head;

  while(curr->next != NULL){
    char* addr = (char*)curr+curr->size+sizeof(mem_entry);
    curr->next = (mem_entry*)addr;
    curr = curr->next;
  }
}

/* Swaps page that is full with empty page */
void swapEmptyPage(int old_page, int new_page){
  void* old_page_ptr = (void*)&mem[old_page*PAGE_SIZE];
  void* new_page_ptr = (void*)&mem[new_page*PAGE_SIZE];
  memcpy(new_page_ptr, old_page_ptr, PAGE_SIZE);
  setMemEntryPtrs(old_page, new_page);
  p_dir->pages[new_page].owner = p_dir->pages[old_page].owner;
  memset(old_page_ptr, 0 , PAGE_SIZE);
  p_dir->pages[old_page].available = 1;
}

/* Swaps two pages that are both taken */
void swapPage(int old_page, int new_page){
  void* old_page_ptr = (void*)&mem[old_page*PAGE_SIZE];
  void* new_page_ptr = (void*)&mem[new_page*PAGE_SIZE];
  void* temp_page_ptr = (void*)&mem[TEMP_PAGE*PAGE_SIZE];
  memcpy(temp_page_ptr, old_page_ptr, PAGE_SIZE); //old into temp
  setMemEntryPtrs(old_page, TEMP_PAGE);
  p_dir->pages[TEMP_PAGE].owner = p_dir->pages[old_page].owner;

  memcpy(old_page_ptr, new_page_ptr, PAGE_SIZE); //new into old
  setMemEntryPtrs(new_page, old_page);
  p_dir->pages[old_page].owner = p_dir->pages[new_page].owner;

  memcpy(new_page_ptr, temp_page_ptr, PAGE_SIZE); //temp into new
  setMemEntryPtrs(TEMP_PAGE, new_page);
  p_dir->pages[new_page].owner = p_dir->pages[TEMP_PAGE].owner;



}
void swapPageFile(int old_page, int new_page){
  lseek(swap, (new_page*PAGE_SIZE - MEM_SIZE), SEEK_SET);
  void* old_page_ptr = (void*)&mem[old_page*PAGE_SIZE];
  void* temp_page_ptr = (void*)&mem[TEMP_PAGE*PAGE_SIZE];

  read(swap, temp_page_ptr, PAGE_SIZE);
  //  p_dir->pages[new_page].head = p_dir->pages[old_page].head;
  //  p_dir->pages[new_page].owner = p_dir->pages[old_page].owner;
  lseek(swap, (new_page*PAGE_SIZE - MEM_SIZE), SEEK_SET);
  write(swap, old_page_ptr, PAGE_SIZE);
  //  p_dir->pages[new_page].head = p_dir->pages[old_page].head;
  //  p_dir->pages[new_page].owner = p_dir->pages[old_page].owner;
  memcpy(temp_page_ptr, old_page_ptr, PAGE_SIZE);
  lseek(swap, 0, SEEK_SET);
  memset(old_page_ptr, 0 , PAGE_SIZE);

}
//Look through page table to find it's pages in regular memory, or in the swap file. If both are full then no more pages can be given out
void movePagesToFront(tcb* curr_thread, int page_fault_num){
  int foundPageForCurr = 0;

  int i;
  for(i = USER_PAGE_START; i < NUM_PAGES; i++){
    if(i >= SHALLOC_PAGE_START && i < 2049){ //These are shalloc pages; Leave them alone!
      continue;
    }
    if(p_dir->pages[i].owner == curr_thread){ //Found page belonging to current thread
       foundPageForCurr = 1;
       mprotect(&mem[i*PAGE_SIZE], PAGE_SIZE,  PROT_READ | PROT_WRITE);
       mprotect(&mem[page_fault_num*PAGE_SIZE], PAGE_SIZE,  PROT_READ | PROT_WRITE);
       /*
       In reality this should actually get the page place that corresponds to the page that faulted.
       So if the page that faulted was 1003 I should get the 3rd page place. For now testing when
       each thread has one page each.
       */
       swapPage(page_fault_num, i);
       mprotect(&mem[i*PAGE_SIZE], PAGE_SIZE, PROT_NONE);
    }
  }

  if(!foundPageForCurr){ //Couldn't find page for current thread, must be new thread
    malloc_init = 0;
    did_segfault = 1;
    int free_page_num = requestPage();
    mprotect(&mem[free_page_num*PAGE_SIZE], PAGE_SIZE,  PROT_READ | PROT_WRITE);
    mprotect(&mem[page_fault_num*PAGE_SIZE], PAGE_SIZE,  PROT_READ | PROT_WRITE);
    swapEmptyPage(page_fault_num, free_page_num);
    mprotect(&mem[free_page_num*PAGE_SIZE], PAGE_SIZE, PROT_NONE);
  }

}

void swapEmptyFile(int old_page, int new_page){
  lseek(swap, (new_page*PAGE_SIZE - MEM_SIZE), SEEK_SET);
  void* old_page_ptr = (void*)&mem[old_page*PAGE_SIZE];
  write(swap, old_page_ptr, PAGE_SIZE);
  p_dir->pages[new_page].head = p_dir->pages[old_page].head;
  p_dir->pages[new_page].owner = p_dir->pages[old_page].owner;

  lseek(swap, 0, SEEK_SET);
  memset(old_page_ptr, 0 , PAGE_SIZE);


}

void* myallocate(unsigned int size, char* file, unsigned int line, int threadreq){
  sigset_t a,b;
  if(threadreq != 0){
  	sigemptyset(&a);
  	sigaddset(&a, SIGPROF);
  	sigprocmask(SIG_BLOCK, &a, &b);
  }
  if(!init){
    initialize();
  }

  if(threadreq == 0){
	int i;
      	if(size == sizeof(my_pthread_mutex_t)){

      		for(i = 0; i < NUM_MUTEXES; i++){
       	 		if(m_dir->mutexes[i].available == 1){
         			 break;
        		}
     		}
		m_dir->mutexes[i].available = 0;
    sigprocmask(SIG_SETMASK, &b, NULL);
		return (&mem[MUTEX_START * PAGE_SIZE + sizeof(mutex_directory) + i*sizeof( my_pthread_mutex_t)]);
	}
      for(i = 0; i < NUM_CONTEXTS; i++){
        if(c_dir->contexts[i].available == 1){
          break;
        }
      }
      if(i == 0){
        if(size == sizeof(ucontext_t)){
          sigprocmask(SIG_SETMASK, &b, NULL);
          return (c_dir->contexts[i].start);
        }
        if(size == 64000/2){
          c_dir->contexts[i].available = 0;
          sigprocmask(SIG_SETMASK, &b, NULL);
          return (c_dir->contexts[i].start + sizeof(ucontext_t));
        }
      }
      if(i == 2){
        if(size == sizeof(ucontext_t)){
          sigprocmask(SIG_SETMASK, &b, NULL);
          return (c_dir->contexts[i].start);
        }
        if(size == sizeof(tcb)){
         sigprocmask(SIG_SETMASK, &b, NULL);
          return (c_dir->contexts[i].start + sizeof(ucontext_t));
        }
        if(size == sizeof(my_pthread_t)){
          c_dir->contexts[i].available = 0;
         sigprocmask(SIG_SETMASK, &b, NULL);
          return (c_dir->contexts[i].start + sizeof(ucontext_t) + sizeof(tcb));
        }
      }
      if(size == sizeof(ucontext_t)){
        sigprocmask(SIG_SETMASK, &b, NULL);
        return (c_dir->contexts[i].start);
      }
      if(size == sizeof(tcb)){
	      c_dir->contexts[i].available = 0;
       sigprocmask(SIG_SETMASK, &b, NULL);
        return (c_dir->contexts[i].start + sizeof(ucontext_t));
      }
      if(size == 64000){
        sigprocmask(SIG_SETMASK, &b, NULL);
        return (c_dir->contexts[i].start + sizeof(ucontext_t) + sizeof(tcb));
      }

  }

  if(size == 0){
    sigprocmask(SIG_SETMASK, &b, NULL);
    return NULL;
  }

  static mem_entry* head;
  mem_entry* temp;
  mem_entry* next;

  if(head != NULL){ //This will trigger a segfault if page is mprotected
    if(did_segfault || head->size == 0){
      did_segfault = 0;
    }
  }

  if(!malloc_init){ //This should actually happen to each thread! Must fix
    head = (mem_entry*)&mem[PAGE_SIZE*USER_PAGE_START]; //points to beginning of user space in mem
    head->next = NULL;
    head->prev = NULL;
    head->available = 1;
    head->size = PAGE_SIZE - sizeof(mem_entry);
    malloc_init = 1;
    p_dir->pages[USER_PAGE_START].head = head;
    p_dir->pages[USER_PAGE_START].owner = root;
    p_dir->pages[USER_PAGE_START].available = 0;
  }

  temp = head;
  int curr_page = getCurrentPage((void*)temp);
  while(temp != NULL){
    if(temp->available != 1 || temp->size < size){
      temp = temp->next;
    }
    else if(temp->size < size+sizeof(mem_entry)){    //Last block case; No space left after
      int p_num = requestPage();
      if(p_num == -1){ //No more pages available
        int p_num = requestSwap();
        if(p_num == -1){
          temp->available = 0;
          sigprocmask(SIG_SETMASK, &b, NULL);
          return NULL;
        }
        else{

            mprotect(&mem[(curr_page+1)*PAGE_SIZE], PAGE_SIZE,  PROT_READ | PROT_WRITE);
            swapEmptyFile(curr_page+1, p_num);
            //mprotect(&mem[curr_page+1*PAGE_SIZE], PAGE_SIZE,  PROT_NONE);
            p_num = curr_page+1;
        }
      }


      if(curr_page != (p_num-1)){ //Pages are not next to each other, gotta swap pages
        mprotect(&mem[(curr_page+1)*PAGE_SIZE], PAGE_SIZE,  PROT_READ | PROT_WRITE);
        mprotect(&mem[p_num*PAGE_SIZE], PAGE_SIZE,  PROT_READ | PROT_WRITE);
        swapEmptyPage(curr_page+1, p_num);
        mprotect(&mem[p_num*PAGE_SIZE], PAGE_SIZE, PROT_NONE);
      }
      p_dir->pages[curr_page+1].head = head;
      p_dir->pages[curr_page+1].owner = p_dir->pages[curr_page].owner;
      p_dir->pages[curr_page+1].place = p_dir->pages[curr_page].place+1;
      temp->size += PAGE_SIZE;
      sigprocmask(SIG_SETMASK, &b, NULL);
      continue;
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
      sigprocmask(SIG_SETMASK, &b, NULL);
      return(char*)temp+sizeof(mem_entry);
    }
  }
  sigprocmask(SIG_SETMASK, &b, NULL);
}

int mydeallocate(void* item, char* file, unsigned int line, int threadreq){
  sigset_t a,b;
	sigemptyset(&a);
  sigaddset(&a, SIGPROF);
  sigprocmask(SIG_BLOCK, &a, &b);
  if(item == NULL){
    sigprocmask(SIG_SETMASK, &b, NULL);
    return -1; //Free failed; cannot free null pointer
  }

  mem_entry* head = p_dir->pages[1001].head;
  mem_entry* curr = head;
  mem_entry* temp;
  mem_entry* next;
  mem_entry* prev;
  temp = (mem_entry*)((char*)item - sizeof(mem_entry));
  if(temp->available == 1){
    sigprocmask(SIG_SETMASK, &b, NULL);
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
    sigprocmask(SIG_SETMASK, &b, NULL);
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
  sigprocmask(SIG_SETMASK, &b, NULL);
  return 0;
}

void* shalloc(size_t size){
  //sigset_t a,b;
	//sigemptyset(&a);
	//sigaddset(&a, SIGPROF);
	//sigprocmask(SIG_BLOCK, &a, &b);
  if(!init){
    initialize();
  }

  if(size == 0){
    //sigprocmask(SIG_SETMASK, &b, NULL);
    return NULL;
  }

  static mem_entry* shalloc_head;
  mem_entry* temp;
  mem_entry* next;

  if(!shalloc_init){
    int i;
    for(i = SHALLOC_PAGE_START; i < SHALLOC_PAGE_START + 4; i++){
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
      //sigprocmask(SIG_SETMASK, &b, NULL);
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
      //  sigprocmask(SIG_SETMASK, &b, NULL);
      return(char*)temp+sizeof(mem_entry);
    }
  }
  //  sigprocmask(SIG_SETMASK, &b, NULL);
}

//Unprotect previous context, protect next context
void swapMem(tcb* prev, tcb* next){
  sigset_t a,b;
	sigemptyset(&a);
	sigaddset(&a, SIGPROF);
	sigprocmask(SIG_BLOCK, &a, &b);

  if(prev == next){
    return;
  }

  int i;
  for(i = USER_PAGE_START; i < SHALLOC_PAGE_START; i++){
    if(p_dir->pages[i].owner == prev){
      mprotect(&mem[i*PAGE_SIZE], PAGE_SIZE, PROT_NONE);
    }
    else if(p_dir->pages[i].owner == next){

      mprotect(&mem[i*PAGE_SIZE], PAGE_SIZE,  PROT_READ | PROT_WRITE);
    }

//    if(c_dir->contexts[i].owner == prev){
//      mprotect(c_dir->contexts[i].start, (64000 + sizeof(tcb) + sizeof(ucontext_t)), PROT_NONE);
//    }
//    else if(c_dir->contexts[i].owner == next){
//      mprotect(c_dir->contexts[i].start, (64000 + sizeof(tcb) + sizeof(ucontext_t)),  PROT_READ | PROT_WRITE);
//    }
  }
   sigprocmask(SIG_SETMASK, &b, NULL);
}
/*
int main(){
  //Check if malloc works.
  printf("If malloc works, should print 12\n");
  int* y = (int*) malloc (sizeof(int));
  *y = 12;
  printf("%d\n",*y);

  //Check if malloc works
  printf("If malloc works, should print 1002\n");
  int* z = (int*) malloc (sizeof(int));
  *z = 1002;
  printf("%d\n",*z);

  //Check if malloc works; Try to malloc(0)
  printf("Attempting to malloc 0 bytes\n");
  malloc(0);


  //free of pointer offset
  printf("Attempting to free an offset of a pointer; offset is not allocated\n");
  char* c;
  c = (char *)malloc( 200 );
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
  c = (char *)malloc( 100 );
  free( c );
  c = (char *)malloc( 100 );
  free( c );

  //Freeing everything
  free(z);

  //Attempt to saturate the memory for small blocks
  printf("Attempting to saturate memory.\n");
  int i;
  for(i = 0; i < 1000; i++){

    char* s = (char*) malloc(200);

    if(s == NULL){
      printf("Memory saturated, returned null pointer!\n");
      break;
    }
  }

  //Tests complete
  printf("Tests complete.\n");
  return 0;
}
*/
