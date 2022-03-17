#include <cassert>
#include "bdd_ptrace.hpp"

Breakpoint::Breakpoint(pid_t pid, addr_t const &addr, const std::string &func_name) {
  program_pid = pid;
  address = addr;
  name = func_name;
  original = 0;
  ExclusiveIO::debug_f("Breakpoint::Breakpoint(%d, 0x%016lX)\n", program_pid, address);
}

bool Breakpoint::enable() {
  ExclusiveIO::debug_f("Breakpoint[0x%016lX]::enable()\n", address);
  original = ptrace(PTRACE_PEEKTEXT, program_pid, address);
  ExclusiveIO::debug_f("Breakpoint[0x%016lX]::enable(): original = 0x%016lX\n", address, original);
  if (ptrace(PTRACE_POKETEXT, program_pid, address, (original & TRAP_MASK) | INT3) == -1) {
    ExclusiveIO::debugError_f("Breakpoint[0x%016lX]::enable(): ptrace error.\n", address);
    return false;
  }
  enabled = true;
  return true;
}

void Breakpoint::disable() {
  ExclusiveIO::debug_f("Breakpoint[0x%016lX]::disable()\n", address);
  if (ptrace(PTRACE_POKETEXT, program_pid, address, original) == -1) {
    ExclusiveIO::debugError_f("Breakpoint[0x%016lX]::disable(): ptrace error.\n", address);
    enabled = false;
  }
}

void TracedProgram::setBreakpoint(Breakpoint &bp) {
  ExclusiveIO::debug_f("TracedProgram::setBreakpoint(0x%016lX)\n", bp.getAddress());
  breakpointsMap.try_emplace(bp.getAddress(), bp);
}

bool TracedProgram::breakpointAtAddress(addr_t address, std::optional<std::string> func_name) {
  ExclusiveIO::debug_f("TracedProgram::breakpointAtAddress(0x%016lX)\n", address);
  Breakpoint bp = (func_name) ?
                  Breakpoint(traced_pid, address, func_name.value()) :
                  Breakpoint(traced_pid, address);
  if (!bp.enable()) return false;
  setBreakpoint(bp);
  return true;
}

bool TracedProgram::breakpointAtAddress(const std::string &strAddress) {
  ExclusiveIO::debug_f("TracedProgram::breakpointAtAddress(%s)\n", strAddress.c_str());
  return breakpointAtAddress((addr_t) strtoul(strAddress.c_str(), (char **) nullptr, 0), std::nullopt);
}

bool TracedProgram::breakpointAtFunction(const std::string &fctName) {
  ExclusiveIO::debug_f("TracedProgram::breakpointAtFunction(%s)\n", fctName.c_str());
  auto programAddress = getTracedRAMAddress();
  ExclusiveIO::debug_f("TracedProgram::breakpointAtFunction(...): programAddress = 0x%016lX\n", programAddress);
  auto elfAddress = elf_file.getFunctionAddress(fctName);
  ExclusiveIO::debug_f("TracedProgram::breakpointAtFunction(...): elfAddress = 0x%016lX\n", elfAddress);
  if (elfAddress == 0) return false;
  return breakpointAtAddress(((addr_t) (elfAddress + programAddress)), fctName);
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
  if (breakpointsMap.empty()) {
    ExclusiveIO::debug_f("TracedProgram::printBreakpointsMap(): no breakpoint to print.\n");
    return;
  }
  std::string message("== Breakpoints ==\n");
  for (const auto &kv: breakpointsMap) {
    std::string buffer;
    buffer.resize(256);
    snprintf(buffer.data(), buffer.size(), "[%s]: %s (0x%016lX)\n",
             (kv.second.isEnabled() ? "X " : " "),
             kv.second.getName().c_str(),
             kv.second.getAddress());
    message.append(buffer);
  }
  ExclusiveIO::hint_f("== Breakpoints ==\n%s== =========== ==\n", message.c_str());
}

Breakpoint &TracedProgram::getHitBreakpoint() {
  addr_t ip = getIP();
  assert(breakpointsMap.contains(ip));
  return breakpointsMap.at(ip);
}


bool TracedProgram::isTrappedAtBreakpoint() const {
  auto ip = getIP();
  printBreakpointsMap();
  return breakpointsMap.contains(ip);
}