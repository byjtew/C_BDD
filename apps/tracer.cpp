//
// Created by byjtew on 08/03/2022.
//

#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <vector>

#include "bdd_ptrace.hpp"
#include "bdd_exclusive_io.hpp"

static bool force_end = false;

std::vector<std::string> readInput() {
  std::vector<std::string> words;
  std::string sentence, word;
  std::getline(std::cin, sentence);
  std::istringstream iss(sentence);
  while (iss >> word) words.push_back(word);
  return words;
}

static std::map<std::string, std::string> usage_map = {
    {"r",                              "Run the traced program."},
    {"run",                            "Run the traced program."},
    {"r / run <parameters>",           "Run the traced program."},

    {"restart",                        "Restart the traced program."},

    {"s",                              "Run one assembly instruction in the traced-program."},
    {"step",                           "Run one assembly instruction in the traced-program."},
    {"s / step",                       "Run one assembly instruction in the traced-program."},

    {"stop",                           "Force the traced program to stop (SIGKILL). Memory leaks may occur."},

    {"kill",                           "Force the traced program to stop (SIGKILL). Memory leaks may occur."},

    {"status",                         "Display the current state of the traced program (exited/segfault/...)."},

    {"functions",                      "Display every functions in the traced program."},
    {"functions <full>",               "Display every functions in the traced program."},

    {"reg",                            "Display every registers values (as %llu only)."},
    {"registers",                      "Display every registers values (as %llu only)."},
    {"reg / registers",                "Display every registers values (as %llu only)."},

    {"d",                              "Display the program (assembly & C) with the next n lines from the current location."},
    {"dump",                           "Display the program (assembly & C) with the next n lines from the current location."},
    {"d / dump <n>",                   "Display the program (assembly & C) with the next n lines from the current location."},

    {"bp",                             "Creates a breakpoint at the specified location."},
    {"bp <address|function-name>",     "Creates a breakpoint at the specified location."},

    {"bp off",                         "Removes a breakpoint from the specified location."},
    {"bp off <address|function-name>", "Removes a breakpoint from the specified location."},

    {"bp show",                        "Display every breakpoints."},

    {"bt",                             "Show the current stack."},
    {"backtrace",                      "Show the current stack."},
    {"bt / backtrace",                 "Show the current stack."},

    {"elf",                            "Show elf information about the traced program."},

    {"help",                           "Show this page."},
    {"man",                            "Show this page."},
    {"help / man",                     "Show this page."},

    {"version",                        "Show the debugger version."},
};


void printUnknwonCommad() {
  ExclusiveIO::error_f("\nUnknown command. Use 'man' to view every available commands.\n");
}

void show_usage_for(const std::string &command) {
  auto it = usage_map.find(command);
  if (it == usage_map.end()) return printUnknwonCommad();
  ExclusiveIO::error_nf("Usage:\n", it->first, "\t\t ", it->second, "\n\n");
}

void manCommand() {
  ExclusiveIO::infoHigh_nf("\n\nCommands:\n"
                           "r, run \t\t\t\t\t\t\t ", usage_map.at("r"), "\n",
                           "s, step \t\t\t\t\t\t ", usage_map.at("s"), "\n",
                           "restart\t\t\t\t\t\t\t ", usage_map.at("restart"), "\n",
                           "stop   \t\t\t\t\t\t\t ", usage_map.at("stop"), "\n",
                           "kill   \t\t\t\t\t\t\t ", usage_map.at("kill"), "\n",
                           "status \t\t\t\t\t\t\t ", usage_map.at("status"), "\n",
                           "functions <full>\t\t\t\t ", usage_map.at("functions"), "\n",
                           "reg, registers\t\t\t\t\t ", usage_map.at("reg"), "\n",
                           "d, dump <n> \t\t\t\t\t ", usage_map.at("d"), "\n",
                           "bp <address|function-name> \t\t ", usage_map.at("bp"), "\n",
                           "bp off <address|function-name> \t ", usage_map.at("bp off"), "\n",
                           "bp show \t\t\t\t\t\t ", usage_map.at("bp show"), "\n",
                           "bt, backtrace \t\t\t\t\t ", usage_map.at("bt"), "\n",
                           "elf \t\t\t\t\t\t\t ", usage_map.at("elf"), "\n",
                           "help, man \t\t\t\t\t\t ", usage_map.at("man"), "\n",
                           "version \t\t\t\t\t\t ", usage_map.at("version"), "\n\n");
}

void onIPStopped(const TracedProgram &traced);

void versionCommand() {
  ExclusiveIO::info_nf("Current version: ", BDD_VERSION, "\n");
}

void runCommand(TracedProgram &traced, std::vector<std::string> &input);

void ipCommand(const TracedProgram &traced);

void functionsCommand(const TracedProgram &traced, std::vector<std::string> &input);

void stepCommand(TracedProgram &traced);

void backtraceCommand(TracedProgram &traced);

void dumpCommand(const TracedProgram &traced);

