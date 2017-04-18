#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

void new_wp(char *EXPR) {
	if(free_ == NULL) {
		printf("Number of watchpoints excceed 32, add failed.\n");
		return ;
	}
	strcpy(free_ -> expr_s, EXPR);
	bool success;
	free_ -> last_val = expr(EXPR, &success);
	free_ -> next = head;
	head = free_;
	free_ = free_ -> next;
	return ;
}

void print_wp() {
	if(head == NULL) {
		printf("There's no watchpoint.\n");
		return ;
	}
	WP *tmp = head;
	printf("watchpoint:\nNO\tEXPR\tVALUE\n");
	for(; tmp != NULL; tmp = tmp -> next) {
		printf("%d\t%s\t%d\n", tmp -> NO, tmp -> expr_s, tmp -> last_val);
	}
	return;
}

void free_wp(int num) {
	if(head == NULL) {
		printf("The NO.%d watchpoint isn't exist.\n", num);
		return ;
	}
	if(head -> NO == num) {
		WP *tmp = head;
		head = head -> next;
		tmp -> next = free_;
		free_ = tmp;
		return ;
	}
	WP *tmp = head -> next, *last = head;
	for(; tmp != NULL; tmp = tmp -> next, last = last -> next) {
		if(tmp -> NO == num) {
			last -> next = tmp -> next;
			tmp -> next = free_;
			free_ = tmp;
			return ;
		}
	}
	printf("The NO.%d watchpoint isn't exist.\n", num);
	return ;
}

void check_wp(int *nemu_state) {
	WP *tmp = head;
	for(; tmp != NULL; tmp = tmp -> next) {
		bool success;
		int val = expr(tmp -> expr_s, &success);
		if(val != tmp -> last_val) {
			printf("the watchpoint for EPXR %s's value is changed from %d to %d.\n", tmp -> expr_s, tmp -> last_val, val);
			tmp -> last_val = val;
			nemu_state = 0;
		}
	}
	return ;
}
