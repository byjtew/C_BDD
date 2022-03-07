//
// Created by byjtew on 05/03/2022.
//


#include "elf_reader.hpp"


bool isElfFile(Elf_Ehdr &file) {
  return file.e_ident[0] == 0x7F
         && file.e_ident[1] == 0x45    /* E */
         && file.e_ident[2] == 0x4C    /* L */
         && file.e_ident[3] == 0x46;    /* F */
}

