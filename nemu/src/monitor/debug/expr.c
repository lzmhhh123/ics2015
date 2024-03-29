#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ, NUM, NUM16, REG, NEQ, AND, OR, VAR

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
	{"/", '/'},						//dev
	{"!=", NEQ},
	{"&&", AND},
	{"\\|\\|", OR},
	{"\\!", '!'},
	{"0[xX][0-9a-fA-F]+", NUM16},
	{"[0-9]+", NUM},       //number
	{"\\$[a-zA-Z]+", REG},
	{"[a-zA-Z_]+[a-zA-Z0-9_]*", VAR}
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

int get_var(char*);

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

				//Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
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
					case '!':
					case OR:
					case AND:
					case NEQ:
					case EQ:
					case ')': break;
					case NUM:
					case VAR:
					case REG:
					case NUM16:
						if(substr_len > 31) {
							printf("The input number is too long.");
							return false;
						}
						strncpy(tokens[nr_token].str, substr_start, substr_len);
						tokens[nr_token].str[substr_len] = '\0';
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
		if(tokens[i].type == '(') cnt++;
		if(tokens[i].type == ')') cnt--;
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
	for(i = p; i <= q; ++i) a[i] = 0;
	for(i = p; i < q; ++i) {
		a[i+1] = a[i];
		if(tokens[i].type == '(') a[i+1]++;
		if(tokens[i].type == ')') a[i+1]--;
		if(a[i+1] <= 0) return false;
	}
	return true;
}

