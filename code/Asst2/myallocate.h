// name:
// username of iLab:
// iLab Server:

#ifndef MYALLOCATE_H
#define MYALLOCATE_H

#include "my_pthread_t.h"

#define THREADREQ 1
#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ)
#define PAGE_SIZE 4096
//sysconf returns a variable

char *mem;

typedef struct _page_entry {
  int start_index;
  struct _mem_entry* head;
  tcb* owner;
} page_entry;

typedef struct _page_directory {
  //page_entry x[8388608/page_size];
  page_entry pages[8388608/PAGE_SIZE];
} page_directory;

typedef struct _mem_entry {
  unsigned int size;
  int available;
  struct _mem_entry* next;
  struct _mem_entry* prev;
} mem_entry;

void* myallocate(unsigned int size, char* file, unsigned int line, int threadreq);
int mydeallocate(void* item, char* file, unsigned int line, int threadreq);
void* shalloc(size_t size);

#endif
