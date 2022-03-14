//
// Created by byjtew on 05/03/2022.
//

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

void func() {
  printf("Printing from a function: [0x%016lX]\n", func);
}

int main() {
  printf("Hello, I am a stable tin program.");
  printf(
      "I have:\n\t- 1 int variable\n\t- 1 char variable\n\t- 1 unsigned long variable\n\t- 1 int vector\n\t- 1 int list structure\n\t- 1 function call\n");

  int a = 16;
  printf("int var: %d\n", a);

  char b = 'c';
  printf("char var: %d\n", b);

  unsigned long ul = 24UL;
  printf("unsigned long var: %lu\n", ul);

  func();

  printf("Goodbye.\n");
  return 0;
}