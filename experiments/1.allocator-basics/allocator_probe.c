#define _GNU_SOURCE

#include <errno.h>
#include <inttypes.h>
#include <malloc.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void probe_sizes(void)
{
  printf("requested, usable, address, address_mod_16\n");
  for(size_t requested = 1; requested <= 256; requested++){
    void *ptr = malloc(requested);
    if(ptr == NULL) {
      perror("malloc");
      exit(EXIT_FAILURE);
    }
    size_t usable = malloc_usable_size(ptr);

    printf("%zu,%zu,%p,%" PRIuPTR "\n",
            requested,
            usable,
            ptr,
            (uintptr_t)ptr%16);
    free(ptr);
  }
}

int main(){
  probe_sizes();
  return 0;
}
