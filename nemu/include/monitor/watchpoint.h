#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;

	/* TODO: Add more members if necessary */
	char expr_s[128];
	int last_val;

} WP;

void new_wp(char *EXPR);
void free_wp(int num);
void print_wp();
void check_wp();

#endif
