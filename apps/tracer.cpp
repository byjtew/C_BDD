//
// Created by byjtew on 08/03/2022.
//

#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <vector>

#include "tracing.hpp"

void command_loop(TracedProgram &traced) {
  bool force_end = false;
  std::string choice, choice_param;
  choice.reserve(20);
  TracedProgram::processPrint("Debug ready.\n");
  do {
    choice.clear();
    choice_param.clear();
    if (traced.isExiting())
      TracedProgram::processPrint("The program exited normally.\n");
    else if (traced.isTrappedAtBreakpoint()) {
      TracedProgram::processPrint("The program hit a breakpoint.\n");
    }

    TracedProgram::processPrint("$ : ");
    TracedProgram::processScan(choice);

    if (choice == "run") {
      if (traced.isDead() || traced.isExiting()) {
        TracedProgram::processPrint("Re-run the program [Y/n]: ");
        TracedProgram::processScan(choice);
        if (choice_param == "y" || choice_param == "Y")
          traced.rerun();

      } else {
        TracedProgram::processPrint("Continuing program.\n");
        traced.ptraceContinue();
      }
    } else if (choice.starts_with("bp")) {
      TracedProgram::processScan(choice_param);

      if (choice_param.starts_with("0x")) { // Hex choice
        if (!traced.breakpointAtAddress(strtoul(choice_param.c_str(), (char **) 0, 0)))
          TracedProgram::processPerror("Breakpoint[%s] failed: wrong address.\n");
        else
          TracedProgram::processPrint("Breakpoint[%s] placed.\n", choice_param.c_str());
      } else { // Name choice
        if (!traced.breakpointAtFunction(choice_param))
          TracedProgram::processPerror("Breakpoint[%s] failed: function does not exists.\n");
        else
          TracedProgram::processPrint("Breakpoint[%s] placed.\n", choice_param.c_str());
      }


    } else if (choice == "step") {
      TracedProgram::processPrint("Stepping program.\n");
      traced.ptraceStep();
    } else if (choice == "stop") {
      TracedProgram::processPrint("Killing program.\n");
      traced.stop();
      force_end = true;
    } else {
      TracedProgram::processPrint("Unknown command.\n");
    }
  } while (traced.isAlive() && !force_end);
  TracedProgram::processPrint("End of the debug.\n");
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
  //command_loop(traced);
  traced.stop();
  return 0;
}
