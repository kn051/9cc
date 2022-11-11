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
  TK_NUM,      // 整数トークン
  TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

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
  ND_RETURN,    // "return"
  ND_NUM,       // Integer
} NodeKind;

// ノードの型
typedef struct Node Node;

struct Node {
  NodeKind kind;  // ノードの型
  Node *next;     // 次のノード
  Node *lhs;      // 左辺
  Node *rhs;      // 右辺
  long val;        // kind が ND_NUM の場合のみ使う
};



Node *program(void);


//
// Code generator (codegen.c)
//

void codegen(Node *node);


