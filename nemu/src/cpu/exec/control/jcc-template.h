#include "cpu/exec/template-start.h"

#if DATA_BYTE == 1
#define CODE_LEN 2
#endif
#if DATA_BYTE == 2
#define CODE_LEN 4
#endif
#if DATA_BYTE == 4
#define CODE_LEN 8
#endif

#define instr je

static void do_execute() {
  if(cpu.ZF == 1) {
    int32_t val = op_src->val;
    val = val << (32 - DATA_BYTE * 8);
    val = val >> (32 - DATA_BYTE * 8);
    cpu.eip += op_src->val;
    if(DATA_BYTE == 2) cpu.eip &= 0xffff;
  }
  print_asm("je $0x%x", cpu.eip + CODE_LEN);
}

make_instr_helper(i)

#undef instr
#undef CODE_LEN
#include "cpu/exec/template-end.h"
