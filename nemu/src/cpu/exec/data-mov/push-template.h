#include "cpu/exec/template-start.h"

#define instr push

static void do_execute() {
  cpu.esp -= DATA_BYTE == 2 ? 2 : 4;
  swaddr_write(cpu.esp, DATA_BYTE, op_src->val);
  print_asm_template1();
}

make_instr_helper(i)
make_instr_helper(r)
make_instr_helper(rm)

#include "cpu/exec/template-end.h"