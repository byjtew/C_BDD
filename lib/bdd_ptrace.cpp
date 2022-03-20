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
  char *args[32];
  args[0] = elf_file_path.data();
  args[parameters.size() + 1] = nullptr;
  ExclusiveIO::debug_f("Executing %s ", elf_file_path.c_str());
  for (unsigned i = 0; i < parameters.size(); i++) {
    args[i + 1] = parameters.at(i);
    ExclusiveIO::debug_f("%s ", args[i + 1]);
  }
  std::cerr << std::endl;
  ExclusiveIO::info_f("ready, pid=%u\n", getpid());
  execv(elf_file_path.c_str(), args);
  ExclusiveIO::info_f("exit.\n");
}

void TracedProgram::initBDD() {
  ExclusiveIO::debug_f("TracedProgram::initBDD()\n");
  int status;
  attachPtrace(status);
  ram_start_address = getTracedRAMAddress();
  placeEveryPendingBreakpoints();
  ExclusiveIO::info_f("ready.\n");
}


void TracedProgram::attachPtrace(int &status) {
  ExclusiveIO::debug_f("TracedProgram::attachPtrace()\n");
  ptrace(PTRACE_ATTACH, traced_pid);
  waitpid(traced_pid, &status, 0);
  ptrace(PTRACE_SETOPTIONS, traced_pid, 0, PTRACE_O_TRACEEXIT);
  ptraceContinue(false);
}

TracedProgram::TracedProgram(const std::string &exec_path) {
  elf_file_path = exec_path;
  elf_file = elf::ElfFile(elf_file_path);
  ExclusiveIO::initialize(getpid());
}


void TracedProgram::resumeBreakpoint() {
  auto bp = getHitBreakpoint();
  bp.disable();
  ptraceBackwardStep();
  ptraceRawStep();
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
  if (isTrappedAtBreakpoint())
    resumeBreakpoint();
  else
    ptraceRawStep();
  showStatus();
}

addr_t TracedProgram::ptraceRawStep() {
  ExclusiveIO::debug_f("TracedProgram::ptraceRawStep()\n");
  ExclusiveIO::lockPrint();
  addr_t rc = ptrace(PTRACE_SINGLESTEP, traced_pid, 0, 0);
  waitAndUpdateStatus();
  ExclusiveIO::unlockPrint();
  return rc;
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

void TracedProgram::stopTraced() const {
  if (isDead()) return;
  ExclusiveIO::debug_f("Stopping the program.\n");
  kill(traced_pid, SIGINT);
}

void TracedProgram::killTraced() const {
  if (isDead()) return;
  ExclusiveIO::debug_f("Killing the program.\n");
  kill(traced_pid, SIGKILL);
}

void TracedProgram::run(std::vector<char *> &parameters) {
  if (isAlive()) {
    std::cerr << " isAlive()" << std::endl;
    clearCurrentProcess();
  }
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


std::optional<user_regs_struct> TracedProgram::getRegisters() const {
  user_regs_struct regs{};
  auto pc = ptrace(PTRACE_GETREGS, traced_pid, nullptr, &regs);
  if (pc < 0) return std::nullopt;
  return regs;
}


bool TracedProgram::hasStarted() const {
  return ram_start_address > 0;
}
