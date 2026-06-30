#include <stdio.h>
int a = 1;

void foo() {

  a = 5;

}



int main ()

{

  int b = a;

  foo();

  int c = a;


  printf("%d\n", b + c);

  return 0;

}