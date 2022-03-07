//
// Created by byjtew on 05/03/2022.
//


#include "elf_reader.hpp"


bool isElfFile(Elf_Ehdr &file) {
  return file.e_ident[0] == ELFMAG0
         && file.e_ident[1] == ELFMAG1    /* E */
         && file.e_ident[2] == ELFMAG2    /* L */
         && file.e_ident[3] == ELFMAG3;    /* F */
}

