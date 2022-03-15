#include "bdd_ptrace.hpp"

Breakpoint::Breakpoint(pid_t pid, addr_t const &addr) {
  program_pid = pid;
  address = addr;
  ExclusiveIO::debug_f("Breakpoint::Breakpoint(%d, 0x%016lX)\n", program_pid, address);
}

bool Breakpoint::enable() {
  ExclusiveIO::debug_f("Breakpoint::enable()\n");
  original = ptrace(PTRACE_PEEKTEXT, program_pid, address);
  ExclusiveIO::debug_f("Breakpoint::enable() [0x%016lX]: original = 0x%016lX\n", address, original);
  if (ptrace(PTRACE_POKETEXT, program_pid, address, (original & TRAP_MASK) | INT3) == -1) {
    ExclusiveIO::debugError_f("Breakpoint::enable() [0x%016lX]: ptrace error.\n", address);
    return false;
  }
  enabled = true;
  return true;
}

bool Breakpoint::disable() {
  if (ptrace(PTRACE_POKETEXT, program_pid, address, original)) {
    ExclusiveIO::debugError_f("Breakpoint::enable() [0x%016lX]: ptrace error.\n", address);
    return false;
  }
  enabled = false;
  return true;
}


void TracedProgram::setBreakpoint(Breakpoint &bp) {
  ExclusiveIO::debug_f("TracedProgram::setBreakpoint(0x%016lX)\n", bp.getAddress());
  breakpointsMap.try_emplace(bp.getAddress(), bp);
}

bool TracedProgram::breakpointAtAddress(addr_t address) {
  ExclusiveIO::debug_f("TracedProgram::breakpointAtAddress(0x%016lX)\n", address);
  Breakpoint bp{traced_pid, address};
  if (!bp.enable()) return false;
  setBreakpoint(bp);
  return true;
}

bool TracedProgram::breakpointAtAddress(const std::string &strAddress) {
  ExclusiveIO::debug_f("TracedProgram::breakpointAtAddress(%s)\n", strAddress.c_str());
  return breakpointAtAddress((addr_t) strtoul(strAddress.c_str(), (char **) nullptr, 0));
}

bool TracedProgram::breakpointAtFunction(const std::string &fctName) {
  ExclusiveIO::debug_f("TracedProgram::breakpointAtFunction(%s)\n", fctName.c_str());
  auto programAddress = getTracedRAMAddress();
  ExclusiveIO::debug_f("TracedProgram::breakpointAtFunction(...): programAddress = 0x%016lX\n", programAddress);
  auto elfAddress = elf_file.getFunctionAddress(fctName);
  ExclusiveIO::debug_f("TracedProgram::breakpointAtFunction(...): elfAddress = 0x%016lX\n", elfAddress);
  if (elfAddress == 0) return false;
  return breakpointAtAddress(elfAddress + programAddress);
}


addr_t TracedProgram::getIP() const {
  auto v = ptrace(PTRACE_PEEKUSER, traced_pid, sizeof(addr_t) * REGISTER_IP);
  auto ip = v - 1;
  ExclusiveIO::debug_f("TracedProgram::getIP(): 0x%016lX\n", ip);
  return ip;
}

addr_t TracedProgram::getElfIP() const {
  auto elf_ip = getIP() - ram_start_address;
  ExclusiveIO::debug_f("TracedProgram::getElfIP(): 0x%016lX\n", elf_ip);
  return elf_ip;
}

void TracedProgram::printBreakpointsMap() const {
  std::string message("== Breakpoints ==\n");
  for (const auto &kv: breakpointsMap) {
    std::string buffer;
    buffer.resize(40);
    snprintf(buffer.data(), buffer.size(), "[%s]: 0x%016lX\n", (kv.second.isEnabled() ? "ENABLED " : "DISABLED"),
             kv.second.getAddress());
    message.append(buffer);
  }
  ExclusiveIO::hint_f("== Breakpoints ==\n%s== =========== ==\n", message.c_str());
}


bool TracedProgram::isTrappedAtBreakpoint() const {
  auto ip = getIP();
  printBreakpointsMap();
  return breakpointsMap.contains(ip);
}