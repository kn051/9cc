#define _GNU_SOURCE
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// トークナイザー (tokenize.c)
//

// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_IDENT,    // 識別子
  TK_NUM,      // 整数トークン
  TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

// トークンの型
typedef struct Token Token;
struct Token {
  TokenKind kind;
  Token *next;
  long val;
  char *str;
  int len;
};

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...);

// エラー箇所を報告する
// loc は入力全体を表す文字列の途中を指しているポインタ
// fmt は入力の先頭を指しているポインタ
void error_at(char *loc, char *fmt, ...);

// 次のトークンが期待している記号の時には、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op);

// 目的：トークンの種類が識別子かどうかを調べる。
// 違う場合は NULL を返す。もしそうなら、トークンを1つ読み進めてそのポインタを返す。
Token *consume_ident(void);

// 次のトークンが期待している記号の時には、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op);

// 次のトークンが数値の場合、トークンを１つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
long expect_number(void);

// 目的：トークンの種類がTK_EOFかどうかを調べる
// at_eof : bool
bool at_eof(void);

// 入力文字列 p をトークナイズしてそれを返す
Token *tokenize();

// 入力プログラム
extern char *user_input;
// 現在着目しているトークン
extern Token *token;

//
// パーサー (parse.c)
//

// ローカル変数の型
typedef struct Var Var;
struct Var {
  Var *next;
  char *name; // 変数の名前
  int offset; // RBPからのオフセット
};

// 抽象構文木のノードの種類
typedef enum {
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LT,        // <
  ND_LE,        // <=
  ND_ASSIGN,    // =
  ND_RETURN,    // "return"
  ND_IF,        // "if"
  ND_WHILE,     // "while"
  ND_FOR,       // "for"
  ND_BLOCK,     // { ... }
  ND_EXPR_STMT, // Expression statement
  ND_VAR,       // Variable
  ND_NUM,       // Integer
} NodeKind;

// ノードの型
typedef struct Node Node;
struct Node {
  NodeKind kind;  // ノードの型
  Node *next;     // 次のノード

  Node *lhs;      // 左辺
  Node *rhs;      // 右辺

  // "if", "while" or "for" statement
  Node *cond;
  Node *then;
  Node *els;
  Node *init;
  Node *inc;

  // Block
  Node *body;
  
  Var *var;      // kind が ND_VAR の場合のみ使う
  long val;       // kind が ND_NUM の場合のみ使う
};


// 関数の型
typedef struct Function Function;
struct Function {
  Node *node;
  Var *locals;
  int stack_size;
};

Function *program(void);


//
// Code generator (codegen.c)
//

void codegen(Function *prog);


