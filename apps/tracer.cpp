//
// Created by byjtew on 08/03/2022.
//

#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <vector>

#include "bdd_ptrace.hpp"
#include "bdd_exclusive_io.hpp"

void command_loop(TracedProgram &traced) {
  bool force_end = false;
  std::string choice, choice_param;
  choice.reserve(20);
  ExclusiveIO::info("Debug ready.\n");
  do {
    choice.clear();
    choice_param.clear();
    if (traced.isExiting())
      ExclusiveIO::info("The program exited normally.\n");
    else if (traced.isTrappedAtBreakpoint()) {
      ExclusiveIO::info("The program hit a breakpoint.\n");
    }

    ExclusiveIO::info("$ : ");
    ExclusiveIO::input(choice);

    if (choice == "run") {
      if (traced.isDead() || traced.isExiting()) {
        ExclusiveIO::info("Re-run the program [Y/n]: ");
        ExclusiveIO::input(choice_param);
        if (choice_param == "y" || choice_param == "Y")
          traced.rerun();

      } else {
        ExclusiveIO::info("Continuing program.\n");
        traced.ptraceContinue();
      }
    } else if (choice.starts_with("bp")) {
      ExclusiveIO::input(choice_param);

      if (choice_param.starts_with("0x")) { // Hex choice
        ExclusiveIO::debug("Placing bp by address at: 0x%016lX\n", choice_param.c_str());
        if (!traced.breakpointAtAddress(choice_param))
          ExclusiveIO::error("Breakpoint[%s] failed: wrong address.\n");
        else
          ExclusiveIO::info("Breakpoint[%s] placed.\n", choice_param.c_str());
      } else { // Name choice
        ExclusiveIO::debug("Placing bp by function name: %s\n", choice_param.c_str());
        if (!traced.breakpointAtFunction(choice_param))
          ExclusiveIO::error("Breakpoint[%s] failed: function does not exists.\n");
        else
          ExclusiveIO::info("Breakpoint[%s] placed.\n", choice_param.c_str());
      }
    } else if (choice == "ip" || choice == "rip" || choice == "eip") {
      ExclusiveIO::info("Current pointer address: 0x%016lX\n", traced.getIP());
    } else if (choice == "step") {
      ExclusiveIO::info("Stepping program.\n");
      traced.ptraceStep();
    } else if (choice == "stop") {
      ExclusiveIO::info("Killing program.\n");
      traced.stop();
      force_end = true;
    } else {
      ExclusiveIO::info("Unknown command.\n");
    }
  } while (traced.isAlive() && !force_end);
  ExclusiveIO::info("End of the debug.\n");
}


int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <program|elf-file>" << std::endl;
    exit(1);
  }
  std::vector<char *> params;
  for (size_t i = 2; i < argc; i++)
    params.emplace_back(argv[i]);
  auto traced = TracedProgram(std::string(argv[1]), params);
  command_loop(traced);
  traced.stop();
  return 0;
}
