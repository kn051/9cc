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
static void verror_at(char *loc, char *fmt, va_list ap) {
  int pos = loc - user_input; // 文字列の途中と先頭を指すポインタの差
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap); // まとめられた変数で処理する
  fprintf(stderr, "\n");
  exit(1);
}

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(loc, fmt, ap);
}

// エラー箇所を報告する
void error_tok(Token *tok, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(tok->str, fmt, ap);
}

// 次のトークンが期待している記号の時には、トークンを1つ読み進めて
// トークンのポインタを返す。違う場合は NULL を返す。
Token *consume(char *op) {
  if (token->kind != TK_RESERVED ||
      strlen(op) != token->len ||
      strncmp(token->str, op, token->len)) // 引数1と引数2を引数3のバイト数分だけ比較する。=だと0、それ以外だと正負の値を返す
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
}

// 目的：文字列を受け取り、現在のトークンとマッチするかどうかを調べる。
// マッチしていれば、トークンを返す。
// peek : char * -> Token || NULL
Token *peek(char *s) {
  if (token->kind != TK_RESERVED || strlen(s) != token->len ||
      strncmp(token->str, s, token->len))
    return NULL;
  return token;
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
void expect(char *s) {
  if (!peek(s))
    error_tok(token, "'%s'ではありません", s);
  token = token->next;
}

// 次のトークンが数値の場合、トークンを１つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
long expect_number(void) {
  if (token->kind != TK_NUM)
    error_tok(token, "数ではありません");
  long val = token->val;
  token = token->next;
  return val;
}

// 目的：現在のトークンが識別子だった場合、トークンを１つ読み進めつつ、現在のトークンの識別子を返す。
// expect_ident : void -> char || NULL
char *expect_ident(void) {
  if (token->kind != TK_IDENT)
    error_tok(token, "識別子ではありません");
  char *s = strndup(token->str, token->len);
  token = token->next;
  return s;
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
  return strncmp(p, q, strlen(q)) == 0; // 文字列 q 分の長さだけ比較する
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

// 目的：文字列を受けとり、return, if, else, while, for, int,
// あるいは、複数文字の区切り記号のどれかに等しければ該当文字列を返す
// *starts_with_reserved : char * -> char
static char *starts_with_reserved(char *p) {
  // キーワード
  static char *kw[] = {"return", "if", "else", "while", "for", "int", "char", "sizeof"};

  for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
    int len = strlen(kw[i]);
    if (startswith(p, kw[i]) && !is_alnum(p[len]))
      return kw[i];
  }

  // 複数文字の区切り記号 (punctuator)
  static char *ops[] = {"==", "!=", "<=", ">="};

  for (int i = 0; i < sizeof(ops) / sizeof(*ops); i++)
    if (startswith(p, ops[i]))
      return ops[i];
  
  return NULL;
}

// 目的：文字を受け取り、エスケープ文字にして返す
// get_escape_char : char -> char
static char get_escape_char(char c) {
  switch (c) {
  case 'a': return '\a';
  case 'b': return '\b';
  case 't': return '\t';
  case 'n': return '\n';
  case 'v': return '\v';
  case 'f': return '\f';
  case 'r': return '\r';
  case 'e': return 27;
  case '0': return 0;
  default : return c;
  }
}

// 目的：トークンと文字列を受けとり、文字列リテラルの新しいトークンを返す
// read_string_literal : Token -> char -> Token
static Token *read_string_literal(Token *cur, char *start) {
  char *p = start + 1;
  char buf[1024];
  int len = 0;

  for (;;) {
    if (len == sizeof(buf))
      error_at(start, "string literal too large");
    if (*p == '\0')
      error_at(start, "unclosed string literal");
    if (*p == '"')
      break;
    
    if (*p == '\\') {
      p++;
      buf[len++] = get_escape_char(*p++);
    } else {
      buf[len++] = *p++;
    }
  }

  Token *tok = new_token(TK_STR, cur, start, p - start + 1);
  tok->contents = malloc(len + 1);
  memcpy(tok->contents, buf, len);
  tok->contents[len] = '\0';
  tok->cont_len = len + 1;
  return tok;
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

    // 文字列リテラルのトークン化
    if (*p == '"') {
      cur = read_string_literal(cur, p);
      p += cur->len;
      continue;
    }

    // 文字列 p がキーワード、あるいは複数文字の区切り記号の場合、新しいトークンを作る
    char *kw = starts_with_reserved(p);
    if (kw) {
      int len = strlen(kw); // 該当キーワードの文字数
      cur = new_token(TK_RESERVED, cur, p, len); // 新たなトークンを作り、cur に繋げる
      p += len; // キーワードの文字数分、入力文字列を読み進める
      continue;
    }

    // 文字列 p が変数の場合、新しい識別子のトークンを作る。
    // 最初の文字は、a~z, A~Z, _のいずれか。
    if (is_alpha(*p)) {
      char *q = p++;
      while (is_alnum(*p))
        p++;
      cur = new_token(TK_IDENT, cur, q, p - q);
      continue;
    }


    // 1文字のpunctuator
    // ispunct() はライブラリ関数
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
