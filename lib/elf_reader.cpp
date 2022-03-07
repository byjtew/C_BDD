//
// Created by byjtew on 05/03/2022.
//


#include <string>
#include <iostream>
#include <fstream>
#include "elf_reader.hpp"

using namespace elf;

void elf::printElfHeader(const Elf_Ehdr &header, FILE *fp) {
  fprintf
      (fp,
       "ELF%d file:\n"
       "Header:\n"
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

void elf::printElfProgramHeaderAt(int index, const Elf_Phdr &pHeader, FILE *fp) {
  fprintf
      (fp,
       "\n\tProgram header [%d]:\n"
       "\t\t- Segment type:             %u\n"
       "\t\t- Segment flags:            %u (%s)\n"
       "\t\t- Segment file offset:      0x%016lX\n"
       "\t\t- Segment virtual address:  0x%016lX\n"
       "\t\t- Segment size in file:     %lu\n"
       "\t\t- Segment size in memory:   %lu\n"
       "\t\t- Segment alignment:        %lu\n",
       index,
       pHeader.p_type,
       pHeader.p_flags, getFlagsAsString(pHeader.p_flags).c_str(),
       pHeader.p_offset,
       pHeader.p_vaddr,
       pHeader.p_filesz,
       pHeader.p_memsz,
       pHeader.p_align
      );
}

void elf::printElfProgramHeaders(const std::vector<Elf_Phdr> &pHeaders, FILE *fp) {
  int index = 0;
  std::for_each(pHeaders.cbegin(), pHeaders.cend(), [&index, fp](const Elf_Phdr &pHeader) {
      printElfProgramHeaderAt(index++, pHeader, fp);
  });
}

ElfFile::ElfFile(const std::string &elf_filepath) {
  std::ifstream input;
  input.open(elf_filepath);
  if (input.fail()) throw std::invalid_argument("Bad input: file.open() failed");

  std::string header_data;
  header_data.resize(64);
  input.read(header_data.data(), (int) header_data.size());
  auto headerPtr = (Elf_Ehdr *) ((void *) header_data.data());
  if (headerPtr == nullptr) throw std::invalid_argument("Null pointer: Elf header");
  header = *headerPtr;
  header_data.clear();

  if (!::isElfFile(header)) throw std::invalid_argument("Not an ELF file");

  std::string pHeaders_data;
  pHeaders_data.resize(header.e_phentsize * header.e_phnum);
  input.seekg((long) header.e_phoff, std::ifstream::beg);
  input.read(pHeaders_data.data(), (int) pHeaders_data.size());
  auto pHeadersPtr = (Elf_Phdr *) ((void *) pHeaders_data.data());
  if (pHeadersPtr == nullptr) throw std::invalid_argument("Null pointer: Elf program-headers");
  programHeaders = std::vector<Elf_Phdr>(pHeadersPtr, pHeadersPtr + header.e_phnum);

  input.close();
}
