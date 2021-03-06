//
// Created by byjtew on 05/03/2022.
//

#include <iostream>

#include "bdd_elf.hpp"

using namespace elf;

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <elf-file|compiled-prog>" << std::endl;
    return 1;
  }

  auto elf_file = ElfFile(argv[1]);
  elf_file.printHeader();
  elf_file.printProgramHeaders();
  elf_file.printSectionsHeaders();
  return 0;
}
