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

char mem[8388608];
char swap[16777216];

int myallocate(int size, int file, int line, int threadreq){
  int num_pages = (size/(int)page_size);
  if(size % page_size > 0)
    num_pages++;

  printf("%d\n", num_pages);
  printf("%lu\n", page_size);
}

int mydeallocate(int size, int file, int line, int threadreq){
  return 0;
}

void* shalloc(size_t size){
  return;
}

int main(){
  myallocate(4500, 0, 0, 0);
  return 0;
}


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
