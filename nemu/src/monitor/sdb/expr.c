#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// implement binary tree
// need method: add_branch

enum { BT_ADD, BT_SUB, BT_MUL, BT_DIV, BT_NUM, BT_EXPR };

typedef struct Node {
  struct Node *parent;
  struct Node *left;
  struct Node *right;
  int type;
  union {
    int value;
    char content[20];
  };
} node_t;

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
    {"[0-9]*", TK_NUM}, // equal
    {"\\(", TK_BKT_L},  // left bracket
    {"\\)", TK_BKT_R},  // right bracker
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
  char str[32];
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
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i,
            rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

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
          memcpy(tokens[nr_token].str, e + position + pmatch.rm_so,
                 sizeof(char) * (pmatch.rm_eo - pmatch.rm_so)); // copy
          tokens[nr_token].str[pmatch.rm_eo - pmatch.rm_so] = '\0';
        }
        break;
      }
    }
    position += pmatch.rm_eo;

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  node_t *head = NULL;
  node_t *temp_node;
  // add branches
  // +, -:

  // assume already checked
  int inside_bkt = false;

  for (int i = 0; i < nr_token; i++) {
    switch (tokens[i].type) {
    case TK_BKT_L:
      inside_bkt++;
      break;
    case TK_BKT_R:
      inside_bkt--;
      break;
    case TK_ADD:
    case TK_SUB:
      if (inside_bkt == 0) {
        temp_node = (node_t *)malloc(sizeof(node_t));
        temp_node->parent = NULL;
        temp_node->left = head;
        temp_node->type = tokens[i].type;
        head = temp_node;
      }
      break;
    case TK_MUL:
    case TK_DIV:
    }
  }

  /* TODO: Insert codes to evaluate the expression. */
  // TODO();

  // inside brackets

  // mul / div

  // plus / minus

  return 0;
}
