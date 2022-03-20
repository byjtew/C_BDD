//
// Created by byjtew on 05/03/2022.
//

#include <stdio.h>
#include <stdint.h>
#include "unistd.h"
#include "stdlib.h"

void func(const char *msg) {
  printf("Printing from a function: %s [0x%016lX]\n", msg, (uintptr_t) func);
}

int main(int argc, char **argv) {
  for (unsigned i = 1; i < argc; i++)
    printf("My %d-th arg is %s\n", i, argv[i]);
  char input[124];
  printf("Hello, I am a stable program. My PID is %d\n", getpid());
  printf(
      "I have:\n\t- 1 int variable\n\t- 1 char variable\n\t- 1 unsigned long variable\n\t- 1 int vector\n\t- 1 int list structure\n\t- 1 function call\n");

  int a = 16;
  printf("int var: %d\n", a);

  char b = 'c';
  printf("char var: %d\n", b);

  unsigned long ul = 24UL;
  printf("unsigned long var: %lu\n", ul);


  func("Test");

  printf("Goodbye.\n");
  return 0;
}