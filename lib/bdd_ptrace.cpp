//
// Created by byjtew on 12/03/2022.
//

#include <fstream>
#include "bdd_ptrace.hpp"

void TracedProgram::initChild(std::vector<char *> &parameters) {
  int status;
  ptrace(PTRACE_TRACEME, &status, 0);
  ExclusiveIO::info("ready, pid=%u\n", getpid());
  parameters.push_back(nullptr);
  execve(elf_file_path.c_str(), parameters.data(), nullptr);
  ExclusiveIO::info("exit.\n");
}

void TracedProgram::initBDD() {
  int status;
  attachBDD(status);
  ram_start_address = getTracedRAMAddress();
  elf_file = elf::ElfFile(elf_file_path);
  ExclusiveIO::info("ready.\n");
}

void TracedProgram::attachBDD(int &status) {
  ptrace(PTRACE_ATTACH, traced_pid);
  waitpid(traced_pid, &status, 0);
  ptrace(PTRACE_SETOPTIONS, traced_pid, 0, PTRACE_O_TRACEEXIT);
  ptraceContinue();
}

TracedProgram::TracedProgram(const std::string &exec_path, std::vector<char *> &parameters) {
  elf_file_path = exec_path;

  ExclusiveIO::initialize(getpid());

  rerun(parameters);
}


void TracedProgram::ptraceContinue() {
  long rc = ptrace(PTRACE_CONT, traced_pid, 0, 0);
  ExclusiveIO::debug("ptraceContinue(): %d\n", rc);
  getProcessStatus();
}

void TracedProgram::ptraceStep() {
  long rc = ptrace(PTRACE_SINGLESTEP, traced_pid, 0, 0);
  ExclusiveIO::debug("ptraceStep(): %d\n", rc);
  getProcessStatus();
}

void TracedProgram::showStatus() const {
  ExclusiveIO::info(
      "=================\nisAlive(): %d\nisStopped(): %d\nisTrapped(): %d\nisExiting(): %d\n=================\n",
      isAlive(), isStopped(), isTrapped(), isExiting());
}

void TracedProgram::getProcessStatus() {
  ExclusiveIO::debug("> getProcessStatus()\n");
  waitpid(traced_pid, &cached_status, 0);
  ExclusiveIO::debug("< getProcessStatus(): %d\n", cached_status);
  showStatus();
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
  ExclusiveIO::debug("Killing the program.\n");
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




