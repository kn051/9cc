#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type Type;

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

void error_tok(Token *tok, char *fmt, ...);

// 目的：文字列を受け取り、現在のトークンとマッチするかどうかを調べる。
// マッチしていれば、トークンを返す。
// peek : char * -> Token || NULL
Token *peek(char *s);

// 次のトークンが期待している記号の時には、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
Token *consume(char *op);

// 目的：トークンの種類が識別子かどうかを調べる。
// 違う場合は NULL を返す。もしそうなら、トークンを1つ読み進めてそのポインタを返す。
Token *consume_ident(void);

// 次のトークンが期待している記号の時には、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op);

// 次のトークンが数値の場合、トークンを１つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
long expect_number(void);

// 目的：現在のトークンが識別子だった場合、トークンを１つ読み進めてその識別子を返す。
// expect_ident : void -> char || NULL
char *expect_ident(void);

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

// 変数の型
typedef struct Var Var;
struct Var {
  char *name;     // 変数の名前
  Type *ty;       // 型
  bool is_local;  // ローカル変数かどうかの真偽値

  // ローカル変数の場合のスタック上の位置
  int offset;     // RBPからのオフセット
};

// ローカル変数の連結リストの型
typedef struct VarList VarList;
struct VarList {
  VarList *next;
  Var *var;
};

// 抽象構文木のノードの種類
typedef enum {
  ND_ADD,       // num + num
  ND_PTR_ADD,   // ptr + num or num + ptr
  ND_SUB,       // num - num
  ND_PTR_SUB,   // ptr - num
  ND_PTR_DIFF,  // ptr - ptr
  ND_MUL,       // *
  ND_DIV,       // /
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LT,        // <
  ND_LE,        // <=
  ND_ASSIGN,    // =
  ND_ADDR,      // unary &
  ND_DEREF,     // unary *
  ND_RETURN,    // "return"
  ND_IF,        // "if"
  ND_WHILE,     // "while"
  ND_FOR,       // "for"
  ND_BLOCK,     // { ... }
  ND_FUNCALL,   // Function Call
  ND_EXPR_STMT, // Expression statement
  ND_VAR,       // Variable
  ND_NUM,       // Integer
  ND_NULL,      // Empty statement
} NodeKind;

// ノードの型
typedef struct Node Node;
struct Node {
  NodeKind kind;  // ノードの種類
  Node *next;     // 次のノード
  Type *ty;       // Type, e.g. int or pointer to int
  Token *tok;     // Representative token

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

  // Function Call
  char *funcname;
  Node *args;
  
  Var *var;      // kind が ND_VAR の場合のみ使う
  long val;       // kind が ND_NUM の場合のみ使う
};


// 関数の型
typedef struct Function Function;
struct Function {
  Function *next;
  char *name;
  VarList *params; // 引数の連結リスト

  Node *node;
  VarList *locals; // ローカル変数の連結リスト
  int stack_size;
};

// プログラムの型
typedef struct {
  VarList *globals;   // グローバル変数
  Function *fns;      // 関数
} Program;

Program *program(void);

//
// type.c
//

// 型の種類
typedef enum {
  TY_INT,     // int型
  TY_PTR,     // ~へのポインタ型
  TY_ARRAY,   // 配列の型
} TypeKind;

struct Type {
  TypeKind kind;
  int size;       // sizeof()の値
  Type *base;     // 〜が指す Type オブジェクトへのポインタ。kind がTY_PTRの場合のみ使う。
  int array_len;  // 配列の要素数を持つ変数
};

extern Type *int_type;

bool is_integer(Type *ty);
Type *pointer_to(Type *base);
Type *array_of(Type *base, int size);
void add_type(Node *node);


//
// Code generator (codegen.c)
//

void codegen(Program *prog);


