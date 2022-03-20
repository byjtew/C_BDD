//
// Created by byjtew on 12/03/2022.
//

#ifndef C_BDD_BDD_PTRACE_HPP
#define C_BDD_BDD_PTRACE_HPP

constexpr auto BDD_VERSION = "1.0b";


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
#include <libunwind-ptrace.h>
#include <queue>
#include <sys/user.h>

#include "bdd_elf.hpp"
#include "bdd_exclusive_io.hpp"

constexpr unsigned max_stack_size = 256;


using instr_t = long;

class Breakpoint {
private:
    bool enabled = false;
    pid_t program_pid;
    addr_t address;
    instr_t original;
    std::string name;

public:

    explicit Breakpoint(pid_t pid, const addr_t &addr, const std::string &func_name = "Unknown");

    [[nodiscard]] bool isEnabled() const { return enabled; }

    [[nodiscard]] addr_t getAddress() const {
      return address;
    }

    /**
     * @return the original instruction
     */
    [[nodiscard]] instr_t getOriginal() const {
      return original;
    }

    /**
     * @return The associated function-name if existing else 'Unknown'
     */
    [[nodiscard]] std::string getName() const {
      return name;
    }

    bool enable();

    void disable();
};

constexpr auto objdump_cmd_format = "objdump -C -D -S -l -w --start-address=0x%016lX --stop-address=0x%016lX %s | tail -n+6";


class TracedProgram {
private:
    // Libunwind: backtrace purpose
    unw_cursor_t unwind_cursor{};

    // Physical first address of the program
    addr_t ram_start_address = 0;

    // Executable file path
    std::string elf_file_path;

    // Elf class object
    elf::ElfFile elf_file;

    pid_t traced_pid{};
    int cached_status = 0;

    // Every breakpoint placed (enabled or not)
    std::map<addr_t, Breakpoint> breakpointsMap;

    std::vector<Breakpoint> pendingBreakpointsMap;


    void initChild(std::vector<char *> &parameters);

    void initBDD();

    void waitAndUpdateStatus();

    void attachUnwind();

    void attachPtrace(int &status);

    [[nodiscard]] addr_t getTracedRAMAddress() const;

    /**
     * Direct instruction backward-step without breakpoint handling.
     * Used in continue & step functions when handling a breakpoint
     */
    void ptraceBackwardStep() const;

    /**
     * If breakpoint:\n
     * \t backward-step \n
     * \t breakpoint disabled\n
     * \t step\n
     * \t breakpoint enabled
     */
    void resumeBreakpoint();

    static void printSiginfo_t(const siginfo_t &info);

    static std::string getSegfaultCodeAsString(siginfo_t &info);

    /**
     * Direct instruction step without breakpoint handling
     * @return the address of the IP after the step
     */
    addr_t ptraceRawStep();

    [[nodiscard]] addr_t getFunctionPhysicalAddress(const std::string &fctName) const;

    void clearCurrentProcess();

    static addr_t strAddr_tToHex(const std::string &strAddress);

public:
    explicit TracedProgram(const std::string &exec_path);


    ~TracedProgram() {
      ptrace(PTRACE_DETACH, traced_pid, 0, 0);
      ExclusiveIO::terminate();
    }

#pragma region breakpoints

    [[nodiscard]] bool breakpointAtFunction(const std::string &fctName);

    /**
     * @return current physical address of the (E|R)IP
     */
    [[nodiscard]] addr_t getIP() const;

#pragma endregion breakpoints

    /**
     * Run the traced-program to the end (except segfaults/breakpoints/...)
     */
    void ptraceContinue(bool lock = true);

    /**
     * Steps one instruction in the traced-program (with automated breakpoints handling)
     */
    void ptraceStep();

#pragma region Status-related

    /**
     * Overview of the traced-program
     */
    void showStatus() const;

    [[nodiscard]] bool isExiting() const;

    [[nodiscard]] bool isSegfault() const;

    [[nodiscard]] bool isTrappedAtBreakpoint() const;

    [[nodiscard]] bool isDead() const;

    [[nodiscard]] bool isAlive() const;

    [[nodiscard]] bool isStopped() const;

    [[nodiscard]] bool isTrapped() const;

#pragma endregion

    [[nodiscard]] elf::ElfFile getElfFile() const {
      return elf_file;
    }

    /**
     * Send a SIGINT to the traced-program
     */
    void stopTraced() const;

    /**
     * Send a SIGKILL to the traced-program
     */
    void killTraced() const;

    /**
     * Execute the traced-program
     * @param parameters
     */
    void run(std::vector<char *> &parameters);

    /**
     * Execute the traced-program
     * @param parameters
     */
    void run();

    /**
     * Adds a breakpoint to the map
     * @param breakpoint
     */
    void setBreakpoint(Breakpoint &breakpoint);

    /**
     * Try to place & enable a breakpoint at the specified location
     * @param strAddress address 0x..... as string type
     * @return success
     */
    bool breakpointAtAddress(const std::string &strAddress);

    /**
     * Try to place & enable a breakpoint at the specified location
     * @param address address where to breakpoint
     * @param func_name related function if any
     * @return success
     */
    [[nodiscard]] bool breakpointAtAddress(addr_t address, std::optional<std::string> func_name);

    void printBreakpointsMap() const;

    /**
     * Dump the assembly program at the specified location
     * @param address location
     * @param offset area of the dump
     * @return parsed dump
     */
    [[nodiscard]] std::string dumpAt(addr_t address, addr_t offset = 20) const;

    /**
     * Dump the assembly program at the current pointer location
     * @param offset area of the dump
     * @return
     */
    [[nodiscard]] std::string dumpAtCurrent(addr_t offset = 60) const;

    /**
     * @return the (E|R)IP as an Elf offset (virtual memory address)
     */
    [[nodiscard]] addr_t getElfIP() const;

    /**
     * @return current breakpoint hit by the (E|R)IP
     */
    [[nodiscard]] Breakpoint &getHitBreakpoint();

    /**
     * Backtrace API
     * @return the parsed backtrace
     */
    [[nodiscard]] std::queue<std::pair<addr_t, std::string>> backtrace();

    /**
     * @return details over the current segfault
     */
    [[nodiscard]] std::pair<std::string, addr_t> getSegfaultData() const;

    [[nodiscard]] std::optional<user_regs_struct> getRegisters() const;

    /**
     * Try to disable a breakpoint at the specified location
     */
    [[nodiscard]] bool disableBreakpointAtFunction(const std::string &func_name);

    /**
     * Try to disable a breakpoint at the specified location
     */
    [[nodiscard]] bool disableBreakpointAtAddress(const std::string &hex_addr_as_str);

    /**
     * Enables every pending breakpoints, happen if any 'bp' has been required before the first 'run'
     * @return the status for each breakpoint
     */
    std::vector<std::pair<Breakpoint, bool>> placeEveryPendingBreakpoints();

    [[nodiscard]] bool hasStarted() const;
};

#endif //C_BDD_BDD_PTRACE_HPP
