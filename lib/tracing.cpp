//
// Created by byjtew on 12/03/2022.
//

#include <fstream>
#include "tracing.hpp"

void TracedProgram::initChild(std::vector<char *> &parameters) {
  int status;
  ptrace(PTRACE_TRACEME, &status, 0);
  processPrint("ready, pid=%u\n", getpid());
  parameters.push_back(nullptr);
  execve(elf_file_path.c_str(), parameters.data(), nullptr);
  processPrint("exit.\n");
}

void TracedProgram::initBDD() {
  int status;
  attachBDD(status);
  ram_start_address = getTracedRAMAddress();
  elf_file = elf::ElfFile(elf_file_path);
  processPrint("ready.\n");
  ptraceContinue();
}

void TracedProgram::attachBDD(int &status) const {
  ptrace(PTRACE_ATTACH, traced_pid);
  waitpid(traced_pid, &status, 0);
  ptrace(PTRACE_SETOPTIONS, traced_pid, 0, PTRACE_O_TRACEEXIT);
}

TracedProgram::TracedProgram(const std::string &exec_path, std::vector<char *> &parameters) {
  elf_file_path = exec_path;
  bdd_pid = getpid();

  initializePrintExclusion();

  rerun(parameters);
}

void TracedProgram::initializePrintExclusion() {
  unlink(MMAP_NAME);
  int fd = open(MMAP_NAME, O_CREAT | O_RDWR, 00600);
  ftruncate(fd, sizeof(debug_synchronisation_t));
  debug_synchronisation_map = (debug_synchronisation_t *) mmap(nullptr,
                                                               sizeof(debug_synchronisation_t),
                                                               PROT_READ | PROT_WRITE,
                                                               MAP_SHARED,
                                                               fd,
                                                               0);

  pthread_mutexattr_t mutexattr;
  if (pthread_mutexattr_init(&mutexattr))
    throw std::invalid_argument("pthread_mutexattr_init");
  if (pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED))
    throw std::invalid_argument("pthread_mutexattr_setpshared");
  if (pthread_mutex_init(&debug_synchronisation_map->print_mutex, &mutexattr))
    throw std::invalid_argument("pthread_mutex_init for print_mutex");

  std::string log_filename;
  log_filename.resize(32);
  time_t t = time(nullptr);
  struct tm tm = *localtime(&t);
  snprintf(log_filename.data(), log_filename.size(), "./logs-%d-%02d-%02d %02d:%02d:%02d.log", tm.tm_year + 1900,
           tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

  log_fp = fopen(log_filename.c_str(), "w");
  if (log_fp == nullptr)
    std::cerr << "log_fp creation error" << std::endl;
  else
    std::cout << "log_fp creation success" << std::endl;
  if (pthread_mutex_init(&debug_synchronisation_map->log_print_mutex, &mutexattr))
    throw std::invalid_argument("pthread_mutex_init for log_print_mutex");
}


void TracedProgram::ptraceContinue() {
  long rc = ptrace(PTRACE_CONT, traced_pid, 0, 0);
  TracedProgram::processPrint("ptraceContinue(): %d\n", rc);
  getProcessStatus();
}

void TracedProgram::ptraceStep() {
  long rc = ptrace(PTRACE_SINGLESTEP, traced_pid, 0, 0);
  TracedProgram::processPrint("ptraceStep(): %d\n", rc);
  getProcessStatus();
}

void TracedProgram::showStatus() const {
  processPrint("=================\n");
  processPrint("isAlive(): %d\n", isAlive());
  processPrint("isStopped(): %d\n", isStopped());
  processPrint("isTrapped(): %d\n", isTrapped());
  processPrint("isExiting(): %d\n", isExiting());
  processPrint("=================\n");
}

void TracedProgram::getProcessStatus() {
  TracedProgram::processLog("> getProcessStatus()\n");
  waitpid(traced_pid, &cached_status, 0);
  TracedProgram::processLog("< getProcessStatus(): %d\n", cached_status);
  auto rc = getIP();
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
  processLog("Killing the program.\n");
  kill(traced_pid, SIGKILL);
}

void TracedProgram::processPrint(const std::string &format, ...) {
  bool child = getpid() != bdd_pid;
  std::string reformat = child ? "\033[32m" : "\033[34m";
  if (format.ends_with("\n") && format.length() > 1)
    reformat.append(child ? "[Child]: " : "[BDD]: ");
  reformat.append(format);
  reformat.append("\033[0m");

  lockPrint();
  va_list args;
  va_start(args, format.data());
  fflush(stdout);
  std::string strbuf;
  vfprintf(stdout, reformat.c_str(), args);
  fflush(stdout);
  va_end(args);
  unlockPrint();
}

void TracedProgram::processPerror(const std::string &format, ...) {
  bool child = getpid() != bdd_pid;
  std::string reformat;
  if (format.ends_with("\n") && format.length() > 1)
    reformat.append(child ? "[Child]: " : "[BDD]: ");
  reformat.append(format);

  lockPrint();
  va_list args;
  va_start(args, format.data());
  fflush(stdout);
  std::string strbuf;
  vfprintf(stderr, reformat.c_str(), args);
  va_end(args);
  unlockPrint();
}

void TracedProgram::processLog(const std::string &format, ...) {
  bool child = getpid() != bdd_pid;
  std::string reformat;
  if (format.ends_with("\n") && format.length() > 1)
    reformat.append(child ? "[Child]: " : "[BDD]: ");
  reformat.append(format);

  lockLogPrint();
  va_list args;

  va_start(args, format.data());
  std::string strbuf;
  vfprintf(getLogFp(), reformat.c_str(), args);
  va_end(args);
  unlockLogPrint();
}

void TracedProgram::processScan(std::string &value) {
  lockPrint();
  std::cin >> value;
  unlockPrint();
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
  std::string map_path;
  map_path.resize(20);
  snprintf(map_path.data(), map_path.size(), "/proc/%d/maps", traced_pid);
  std::ifstream input(map_path);
  if (input.fail())
    throw std::invalid_argument("Bad input: TracedProgram::getTracedRAMAddress(): file.open() failed");
  std::string buffer;
  getline(input, buffer);
  input.close();
  auto ram_address = buffer.substr(0, buffer.find('-'));
  ram_address.insert(0, "0x");
  return strtoul(ram_address.c_str(), (char **) nullptr, 0);
}

FILE *TracedProgram::getLogFp() {
  return log_fp;
}



