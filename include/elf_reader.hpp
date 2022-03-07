//
// Created by byjtew on 05/03/2022.
//

#ifndef C_BDD_ELF_READER_HPP
#define C_BDD_ELF_READER_HPP

#include <cstdint>
#include <vector>
#include "elf.h"

#if INTPTR_MAX == INT64_MAX // 64 BITS ARCHITECTURE
#define ARCHITECTURE 64
using Elf_Ehdr = Elf64_Ehdr;
using Elf_Phdr = Elf64_Phdr;

#elif INTPTR_MAX == INT32_MAX // 32 BITS ARCHITECTURE
#define ARCHITECTURE 32
using Elf_Ehdr = Elf32_Ehdr;
using Elf_Phdr = Elf32_Phdr;

#endif


enum {
    ExecuteFlag = 1, WriteFlag = 2, ReadFlag = 4
};


[[maybe_unused]] void printElfHeader(const Elf_Ehdr &header, FILE *fp = stdout);

[[nodiscard]] bool isElfFile(Elf_Ehdr &file);

[[nodiscard]] Elf_Ehdr readElfHeaderFromFile(const std::string &path);


[[maybe_unused]] void
printElfProgramHeader(const Elf_Ehdr &header, const std::vector<Elf64_Phdr> &pHeaders, FILE *fp = stdout);

[[nodiscard]] std::vector<Elf64_Phdr> readElfProgramHeadersFromFile(const Elf_Ehdr &header, const std::string &path);

#endif //C_BDD_ELF_READER_HPP

