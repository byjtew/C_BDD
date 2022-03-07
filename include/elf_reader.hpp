//
// Created by byjtew on 05/03/2022.
//

#ifndef C_BDD_ELF_READER_HPP
#define C_BDD_ELF_READER_HPP

#include <cstdint>
#include "elf.h"

#if INTPTR_MAX == INT64_MAX // 64 BITS ARCHITECTURE
using Elf_Ehdr = Elf64_Ehdr;
#elif INTPTR_MAX == INT32_MAX // 32 BITS ARCHITECTURE
using Elf_Ehdr = Elf32_Ehdr;
#endif

[[nodiscard]] bool isElfFile(Elf_Ehdr &file);

#endif //C_BDD_ELF_READER_HPP
