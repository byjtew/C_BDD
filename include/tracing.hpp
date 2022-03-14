//
// Created by byjtew on 12/03/2022.
//

#ifndef C_BDD_TRACING_HPP
#define C_BDD_TRACING_HPP

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
#include <mutex>
#include <sys/mman.h>
#include <pthread.h>
#include <stack>
#include <ostream>
#include <sys/ptrace.h>
#include <execution>
#include <iostream>
#include <cstdio>
#include <pthread.h>
#include <sys/mman.h>
#include <cstdarg>
#include <fcntl.h>
#include <fstream>

#include "elf_reader.hpp"

using instr_t = long;


class Breakpoint {
private:
    bool enabled = false;
    pid_t program_pid;
    addr_t address;
    instr_t original;
public:

    Breakpoint(pid_t pid, const addr_t &addr);

    [[nodiscard]] bool enable();

    [[nodiscard]] bool disable();

    [[nodiscard]] bool isEnabled() const { return enabled; }

    [[nodiscard]] addr_t getAddress() const {
      return address;
    }

    [[nodiscard]] instr_t getOriginal() const {
      return original;
    }
};


#define MMAP_NAME         "/tmp/mmap_bdd_stdoutmutex"

typedef struct debug_synchronisation {
    pthread_mutex_t print_mutex;
    pthread_mutex_t log_print_mutex;
} debug_synchronisation_t;

class TracedProgram {
private:
    addr_t ram_start_address = 0;
    std::string elf_file_path;

    inline static debug_synchronisation_t *debug_synchronisation_map;
    inline static FILE *log_fp;

    inline static pid_t bdd_pid;
    elf::ElfFile elf_file;
    pid_t traced_pid;
    int cached_status = 0;

    std::map<addr_t, Breakpoint> breakpointsMap;

    static void lockPrint() {
      pthread_mutex_lock(&debug_synchronisation_map->print_mutex);
    }

    static void unlockPrint() {
      pthread_mutex_unlock(&debug_synchronisation_map->print_mutex);
    }

    static void lockLogPrint() {
      pthread_mutex_lock(&debug_synchronisation_map->log_print_mutex);
    }

    static void unlockLogPrint() {
      pthread_mutex_unlock(&debug_synchronisation_map->log_print_mutex);
    }

    void initChild(std::vector<char *> &parameters);

    void initBDD();

    void getProcessStatus();

    void attachBDD(int &status) const;

    [[nodiscard]] addr_t getTracedRAMAddress() const;

    static FILE *getLogFp();

public:
    TracedProgram(const std::string &exec_path, std::vector<char *> &parameters);


    ~TracedProgram() {
      /*int status;
      if (waitpid(traced_pid, &status, WNOHANG))
        kill(traced_pid, SIGKILL);*/
      munmap(debug_synchronisation_map, sizeof(debug_synchronisation_t));
    }

#pragma region breakpoints

    [[nodiscard]] bool breakpointAtFunction(const std::string &fct_name);

    [[nodiscard]] addr_t getIP() const;

#pragma endregion breakpoints

    void ptraceContinue();

    void ptraceStep();

    [[nodiscard]] bool isDead() const;

    [[nodiscard]] bool isAlive() const;

    [[nodiscard]] bool isStopped() const;

    [[nodiscard]] bool isTrapped() const;

    void stop() const;

    void rerun(std::vector<char *> &args);

    void rerun();

    static void initializePrintExclusion();

    static void processPrint(const std::string &formatformat, ...);

    static void processScan(std::string &value);

    static void processPerror(const std::string &format, ...);

    static void processLog(const std::string &formatformat, ...);

    void showStatus() const;

    [[nodiscard]] std::string getTrapName() const;

    [[nodiscard]] bool isExiting() const;

    void setBreakpoint(Breakpoint &bp);

    [[nodiscard]] bool isTrappedAtBreakpoint() const;


    void clearLoadedElf();


    bool breakpointAtAddress(const std::string &strAddress);

    bool breakpointAtAddress(addr_t address);

    void printBreakpointsMap() const;


};

#endif //C_BDD_TRACING_HPP
