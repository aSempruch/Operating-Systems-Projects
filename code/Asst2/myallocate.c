// name:
// username of iLab:
// iLab Server:

#include "myallocate.h"
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<signal.h>
#include<sys/time.h>

char mem[8388608];
char swap[16777216];

int myallocate(int size, __FILE__, __LINE__, THREADREQ){

}

int mydeallocate(int size, __FILE__, __LINE__, THREADREQ){

}

void* shalloc(size_t size){

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
