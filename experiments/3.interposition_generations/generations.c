#define _GNU_SOURCE

#include <malloc.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>

int flag = 0;

enum state {
  UNALLOCATED, ALLOCATED, FREE  
};

struct gen{
  void *base;
  int size;
  int generation;
  enum state s;
};

struct gen generations[4];
int count = 0;

void *malloc(size_t size){
  void *(*mallocp)(size_t size);
  char *err;

  mallocp = dlsym(RTLD_NEXT, "malloc");
  if((err = dlerror()) != NULL) exit(EXIT_FAILURE);

  char *ptr = mallocp(size);

  if(flag){
    //Fill the array to keep track of allocations
    if(count < 4){
      generations[count].base = ptr;
      generations[count].generation = 1;
      generations[count].size = size;
      generations[count].s = ALLOCATED;
      count++;
    }
    //Otherwise, its already filled
    else{
      
    }
  }
  else{
    write(STDERR_FILENO, "startup malloc\n", 15);
  }

  return ptr;

}

