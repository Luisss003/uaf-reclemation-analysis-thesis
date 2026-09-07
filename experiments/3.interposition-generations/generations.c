#define _GNU_SOURCE

#include <malloc.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>

enum state {
  UNALLOCATED, ALLOCATED, FREE 
};

enum operation {
  ALLOC, DEALLOC
};

enum reused {
  TRUE, FALSE, NA
};

struct history {
  enum operation op;
  void *base;
  int size;
  int gen;
  enum state s;
  enum reused r;
};

static struct history history[4096];
static int history_index = 0;
static int generation = 0;

void *malloc(size_t size){
  void *(*mallocp)(size_t size);
  char *err;

  mallocp = dlsym(RTLD_NEXT, "malloc");
  if((err = dlerror()) != NULL) exit(EXIT_FAILURE);

  char *ptr = mallocp(size);
  if(ptr == NULL) exit(EXIT_FAILURE);

  if(history_index >= 4096) exit(EXIT_FAILURE);

  history[history_index].op = ALLOC;
  history[history_index].base = ptr;
  history[history_index].size = size;
  history[history_index].gen = generation;
  history[history_index].s = ALLOCATED;
  history[history_index].r = FALSE;
  history_index++;

  for(int i = history_index-2; i >= 0; i--){
    if (history[i].base == ptr) {
      history[history_index-1].r = TRUE;
      history[history_index-1].gen = history[i].gen + 1;
      break;
    }
  }

  return ptr;

}

void free(void *ptr){
  void *(*freep)(void *ptr);
  char *err;

  freep = dlsym(RTLD_NEXT, "free");
  if((err = dlerror()) != NULL) exit(EXIT_FAILURE);

  freep(ptr);

  history[history_index].op = DEALLOC;
  history[history_index].base = ptr;
  history[history_index].size = 0;
  history[history_index].gen = generation;
  history[history_index].s = FREE;
  history[history_index].r = NA;
  history_index++;

}
