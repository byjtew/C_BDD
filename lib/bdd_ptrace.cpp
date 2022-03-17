//
// Created by byjtew on 12/03/2022.
//

#include <sys/user.h>
#include "bdd_ptrace.hpp"

void TracedProgram::initChild(std::vector<char *> &parameters) {
  usleep(50000); // 50 ms wait, so the BDD (parent process) can attach.
  ExclusiveIO::debug_f("TracedProgram::initChild()\n");
  int status;
  ptrace(PTRACE_TRACEME, &status, 0);
  parameters.push_back(nullptr);
  ExclusiveIO::info_f("ready, pid=%u\n", getpid());
  execve(elf_file_path.c_str(), parameters.data(), nullptr);
  ExclusiveIO::info_f("exit.\n");
}

void TracedProgram::initBDD() {
  ExclusiveIO::debug_f("TracedProgram::initBDD()\n");
  int status;
  attachPtrace(status);
  ram_start_address = getTracedRAMAddress();
  elf_file = elf::ElfFile(elf_file_path);
  ExclusiveIO::info_f("ready.\n");
}

void TracedProgram::attachUnwind() {
  ExclusiveIO::debug_f("TracedProgram::attachUnwind()\n");
  auto as = unw_create_addr_space(&_UPT_accessors, 0);
  auto context = _UPT_create(traced_pid);
  if (unw_init_remote(&unwind_cursor, as, context) != 0) {
    ExclusiveIO::debugError_f("TracedProgram::attachUnwind(): cannot initialize cursor for remote unwinding\n");
    throw std::invalid_argument("TracedProgram::attachUnwind(): cannot initialize cursor for remote unwinding\n");
  }
}

void TracedProgram::attachPtrace(int &status) {
  ExclusiveIO::debug_f("TracedProgram::attachPtrace()\n");
  ptrace(PTRACE_ATTACH, traced_pid);
  waitpid(traced_pid, &status, 0);
  ptrace(PTRACE_SETOPTIONS, traced_pid, 0, PTRACE_O_TRACEEXIT);
  ptraceContinue(false);
}

TracedProgram::TracedProgram(const std::string &exec_path, std::vector<char *> &parameters) {
  elf_file_path = exec_path;

  ExclusiveIO::initialize(getpid());

  run(parameters);
}


void TracedProgram::resumeBreakpoint() {
  auto bp = getHitBreakpoint();
  bp.disable();
  ptraceBackwardStep();
  ptraceStep();
  bp.enable();
}

void TracedProgram::ptraceContinue(bool lock) {
  if (isTrappedAtBreakpoint())
    resumeBreakpoint();

  ExclusiveIO::debug_f("TracedProgram::ptraceContinue(): locking.\n");
  if (lock)
    ExclusiveIO::lockPrint();
  ptrace(PTRACE_CONT, traced_pid, 0, 0);
  waitAndUpdateStatus();
  if (lock)
    ExclusiveIO::unlockPrint();
  ExclusiveIO::debug_f("TracedProgram::ptraceContinue(): unlocking.\n");
}


void TracedProgram::ptraceBackwardStep() const {
  long rc = ptrace(PTRACE_POKEUSER, traced_pid, sizeof(addr_t) * RIP, getIP());
  ExclusiveIO::debug_f("TracedProgram::ptraceBackwardStep(): %d\n", rc);
}

void TracedProgram::ptraceStep() {
  ExclusiveIO::debug_f("TracedProgram::ptraceStep()\n");
  ExclusiveIO::lockPrint();
  long rc = ptrace(PTRACE_SINGLESTEP, traced_pid, 0, 0);
  waitAndUpdateStatus();
  ExclusiveIO::unlockPrint();
  ExclusiveIO::debug_f("TracedProgram::ptraceStep(): %ld\n", rc);
  showStatus();
}

void TracedProgram::showStatus() const {
  ExclusiveIO::info_f(
      "=================\nisAlive(): %d\nisStopped(): %d\nisTrapped(): %d\nisSegfault(): %d\nisExiting(): %d\n=================\n",
      isAlive(), isStopped(), isTrapped(), isSegfault(), isExiting());
  if (isSegfault()) {
    auto data = getSegfaultData();
    ExclusiveIO::debug_f("Segfault information: %s (0x%016lX)\n", data.first.c_str(), data.second);
  }
}

void TracedProgram::waitAndUpdateStatus() {
  waitpid(traced_pid, &cached_status, 0);
}

