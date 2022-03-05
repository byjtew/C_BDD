//
// Created by byjtew on 05/03/2022.
//

#include <iostream>

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <elf-file|compiled-prog>" << std::endl;
    return 1;
  }
  return 0;
}