void registersCommand(const TracedProgram &traced);

void restartCommand(TracedProgram &traced);

void bpCommand(TracedProgram &traced, std::vector<std::string> &input);

void bpOffCommand(TracedProgram &traced, const std::vector<std::string> &input);

void statusCommand(const TracedProgram &traced);

void elfCommand(const TracedProgram &traced);

void stopCommand(const TracedProgram &traced);

void killCommand(const TracedProgram &traced);

void bpShowCommand(TracedProgram &traced);

void command_loop(TracedProgram &traced) {
  std::vector<std::string> input;
  ExclusiveIO::info_f("Debug ready.\n");

  do {
    input.clear();
    onIPStopped(traced);

    ExclusiveIO::info_f("$ : ");
    input = readInput();
    if (input.empty()) continue;
    std::string choice = input.at(0);

    if (choice == "run" || choice == "r")
      runCommand(traced, input);
    else if ((choice == "restart") && (traced.isAlive() && !traced.isExiting()))
      restartCommand(traced);
    else if (choice.starts_with("bp") && input.size() > 1 && input.at(1).starts_with("off"))
      bpOffCommand(traced, input);
    else if (choice.starts_with("bp") && input.size() > 1 && input.at(1).starts_with("show"))
      bpShowCommand(traced);
    else if (choice.starts_with("bp"))
      bpCommand(traced, input);
    else if (choice == "ip" || choice == "rip" || choice == "eip")
      ipCommand(traced);
    else if (choice == "functions")
      functionsCommand(traced, input);
    else if (choice == "step")
      stepCommand(traced);
    else if (choice == "bt" || choice == "backtrace")
      backtraceCommand(traced);
    else if (choice == "reg" || choice == "registers")
      registersCommand(traced);
    else if (choice == "dump")
      dumpCommand(traced);
    else if (choice == "man" || choice == "help")
      manCommand();
    else if (choice == "version")
      versionCommand();
    else if (choice == "status")
      statusCommand(traced);
    else if (choice == "elf")
      elfCommand(traced);
    else if (choice == "stop")
      stopCommand(traced);
    else if (choice == "kill")
      killCommand(traced);
    else
      show_usage_for(choice);
  } while (!force_end);
  ExclusiveIO::info_f("End of the debug.\n");
}


#pragma region User-action functions

void restart(TracedProgram &traced) {
  ExclusiveIO::info_f("Re-launching the program.\n");
  traced.run();
}

void runCommand(TracedProgram &traced, std::vector<std::string> &input) {
  std::string validation_input;
  if (!traced.hasStarted()) { // Executing
    std::vector<char *> params;
    for (unsigned i = 1; i < input.size(); i++)
      params.push_back(input.at(i).data());
    traced.run(params);
    traced.ptraceContinue();
  } else if (traced.isDead() || traced.isExiting()) { // Restart
    ExclusiveIO::info_f("Re-run the program [Y/n]: ");

    ExclusiveIO::input(validation_input);
    if (validation_input == "y" || validation_input == "Y")
      restart(traced);

  } else {
    ExclusiveIO::info_f("Continuing program.\n");
    traced.ptraceContinue();
  }
}

void ipCommand(const TracedProgram &traced) {
  ExclusiveIO::info_f("Current pointer address: 0x%016lX\n", traced.getIP());
}


void killCommand(const TracedProgram &traced) {
  {
    ExclusiveIO::info_f("Killing program.\n");
    traced.killTraced();
    force_end = true;
  }
}

void stopCommand(const TracedProgram &traced) {
  std::string validation_input;
  {
    ExclusiveIO::info_f("Stopping program.\n");
    traced.stopTraced();
    usleep(200000); // Waiting for the traced program to stop
    if (traced.isAlive()) {
      ExclusiveIO::info_f("The program doesn't stop, do you want to force it [Y/n]: \n");
      ExclusiveIO::input(validation_input);
      if (validation_input.starts_with('Y') || validation_input.starts_with('y')) {
        traced.killTraced();
        force_end = true;
      }
    } else
      force_end = true;
  }
}

void elfCommand(const TracedProgram &traced) {
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
  std::string index_selected;
  ExclusiveIO::input(index_selected);
  if (index_selected == "1") {
    traced.getElfFile().printHeader();
  } else if (std::find(possibles_index.cbegin(), possibles_index.cend(), index_selected) !=
             possibles_index.cend()) {
    int asked_index = (int) strtoul(index_selected.c_str(), nullptr, 0);
    traced.getElfFile().printSectionHeaderAt(asked_index - 2);
  } else {
    ExclusiveIO::error_f("Unknown index.\n");
  }
  traced.showStatus();
}

void statusCommand(const TracedProgram &traced) { traced.showStatus(); }

