//
// Created by byjtew on 05/03/2022.
//


#include <string>
#include <iostream>
#include <fstream>
#include "elf_reader.hpp"

void printElfHeader(const Elf_Ehdr &header, FILE *fp) {
  fprintf
      (fp,
       "ELF%d file:\n"
       "\n"
       "Header:"
       "-------------------------\n"
       "\n"
       "\t- Entry point virtual address:        0x%016lX\n"
       "\t- Program header table file offset:   %lu\n"
       "\t- Section header table file offset:   %lu\n"
       "\t- Processor-specific flags:           %u\n"
       "\t- ELF header size:                    %hu\n"
       "\t- Program header table entry size:    %hu\n"
       "\t- Program header table entry count:   %hu\n"
       "\t- Section header table entry size:    %hu\n"
       "\t- Section header table entry count:   %hu\n"
       "\t- Section header string table index:  %hu\n",
       ARCHITECTURE,
       header.e_entry,
       header.e_phoff,
       header.e_shoff,
       header.e_flags,
       header.e_ehsize,
       header.e_phentsize,
       header.e_phnum,
       header.e_shentsize,
       header.e_shnum,
       header.e_shstrndx
      );
}

Elf_Ehdr readElfHeaderFromFile(const std::string &path) {
  std::ifstream input;
  input.open(path);
  if (input.fail()) throw std::invalid_argument("Bad input");

  std::string data;
  data.resize(64);
  input.read(data.data(), (int) data.size());
  input.close();

  auto *header = (Elf_Ehdr *) ((void *) data.data());
  if (!isElfFile(*header)) throw std::invalid_argument("Not an ELF file");

  return *header;
}


bool isElfFile(Elf_Ehdr &file) {
  return file.e_ident[0] == ELFMAG0
         && file.e_ident[1] == ELFMAG1    /* E */
         && file.e_ident[2] == ELFMAG2    /* L */
         && file.e_ident[3] == ELFMAG3;    /* F */
}

