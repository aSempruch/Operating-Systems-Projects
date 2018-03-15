// name:
// username of iLab:
// iLab Server:

#include "myallocate.h"
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<signal.h>
#include<sys/time.h>

#define page_size sysconf(_SC_PAGE_SIZE)

static char* mem = memalign(page_size, 8388608);
//static char* swap = (char*)memalign(page_size,16777216);
static page_directory* page_dir;
static void* memlist[8388608/sizeof(mem_entry) + 1]; //temporary, every page should have a memlist
static int init = 0;
static int malloc_init = 0;

int findIndex(){
    int i;
    for(i = 0; i < 5000/sizeof(mem_entry) + 1; i++){
        if(memlist[i] == 0){
            return i;
        }
    }
    return -1;
}

int initialize(){
  page_dir = (page_directory*)mem;
  page_dir->pages = page_entry[8388608/page_size];
  int i;
  for(i = 0; i < 8388608/page_size; i++){
    page_entry page;
    page.start_index = i*page_size;
    page.head = NULL;
    page.owner = NULL;
    page_dir->pages[i] = page;
  }
}

void* myallocate(unsigned int size, char* file, unsigned int line, int threadreq){
  /*int num_pages = (size/(int)page_size);
  if(size % page_size > 0)
    num_pages++;*/
  if(size == 0)
    return NULL;

  static mem_entry* head;
  mem_entry* temp;
  mem_entry* next;

  // if(!init){
  //   initialize();
  // }

  if(!malloc_init){
    int i;
    for(i = 0; i < 8388608/sizeof(mem_entry) + 1; i++){
      memlist[i] = NULL;
    }

    head = (mem_entry*)mem; //points to beginning of memory (MUST CHANGE TO POINT TO BEGINNING OF PAGE)
    head->next = NULL;
    head->prev = NULL;
    head->available = 1;
    head->size = page_size - sizeof(mem_entry);
    malloc_init = 1;
  }

  temp = head;

  while(temp != NULL){
    if(temp->available != 1 || temp->size < size){
      temp = temp->next;
    }
    else if(temp->size < size+sizeof(mem_entry)){    //Last block case; No space left after
      temp->available = 0;
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
      memlist[findIndex()] = temp;
      return(char*)temp+sizeof(mem_entry);
    }
  }
}

int mydeallocate(void* item, char* file, unsigned int line, int threadreq){
  if(item == NULL){
    return -1; //Free failed; cannot free null pointer
  }

  mem_entry* temp;
  mem_entry* next;
  mem_entry* prev;
  temp = (mem_entry*)((char*)item - sizeof(mem_entry));
  if(temp->available == 1){
    return -1; //Free failed; pointer has not been malloc'ed or has already been freed
  }

  int i;
  int isvalid = 0;
  for(i = 0; i < 8388608/sizeof(mem_entry) + 1; i++){
    if(memlist[i] == temp && temp->available == 0){
      isvalid = 1;
    }
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

  if(next != NULL && next->available == 1){               //Current block is freed, next is available, previous block becomes bigger.
    prev->size = prev->size + sizeof(mem_entry) + next->size;
    prev->next = next->next;
    if(next->next != NULL){
      next->next->prev = prev;
    }
  }

  return 0;
}

void* shalloc(size_t size){
  return;
}

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

  //Attempt to saturate the memory for small blocks
  printf("Attempting to saturate memory.\n");
  int increment;
  for(increment = 0; increment < 1000; increment = increment + sizeof(int)){
    int* s = (int*) malloc(sizeof(int));
    if(s == NULL){
      break;
    }
  }

  //Tests complete
  printf("Tests complete.\n");
  return 0;
}


/*
myallocate.c:14:20: warning: initialization makes pointer from integer without a cast [enabled by default]
 static char* mem = memalign(page_size, 8388608);
                    ^
myallocate.c:14:1: error: initializer element is not constant
 static char* mem = memalign(page_size, 8388608);
 ^
myallocate.c: In function ‘initialize’:
myallocate.c:33:21: error: expected expression before ‘page_entry’
   page_dir->pages = page_entry[8388608/page_size];
*/

/*
    Mem : { [ page ] [ page ] [ page ] [ page ] [ page ] [ page ] [ page ] }
    Page : size = sysconf( _SC_PAGE_SIZE)

      table_entry page_table[]
      struct table_entry{
        int start_index
        int end_index
        // (end_index - start_index) % sysconf(_SC_PAGE_SIZE) = 0
      }


    Project Notes:
      - You should reserve memory for a thread on the granularity of a system page. You can discover the system page size with "sysconf( _SC_PAGE_SIZE)"

*/
