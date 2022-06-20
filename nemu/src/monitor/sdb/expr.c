#include "common.h"
#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

enum {
  TK_NOTYPE = 256,
  TK_ADD,
  TK_SUB,
  TK_MUL,
  TK_DIV,
  TK_NUM,
  TK_BKT_L,
  TK_BKT_R,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
    {"\\s", TK_NOTYPE}, // spaces
    {"\\+", TK_ADD},    // plus
    {"-", TK_SUB},      // minus
    {"\\*", TK_MUL},    // multiply
    {"/", TK_DIV},      // divide
    {"\\(", TK_BKT_L},  // left bracket
    {"\\)", TK_BKT_R},  // right bracker
    {"[0-9]+", TK_NUM}, // equal
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.>
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  int num;
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e) {
  /*
    Eric's comments
    Here recursively process the string until No More Bracket Pair Found
  */
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 &&
          pmatch.rm_so == 0) {
        // Pattern found at [(e + position + pmatch.so), (e + position +
        // pmatch.eo)]
        char *substr_start = e + position + pmatch.rm_so;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i,
            rules[i].regex, position, substr_len, substr_len, substr_start);

        switch (rules[i].token_type) {
        case TK_NOTYPE:
          break;
        case TK_ADD:
        case TK_SUB:
        case TK_MUL:
        case TK_DIV:
        case TK_BKT_L:
        case TK_BKT_R:
          tokens[nr_token++].type = rules[i].token_type;
          break;
        case TK_NUM:
          tokens[nr_token].type = TK_NUM;
          tokens[nr_token++].num = strtol(substr_start, NULL, 10);
          break;
        }
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    } else {
      position += pmatch.rm_eo;
    }
  }

  return true;
}

int _eval(Token *tokens, int token_n) {
  // brackets
  int bkt_left, bkt_right, bkt_level;

  // eliminate all the brackets
  for (int tk_idx = 0; tk_idx < token_n; tk_idx = bkt_right + 1) {
    bkt_left = -1;
    bkt_right = -1;
    for (int i = tk_idx; i < token_n; i++) {
      if (tokens[i].type == TK_BKT_L) {
        bkt_left = i;
        break;
      }
    }
    bkt_level = 1;

    for (int i = bkt_left + 1; i < token_n; i++) {
      if (tokens[i].type == TK_BKT_L) {
        bkt_level++;
      } else if (tokens[i].type == TK_BKT_R) {
        bkt_level--;
        if (bkt_level == 0) {
          bkt_right = i;
          break;
        }
      }
    }

    if (bkt_left != -1 && bkt_right != -1) {
      tokens[bkt_left].num =
          _eval(tokens + bkt_left + 1, bkt_right - bkt_left - 1);
      tokens[bkt_left].type = TK_NUM;
      for (int i = bkt_left + 1; i <= bkt_right; i++) {
        tokens[i].type = TK_NOTYPE;
      }
    } else {
      break;
    }
  }

  // print_tokens(tokens, 32);
  // no bracket cases

  // find all * and /
  Token *tk_left = NULL, *tk_right = NULL, *tk_operator = NULL;

  int result = 0;

  for (int i = 0; i < token_n; i++) {
    switch (tokens[i].type) {
    case TK_NUM:
      if (tk_operator == NULL) {
        tk_left = tokens + i;
        tk_right = NULL;
      } else {
        tk_right = tokens + i;
      }
      break;
    case TK_MUL:
    case TK_DIV:
      tk_operator = tokens + i;
      break;
    }

    if (tk_right) {
      switch (tk_operator->type) {
      case TK_MUL:
        result = tk_left->num * tk_right->num;
        break;
      case TK_DIV:
        result = tk_left->num / tk_right->num;
        break;
      }

      tk_left->type = TK_NUM;
      tk_left->num = result;

      tk_operator->type = TK_NOTYPE;
      tk_operator = NULL;

      tk_right->type = TK_NOTYPE;
      tk_right = NULL;
    }
  }

  // add and sub
  result = 0;
  int operator= TK_ADD;

  for (int i = 0; i < token_n; i++) {
    switch (tokens[i].type) {
    case TK_ADD:
    case TK_SUB:
      operator= tokens[i].type;
      break;
    case TK_NUM:
      if (operator== TK_ADD)
        result += tokens[i].num;
      else if (operator== TK_SUB)
        result -= tokens[i].num;
      break;
    }
  }

  return result;
}

void print_tokens() {
  for (int i = 0; i < 32; i++) {
    if (tokens[i].type == TK_ADD)
      printf(" + ");
    else if (tokens[i].type == TK_SUB)
      printf(" - ");
    else if (tokens[i].type == TK_MUL)
      printf(" * ");
    else if (tokens[i].type == TK_DIV)
      printf(" / ");
    else if (tokens[i].type == TK_BKT_L)
      printf(" ( ");
    else if (tokens[i].type == TK_BKT_R)
      printf(" ) ");
    else if (tokens[i].type == TK_NUM)
      printf(" %d ", tokens[i].num);
  }
  printf("\n");
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  print_tokens();
  return (word_t)_eval(tokens, nr_token);
}