//
// Created by byjtew on 05/03/2022.
//

#include <iostream>

#include "elf_reader.hpp"

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <elf-file|compiled-prog>" << std::endl;
    return 1;
  }

  auto hdr = readElfHeaderFromFile(argv[1]);
  printElfHeader(hdr);

  auto pHeaders = readElfProgramHeadersFromFile(hdr, argv[1]);

  printElfProgramHeader(hdr, pHeaders);

  return 0;
}
