#include "9cc.h"

char *user_input;
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
  fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap); // まとめられた変数で処理する
  fprintf(stderr, "\n");
  exit(1);
}

// 次のトークンが期待している記号の時には、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op) {
  if (token->kind != TK_RESERVED ||
      strlen(op) != token->len ||
      memcmp(token->str, op, token->len)) // 引数1と引数2を引数3のバイト数分だけ比較する。=だと0、それ以外だと正負の値を返す
    return false;
  token = token->next;
  return true;
}

// 目的：トークンの種類が識別子かどうかを調べる。
// 違う場合は NULL を返す。もしそうなら、トークンを1つ読み進めてそのポインタを返す。
// consume_ident : Void -> NULL || *Token
Token *consume_ident(void) {
  if (token->kind != TK_IDENT)
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
}

// 次のトークンが期待している記号の時には、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
  if (token->kind != TK_RESERVED ||
      strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    error_at(token->str, "'%s'ではありません", op);
  token = token->next;
}

// 次のトークンが数値の場合、トークンを１つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
long expect_number(void) {
  if (token->kind != TK_NUM)
    error_at(token->str, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

// 目的：トークンの種類がTK_EOFかどうかを調べる
// at_eof : bool
bool at_eof() {
  return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurに繋げる
static Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str  = str;
  tok->len  = len;
  cur->next = tok;
  return tok;
}

// 目的：2つの文字列が等しいかどうかを調べる
// startswith : char * -> char * ->　bool
static bool startswith(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}

// 目的：文字 c が a ~ z, A ~ Z, _ かを調べる
// is_alpha : char -> bool
static bool is_alpha(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

// 目的：文字 c が is_alpha か 0 ~ 9 かを調べる
// is_alnum : char -> bool
static bool is_alnum(char c) {
  return is_alpha(c) || ('0' <= c && c <= '9');
}


// 入力文字列 p をトークナイズしてそれを返す
Token *tokenize(void) {
  char *p = user_input;
  Token head = {};
  Token *cur = &head;

  while (*p) {
    // 空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    // 文字列 p が "return" の場合、新しいトークンを作る
    if (startswith(p, "return") && !is_alnum(p[6])) {
      cur = new_token(TK_RESERVED, cur, p, 6);
      p += 6;
      continue;
    }

    // 文字列 p が変数の場合、新しい識別子のトークンを作る
    if (is_alpha(*p)) {
      char *q = p;
      while (is_alnum(*p))
        p++;
      cur = new_token(TK_IDENT, cur, q, p - q);
      continue;
    }

    // 複数文字のpunctuator
    if (startswith(p, "==") || startswith(p, "!=") ||
        startswith(p, "<=") || startswith(p, ">=")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    // 1文字のpunctuator
    if (ispunct(*p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    // 整数リテラル
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}