#include <isa.h>
#include <stdlib.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_PLUS, TK_SUB, TK_MULTI, TK_DIV, TK_OPEN_PAREN, TK_CLOSE_PAREN, TK_NUM,

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"==", TK_EQ},        // equal
  {"\\+", TK_PLUS},     // plus
  {"\\-", TK_SUB},      // subtract
  {"/", TK_DIV},        // divide
  {"\\*", TK_MULTI},    // multiply
  {"\\(", TK_OPEN_PAREN}, // open paren
  {"\\)", TK_CLOSE_PAREN},// close paren
  {"[0-9]+", TK_NUM},   // number
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          default: 
            tokens[nr_token].type = rules[i].token_type;
            memset(tokens[nr_token].str, 0, sizeof(tokens[nr_token].str));
            memcpy(tokens[nr_token].str, substr_start, substr_len);
            nr_token++;
            break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static int tk_priority(int tk) {
  switch (tk)
  {
  case TK_EQ:
    return 1;
  case TK_PLUS:
    return 2;
  case TK_SUB:
    return 2;
  case TK_DIV:
    return 3;
  case TK_MULTI:
    return 3;
  default:
    return 0;
  }
}

static int find_split_pos(int tk_start, int tk_end) {
  int pos = tk_start;
  int paren_flag = 0;
  for (int i = tk_start; i <= tk_end; i ++) {
    if (tokens[i].type == TK_OPEN_PAREN) {
      paren_flag ++;
    } else if (tokens[i].type == TK_CLOSE_PAREN) {
      paren_flag --;
    } else if (tokens[i].type != TK_NUM && tk_priority(tokens[i].type) >= tk_priority(tokens[i].type) && paren_flag == 0) {
      pos = i;
    }
  }
  return pos;
}

static bool match_paren(int tk_start, int tk_end) {
  if (tokens[tk_start].type != TK_OPEN_PAREN || tokens[tk_end].type != TK_CLOSE_PAREN) {
    return false;
  }
  while (tk_start < tk_end) {
    if (tokens[tk_start].type != TK_OPEN_PAREN && tokens[tk_start].type != TK_CLOSE_PAREN) {
      tk_start ++;
    }
    if (tokens[tk_end].type != TK_OPEN_PAREN && tokens[tk_end].type != TK_CLOSE_PAREN) {
      tk_end --;
    }
    if (tokens[tk_start].type == TK_OPEN_PAREN && tokens[tk_end].type == TK_CLOSE_PAREN) {
      tk_start ++;
      tk_end --;
    }
  }
  return tokens[tk_start].type != TK_CLOSE_PAREN && tokens[tk_start].type != TK_OPEN_PAREN;
}

static word_t calc_expr(int head, int tail) {
  if (head > tail) {
    Log("head > tail");
    return 0;
  } else if (head == tail) {
    if (tokens[head].type == TK_NUM) {
      return (word_t)(atoi(tokens[head].str));
    } else {
      Log("not num");
      return 0;
    }
  } else if (match_paren(head, tail)) {
    return calc_expr(head + 1, tail - 1);
  } else {
    int pos = find_split_pos(head, tail);
    int left_val = calc_expr(head, pos - 1);
    int right_val = calc_expr(pos + 1, tail);
    Log("left value:%d right value: %d, token type: %d", 
        left_val, right_val, tokens[pos].type);

    switch (tokens[pos].type)
    {
    case TK_PLUS:
      return left_val + right_val;
    case TK_SUB:
      return left_val - right_val;
    case TK_MULTI:
      return left_val * right_val;
    case TK_DIV: 
      return right_val==0?-1:left_val/right_val;
    default:
      return 0;
    }
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    printf("parse token failed");
    return 0;
  }

  word_t res = calc_expr(0, nr_token-1);
  Log("calc expression result:%d", res);
  *success = true;
  return res;
}



