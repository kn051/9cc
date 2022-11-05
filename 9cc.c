#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// トークナイザー
//

// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_NUM,      // 整数トークン
  TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
  TokenKind kind; // トークンの型
  Token *next;    // 次の入力トークン
  int val;        // kindがTK_NUMの場合、その数値
  char *str;      // トークン文字列
};

// 入力プログラム
char *user_input;

// 現在着目しているトークン
Token *token;

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// エラー箇所を報告する
// loc は入力全体を表す文字列の途中を指しているポインタ
// fmt は入力の先頭を指しているポインタ
void error_at(char *loc, char *fmt, ...) {
  va_list ap; 
  va_start(ap, fmt); // 可変長引数を1個の変数にまとめる

  int pos = loc - user_input; // 文字列の途中と先頭を指すポインタの差
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " "); // pos個の空白を出力
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap); // まとめられた変数で処理する
  fprintf(stderr, "\n");
  exit(1);
}

// 次のトークンが期待している記号の時には、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op)
    return false;
  token = token->next;
  return true;
}

// 次のトークンが期待している記号の時には、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op)
    error_at(token->str, "'%c'ではありません", op);
  token = token->next;
}

// 次のトークンが数値の場合、トークンを１つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
  if (token->kind != TK_NUM)
    error_at(token->str, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

// 入力文字列 p をトークナイズしてそれを返す
Token *tokenize() {
  char *p = user_input;
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // 空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (strchr("+-*/()", *p)) {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p);
  return head.next;
}

//
// パーサー
//

// 抽象構文木のノードの種類
typedef enum {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
  ND_NUM,
} NodeKind;

// ノードの型
typedef struct Node Node;

struct Node {
  NodeKind kind;
  Node *lhs;
  Node *rhs;
  int val;
};

// 目的：Nodeを新しく作る
// new_node : NodeKind -> Node
Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

// 目的：右辺と左辺を受け取る2項演算子のNodeを作る
// new_binary : NodeKind -> Node -> Node -> Node
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

// 目的：数値のノードを新しく作る
// new_num : int -> Node
Node *new_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

// 再帰的に使えるように先に宣言しておく
Node *expr();
Node *mul();
Node *unary();
Node *primary();

// 目的：＋とーの演算子をパースする
// expr : Node
// expr = mul ("+" mul | "-" mul)*
Node *expr() {
  Node *node = mul();

  for (;;) {
    if (consume('+'))
      node = new_binary(ND_ADD, node, mul());
    else if (consume('-'))
      node = new_binary(ND_SUB, node, mul());
    else
      return node;
  }
}

// 目的：*と/の演算子をパースする
// mul : Node
// mul = unary ("*" unary | "/" unary)*
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume('*'))
      node = new_binary(ND_MUL, node, unary());
    else if (consume('/'))
      node = new_binary(ND_DIV, node, unary());
    else
      return node;
  }
}

// 目的：正負の記号をパースする
// unary : Node
// unary = ("+" | "-")? unary | primary
Node *unary() {
  if (consume('+'))
    return unary();
  if (consume('-'))
    return new_binary(ND_SUB, new_num(0), unary());
  return primary();
}


// 目的：(expr)とnumをパースする
// primary : Node
// primary = "(" expr ")" | num
Node *primary() {
  if (consume('(')) {
    Node *node = expr();
    expect(')');
    return node;
  }

  return new_num(expect_number());
}


//
// Code generator
//

// 目的：Node を受け取り、スタックマシンの要領でアセンブリコードを吐き出す
// gen : Node -> アセンブリコードの吐き出し
void gen (Node *node) {
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  }

  printf("  push rax\n");
}



int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: 引数の個数が正しくありません", argv[0]);

  // トークナイズしてパースする
  user_input = argv[1];
  token = tokenize();
  Node *node = expr();

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // 抽象構文木を降りながらコード生成
  gen(node);

  // スタックトップに式全体の値が残っているはずなので、
  // それをRAXにロードして関数からの返り値とする
  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}
