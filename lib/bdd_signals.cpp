//
// Created by byjtew on 18/03/2022.
//

#include <cassert>
#include "bdd_ptrace.hpp"


bool TracedProgram::isDead() const {
  return WIFEXITED(cached_status);
}


bool TracedProgram::isAlive() const {
  return !isDead();
}

bool TracedProgram::isStopped() const {
  return WIFSTOPPED(cached_status);
}

bool TracedProgram::isSegfault() const {
  if (!isStopped()) return false;
  return WSTOPSIG(cached_status) == SIGSEGV;
}

bool TracedProgram::isTrapped() const {
  if (!isStopped()) return false;
  return WSTOPSIG(cached_status) == SIGTRAP;
}

bool TracedProgram::isExiting() const {
  if (!isTrapped()) return false;
  auto event = (cached_status >> 16) & 0xffff;
  return event == PTRACE_EVENT_EXIT;
}


void TracedProgram::printSiginfo_t(const siginfo_t &info) {
  ExclusiveIO::debug_f("TracedProgram::printSiginfo_t():\n- code: \t%d\n- errno: \t%d\n- signo: \t%d\n", info.si_code,
                       info.si_errno, info.si_signo);
}


std::pair<std::string, addr_t> TracedProgram::getSegfaultData() const {
  ExclusiveIO::debug_f("TracedProgram::getSegfaultData()\n");
  siginfo_t info;
  auto pc = ptrace(PTRACE_GETSIGINFO, traced_pid, nullptr, &info);
  if (pc < 0) return std::make_pair("Error fetching signal information", 0);
  ExclusiveIO::debug_f("TracedProgram::getSegfaultData(): ptrace(PTRACE_GETSIGINFO, ...) => %d\n", pc);
  printSiginfo_t(info);
  return std::make_pair<>(getSegfaultCodeAsString(info), (addr_t) info.si_addr);
}


std::string TracedProgram::getSegfaultCodeAsString(siginfo_t &info) {
  switch (info.si_signo) {
    case SIGSEGV:
      switch (info.si_code) {
        case SEGV_MAPERR:
          return "SEGV_MAPERR: Address not mapped";
        case SEGV_ACCERR:
          return "SEGV_ACCERR: Invalid permissions";
        case SEGV_BNDERR:
          return "SEGV_BNDERR: Bounds checking failure";
        case SEGV_PKUERR:
          return "SEGV_PKUERR: Protection key checking failure";
        case SEGV_ACCADI:
          return "SEGV_ACCADI: ADI not enabled for mapped object";
        case SEGV_ADIDERR:
          return "SEGV_ADIDERR: Disrupting MCD error";
        case SEGV_ADIPERR:
          return "SEGV_ADIPERR: Precise MCD exception";
        default:
          return "SIGSEGV: Unknown code " + std::to_string(info.si_code);
      }
    case SIGFPE:
      switch (info.si_code) {
        case FPE_INTDIV:
          return "FPE_INTDIV: Integer divide-by-zero";
        case FPE_INTOVF:
          return "FPE_INTOVF: Integer overflow";
        case FPE_FLTDIV:
          return "FPE_FLTDIV: Floating point divide-by-zero";
        case FPE_FLTOVF:
          return "FPE_FLTOVF: Floating point overflow";
        case FPE_FLTUND:
          return "FPE_FLTUND: Floating point underflow";
        case FPE_FLTRES:
          return "FPE_FLTRES: Floating point inexact result";
        case FPE_FLTINV:
          return "FPE_FLTINV: Invalid floating point operation";
        case FPE_FLTSUB:
          return "FPE_FLTSUB: Subscript out of range";
        default:
          return "SIGFPE: Unknown code " + std::to_string(info.si_code);
      }
    case SIGILL:
      switch (info.si_code) {
        case ILL_ILLOPC:
          return "ILL_ILLOPC: Illegal opcode";
        case ILL_ILLOPN:
          return "ILL_ILLOPN: Illegal operand";
        case ILL_ILLADR:
          return "ILL_ILLADR: Illegal addressing mode";
        case ILL_ILLTRP:
          return "ILL_ILLTRP: Illegal trap";
        case ILL_PRVOPC:
          return "ILL_PRVOPC: Privileged opcode";
        case ILL_PRVREG:
          return "ILL_PRVREG: Privileged register";
        case ILL_COPROC:
          return "ILL_COPROC: Coprocessor error";
        case ILL_BADSTK:
          return "ILL_BADSTK: Internal stack error";
        default:
          return "SIGILL: Unknown code " + std::to_string(info.si_code);
      }
    case SIGBUS:
      switch (info.si_code) {
        case BUS_ADRALN:
          return "BUS_ADRALN: Invalid address alignment";
        case BUS_ADRERR:
          return "BUS_ADRERR: Non-existent physical address";
        case BUS_OBJERR:
          return "BUS_OBJERR: Object-specific hardware error";
        default:
          return "SIGBUS: Unknown code " + std::to_string(info.si_code);
      }
    default:
      return "[SIGNAL " + std::to_string(info.si_signo) + "]: Unknown";
  }
}

std::vector<std::pair<Breakpoint, bool>> TracedProgram::placeEveryPendingBreakpoints() {
  assert(isAlive());
  std::vector<std::pair<Breakpoint, bool>> status;
  for (Breakpoint &bp: pendingBreakpointsMap) {
    addr_t real_addr =
        bp.getAddress() < getTracedRAMAddress() ? bp.getAddress() + getTracedRAMAddress() : bp.getAddress();

    status.emplace_back(bp, breakpointAtAddress(real_addr, bp.getName()));
  }
  pendingBreakpointsMap.clear();
  return status;
}