bool TracedProgram::isDead() const {
  return WIFEXITED(cached_status);
}


bool TracedProgram::isAlive() const {
  return !isDead();
}

bool TracedProgram::isStopped() const {
  return WIFSTOPPED(cached_status);
}

bool TracedProgram::isSegfault() const {
  if (!isStopped()) return false;
  return WSTOPSIG(cached_status) == SIGSEGV;
}

bool TracedProgram::isTrapped() const {
  if (!isStopped()) return false;
  return WSTOPSIG(cached_status) == SIGTRAP;
}

bool TracedProgram::isExiting() const {
  if (!isTrapped()) return false;
  auto event = (cached_status >> 16) & 0xffff;
  return event == PTRACE_EVENT_EXIT;
}

void TracedProgram::stopTraced() const {
  if (isDead()) return;
  ExclusiveIO::debug_f("Stopping the program.\n");
  kill(traced_pid, SIGSTOP);
}

void TracedProgram::killTraced() const {
  if (isDead()) return;
  ExclusiveIO::debug_f("Killing the program.\n");
  kill(traced_pid, SIGKILL);
}

void TracedProgram::run(std::vector<char *> &parameters) {
  if (isAlive())
    clearCurrentProcess();

  do {
    traced_pid = fork();
    switch (traced_pid) {
      case -1:
        std::cerr << "fork_error" << std::endl;
        break;
      case 0:
        initChild(parameters);
        break;
      default:
        initBDD();
        break;
    }
  } while (traced_pid == -1 && errno == EAGAIN);
}

void TracedProgram::run() {
  std::vector<char *> args_empty;
  run(args_empty);
}

void TracedProgram::clearCurrentProcess() {
  stopTraced();
  usleep(200000); // 200 ms
  if (isAlive())
    killTraced();
  breakpointsMap.clear();
  ram_start_address = 0;
  traced_pid = 0;
  cached_status = 0;
}

addr_t TracedProgram::getTracedRAMAddress() const {
  if (ram_start_address > 0) return ram_start_address;
  std::string buffer;
  buffer.resize(24);
  auto size_path = snprintf(buffer.data(), buffer.size(), "/proc/%d/maps", traced_pid);
  buffer.resize(size_path);
  std::ifstream input(buffer);
  if (input.fail())
    throw std::invalid_argument("Bad input: TracedProgram::getTracedRAMAddress(): file.initialize() failed");
  buffer.clear();
  getline(input, buffer);
  input.close();
  buffer = buffer.substr(0, buffer.find('-'));
  buffer.insert(0, "0x");
  return strtoul(buffer.c_str(), (char **) nullptr, 0);
}

std::string getOutputFromExec(const char *cmd) {
  ExclusiveIO::debug_nf("::getOutputFromExec: ", cmd, "\n");
  std::string result;
  char buffer[256];
  char *read_data;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe)
    throw std::runtime_error("popen() failed!");
  do {
    read_data = fgets(buffer, 256, pipe.get());
    if (read_data != nullptr)
      result.append(std::string(read_data));
  } while (read_data != nullptr);
  return result;
}

constexpr auto objdump_cmd_format = "objdump -C -D -S -l -w --start-address=0x%016lX --stop-address=0x%016lX %s | tail -n+6";

std::string TracedProgram::dumpAt(addr_t address, addr_t offset) const {
  std::string cmd;
  cmd.resize(256);
  auto size = sprintf(cmd.data(), objdump_cmd_format, address - 2, address + offset,
                      elf_file_path.c_str());
  cmd.resize(size);
  return getOutputFromExec(cmd.c_str());
}

std::string TracedProgram::dumpAtCurrent(addr_t offset) const {
  return dumpAt(getElfIP(), offset);
}

std::queue<std::pair<addr_t, std::string>> TracedProgram::backtrace() {
  ExclusiveIO::debug_f("TracedProgram::backtrace()\n");
  std::queue<std::pair<addr_t, std::string>> queue;
  unw_word_t offset, pc;
  char sym[256];

  attachUnwind();

  do {
    if (unw_get_reg(&unwind_cursor, UNW_REG_IP, &pc)) {
      ExclusiveIO::debugError_f("TracedProgram::backtrace(): cannot read program counter\n");
      return queue;
    }

    if (unw_get_proc_name(&unwind_cursor, sym, sizeof(sym), &offset) == 0) {
      ExclusiveIO::debug_f("TracedProgram::backtrace(): (%s+0x%016lx)\n", sym, offset);
      queue.push(std::make_pair(offset, sym));
    } else
      ExclusiveIO::debugError_f("TracedProgram::backtrace(): no symbol name found\n");
  } while (unw_step(&unwind_cursor) > 0 && queue.size() < max_stack_size);

  return queue;
}

