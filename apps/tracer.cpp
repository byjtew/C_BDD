//
// Created by byjtew on 08/03/2022.
//

#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cerrno>
#include <sys/ptrace.h>
#include <linux/ptrace.h>
#include <sys/wait.h>
#include <vector>

void trap_inferior_continue(const pid_t &traced) {
  pid_t inferior_pid = traced;
  ptrace(static_cast<__ptrace_request>(PTRACE_CONT), inferior_pid, &inferior_pid, &inferior_pid);
}


static void attach_to_traced(const pid_t &traced) {
  std::string choice;
  choice.reserve(20);
  int ignored = 0;
  for (;;) {
    int status;
    waitpid(traced, &status, 0);

    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
      std::cout << "Inferior stopped on SIGTRAP:" << std::endl << "Continue [y/n]: " << std::flush;
      std::cin >> choice;
      if (choice == "n") {
        printf("\nkillin' da baby...\n");
        ptrace(static_cast<__ptrace_request>(PTRACE_KILL), traced, ignored, ignored);
        break;
      } else {
        printf("\ncontinuing...\n");
        ptrace(static_cast<__ptrace_request>(PTRACE_CONT), traced, ignored, ignored);
      }
      //ptrace(PTRACE_SINGLESTEP, pid, 0, 0);
    } else if (WIFEXITED(status)) {
      printf("Inferior exited - debugger terminating...\n");
      exit(0);
    }
  }
}

void setup_traced(const std::string &elf_filepath) {
  pid_t ignored = 0;
  int ignored_ptr;
  ptrace(static_cast<__ptrace_request>(PTRACE_TRACEME), ignored, &ignored, &ignored);
  std::vector<char *> args;
  args.push_back(nullptr);
  execv(elf_filepath.c_str(), args.data());
}

void runAndTrace(const std::string &elf_filepath) {
  std::cout << "> runAndTrace(" << elf_filepath << ")" << std::endl;

  pid_t traced;
  do {
    traced = fork();
    switch (traced) {
      case -1:  // error
        break;
      case 0:   // inferior
        std::cout << "Child pid: " << getpid() << std::endl;
        setup_traced(elf_filepath);
        break;
      default:  // debugger
        attach_to_traced(traced);
    }
  } while (traced == -1 && errno == EAGAIN);
  std::cout << "I am out of the switch, my pid is: " << getpid() << std::endl;
  trap_inferior_continue(traced);
  std::cout << "< runAndTrace()" << std::endl;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <program|elf-file>" << std::endl;
    exit(1);
  }
  std::cout << "Parent pid: " << getpid() << std::endl;
  runAndTrace(argv[1]);
  return 0;
}
