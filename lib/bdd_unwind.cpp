//
// Created by byjtew on 18/03/2022.
//

#include "bdd_ptrace.hpp"

void TracedProgram::attachUnwind() {
  ExclusiveIO::debug_f("TracedProgram::attachUnwind()\n");
  auto as = unw_create_addr_space(&_UPT_accessors, 0);
  auto context = _UPT_create(traced_pid);
  if (unw_init_remote(&unwind_cursor, as, context) != 0) {
    ExclusiveIO::debugError_f("TracedProgram::attachUnwind(): cannot initialize cursor for remote unwinding\n");
    throw std::invalid_argument("TracedProgram::attachUnwind(): cannot initialize cursor for remote unwinding\n");
  }
}

std::queue<std::pair<addr_t, std::string>> TracedProgram::backtrace() {
  ExclusiveIO::debug_f("TracedProgram::backtrace()\n");
  std::queue<std::pair<addr_t, std::string>> queue;
  unw_word_t offset, pc;
  char sym[256];

  attachUnwind();

  do {
    if (unw_get_reg(&unwind_cursor, UNW_REG_IP, &pc)) {
      ExclusiveIO::debugError_f("TracedProgram::backtrace(): cannot read program counter\n");
      return queue;
    }

    if (unw_get_proc_name(&unwind_cursor, sym, sizeof(sym), &offset) == 0) {
      ExclusiveIO::debug_f("TracedProgram::backtrace(): (%s+0x%016lx)\n", sym, offset);
      queue.push(std::make_pair(offset, sym));
    } else
      ExclusiveIO::debugError_f("TracedProgram::backtrace(): no symbol name found\n");
  } while (unw_step(&unwind_cursor) > 0 && queue.size() < max_stack_size);

  return queue;
}


