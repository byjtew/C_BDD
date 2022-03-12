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
#include <queue>
#include <stack>
#include "elf_reader.hpp"

#define MMAP_NAME         "/tmp/mmap_bdd_stdoutmutex"

typedef struct debug_synchronisation {
    pthread_mutex_t print_mutex;
} debug_synchronisation_t;

class TracedProgram {
private:
    inline static debug_synchronisation_t *debug_synchronisation_map;
    inline static pid_t bdd_pid;
    elf::ElfFile elf_file;
    pid_t traced_pid;
    int cached_status = 0;

    static void lockPrint() {
      pthread_mutex_lock(&debug_synchronisation_map->print_mutex);
    }

    static void unlockPrint() {
      pthread_mutex_unlock(&debug_synchronisation_map->print_mutex);
    }

    void initChild(const std::string &exec_path, std::vector<char *> &parameters);

    void initBDD(const std::string &exec_path);

    void getProcessStatus();

public:
    TracedProgram(const std::string &exec_path, std::vector<char *> &parameters);


    ~TracedProgram() {
      /*int status;
      if (waitpid(traced_pid, &status, WNOHANG))
        kill(traced_pid, SIGKILL);*/
      munmap(debug_synchronisation_map, sizeof(debug_synchronisation_t));
    }

    void ptraceContinue();

    void ptraceStep();

    bool isDead() const;

    bool isAlive() const;

    bool isStopped() const;

    bool isTrapped() const;

    void stop() const;

    static void processPrint(const std::string &formatformat, ...);

    static void processScan(std::string &value);


    void showStatus() const;

    std::string getTrapName() const;

    bool isExiting() const;
};

#endif //C_BDD_TRACING_HPP
