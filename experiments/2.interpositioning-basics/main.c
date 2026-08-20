#include <malloc.h>

int main(){
  char* p = malloc(1);
  free(p);
  return 0;
}
