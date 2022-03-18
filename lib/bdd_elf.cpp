//
// Created by byjtew on 05/03/2022.
//


#include <string>
#include <iostream>
#include <fstream>
#include <execution>

#include "bdd_elf.hpp"

using namespace elf;

void ElfFile::printHeader(FILE *fp) const {
  fprintf
      (fp,
       "ELF File Header:\nHeader:\n-----------------------------\n\n"
       "\t- Object file typ:                    0x%016hX\n"
       "\t- Architecture:                       0x%016hX\n"
       "\t- Object file version:                0x%016X\n"
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
       header.e_type,
       header.e_machine,
       header.e_version,
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


bool elf::isElfFile(Elf_Ehdr &header) {
  return header.e_ident[0] == ELFMAG0
         && header.e_ident[1] == ELFMAG1    /* E */
         && header.e_ident[2] == ELFMAG2    /* L */
         && header.e_ident[3] == ELFMAG3;    /* F */
}

std::string getFlagsAsString(unsigned int flags) {
  if (flags & ReadFlag && flags & WriteFlag && flags & ExecuteFlag) return "rwx";
  else if (flags & ReadFlag && flags & WriteFlag) return "rw-";
  else if (flags & ReadFlag && flags & ExecuteFlag) return "r-x";
  else if (flags & WriteFlag && flags & ExecuteFlag) return "-wx";
  else if (flags & ReadFlag) return "r--";
  else if (flags & WriteFlag) return "-w-";
  else if (flags & ExecuteFlag) return "--x";
  return "---";
}

void ElfFile::printProgramHeaderAt(int index, FILE *fp) const {
  auto pHeader = programHeaders.at(index);
  fprintf
      (fp,
       "\n\tProgram header [%d]:\n"
       "\t\t- Segment type:             %u\n"
       "\t\t- Segment flags:            %u (%s)\n"
       "\t\t- Segment file offset:      0x%016lX\n"
       "\t\t- Segment virtual address:  0x%016lX\n"
       "\t\t- Segment physical address: 0x%016lX\n"
       "\t\t- Segment size in file:     %lu\n"
       "\t\t- Segment size in memory:   %lu\n"
       "\t\t- Segment alignment:        %lu\n",
       index,
       pHeader.p_type,
       pHeader.p_flags, getFlagsAsString(pHeader.p_flags).c_str(),
       pHeader.p_offset,
       pHeader.p_vaddr,
       pHeader.p_paddr,
       pHeader.p_filesz,
       pHeader.p_memsz,
       pHeader.p_align
      );
}

void ElfFile::printSymbolEntry(unsigned index, const Elf_SymRef &sym, const Elf_Shdr &sHdr, FILE *fp) {
  fprintf
      (fp,
       "    ### Symbol (%u):\n"
       "    \n"
       "        - Symbol name:                 %u (%s)\n"
       "        - Symbol type:                 %s\n"
       "        - Symbol bindings:             %s\n"
       "        - Section table index:         %hu\n"
       "        - Symbol value:                0x%016lX\n"
       "        - Size of object:              %lu\n"
       "        - Other:                       %hhu\n"
       "    \n",
       index,
       sym.st_name,
       getSymbolName(sHdr, sym).c_str(),
       getSymbolTypeAsString(sym).c_str(),
       getSymbolBindingAsString(sym).c_str(),
       sym.st_shndx,
       sym.st_value,
       sym.st_size,
       sym.st_other
      );
}

Elf_SymRef
ElfFile::getSymbolSectionAt(unsigned int index, unsigned offset) const {
  return *((Elf_SymRef *) (getSectionDataPtrAt(index) + offset));
}

void ElfFile::printSymbolEntries(FILE *fp) {
  auto headers = getSectionHeaderIndexesByType(Elf_SectionTypeLinkerSymbolTable);
  unsigned index = 0;
  std::for_each(headers.cbegin(), headers.cend(), [fp, &index, this](const unsigned &e) {
      Elf_Shdr sHdr = sectionsHeaders.at(e);
      for (unsigned i = 0; i < getSymbolCount(sHdr); i++)
        printSymbolEntry(i, getSymbolSectionAt(e, i * sHdr.sh_entsize), sHdr, fp);
      index += 1;
  });
}

void ElfFile::printProgramHeaders(FILE *fp) const {
  for (int i = 0; i < programHeaders.size(); i++)
    printProgramHeaderAt(i, fp);
}


std::string ElfFile::getSectionTypeAsString(const Elf_Shdr &sHeader) {
  switch (sHeader.sh_type) {
    case SHT_PROGBITS:
      return "Program data";
    case SHT_SYMTAB:
      return "Symbol table";
    case SHT_STRTAB:
      return "String table";
    case SHT_RELA:
      return "Relocation entries with addends";
    case SHT_HASH:
      return "Symbol hash table";
    case SHT_DYNAMIC:
      return "Dynamic linking information";
    case SHT_NOTE:
      return "Notes";
    case SHT_NOBITS:
      return "Program space with no data (bss)";
    case SHT_REL:
      return "Relocation entries, no addends";
    case SHT_DYNSYM:
      return "Dynamic linker symbol table";
    default:
      return "Unused";
  }
}

void ElfFile::printSectionHeaderAt(int index, FILE *fp) const {
  if (index < 0 || index >= sectionsHeaders.size()) return;
  auto sHeader = sectionsHeaders.at(index);
  fprintf
      (fp,
       "\n\tSection header (%d):\n"
       "\t\t- Section name:                       %u (%s)\n"
       "\t\t- Section type:                       %u (%s)\n"
       "\t\t- Section flags:                      %lu (%s)\n"
       "\t\t- Section virtual address:            0x%016lX\n"
       "\t\t- Section file offset:                %lu\n"
       "\t\t- Section size in bytes:              %lu\n"
       "\t\t- Link to another section:            %u\n"
       "\t\t- Additional section information:     %u\n"
       "\t\t- Section alignment:                  %lu\n"
       "\t\t- Entry size if section holds table:  %lu\n",
       index,
       sHeader.sh_name, getSectionName(sHeader).c_str(),
       sHeader.sh_type, getSectionTypeAsString(sHeader).c_str(),
       sHeader.sh_flags, getFlagsAsString(sHeader.sh_flags).c_str(),
       sHeader.sh_addr,
       sHeader.sh_offset,
       sHeader.sh_size,
       sHeader.sh_link,
       sHeader.sh_info,
       sHeader.sh_addralign,
       sHeader.sh_entsize
      );
}

void ElfFile::printSectionsHeaders(FILE *fp) {
  for (int i = 0; i < sectionsHeaders.size(); i++)
    printSectionHeaderAt(i, fp);
}


ElfFile::ElfFile(const std::string &elf_filepath) {
  std::ifstream input;
  input.open(elf_filepath);
  if (input.fail()) throw std::invalid_argument("Bad input: file.initialize() failed");

#pragma region Elf Header
  std::string header_data;
  header_data.resize(64);
  input.read(header_data.data(), (int) header_data.size());
  auto headerPtr = (Elf_Ehdr *) ((addr_t) header_data.data());
  if (headerPtr == nullptr) throw std::invalid_argument("Null pointer: Elf header");
  header = *headerPtr;
  header_data.clear();
#pragma endregion

  if (!isElfFile(header)) throw std::invalid_argument("Not an ELF file");

#pragma region Program Deaders
  std::string pHeaders_data;
  pHeaders_data.resize(header.e_phentsize * header.e_phnum);
  input.seekg((long) header.e_phoff, std::ifstream::beg);
  input.read(pHeaders_data.data(), (int) pHeaders_data.size());
  auto pHeadersPtr = (Elf_Phdr *) ((addr_t) pHeaders_data.data());
  if (pHeadersPtr == nullptr) throw std::invalid_argument("Null pointer: Elf program-headers");
  programHeaders = std::vector<Elf_Phdr>(pHeadersPtr, pHeadersPtr + header.e_phnum);
  pHeaders_data.clear();
#pragma endregion

#pragma region Section Headers
  std::string sHeaders_data;
  sHeaders_data.resize(header.e_shentsize * header.e_shnum);
  input.seekg((long) header.e_shoff, std::ifstream::beg);
  input.read(sHeaders_data.data(), (int) sHeaders_data.size());
  auto sHeadersPtr = (Elf_Shdr *) ((addr_t) sHeaders_data.data());
  if (sHeadersPtr == nullptr) throw std::invalid_argument("Null pointer: Elf sections-headers");
  sectionsHeaders = std::vector<Elf_Shdr>(sHeadersPtr, sHeadersPtr + header.e_shnum);
  sHeaders_data.clear();
#pragma endregion

#pragma region Sections data
  std::vector<char> buffer;
  std::for_each(sectionsHeaders.cbegin(), sectionsHeaders.cend(), [&buffer, &input, this](const Elf_Shdr &sHdr) {
      buffer.resize(sHdr.sh_size);
      input.seekg((long) sHdr.sh_offset, std::ifstream::beg);
      input.read(buffer.data(), (int) buffer.size());
      sectionsData.push_back(buffer);
  });
  buffer.clear();
#pragma endregion

  input.close();
}

Elf_Shdr ElfFile::getSectionHeaderByType(Elf_SectionType type) const {
  return *std::find_if(std::execution::par, sectionsHeaders.cbegin(), sectionsHeaders.cend(),
                       [type](const Elf_Shdr &each) {
                           return each.sh_type == type;
                       });
}

std::vector<unsigned> ElfFile::getSectionHeaderIndexesByType(Elf_SectionType type) const {
  std::vector<unsigned> headers;
  unsigned current = 0;
  std::for_each(sectionsHeaders.cbegin(), sectionsHeaders.cend(), [&current, &headers, type](const Elf_Shdr &each) {
      if (each.sh_type == type) headers.push_back(current);
      current++;
  });
  return headers;
}

std::vector<Elf_Shdr> ElfFile::getSectionsHeaderByType(Elf_SectionType type) const {
  std::vector<Elf_Shdr> headers;
  std::for_each(sectionsHeaders.cbegin(), sectionsHeaders.cend(), [&headers, type](const Elf_Shdr &each) {
      if (each.sh_type == type) headers.push_back(each);
  });
  return headers;
}

std::string ElfFile::getSymbolTypeAsString(const Elf_SymRef &sym) {
  switch (sym.st_info & 0x0F) {
    case Elf_SymbolTypeNone:
      return "No type specified";
    case Elf_SymbolTypeDataObject:
      return "Data object";
    case Elf_SymbolTypeFunctionEntryPoint:
      return "Function entry point";
    case Elf_SymbolTypeSection:
      return "Symbol associated with a section";
    case Elf_SymbolTypeFile:
      return "Source file associated with the object file";
    default:
      return "NULL";
  }
}

std::string ElfFile::getNameFromStringTable(unsigned strTableIndex, unsigned offset) const {
  auto data_ptr = getSectionDataPtrAt(strTableIndex);
  if (data_ptr == nullptr) return "Unnamed";
  return data_ptr + offset;
}

std::string ElfFile::getSectionName(const Elf_Shdr &sHeader) const {
  return getNameFromStringTable(header.e_shstrndx, sHeader.sh_name);
}

std::string ElfFile::getSymbolName(const Elf_Shdr &sHeader, const Elf_SymRef &sym) const {
  auto raw_name = getNameFromStringTable(sHeader.sh_link, sym.st_name);
  return raw_name.c_str();
}

std::string ElfFile::getSymbolBindingAsString(const Elf_SymRef &sym) {
  switch (sym.st_info) {
    case Elf_SymbolBindingLocal:
      return "Local";
    case Elf_SymbolBindingGlobal:
      return "Global";
    case Elf_SymbolBindingWeak:
      return "Weak";
    default:
      return "NULL";
  }
}

const char *ElfFile::getSectionDataPtrAt(unsigned int index) const {
  if (index >= sectionsData.size()) return nullptr;
  return sectionsData.at(index).data();
}

std::vector<std::pair<addr_t, std::string>> ElfFile::getFunctionsList() const {
  std::vector<std::pair<addr_t, std::string>> functions;
  auto symbolHeaders = getSectionHeaderIndexesByType(Elf_SectionTypeLinkerSymbolTable);
  for (const auto &e: symbolHeaders) {
    Elf_Shdr sHdr = sectionsHeaders.at(e);
    for (unsigned i = 0; i < getSymbolCount(sHdr); i++) {
      Elf_SymRef symbolSectionData = getSymbolSectionAt(e, i * sHdr.sh_entsize);
      if ((symbolSectionData.st_info & 0x0F) != Elf_SymbolTypeFunctionEntryPoint) continue;
      auto address = symbolSectionData.st_value;
      auto name = getSymbolName(sHdr, symbolSectionData);
      if (!name.empty())
        functions.emplace_back(address, name.substr(0, name.find('(')));
    }
  }
  return functions;
}

addr_t ElfFile::getFunctionAddress(const std::string &fct_name) const {
  auto list = getFunctionsList();
  return std::find_if(std::execution::par, list.cbegin(), list.cend(),
                      [fct_name](const std::pair<addr_t, std::string> &e) {
                          return e.second == fct_name;
                      })->first;
}

unsigned ElfFile::getSymbolCount(const Elf_Shdr &sHdr) {
  return (unsigned) sHdr.sh_size / sHdr.sh_entsize;
}

std::vector<std::pair<std::string, std::string>> ElfFile::getSymbolsNames() const {
  std::vector<std::pair<std::string, std::string>> names;
  std::for_each(sectionsHeaders.begin(), sectionsHeaders.end(), [this, &names](const Elf_Shdr &e) {
      names.emplace_back(getSectionTypeAsString(e), getSectionName(e));
  });
  return names;
}


