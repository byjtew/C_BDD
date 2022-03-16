//
// Created by byjtew on 12/03/2022.
//

#ifndef C_BDD_BDD_PTRACE_HPP
#define C_BDD_BDD_PTRACE_HPP

#define INT3 0xCC

#if INTPTR_MAX == INT64_MAX // 64 BITS ARCHITECTURE
#define REGISTER_IP RIP
#define TRAP_MASK   0xFFFFFFFFFFFFFF00

#elif INTPTR_MAX == INT32_MAX // 32 BITS ARCHITECTURE
#define REGISTER_IP EIP
#define TRAP_MASK   0xFFFFFF00
#endif

#include <iostream>
#include <csignal>
#include <string>
#include <sys/wait.h>
#include <sys/reg.h>
#include <vector>
#include <stack>
#include <ostream>
#include <sys/ptrace.h>
#include <execution>
#include <cstdio>
#include <cstdarg>
#include <fcntl.h>
#include <fstream>

#include "bdd_elf.hpp"
#include "bdd_exclusive_io.hpp"

using instr_t = long;

class Breakpoint {
private:
    bool enabled = false;
    pid_t program_pid;
    addr_t address;
    instr_t original;
public:

    Breakpoint(pid_t pid, const addr_t &addr);

    bool enable();

    void disable();

    [[nodiscard]] bool isEnabled() const { return enabled; }

    [[nodiscard]] addr_t getAddress() const {
      return address;
    }

    [[nodiscard]] instr_t getOriginal() const {
      return original;
    }
};


class TracedProgram {
private:
    addr_t ram_start_address = 0;
    std::string elf_file_path;


    elf::ElfFile elf_file;
    pid_t traced_pid;
    int cached_status = 0;

    std::map<addr_t, Breakpoint> breakpointsMap;


    void initChild(std::vector<char *> &parameters);

    void initBDD();

    void waitAndUpdateStatus();

    void attachBDD(int &status);

    [[nodiscard]] addr_t getTracedRAMAddress() const;

    void ptraceBackwardStep() const;

    void resumeBreakpoint();

public:
    TracedProgram(const std::string &exec_path, std::vector<char *> &parameters);


    ~TracedProgram() {
      ExclusiveIO::terminate();
    }

#pragma region breakpoints

    [[nodiscard]] bool breakpointAtFunction(const std::string &fctName);

    [[nodiscard]] addr_t getIP() const;

#pragma endregion breakpoints

    void ptraceContinue(bool lock = true);

    void ptraceStep();

    [[nodiscard]] bool isDead() const;

    [[nodiscard]] bool isAlive() const;

    [[nodiscard]] bool isStopped() const;

    [[nodiscard]] bool isTrapped() const;

    void stop() const;

    void rerun(std::vector<char *> &args);

    void rerun();


    void showStatus() const;

    [[nodiscard]] std::string getTrapName() const;

    [[nodiscard]] bool isExiting() const;

    void setBreakpoint(Breakpoint &bp);

    [[nodiscard]] bool isTrappedAtBreakpoint() const;

    void clearLoadedElf();

    bool breakpointAtAddress(const std::string &strAddress);

    bool breakpointAtAddress(addr_t address);

    void printBreakpointsMap() const;

    [[nodiscard]] std::string dumpAt(addr_t address, addr_t offset = 20) const;

    [[nodiscard]] std::string dumpAtCurrent(addr_t offset = 60) const;

    [[nodiscard]] addr_t getElfIP() const;

    [[nodiscard]] Breakpoint &getHitBreakpoint();
};

#endif //C_BDD_BDD_PTRACE_HPP
