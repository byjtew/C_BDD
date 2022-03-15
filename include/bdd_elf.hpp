//
// Created by byjtew on 05/03/2022.
//

#ifndef C_BDD_BDD_ELF_HPP
#define C_BDD_BDD_ELF_HPP

#include <cstdint>
#include <vector>
#include <map>
#include "elf.h"

#if INTPTR_MAX == INT64_MAX // 64 BITS ARCHITECTURE
#define ARCHITECTURE 64
using Elf_Ehdr = Elf64_Ehdr;
using Elf_Phdr = Elf64_Phdr;
using Elf_Shdr = Elf64_Shdr;
using Elf_SymRef = Elf64_Sym;

#elif INTPTR_MAX == INT32_MAX // 32 BITS ARCHITECTURE
#define ARCHITECTURE 32
using Elf_Ehdr = Elf32_Ehdr;
using Elf_Phdr = Elf32_Phdr;
using Elf_Shdr = Elf32_Shdr;
using Elf_SymRef = Elf32_Sym

#endif

using addr_t = uintptr_t;

namespace elf {
    enum {
        ExecuteFlag = 1, WriteFlag = 2, ReadFlag = 4
    };
    typedef enum {
        Elf_SectionTypeUnused = 0,
        Elf_SectionTypeProgBits = 1,
        Elf_SectionTypeLinkerSymbolTable = 2,
        Elf_SectionTypeStringTable = 3,
        Elf_SectionTypeRelaRelocationEntries = 4,
        Elf_SectionTypeSymbolHashTable = 5,
        Elf_SectionTypeDynamicLinkingTables = 6,
        Elf_SectionTypeNoteInformation = 7,
        Elf_SectionTypeUninitializedSpace = 8,
        Elf_SectionTypeRelRelocationEntries = 9,
        Elf_SectionTypeDynamicLoaderSymbolTable = 11
    } Elf_SectionType;

    typedef enum {
        Elf_SymbolTypeNone = 0,
        Elf_SymbolTypeDataObject = 1,
        Elf_SymbolTypeFunctionEntryPoint = 2,
        Elf_SymbolTypeSection = 3,
        Elf_SymbolTypeFile = 4
    } Elf_SymbolType;
    typedef enum {
        Elf_SymbolBindingLocal = 0,
        Elf_SymbolBindingGlobal = 1,
        Elf_SymbolBindingWeak = 2
    } Elf_SymbolBinding;

    [[nodiscard]] bool isElfFile(Elf_Ehdr &header);

    class ElfFile {
    private:
        Elf_Ehdr header{};
        std::vector<Elf_Phdr> programHeaders;
        std::vector<Elf_Shdr> sectionsHeaders;
        std::vector<std::vector<char>> sectionsData;


        /**
        * @cite https://refspecs.linuxfoundation.org/elf/gabi4+/ch4.sheader.html
        * @param sectionHeader
        */
        [[maybe_unused]] [[nodiscard]] static std::string getSectionTypeAsString(const Elf_Shdr &sHeader);

        [[maybe_unused]] [[nodiscard]] std::string
        getNameFromStringTable(unsigned int strTableIndex, unsigned int offset) const;

        [[maybe_unused]] [[nodiscard]] std::string getSymbolName(const Elf_Shdr &sHeader, const Elf_SymRef &sym) const;

        [[maybe_unused]] [[nodiscard]] std::string getSectionName(const Elf_Shdr &sHeader);

        [[maybe_unused]] [[nodiscard]] std::vector<unsigned int>
        getSectionHeaderIndexesByType(Elf_SectionType type) const;

        [[maybe_unused]] [[nodiscard]] std::vector<Elf_Shdr> getSectionsHeaderByType(Elf_SectionType type) const;

        [[maybe_unused]] [[nodiscard]] Elf_Shdr getSectionHeaderByType(Elf_SectionType type) const;

        [[maybe_unused]] [[nodiscard]] static std::string getSymbolTypeAsString(const Elf_SymRef &sym);

        [[maybe_unused]] [[nodiscard]] static std::string getSymbolBindingAsString(const Elf_SymRef &sym);

        [[maybe_unused]] [[nodiscard]] const char *getSectionDataPtrAt(unsigned index) const;

        static unsigned getSymbolCount(const Elf_Shdr &sHdr);

    public:
        explicit ElfFile(const std::string &eachStrTable);

        ElfFile() = default;

        ~ElfFile() = default;

        [[maybe_unused]] void printHeader(FILE *fp = stdout) const;

        [[maybe_unused]] void printProgramHeaders(FILE *fp = stdout) const;

        [[maybe_unused]] void printProgramHeaderAt(int index, FILE *fp = stdout) const;

        [[maybe_unused]] void printSectionsHeaders(FILE *fp = stdout);

        [[maybe_unused]] void printSectionHeaderAt(int index, FILE *fp = stdout);

        [[maybe_unused]] void printSymbolEntries(FILE *fp = stdout);

        [[maybe_unused]] void printSymbolEntry(unsigned index, const Elf_SymRef &sym, const Elf_Shdr &sHdr, FILE *fp);

        [[maybe_unused]] [[nodiscard]] addr_t getFunctionAddress(const std::string &fct_name) const;

        [[nodiscard]] Elf_SymRef getSymbolSectionAt(unsigned int index, unsigned offset) const;

        [[nodiscard]] std::vector<std::pair<addr_t, std::string>> getFunctionsList() const;
    };


} // namespace elf

#endif //C_BDD_BDD_ELF_HPP

