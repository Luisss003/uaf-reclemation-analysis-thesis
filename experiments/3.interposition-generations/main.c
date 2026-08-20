#include <malloc.h>
#include <dlfcn.h>

int main(){
  int *flag = (int *)dlsym(RTLD_DEFAULT, "flag"); 
  if(flag == NULL) return 1;
  *flag = 1;

  void* one = malloc(32);
  void* two = malloc(32);
  void* three = malloc(32);
  void* four = malloc(32);

  return 0;
}
