// name:
// username of iLab:
// iLab Server:

#ifndef MYALLOCATE_H
#define MYALLOCATE_H

#include "my_pthread_t.h"

#define THREADREQ 1
#define LIBRARYREQ 2
#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ)
#define PAGE_SIZE 4096
#define MEM_SIZE 8388608
#define OS_SIZE 4100096
#define NUM_PAGES 2048
#define USER_PAGE_START 1001
#define SHALLOC_PAGE_START 2045
#define CONTEXT_START 100
#define NUM_CONTEXTS 55
#define MUTEX_START 64

typedef struct _mem_entry {
  unsigned int size;
  int available;
  struct _mem_entry* next;
  struct _mem_entry* prev;
} mem_entry;

typedef struct _page_entry {
  int start_index;
  int available;
  struct _mem_entry* head;
  tcb* owner;
} page_entry;


typedef struct _page_directory {
  page_entry pages[NUM_PAGES];
} page_directory;

typedef struct _context_entry {
  void* start;
  int available;
  tcb* owner;
}context_entry;

typedef struct _context_directory {
  context_entry contexts[NUM_CONTEXTS];
} context_directory;

typedef struct _mutex_entry{

}mutex_entry;

typedef struct _mutex_directory {

}mutex_directory;

char *mem;
page_directory *p_dir;
context_directory *c_dir;
mutex_directory *m_dir;

int requestPage();
void* myallocate(unsigned int size, char* file, unsigned int line, int threadreq);
int mydeallocate(void* item, char* file, unsigned int line, int threadreq);
void* shalloc(size_t size);

#endif