int myeval(int p, int q, bool *success) {
		int ret = 0;
    if(p > q) {
				*success = false;
				return 0;
        /* Bad expression */
    }
    else if(p == q) {
        /* Single token.
         * For now this token should be a number.
         * Return the value of the number.
         */
				 if(tokens[p].type == NUM) {
					 char *val = tokens[p].str;
					 while(*val) {
						 ret = ret * 10 + *val - '0';
						 val++;
					 }
					 return ret;
				 }
				 else if(tokens[p].type == NUM16) {
					 char *val = tokens[p].str + 2;
					 while(*val) {
						 if(*val >= '0' && *val <= '9')
						 	ret = ret * 16 + *val - '0';
						 else if(*val >= 'a' && *val <= 'f')
						 	ret = ret * 16 + *val - 'a' + 10;
						 else ret = ret * 16 + *val - 'A' + 10;
						 val++;
					 }
					 return ret;
				 }
				 else if(tokens[p].type == VAR) {
					 ret = get_var(tokens[p].str);
					 if(ret == -1) {
						 *success = false;
						 printf("Can't find variable: %s\n", tokens[p].str);
						 return 0;
					 }
					 else return ret;
				 }
				 else if(tokens[p].type == REG) {
					 char *reg = tokens[p].str + 1;
					 int i = 0;
					 if(strcmp(reg, "eip") == 0) {
						 return cpu.eip;
					 }
					 if(strcmp(reg, "cf") == 0) {
						 return cpu.CF;
					 }
					 if(strcmp(reg, "pf") == 0) {
						 return cpu.PF;
					 }
					 if(strcmp(reg, "zf") == 0) {
						 return cpu.ZF;
					 }
					 if(strcmp(reg, "sf") == 0) {
						 return cpu.SF;
					 }
					 if(strcmp(reg, "if") == 0) {
						 return cpu.IF;
					 }
					 if(strcmp(reg, "df") == 0) {
						 return cpu.DF;
					 }
					 if(strcmp(reg, "of") == 0) {
						 return cpu.OF;
					 }
					 for(i = 0; i < 8; ++i)
          	if(strcmp(regsl[i], reg) == 0)
        			return reg_l(i);
           for(i = 0; i < 8; ++i)
        		if(strcmp(regsw[i], reg) == 0)
            	return reg_w(i);
           for(i = 0; i < 8; ++i)
            if(strcmp(regsb[i], reg) == 0)
            	return reg_b(i);
           printf("didn't find register : %s\n", tokens[p].str);
           *success = false;
					 return 0;
				 }

				 *success = false;
				 return 0;
    }
    else if(check_parentheses(p, q) == true) {
        /* The expression is surrounded by a matched pair of parentheses.
         * If that is the case, just throw away the parentheses.
         */
        return myeval(p + 1, q - 1, success);
    }
    else {
        /* We should do more things here. */
				int i;
				bool flag = true;
				if(tokens[p].type == '-' || tokens[p].type == '+' || tokens[p].type == '*' || tokens[p].type == '/') flag = false;
				if(tokens[q].type == '-' || tokens[q].type == '+' || tokens[q].type == '*' || tokens[q].type == '/') flag = false;
				if(!flag) {
					*success = false;
					printf("Your operation is illegal.\n");
					return 0;
				}

				int cnt = 0;
				for(i = p; i <= q; ++i) {
					if(tokens[i].type == '(') cnt++;
					if(tokens[i].type == ')') cnt--;
					if(cnt == 0 && (tokens[i].type == EQ || tokens[i].type == NEQ)) {
						int val1 = myeval(p, i - 1, success), val2 = myeval(i + 1, q, success);
						if(tokens[i].type == EQ) return val1 == val2;
						if(tokens[i].type == NEQ) return val1 != val2;
					}
				}

				cnt = 0;
				for(i = p; i <= q; ++i) {
					if(tokens[i].type == '(') cnt++;
					if(tokens[i].type == ')') cnt--;
					if(cnt == 0 && tokens[i].type == OR) {
						int val1 = myeval(p, i - 1, success), val2 = myeval(i + 1, q, success);
						return val1 || val2;
					}
				}

				cnt = 0;
				for(i = p; i <= q; ++i) {
					if(tokens[i].type == '(') cnt++;
					if(tokens[i].type == ')') cnt--;
					if(cnt == 0 && tokens[i].type == AND) {
						int val1 = myeval(p, i - 1, success), val2 = myeval(i + 1, q, success);
						return val1 && val2;
					}
				}

				cnt = 0;
				for(i = p; i <= q; ++i) {
					if(tokens[i].type == '(') cnt++;
					if(tokens[i].type == ')') cnt--;
					if(cnt == 0 && (tokens[i].type == '-' || tokens[i].type == '+')) {
						int val1 = myeval(p, i - 1, success), val2 = myeval(i + 1, q, success);
						if(tokens[i].type == '-') return val1 - val2;
						if(tokens[i].type == '+') return val1 + val2;
					}
				}

				cnt = 0;
				for(i = p; i <= q; ++i) {
					if(tokens[i].type == '(') cnt++;
					if(tokens[i].type == ')') cnt--;
					if(cnt == 0 && (tokens[i].type == '-' || tokens[i].type == '+')) {
						int val1 = myeval(p, i - 1, success), val2 = myeval(i + 1, q, success);
						if(tokens[i].type == '-') return val1 - val2;
						if(tokens[i].type == '+') return val1 + val2;
					}
				}

				cnt = 0;
				for(i = p; i <= q; ++i) {
					if(tokens[i].type == '(') cnt++;
					if(tokens[i].type == ')') cnt--;
					if(cnt == 0 && ((tokens[i].type == '*' && i != p && (tokens[i-1].type == NUM || tokens[i-1].type == NUM16 || tokens[i-1].type == REG || tokens[i-1].type == ')')) || tokens[i].type == '/')) {
						int val1 = myeval(p, i - 1, success), val2 = myeval(i + 1, q, success);
						if(tokens[i].type == '*') return val1 * val2;
						if(tokens[i].type == '/') return val1 / val2;
					}
				}

				if(tokens[p].type == '*') {
					int pos = myeval(p + 1, q, success);
					int i = 0;
					int ret = 0;
					for(; i < 4; ++i) {
						ret = ret * 256 + *(unsigned char *)hwa_to_va(pos + i);
					}
					return ret;
				}
				if(tokens[p].type == '!') return !myeval(p + 1, q, success);

				*success = false;
				return 0;
    }
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	/* TODO: Insert codes to myevaluate the expression. */
	*success = true;
	int val = myeval(0, nr_token - 1, success);
	return val;
}
