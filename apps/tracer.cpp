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
  std::string choice;
  choice.reserve(20);
  TracedProgram::processPrint("Debug ready.\n");
  do {
    if (traced.isExiting())
      TracedProgram::processPrint("The program exited normally.\n");
    TracedProgram::processPrint("$ : ");
    TracedProgram::processScan(choice);
    if (choice == "run") {
      TracedProgram::processPrint("Running program.\n");
      traced.ptraceContinue();
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
  command_loop(traced);
  return 0;
}
