#include "tracing.hpp"


Breakpoint::Breakpoint(pid_t pid, addr_t const &addr) {
  program_pid = pid;
  address = addr;
  TracedProgram::processPrint("Breakpoint::Breakpoint(%d, 0x%016lX)\n", pid, address);
}

bool Breakpoint::enable() {
  original = ptrace(PTRACE_PEEKTEXT, program_pid, address);
  TracedProgram::processPrint("Breakpoint::enable() [0x%016lX]: original = 0x%016lX\n", address, original);
  if (ptrace(PTRACE_POKETEXT, program_pid, address, (original & TRAP_MASK) | INT3) == -1) {
    TracedProgram::processPerror("Breakpoint::enable() [0x%016lX]: ptrace error.\n", address);
    return false;
  }
  enabled = true;
  return true;
}

bool Breakpoint::disable() {
  if (ptrace(PTRACE_POKETEXT, program_pid, address, original)) {
    TracedProgram::processPerror("Breakpoint::enable() [0x%016lX]: ptrace error.\n", address);
    return false;
  }
  enabled = false;
  return true;
}

bool Breakpoint::isEnabled() const {
  return enabled;
}


void TracedProgram::setBreakpoint(Breakpoint &bp) {
  TracedProgram::processPrint("TracedProgram::setBreakpoint(0x%016lX)\n", bp.getAddress());
  breakpointsMap.try_emplace(bp.getAddress(), bp);
  //breakpointsMap[bp.getAddress()] = bp;
}

bool TracedProgram::breakpointAtAddress(addr_t address) {
  Breakpoint bp{traced_pid, address};
  if (!bp.enable()) return false;
  setBreakpoint(bp);
  return true;
}

bool TracedProgram::breakpointAtAddress(const std::string &strAddress) {
  return breakpointAtAddress((addr_t) strAddress.data());
}

bool TracedProgram::breakpointAtFunction(const std::string &fct_name) {
  auto prog_offset = getTracedProgExecAddress();
  processPrint("RAM offset: 0x%016lX\n", prog_offset);
  auto addr = elf_file.getFunctionAddress(fct_name);
  processPrint("addr value: 0x%016lX\n", addr);
  if (addr == 0) return false;
  return breakpointAtAddress(addr + prog_offset);

}


addr_t TracedProgram::getIP() const {
  auto v = ptrace(PTRACE_PEEKUSER, traced_pid, sizeof(addr_t) * REGISTER_IP);
  auto ip = v - 1;
  TracedProgram::processPrint("TracedProgram::getIP(): 0x%016lX\n", ip);
  return ip;
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
  processPrint("== Breakpoints ==\n%s== =========== ==\n", message.c_str());
}


bool TracedProgram::isTrappedAtBreakpoint() const {
  auto ip = getIP();
  printBreakpointsMap();
  return breakpointsMap.contains(ip);
}