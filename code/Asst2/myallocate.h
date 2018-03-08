// name:
// username of iLab:
// iLab Server:

#ifndef MYALLOCATE_H
#define MYALLOCATE_H

#include "my_pthread_t.h"

typedef struct _page_entry {
  int start_index;
  tcb* owner;
} page_entry;

int myallocate(int size, int file, int line, int threadreq);
int mydeallocate(int size, int file, int line, int threadreq);
void* shalloc(size_t size);

#endif
