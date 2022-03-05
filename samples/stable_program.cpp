//
// Created by byjtew on 05/03/2022.
//

#include <iostream>
#include <vector>

struct struct_s {
    size_t length;
    int *arr;
};

void func() {
  std::cout << "Printing from a function !" << std::endl;
}

int main() {
  std::cout << "Hello, I am a stable tiny program." << std::endl;
  std::cout
      << "I have:\n\t- 1 int variable\n\t- 1 char variable\n\t- 1 unsigned long variable\n\t- 1 int vector\n\t- 1 int list structure\n\t- 1 function call"
      << std::endl;

  int a = 16;
  std::cout << "int var: " << a << std::endl;

  char b = 'c';
  std::cout << "char var: " << b << std::endl;

  unsigned long ul = 24UL;
  std::cout << "unsigned long var: " << ul << std::endl;

  std::vector v = {1, 2, 3, 4, 5};
  std::cout << "int array with 5 elements var: [" << v[0] << "," << v[1] << "," << v[2] << "," << v[3] << "," << v[4]
            << "]" << std::endl;

  struct_s structure{};
  structure.length = 3;
  structure.arr = (int *) malloc(structure.length * sizeof(int));
  for (int i = 12; i < 12 + structure.length; i++)
    structure.arr[i - 12] = i;
  std::cout << "int list structure with 3 elements var: [" << structure.arr[0] << "," << structure.arr[1] << ","
            << structure.arr[2] << "]"
            << std::endl;

  func();


  free(structure.arr);
  std::cout << "Goodbye." << std::endl;
  return 0;
}