void registersCommand(const TracedProgram &traced) {
  auto reg = traced.getRegisters();
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

void dumpCommand(const TracedProgram &traced) {
  ExclusiveIO::info_f("Dumping current IP program area:\n");
  ExclusiveIO::infoHigh_nf("\n", traced.dumpAtCurrent(), "\n");
}


void backtraceCommand(TracedProgram &traced) {
  auto stack = traced.backtrace();
  ExclusiveIO::debug_f("::printStack()\n");
  std::string msg, row;
  while (!stack.empty()) {
    auto top = stack.front();
    row.resize(512);
    auto size = std::sprintf(row.data(), "[0x%016lX]: %s\n", top.first, top.second.c_str());
    row.resize(size);
    msg.append(row);
    stack.pop();
  }
  ExclusiveIO::info_f("Stack:\n%s\n", msg.c_str());
}

void stepCommand(TracedProgram &traced) {
  ExclusiveIO::info_f("Stepping program.\n");
  traced.ptraceStep();
}


void functionsCommand(const TracedProgram &traced, std::vector<std::string> &input) {
  auto functionsList = traced.getElfFile().getFunctionsList();
  auto full_details = (input.size() > 1 && input.at(1).starts_with("full"));
  ExclusiveIO::info_f("Functions:\n");
  std::for_each(functionsList.begin(), functionsList.end(), [full_details](const auto &it) {
      if (it.first > 0 || full_details)
        ExclusiveIO::info_f("[0x%016lX]: %s\n", it.first, it.second.c_str());
  });
}

void bpShowCommand(TracedProgram &traced) {
  traced.printBreakpointsMap();
}

void bpOffCommand(TracedProgram &traced, const std::vector<std::string> &input) {
  if (input.size() == 1)
    return show_usage_for("bp <address|function-name>");
  std::string bp_choice = input.at(1);
  if (bp_choice.starts_with("0x")) { // Hex choice
    ExclusiveIO::debug_f("Placing bp by address at: 0x%016lX\n", bp_choice.c_str());
    if (!traced.disableBreakpointAtAddress(bp_choice))
      ExclusiveIO::error_f("Breakpoint[%s] failed: wrong address.\n");
    else
      ExclusiveIO::info_f("Breakpoint[%s] disabled.\n", bp_choice.c_str());
  } else { // Name choice
    ExclusiveIO::debug_f("Placing bp by function name: %s\n", bp_choice.c_str());
    if (!traced.disableBreakpointAtFunction(bp_choice))
      ExclusiveIO::error_f("Breakpoint[%s] failed: function does not exists.\n");
    else
      ExclusiveIO::info_f("Breakpoint[%s] disabled.\n", bp_choice.c_str());
  }
  bpShowCommand(traced);
}

void bpCommand(TracedProgram &traced, std::vector<std::string> &input) {
  if (input.size() == 1)
    return show_usage_for("bp off <address|function-name>");

  std::string bp_choice = input.at(1);
  if (bp_choice.starts_with("0x")) { // Hex choice
    ExclusiveIO::debug_f("Placing bp by address at: 0x%016lX\n", bp_choice.c_str());
    if (!traced.breakpointAtAddress(bp_choice))
      ExclusiveIO::error_f("Breakpoint[%s] failed: wrong address.\n");
    else
      ExclusiveIO::info_f("Breakpoint[%s] placed.\n", bp_choice.c_str());
  } else { // Name choice
    ExclusiveIO::debug_f("Placing bp by function name: %s\n", bp_choice.c_str());
    if (!traced.breakpointAtFunction(bp_choice))
      ExclusiveIO::error_f("Breakpoint[%s] failed: function does not exists.\n");
    else
      ExclusiveIO::info_f("Breakpoint[%s] placed.\n", bp_choice.c_str());
  }
  bpShowCommand(traced);
}

void restartCommand(TracedProgram &traced) {
  std::string validation_input;
  ExclusiveIO::info_f("Restart the program [Y/n]: ");
  ExclusiveIO::input(validation_input);
  if (validation_input == "y" || validation_input == "Y")
    restart(traced);
}

#pragma endregion


void onIPStopped(const TracedProgram &traced) {
  if (traced.isExiting())
    ExclusiveIO::info_f("The program exited normally.\n");
  else if (traced.isTrappedAtBreakpoint()) {
    ExclusiveIO::info_f("The program hit a breakpoint.\n");
  } else if (traced.isSegfault()) {
    auto seg_data = traced.getSegfaultData();
    ExclusiveIO::info_f("The program has a segfault: %s (at 0x%016lX)\n", seg_data.first.c_str(), seg_data.second);
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <program|elf-file>" << std::endl;
    exit(1);
  }
  std::vector<std::string> args;
  for (unsigned i = 1; i < argc; i++)
    args.emplace_back(argv[i]);
  if (!args.empty()) {
    if (args[0] == "-h" || args[0] == "--help") {
      manCommand();
      return 0;
    } else if (args[0] == "-v" || args[0] == "--version") {
      versionCommand();
      return 0;
    }
  }

  auto traced = TracedProgram(std::string(argv[1]));
  command_loop(traced);
  traced.stopTraced();
  return 0;
}
