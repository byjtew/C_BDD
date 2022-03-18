//
// Created by byjtew on 17/03/2022.
//
#include "stdlib.h"
#include "stdio.h"

void alloc_done() {
  printf("Now, let's free these bad boys, except one.\n");
}

int main() {
  printf("You can breakpoint at 'alloc_done', allocations will be done.\n");
  printf("This program:\n");
  printf(" - Allocates a char* with malloc, size=16.\n");
  char *arr1 = (char *) malloc(16 * sizeof(char));
  printf(" - Allocates a char[] with malloc, size=24.\n");
  char arr2[24];
  printf(" - Allocates a int* with calloc, size=4.\n");
  int *arr3 = (int *) calloc(4, sizeof(int));

  alloc_done();

  printf(" - Free a int* with calloc, size=4.\n");
  free(arr1);
  free(arr3);
}