#include <malloc.h>
#include <dlfcn.h>

int main(){
  void *ptrs[5];

  for(int i = 0; i < 5; i++){
    ptrs[i] = malloc(32); 
  }

  for(int i = 0; i < 5; i++){
    free(ptrs[i]);
  }

  void *one = malloc(32);
  void *two = malloc(32);
  void *three = malloc(32);

  free(one);
  free(two);
  free(three);
  return 0;
}
