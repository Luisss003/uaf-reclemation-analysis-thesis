//In this version, we will stick to syscalls to prevent perturbation
#ifdef RUNTIME
#define _GNU_SOURCE

#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

void *malloc(size_t size){
  void *(*mallocp)(size_t size);
  char *error;

  mallocp = dlsym(RTLD_NEXT, "malloc");
  if((error = dlerror()) != NULL) {
//    printf("FAIL");
    exit(EXIT_FAILURE);
  }

  char *ptr = mallocp(size);
 /*
  fprintf(stderr, 
    "Start of User Heap Mem: %p\n"
    "Start of Chunk Metadata: %p\n"
    "size: 0x%lx\n"
    "prev_size: %lx\n"
    ,ptr, ptr-16, *(unsigned long *)(ptr-8), *(unsigned long *)(ptr-16)); */
  return ptr;
}

void free(void* ptr){
  void *(*freep)(void* ptr);
  char *error;

  freep = dlsym(RTLD_NEXT, "free");
  if((error = dlerror()) != NULL){
    //printf("ERROR");
    exit(EXIT_FAILURE);
  }

  freep(ptr);
  /* fprintf(stderr, 
  "Start of User Mem of Freed Chunk: %p\n"
  "Start of Freed Chunk Metadata: %p\n"
  "size: 0x%lx\n"
  "prev_size: %lx\n"
  "forward pointer to next chunk in list of freed chunks: %p"
  , ptr, ptr-16, *(unsigned long *)(ptr-8), *(unsigned long *)(ptr-16)
  , *(void**)(ptr+8)); */
}
#endif
