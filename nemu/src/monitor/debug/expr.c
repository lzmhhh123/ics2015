#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ, NUM

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE},				// spaces
	{"\\+", '+'},					// plus
	{"==", EQ},						// equal
	{"\\(", '('},					//left (
	{"\\)", ')'},					//right )
	{"-", '-'},						//sub
	{"\\*", '*'},					//mul
	{"/", '/'}						//dev
	{"[0-9]+", NUM}       //number
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;

	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array ``tokens''. For certain
				 * types of tokens, some extra actions should be performed.
				 */

				tokens[nr_token].type = rules[i].token_type;

				switch(rules[i].token_type) {
					case '+':
					case '-':
					case '*':
					case '/':
					case '(':
					case ')': break;
					case NUM:
						if(substr_len > 31) {
							printf("The input number is too long.");
							return false;
						}
						strncpy(tokens[nr_token].str, substr_start, substr_len);
						tokens[substr_len] = '\0';
						break;
					case NOTYPE:
						nr_token--;
						break;
					default:
						panic("please implement me");
						return false;
				}
				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
		nr_token ++;
	}

	//Check the bracket
	int cnt = 0;
	bool flag = true;
	for(int i = 0; i < nr_token; ++i) {
		if(tokens[i] == '(') cnt++;
		if(tokens[i] == ')') cnt--;
		if(cnt < 0) flag = false;
	}
	if(cnt > 0) flag = false;
	if(!flag) {
		printf("The bracket isn't match.\n");
		return false;
	}

	return true;
}

bool check_parentheses(int p, int q) {
	int a[40], i;
	for(i = p; i < q; ++i) {
		a[i+1] = a[i];
		if(tokens[i].type == '(') a[i+1] ++;
		if(a[i+1] <= 0) return false;
	}
	return true;
}

int eval(int p, int q, bool *success) {
		int ret = 0;
    if(p > q) {
				success = false;
				return 0;
        /* Bad expression */
    }
    else if(p == q) {
        /* Single token.
         * For now this token should be a number.
         * Return the value of the number.
         */
				 if(token[p].type == NUM) {
					 char *val = tokens[p].str;
					 while(val) {
						 ret = ret * 10 + *val - '0';
						 val++;
					 }
					 return ret;
				 }
				 else {
					 success = false;
					 return 0;
				 }
    }
    else if(check_parentheses(p, q) == true) {
        /* The expression is surrounded by a matched pair of parentheses.
         * If that is the case, just throw away the parentheses.
         */
        return eval(p + 1, q - 1, success);
    }
    else {
        /* We should do more things here. */
				int i;
				bool flag = true;
				if(tokens[p].type == '-' || tokens[p].type == '+' || tokens[p].type == '*' || tokens[p].type == '/') flag = false;
				if(tokens[q].type == '-' || tokens[q].type == '+' || tokens[q].type == '*' || tokens[q].type == '/') flag = false;
				if(!flag) {
					success = false;
					printf("Your operation is illegal.\n");
					return 0;
				}

				int cnt = 0;
				for(i = p; i <= q; ++i) {
					if(tokens[i].type == '(') cnt++;
					if(tokens[i].type == ')') cnt--;
					if(cnt == 0 && (tokens[i].type == '-' || tokens[i].type == '+')) {
						int val1 = eval(p, i - 1, success), val2 = eval(i + 1, q, success);
						if(tokens[i].type == '-') return val1 - val2;
						if(tokens[i].type == '+') return val1 + val2;
					}
				}

				for(i = p; i <= q; ++i) {
					if(tokens[i].type == '(') cnt++;
					if(tokens[i].type == ')') cnt--;
					if(cnt == 0 && (tokens[i].type == '*' || tokens[i].type == '/')) {
						int val1 = eval(p, i - 1, success), val2 = eval(i + 1, q, success);
						if(tokens[i].type == '*') return val1 * val2;
						if(tokens[i].type == '/') return val1 / val2;
				}
    }
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	/* TODO: Insert codes to evaluate the expression. */

	int val = eval(0, nr_token - 1, success);
	if(!*success) {
		panic("please implement me");
		return 0;
	}
	return val;
}
