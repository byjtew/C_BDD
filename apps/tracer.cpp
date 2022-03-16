//
// Created by byjtew on 08/03/2022.
//

#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <vector>

#include "bdd_ptrace.hpp"
#include "bdd_exclusive_io.hpp"

void printFunctionsList(std::vector<std::pair<addr_t, std::string>> &functionsList, bool full_details) {
  ExclusiveIO::debug_f("::printFunctionsList()\n");
  ExclusiveIO::info_f("Functions:\n");
  std::for_each(functionsList.begin(), functionsList.end(), [full_details](const auto &it) {
      if (it.first > 0 || full_details)
        ExclusiveIO::info_f("[0x%016lX]: %s\n", it.first, it.second.c_str());
  });
}

void printStack(std::queue<std::pair<addr_t, std::string>> &parsedStack) {
  ExclusiveIO::debug_f("::printStack()\n");
  std::string msg, row;
  while (!parsedStack.empty()) {
    auto top = parsedStack.front();
    row.resize(512);
    auto size = std::sprintf(row.data(), "[0x%016lX]: %s\n", top.first, top.second.c_str());
    row.resize(size);
    msg.append(row);
    parsedStack.pop();
  }
  ExclusiveIO::info_f("Stack:\n%s\n", msg.c_str());
}

void command_loop(TracedProgram &traced) {
  bool force_end = false;
  std::string choice, choice_param;
  choice.reserve(20);
  ExclusiveIO::info_f("Debug ready.\n");
  do {
    choice.clear();
    choice_param.clear();
    if (traced.isExiting())
      ExclusiveIO::info_f("The program exited normally.\n");
    else if (traced.isTrappedAtBreakpoint()) {
      ExclusiveIO::info_f("The program hit a breakpoint.\n");
    }

    ExclusiveIO::info_f("$ : ");
    ExclusiveIO::input(choice);

    if (choice == "run") {
      if (traced.isDead() || traced.isExiting()) {
        ExclusiveIO::info_f("Re-run the program [Y/n]: ");
        ExclusiveIO::input(choice_param);
        if (choice_param == "y" || choice_param == "Y")
          traced.rerun();

      } else {
        ExclusiveIO::info_f("Continuing program.\n");
        traced.ptraceContinue();
      }
    } else if (choice.starts_with("bp")) {
      ExclusiveIO::input(choice_param);

      if (choice_param.starts_with("0x")) { // Hex choice
        ExclusiveIO::debug_f("Placing bp by address at: 0x%016lX\n", choice_param.c_str());
        if (!traced.breakpointAtAddress(choice_param))
          ExclusiveIO::error_f("Breakpoint[%s] failed: wrong address.\n");
        else
          ExclusiveIO::info_f("Breakpoint[%s] placed.\n", choice_param.c_str());
      } else { // Name choice
        ExclusiveIO::debug_f("Placing bp by function name: %s\n", choice_param.c_str());
        if (!traced.breakpointAtFunction(choice_param))
          ExclusiveIO::error_f("Breakpoint[%s] failed: function does not exists.\n");
        else
          ExclusiveIO::info_f("Breakpoint[%s] placed.\n", choice_param.c_str());
      }
    } else if (choice == "ip" || choice == "rip" || choice == "eip") {
      ExclusiveIO::info_f("Current pointer address: 0x%016lX\n", traced.getIP());
    } else if (choice == "functions") {
      auto functionsList = traced.getElfFile().getFunctionsList();
      printFunctionsList(functionsList, false);
    } else if (choice == "step") {
      ExclusiveIO::info_f("Stepping program.\n");
      traced.ptraceStep();
    } else if (choice == "stack" || choice == "s") {
      auto stack = traced.backtrace();
      printStack(stack);
    } else if (choice == "dump") {
      ExclusiveIO::info_f("Dumping current IP program area:\n");
      ExclusiveIO::infoHigh_nf("\n", traced.dumpAtCurrent(), "\n");
    } else if (choice == "stop") {
      ExclusiveIO::info_f("Killing program.\n");
      traced.stop();
      force_end = true;
    } else {
      ExclusiveIO::info_f("Unknown command.\n");
    }
  } while (traced.isAlive() && !force_end);
  ExclusiveIO::info_f("End of the debug_f.\n");
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
