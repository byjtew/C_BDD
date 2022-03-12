//
// Created by byjtew on 12/03/2022.
//
#include <ostream>
#include <sys/ptrace.h>
#include <execution>
#include <iostream>
#include <stdio.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <cassert>
#include <fstream>
#include <cstdarg>
#include <fcntl.h>

#include "tracing.hpp"

void TracedProgram::initChild(const std::string &exec_path, std::vector<char *> &parameters) {
  int status;
  ptrace(PTRACE_TRACEME, &status, 0);
  processPrint("start.\n");
  parameters.push_back(nullptr);
  execve(exec_path.c_str(), parameters.data(), nullptr);
  processPrint("exit.\n");
}

void TracedProgram::initBDD(const std::string &exec_path) {
  int status;
  ptrace(PTRACE_ATTACH, traced_pid);
  waitpid(traced_pid, &status, 0);
  ptrace(PTRACE_SETOPTIONS, traced_pid, 0, PTRACE_O_TRACEEXIT);
  elf_file = elf::ElfFile(exec_path);
  processPrint("ready.\n");
  ptraceContinue();
}


TracedProgram::TracedProgram(const std::string &exec_path, std::vector<char *> &parameters) {
  bdd_pid = getpid();

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
  int rc = pthread_mutexattr_init(&mutexattr);
  if (rc != 0)
    throw std::invalid_argument("pthread_mutexattr_init");
  rc = pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
  if (rc != 0)
    throw std::invalid_argument("pthread_mutexattr_setpshared");
  rc = pthread_mutex_init(&debug_synchronisation_map->print_mutex, &mutexattr);
  if (rc != 0)
    throw std::invalid_argument("pthread_mutex_init");

  do {
    traced_pid = fork();
    switch (traced_pid) {
      case -1:
        std::cerr << "fork_error" << std::endl;
        break;
      case 0:
        initChild(exec_path, parameters);
        break;
      default:
        initBDD(exec_path);
        break;
    }
  } while (traced_pid == -1 && errno == EAGAIN);
}


void TracedProgram::ptraceContinue() {
  int rc = ptrace(PTRACE_CONT, traced_pid, 0, 0);
  TracedProgram::processPrint("ptraceContinue(): %d\n", rc);
  getProcessStatus();
}

void TracedProgram::ptraceStep() {
  int rc = ptrace(PTRACE_SINGLESTEP, traced_pid, 0, 0);
  TracedProgram::processPrint("ptraceStep(): %d\n", rc);
  getProcessStatus();
}

void TracedProgram::showStatus() const {
  processPrint("=================\n");
  processPrint("isAlive(): %d\n", isAlive());
  processPrint("isStopped(): %d\n", isStopped());
  processPrint("isExiting(): %d\n", isExiting());
  //processPrint("isAlive(): %s\n", isAlive());
  processPrint("=================\n");
}

void TracedProgram::getProcessStatus() {
  TracedProgram::processPrint("> getProcessStatus()\n");
  waitpid(traced_pid, &cached_status, 0);
  TracedProgram::processPrint("< getProcessStatus(): %d\n", cached_status);
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

void TracedProgram::processScan(std::string &value) {
  lockPrint();
  std::cin >> value;
  unlockPrint();
}
