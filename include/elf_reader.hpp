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
using Elf_Shdr = Elf64_Shdr;

#elif INTPTR_MAX == INT32_MAX // 32 BITS ARCHITECTURE
#define ARCHITECTURE 32
using Elf_Ehdr = Elf32_Ehdr;
using Elf_Phdr = Elf32_Phdr;
using Elf_Shdr = Elf32_Shdr;

#endif

namespace elf {
    enum {
        ExecuteFlag = 1, WriteFlag = 2, ReadFlag = 4
    };

    [[nodiscard]] bool isElfFile(Elf_Ehdr &header);

    class ElfFile {
    private:
        Elf_Ehdr header{};
        std::vector<Elf_Phdr> programHeaders;
        std::vector<Elf_Shdr> sectionsHeaders;

        /**
        * @cite https://refspecs.linuxfoundation.org/elf/gabi4+/ch4.sheader.html
        * @param sectionHeader
        */
        static std::string getSectionTypeAsString(const Elf_Shdr &sHeader);

        [[nodiscard]] static std::string getSectionsNameAsString(const Elf_Shdr &sHeader);

    public:
        explicit ElfFile(const std::string &elf_filepath);

        ElfFile() = default;

        ~ElfFile() = default;

        [[maybe_unused]] void printHeader(FILE *fp = stdout) const;

        [[maybe_unused]] void printProgramHeaders(FILE *fp = stdout) const;

        [[maybe_unused]] void printProgramHeaderAt(int index, FILE *fp = stdout) const;

        [[maybe_unused]] void printSectionsHeaders(FILE *fp = stdout) const;

        [[maybe_unused]] void printSectionHeaderAt(int index, FILE *fp = stdout) const;
    };


} // namespace elf

#endif //C_BDD_ELF_READER_HPP

