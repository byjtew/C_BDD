//
// Created by byjtew on 08/03/2022.
//

#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <vector>

#include "bdd_ptrace.hpp"
#include "bdd_exclusive_io.hpp"

void printRegisters(std::optional<user_regs_struct> reg) {
  if (!reg)
    return ExclusiveIO::error_f("Impossible to read registers.\n");
#ifdef __x86_64__
  ExclusiveIO::info_f(
      "Registers [64-bits]:\nrsi: \t\t%llu\nrdi: \t\t%llu\nrip: \t\t%llu\nr[a-d]x: \t%llu %llu %llu %llu\nr[8-15]: \t%llu %llu %llu %llu %llu %llu %llu %lu\n\n",
      reg->rsi, reg->rdi, reg->rip, reg->rax, reg->rbx, reg->rcx, reg->rdx, reg->r8, reg->r9, reg->r10, reg->r11,
      reg->r12,
      reg->r13, reg->r14, reg->r15);
#else
  ExclusiveIO::info_f("Registers [32-bits]:\nesi: \t\t%lu\nedi: \t\t%lu\neip: \t\t%lu\nr[a-d]x: \t%lu %lu %lu %lu\n\n", reg->esi,
                      reg->edi, reg->eip, reg->eax, reg->ebx, reg->ecx, reg->edx);
#endif
}

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

void restart(TracedProgram &traced) {
  ExclusiveIO::info_f("Re-launching the program.\n");
  traced.run();
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
    } else if (traced.isSegfault()) {
      auto seg_data = traced.getSegfaultData();
      ExclusiveIO::info_f("The program has a segfault: %s (at 0x%016lX)\n", seg_data.first.c_str(), seg_data.second);
    }

    ExclusiveIO::info_f("$ : ");
    ExclusiveIO::input(choice);
    // TODO: Parse input char-by-char to handle optional parameters

    if (choice == "run") {
      if (traced.isDead() || traced.isExiting()) {
        ExclusiveIO::info_f("Re-run the program [Y/n]: ");
        ExclusiveIO::input(choice_param);
        if (choice_param == "y" || choice_param == "Y")
          restart(traced);

      } else {
        ExclusiveIO::info_f("Continuing program.\n");
        traced.ptraceContinue();
      }
    } else if ((choice == "restart") && (traced.isAlive() && !traced.isExiting())) {
      ExclusiveIO::info_f("Restart the program [Y/n]: ");
      ExclusiveIO::input(choice_param);
      if (choice_param == "y" || choice_param == "Y")
        restart(traced);
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
      printFunctionsList(functionsList, true || choice_param == "full");
    } else if (choice == "step") {
      ExclusiveIO::info_f("Stepping program.\n");
      traced.ptraceStep();
    } else if (choice == "stack" || choice == "s") {
      auto stack = traced.backtrace();
      printStack(stack);
    } else if (choice == "dump") {
      ExclusiveIO::info_f("Dumping current IP program area:\n");
      ExclusiveIO::infoHigh_nf("\n", traced.dumpAtCurrent(), "\n");
    } else if (choice == "reg" || choice == "registers") {
      printRegisters(traced.getRegisters());
    } else if (choice == "status") {
      traced.showStatus();
    } else if (choice == "elf") {
      auto types_and_names = traced.getElfFile().getSymbolsNames();
      std::string hint("Elf available informations:\n1. Program header\n");
      std::vector<std::string> possibles_index;
      int index = 2;
      std::for_each(types_and_names.cbegin(), types_and_names.cend(), [&possibles_index, &index, &hint](const auto &e) {
          hint.append(std::to_string(index) + ". Section <" + e.second + ">\n");
          possibles_index.push_back(std::to_string(index));
          index += 1;
      });
      ExclusiveIO::infoHigh_nf(hint);
      ExclusiveIO::infoHigh_nf("Type the index that you want to display: ");
      ExclusiveIO::input(choice_param);
      if (choice_param == "1") {
        traced.getElfFile().printHeader();
      } else if (std::find(possibles_index.cbegin(), possibles_index.cend(), choice_param) != possibles_index.cend()) {
        int asked_index = (int) strtoul(choice_param.c_str(), nullptr, 0);
        traced.getElfFile().printSectionHeaderAt(asked_index - 2);
      } else {
        ExclusiveIO::error_f("Unknown index.\n");
      }
      traced.showStatus();
    } else if (choice == "stop") {
      ExclusiveIO::info_f("Stopping program.\n");
      traced.stopTraced();
      usleep(200000);
      if (traced.isAlive()) {
        ExclusiveIO::info_f("The program doesn't stop, do you want to force it [Y/n]: \n");
        ExclusiveIO::input(choice_param);
        if (choice_param.starts_with('Y') || choice_param.starts_with('y')) {
          traced.killTraced();
          force_end = true;
        }
      } else
        force_end = true;
    } else if (choice == "kill") {
      ExclusiveIO::info_f("Killing program.\n");
      traced.killTraced();
      force_end = true;
    } else {
      ExclusiveIO::error_f("Unknown command.\n");
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
  traced.stopTraced();
  return 0;
}
