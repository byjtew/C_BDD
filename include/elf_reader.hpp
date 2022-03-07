//
// Created by byjtew on 05/03/2022.
//

#ifndef C_BDD_ELF_READER_HPP
#define C_BDD_ELF_READER_HPP

#include "elf.h"

using Elf_Ehdr = Elf32_Ehdr;

[[nodiscard]] bool isElfFile(Elf_Ehdr &file);

#endif //C_BDD_ELF_READER_HPP
