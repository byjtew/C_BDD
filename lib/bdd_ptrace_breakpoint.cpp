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


void TracedProgram::setBreakpoint(Breakpoint &breakpoint) {
  ExclusiveIO::debug_f("TracedProgram::setBreakpoint(0x%016lX)\n", breakpoint.getAddress());
  if (!hasStarted()) pendingBreakpointsMap.emplace_back(breakpoint);
  else
    breakpointsMap.try_emplace(breakpoint.getAddress(), breakpoint);
}

bool TracedProgram::breakpointAtAddress(addr_t address, std::optional<std::string> func_name) {
  ExclusiveIO::debug_f("TracedProgram::breakpointAtAddress(0x%016lX)\n", address);
  Breakpoint bp = (func_name) ?
                  Breakpoint(traced_pid, address, func_name.value()) :
                  Breakpoint(traced_pid, address);

  if (hasStarted() && !bp.enable()) return false;
  setBreakpoint(bp);
  return true;
}

bool TracedProgram::breakpointAtAddress(const std::string &strAddress) {
  ExclusiveIO::debug_f("TracedProgram::breakpointAtAddress(%s)\n", strAddress.c_str());
  return breakpointAtAddress(strAddr_tToHex(strAddress), std::nullopt);
}

addr_t TracedProgram::strAddr_tToHex(const std::string &strAddress) {
  return (addr_t) strtoul(strAddress.c_str(), (char **) nullptr, 0);
}

bool TracedProgram::breakpointAtFunction(const std::string &fctName) {
  ExclusiveIO::debug_f("TracedProgram::breakpointAtFunction(%s)\n", fctName.c_str());
  if (!hasStarted()) {
    auto pc = breakpointAtAddress(elf_file.getFunctionAddress(fctName), fctName);
    ExclusiveIO::debug_f("TracedProgram::breakpointAtFunction(%s): pending bp, ret code = %d\n", fctName.c_str(), pc);
    return true;
  }
  addr_t func_addr = getFunctionPhysicalAddress(fctName);
  if (func_addr == 0) return false;
  return breakpointAtAddress(func_addr, fctName);
}

addr_t TracedProgram::getFunctionPhysicalAddress(const std::string &fctName) const {
  assert(isAlive());
  addr_t programAddress = getTracedRAMAddress();
  addr_t elfAddress = elf_file.getFunctionAddress(fctName);
  return programAddress + elfAddress;
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
  if (breakpointsMap.empty() && pendingBreakpointsMap.empty()) {
    ExclusiveIO::hint_f("No breakpoints yet.\n");
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
  for (const auto &bp: pendingBreakpointsMap) {
    std::string buffer;
    buffer.resize(256);
    snprintf(buffer.data(), buffer.size(), "[%s]: %s (0x%016lX)\n",
             ("PENDING"),
             bp.getName().c_str(),
             bp.getAddress());
    message.append(buffer);
  }
  ExclusiveIO::hint_f("== Breakpoints ==\n%s", message.c_str());
  if (!pendingBreakpointsMap.empty())
    ExclusiveIO::hint_f("* PENDING breakpoints are placed after the first 'run'\n");
  ExclusiveIO::hint_f("== =========== ==\n");
}


Breakpoint &TracedProgram::getHitBreakpoint() {
  addr_t ip = getIP();
  assert(breakpointsMap.contains(ip));
  return breakpointsMap.at(ip);
}


bool TracedProgram::isTrappedAtBreakpoint() const {
  auto ip = getIP();
  return breakpointsMap.contains(ip);
}

bool TracedProgram::disableBreakpointAtFunction(const std::string &func_name) {
  addr_t func_addr = getFunctionPhysicalAddress(func_name);
  if (func_addr == 0 || !breakpointsMap.contains(func_addr)) return false;
  auto bp = breakpointsMap.at(func_addr);
  bp.disable();
  return true;
}

bool TracedProgram::disableBreakpointAtAddress(const std::string &hex_addr_as_str) {
  addr_t parsed_addr = strAddr_tToHex(hex_addr_as_str);
  if (!breakpointsMap.contains(parsed_addr)) return false;
  auto bp = breakpointsMap.at(parsed_addr);
  bp.disable();
  return true;
}