void TracedProgram::printSiginfo_t(const siginfo_t &info) {
  ExclusiveIO::debug_f("TracedProgram::printSiginfo_t():\n- code: \t%d\n- errno: \t%d\n- signo: \t%d\n", info.si_code,
                       info.si_errno, info.si_signo);
}

std::pair<std::string, addr_t> TracedProgram::getSegfaultData() const {
  ExclusiveIO::debug_f("TracedProgram::getSegfaultData()\n");
  siginfo_t info;
  auto pc = ptrace(PTRACE_GETSIGINFO, traced_pid, nullptr, &info);
  if (pc < 0) return std::make_pair("Error fetching signal information", 0);
  ExclusiveIO::debug_f("TracedProgram::getSegfaultData(): ptrace(PTRACE_GETSIGINFO, ...) => %d\n", pc);
  printSiginfo_t(info);
  return std::make_pair<>(getSegfaultCodeAsString(info), (addr_t) info.si_addr);
}

std::string TracedProgram::getSegfaultCodeAsString(siginfo_t &info) {
  switch (info.si_signo) {
    case SIGSEGV:
      switch (info.si_code) {
        case SEGV_MAPERR:
          return "SEGV_MAPERR: Address not mapped";
        case SEGV_ACCERR:
          return "SEGV_ACCERR: Invalid permissions";
        default:
          return "SIGSEGV: Unknown code " + std::to_string(info.si_code);
      }
    case SIGFPE:
      switch (info.si_code) {
        case FPE_INTDIV:
          return "FPE_INTDIV: Integer divide-by-zero";
        case FPE_INTOVF:
          return "FPE_INTOVF: Integer overflow";
        case FPE_FLTDIV:
          return "FPE_FLTDIV: Floating point divide-by-zero";
        case FPE_FLTOVF:
          return "FPE_FLTOVF: Floating point overflow";
        case FPE_FLTUND:
          return "FPE_FLTUND: Floating point underflow";
        case FPE_FLTRES:
          return "FPE_FLTRES: Floating point inexact result";
        case FPE_FLTINV:
          return "FPE_FLTINV: Invalid floating point operation";
        case FPE_FLTSUB:
          return "FPE_FLTSUB: Subscript out of range";
        default:
          return "SIGFPE: Unknown code " + std::to_string(info.si_code);
      }
    case SIGILL:
      switch (info.si_code) {
        case ILL_ILLOPC:
          return "ILL_ILLOPC: Illegal opcode";
        case ILL_ILLOPN:
          return "ILL_ILLOPN: Illegal operand";
        case ILL_ILLADR:
          return "ILL_ILLADR: Illegal addressing mode";
        case ILL_ILLTRP:
          return "ILL_ILLTRP: Illegal trap";
        case ILL_PRVOPC:
          return "ILL_PRVOPC: Privileged opcode";
        case ILL_PRVREG:
          return "ILL_PRVREG: Privileged register";
        case ILL_COPROC:
          return "ILL_COPROC: Coprocessor error";
        case ILL_BADSTK:
          return "ILL_BADSTK: Internal stack error";
        default:
          return "SIGILL: Unknown code " + std::to_string(info.si_code);
      }
    case SIGBUS:
      switch (info.si_code) {
        case BUS_ADRALN:
          return "BUS_ADRALN: Invalid address alignment";
        case BUS_ADRERR:
          return "BUS_ADRERR: Non-existent physical address";
        case BUS_OBJERR:
          return "BUS_OBJERR: Object-specific hardware error";
        default:
          return "SIGBUS: Unknown code " + std::to_string(info.si_code);
      }
    default:
      return "[SIGNAL " + std::to_string(info.si_signo) + "]: Unknown";
  }
}

std::optional<user_regs_struct> TracedProgram::getRegisters() const {
  user_regs_struct regs{};
  auto pc = ptrace(PTRACE_GETREGS, traced_pid, nullptr, &regs);
  if (pc < 0) return std::nullopt;
  return regs;
}

