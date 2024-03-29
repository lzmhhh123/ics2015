#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the ``readline'' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_s(char *args) {
	cpu_exec(1);
	return 0;
}

static int cmd_p(char *args) {
	bool issucceed = false;
	int val = expr(args, &issucceed);
	if(issucceed) {
		printf("ans = %d\nYour EXPR is successful.\n", val);
	}
	else {
		printf("Your EXPR is failed.\n");
	}
	return 0;
}

static int cmd_w(char *args) {
	bool success = false;
	expr(args, &success);
	if(success == false) {
		printf("Your EXPR is illegal.\n");
		return 0;
	}
	new_wp(args);
	return 0;
}

static int cmd_d(char *args) {
	if(args == NULL) {
		printf("Please input a num.\n");
		return 0;
	}
	int num;
	if(sscanf(args, "%d", &num) == 0) {
		printf("Input not a number.\n");
		return 0;
	}
	free_wp(num);
	return 0;
}

bool get_fun(uint32_t, char*);
static int cmd_bt(char *args) {
	if(args != NULL)
		printf("(there is no need to input any arguments)\n");
	uint32_t tmp = cpu.ebp;
	uint32_t addr = cpu.eip;
	char name[32];
	int i = 0, j;
	while(get_fun(addr, name)) {
		name[31] = '\0';
		printf("#%02d  %08x in %s(", i++, addr, name);
		for(j = 2; j < 6; ++j) {
			if(tmp + j * 4 > 0 && tmp + j * 4 < 0x8000000)
				printf("%d, ", swaddr_read(tmp + j * 4, 4));
		}
		if(tmp + j * 4 > 0 && tmp + j * 4 < 0x8000000)
			printf("%d", swaddr_read(tmp + j * 4, 4));
		printf(")\n");
		addr = swaddr_read(tmp + 4, 4);
		tmp = swaddr_read(tmp, 4);
	}
	return 0;
}

static int cmd_info(char *args) {
	if(*args == 'r') {
		int i;
		for(i = R_EAX; i <= R_EDI; ++i) {
			printf("%s: 0x%.08x\n", regsl[i], cpu.gpr[i]._32);
		}
		printf("eip: 0x%.08x\n", cpu.eip);
		printf("eflags: 0x%.08x\n", cpu.eflags);
		printf("CF: 0x%.08x\n",cpu.CF);
    printf("PF: 0x%.08x\n",cpu.PF);
		printf("ZF: 0x%.08x\n",cpu.ZF);
		printf("SF: 0x%.08x\n",cpu.SF);
		printf("IF: 0x%.08x\n",cpu.IF);
		printf("DF: 0x%.08x\n",cpu.DF);
		printf("OF: 0x%.08x\n",cpu.OF);
	}
	else if(*args == 'w') {
		print_wp();
	}
	return 0;
}

static int cmd_x(char *args) {
	int len, pos;
	sscanf(args, "%d 0x%x", &len, &pos);
	int i;
	for(i = 0; i <= len; ++i) {
    printf("0x%02x ", *(unsigned char *)hwa_to_va(pos + i));
	}
	printf("\n");
	return 0;
}

static int cmd_help(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },

	/* TODO: Add more commands */
	{ "s", "Step the execution of the program", cmd_s},
	{ "info", "Display the values of registers by operation (r)\nDisplay the monitor message by operation (w)", cmd_info},
	{ "x", "Display the values of memory by operation: (lenth) (start position use 0x())", cmd_x},
	{ "p", "p EXPR: Calculate the EXPR print the result", cmd_p},
	{ "w", "w EXPR: Add a watchpoint for EXPR", cmd_w},
	{ "d", "d NO: Delete the number NO watchpoint", cmd_d},
	{ "bt", "bt: Print backtrace of all stack frames", cmd_bt}

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
