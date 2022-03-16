//
// Created by byjtew on 12/03/2022.
//

#include "bdd_ptrace.hpp"

void TracedProgram::initChild(std::vector<char *> &parameters) {
  int status;
  ptrace(PTRACE_TRACEME, &status, 0);
  ExclusiveIO::info_f("ready, pid=%u\n", getpid());
  parameters.push_back(nullptr);
  execve(elf_file_path.c_str(), parameters.data(), nullptr);
  ExclusiveIO::info_f("exit.\n");
}

void TracedProgram::initBDD() {
  int status;
  attachBDD(status);
  ram_start_address = getTracedRAMAddress();
  elf_file = elf::ElfFile(elf_file_path);
  ExclusiveIO::info_f("ready.\n");
}

void TracedProgram::attachBDD(int &status) {
  ptrace(PTRACE_ATTACH, traced_pid);
  waitpid(traced_pid, &status, 0);
  ptrace(PTRACE_SETOPTIONS, traced_pid, 0, PTRACE_O_TRACEEXIT);
  ptraceContinue(false);
}

TracedProgram::TracedProgram(const std::string &exec_path, std::vector<char *> &parameters) {
  elf_file_path = exec_path;

  ExclusiveIO::initialize(getpid());

  rerun(parameters);
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
      "=================\nisAlive(): %d\nisStopped(): %d\nisTrapped(): %d\nisExiting(): %d\n=================\n",
      isAlive(), isStopped(), isTrapped(), isExiting());
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

bool TracedProgram::isTrapped() const {
  if (!isStopped()) return false;
  return WSTOPSIG(cached_status) == SIGTRAP;
}

bool TracedProgram::isExiting() const {
  if (!isTrapped()) return false;
  auto event = (cached_status >> 16) & 0xffff;
  return event == PTRACE_EVENT_EXIT;
}

// TODO: TracedProgram::getTrapName()
std::string TracedProgram::getTrapName() const {
  return "Not implemented.";
}

void TracedProgram::stop() const {
  if (isDead()) return;
  ExclusiveIO::debug_f("Killing the program.\n");
  kill(traced_pid, SIGKILL);
}

void TracedProgram::rerun(std::vector<char *> &parameters) {
  if (isAlive()) stop();
  clearLoadedElf();
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

void TracedProgram::rerun() {
  std::vector<char *> args_empty;
  rerun(args_empty);
}

void TracedProgram::clearLoadedElf() {
  // TODO: Well...
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

constexpr auto objdump_cmd_format = "objdump -C -D -S -w --start-address=0x%016lX --stop-address=0x%016lX %s | tail -n+6";

std::string TracedProgram::dumpAt(addr_t address, addr_t offset) const {
  std::string cmd;
  cmd.resize(256);
  auto size = sprintf(cmd.data(), objdump_cmd_format, address - offset / 2, address + 5 * offset,
                      elf_file_path.c_str());
  cmd.resize(size);
  return getOutputFromExec(cmd.c_str());
}

std::string TracedProgram::dumpAtCurrent(addr_t offset) const {
  return dumpAt(getElfIP(), offset);
}





