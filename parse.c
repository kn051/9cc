#include "9cc.h"

// ローカル変数の連結リスト。パース中に作られたローカル変数はここに格納される
static VarList *locals;
// グローバル変数の連結リスト。
static VarList *globals;

static VarList *scope;

// 目的：トークン列を受け取り、名前で変数を検索する。見つからなかったらNULLを返す。
// *find_var : *Token -> Var || NULL
static Var *find_var(Token *tok) {
  // ローカル変数を見つける
  for (VarList *vl = scope; vl; vl = vl->next) {
    Var *var = vl->var;
    if (strlen(var->name) == tok->len && !strncmp(tok->str, var->name, tok->len))
      return var;
  }

  // グローバル変数を見つける
  for (VarList *vl = globals; vl; vl = vl->next) {
    Var *var = vl->var;
    if (strlen(var->name) == tok->len && !strncmp(tok->str, var->name, tok->len))
      return var;
  }
  return NULL;
}


// 目的：Nodeを新しく作る
// new_node : NodeKind -> Node
static Node *new_node(NodeKind kind, Token *tok) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->tok = tok;
  return node;
}

// 目的：右辺と左辺を持つ2項演算子の Node を作る
// new_binary : NodeKind -> Node -> Node -> Node
static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
  Node *node = new_node(kind, tok);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

// 目的：左辺のみを持つ単項演算子の Node を作る
// new_unary : NodeKind -> Node -> Token -> Node
static Node *new_unary(NodeKind kind, Node *expr, Token *tok) {
  Node *node = new_node(kind, tok);
  node->lhs = expr;
  return node;
}

// 目的：数値のノードを新しく作る
// new_num : int -> Node
static Node *new_num(long val, Token *tok) {
  Node *node = new_node(ND_NUM, tok);
  node->val = val;
  return node;
}

// 目的：Var 型のポインタを受け取り、変数のノードを新しく作る
// new_var_node : *Var -> Node
static Node *new_var_node(Var *var, Token *tok) {
  Node *node = new_node(ND_VAR, tok);
  node->var = var;
  return node;
}

// 目的：引数にとった型と名前の変数を新たに作る。ローカル変数かどうかの真偽値も入れる。
// *new_lvar : char * -> Type -> bool -> Var
static Var *new_var(char *name, Type *ty, bool is_local) {
  // 引数の名前と型を持つ変数を作る
  Var *var = calloc(1, sizeof(Var));
  var->name = name;
  var->ty = ty;
  var->is_local = is_local;

  VarList *sc = calloc(1, sizeof(VarList));
  sc->var = var;
  sc->next = scope;
  scope = sc;
  return var;
}

// 新たなローカル変数を作る。そして、ローカル変数の連結リストに繋げる。
// new_lvar : char * -> Type -> var
static Var *new_lvar(char *name, Type *ty) {
  Var *var = new_var(name, ty, true);

  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = var;
  vl->next = locals;
  locals = vl;
  return var;
}

// 新たなグローバル変数を作る。そして、グローバル変数の連結リストに繋げる。
// new_gvar : char * -> Type -> var
static Var *new_gvar(char *name, Type *ty) {
  Var *var = new_var(name, ty, false);

  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = var;
  vl->next = globals;
  globals = vl;
  return var;
}

// 目的：文字列リテラル用のメモリを確保する？
// new_label : void -> char
static char *new_label(void) {
  static int cnt = 0;
  char buf[20];
  sprintf(buf, ".L.data.%d", cnt++); // buf に cnt を代入した".L.data.%d"を格納する
  return strndup(buf, 20);  // buf に格納された文字列を複製して返す
}

static Function *function(void);
static Type *basetype(void);
static void global_var(void);
static Node *declaration(void);
static Node *stmt(void);
static Node *stmt2(void);
static Node *expr(void);
static Node *assign(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *postfix(void);
static Node *primary(void);

// 目的： トークンを1つ先読みし、次のトップレベルが関数かグローバル変数かを調べる。
// is_function : void -> bool
static bool is_function(void) {
  Token *tok = token;
  basetype();
  bool isfunc = consume_ident() && consume("(");
  token = tok;
  return isfunc;
}

// program = (global-var | function)*
// program : void -> Function
Program *program(void) {
  Function head = {};
  Function *cur = &head;
  globals = NULL;

  // トークンが関数の場合、パースした関数を連結していく。
  // トークンがグローバル変数の場合、パースしたグローバル変数を連結していく。
  while (!at_eof()) {
    if (is_function()) {
      cur->next = function();
      cur = cur->next;
    } else {
      global_var();
    }
  }
  
  Program *prog = calloc(1, sizeof(Program));
  prog->globals = globals;    // プログラムに含まれるグローバル変数
  prog->fns = head.next;      // プログラムに含まれる関数
  return prog;
}

// 目的：型をつける
// basetype = ("char" | "int") "*"*
// basetype : void -> Type
static Type *basetype(void) {
  Type *ty;
  if (consume("char")) {
    ty = char_type;
  } else {
    expect("int");
    ty = int_type;
  }

  while (consume("*"))
    ty = pointer_to(ty);
  return ty;
}

// 目的：Type 型の base ポインタを受け取り、base の指す型と配列の要素数を持つ配列を返す
// read_type_suffix : Type -> Type
static Type *read_type_suffix(Type *base) {
  if (!consume("["))
    return base;
  int sz = expect_number();
  expect("]");
  base = read_type_suffix(base);
  return array_of(base, sz);
}

// 目的：関数の引数を1つパースする
// read_func_param : void -> VarList
static VarList *read_func_param(void) {
  Type *ty = basetype();
  char *name = expect_ident();
  ty = read_type_suffix(ty);

  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = new_lvar(name, ty);
  return vl;
}

// 目的：関数の引数を全てパースする
// read_func_params : void -> NULL || VarList
static VarList *read_func_params(void) {
  if (consume(")"))
    return NULL;

  VarList *head = read_func_param();
  VarList *cur = head;

  while (!consume(")")) {
    expect(",");
    cur->next = read_func_param();
    cur = cur->next;
  }

  return head;
}

// 目的：関数をパースする
// function = basetype ident "(" params? ")" "{" stmt* "}"
// params   = param ("," param)*
// param    = basetype ident
static Function *function(void) {
  locals = NULL;

  Function *fn = calloc(1, sizeof(Function));
  basetype();
  fn->name = expect_ident();
  expect("(");

  VarList *sc = scope;
  fn->params = read_func_params();
  expect("{");

  Node head = {};
  Node *cur = &head;

  while (!consume("}")) {
    cur->next = stmt();
    cur = cur->next;
  }
  scope = sc;

  fn->node = head.next;
  fn->locals = locals;
  return fn;
}

// 目的：グローバル変数をパースする。program()で使う。
// global-var = basetype ident ("[" num "]")* ";"
// global_var : void -> void
static void global_var(void) {
  Type *ty = basetype();
  char *name = expect_ident();
  ty = read_type_suffix(ty);
  expect(";");
  new_gvar(name, ty);
}

// 目的：変数宣言をパースする
// declaration = basetype ident ("[" num "]")* ("=" expr) ";"
// declaration : void -> Node
static Node *declaration(void) {
  Token *tok =token;
  Type *ty = basetype();
  char *name = expect_ident();
  ty = read_type_suffix(ty);
  Var *var = new_lvar(name, ty);

  if (consume(";"))
    return new_node(ND_NULL, tok);
  
  expect("=");
  Node *lhs = new_var_node(var, tok);
  Node *rhs = expr();
  expect(";");
  Node *node = new_binary(ND_ASSIGN, lhs, rhs, tok);
  return new_unary(ND_EXPR_STMT, node, tok);
}


static Node *read_expr_stmt(void) {
  Token *tok = token;
  return new_unary(ND_EXPR_STMT, expr(), tok);
}

// 目的：次のトークンが該当する型を持っているかどうか調べる
// is_typename : void -> bool
static bool is_typename(void) {
  return peek("char") || peek("int");
}

// stmt : void -> Node
// stmt = stmt2
static Node *stmt(void) {
  Node *node = stmt2();
  add_type(node);
  return node;
}

// stmt2 : void -> Node
// stmt2 = "return" expr ";"
//       | "if" "(" expr ")" stmt ("else" stmt)?
//       | "while" "(" expr ")" stmt
//       | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//       | "{" stmt* "}"
//       | declaration
//       | expr ";"
static Node *stmt2(void) {
  Token *tok;
  if (tok = consume("return")) {
    Node *node = new_unary(ND_RETURN, expr(), tok);
    expect(";");
    return node;
  }
  
  // if 文のパース
  if (tok = consume("if")) {
    Node *node = new_node(ND_IF, tok);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    if (consume("else"))
      node->els = stmt();
    return node;
  }

  // while 文のパース
  if (tok = consume("while")) {
    Node *node = new_node(ND_WHILE, tok);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    return node;
  }

  // for 文のパース
  if (tok = consume("for")) {
    Node *node = new_node(ND_FOR, tok);
    expect("(");
    if (!consume(";")) {
      node->init = read_expr_stmt();
      expect(";");
    }
    if (!consume(";")) {
      node->cond = expr();
      expect(";");
    }
    if (!consume(")")) {
      node->inc = read_expr_stmt();
      expect(")");
    }
    node->then = stmt();
    return node;
  }

  // ブロックのパース
  if (tok = consume("{")) {
    Node head = {};
    Node *cur = &head;

    VarList *sc = scope;
    while (!consume("}")) {
      cur->next = stmt();
      cur = cur->next;
    }
    scope = sc;

    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    return node;
  }

  if (is_typename())
    return declaration();

  Node *node = read_expr_stmt();
  expect(";");
  return node;
}

// expr : Node
// expr = assign
static Node *expr(void) {
  return assign();
}

// assign : void -> Node
// assign = equality ("=" assign)?
static Node *assign(void) {
  Node *node = equality();
  Token *tok;
  if (tok = consume("="))
    node = new_binary(ND_ASSIGN, node, assign(), tok);
  return node;
}

// 目的：== と != をパースする
// equality : Node
// equality = relational ("==" relational | "!=" relational)*
static Node *equality(void) {
  Node *node = relational();
  Token *tok;

  for (;;) {
    if (tok = consume("=="))
      node = new_binary(ND_EQ, node, relational(), tok);
    else if (tok = consume("!="))
      node = new_binary(ND_NE, node, relational(), tok);
    else
      return node;
  }
}

// 目的：<, <=, >, >= をパースする
// relational : Node
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(void) {
  Node *node = add();
  Token *tok;

  for (;;) {
    if (tok = consume("<"))
      node = new_binary(ND_LT, node, add(), tok);
    else if (tok = consume("<="))
      node = new_binary(ND_LE, node, add(), tok);
    else if (tok = consume(">"))
      node = new_binary(ND_LT, add(), node, tok);
    else if (tok = consume(">="))
      node = new_binary(ND_LE, add(), node, tok);
    else
      return node;
  }
}

// 目的：左辺値と右辺値を受け取り、型情報を加えて、式に応じた Node を返す
// new_add : Node -> Node -> Token -> Node
static Node *new_add(Node *lhs, Node *rhs, Token *tok) {
  add_type(lhs);
  add_type(rhs);

  if (is_integer(lhs->ty) && is_integer(rhs->ty))
    return new_binary(ND_ADD, lhs, rhs, tok);
  if (lhs->ty->base && is_integer(rhs->ty))
    return new_binary(ND_PTR_ADD, lhs, rhs, tok);
  if (is_integer(lhs->ty) && rhs->ty->base)
    return new_binary(ND_PTR_ADD, rhs, lhs, tok);
  error_tok(tok, "演算子が間違っています");
}

// 目的：左辺値と右辺値を受け取り、型情報を加えて、式に応じた Node を返す
// new_sub : Node -> Node -> Token -> Node
static Node *new_sub(Node *lhs, Node *rhs, Token *tok) {
  add_type(lhs);
  add_type(rhs);

  if (is_integer(lhs->ty) && is_integer(rhs->ty))
    return new_binary(ND_SUB, lhs, rhs, tok);
  if (lhs->ty->base && is_integer(rhs->ty))
    return new_binary(ND_PTR_SUB, lhs, rhs, tok);
  if (lhs->ty->base && rhs->ty->base)
    return new_binary(ND_PTR_DIFF, lhs, rhs, tok);
  error_tok(tok, "演算子が間違っています");
}

// 目的：＋とーの演算子をパースする
// add : Node
// add = mul ("+" mul | "-" mul)*
static Node *add(void) {
  Node *node = mul();
  Token *tok;

  for (;;) {
    if (tok = consume("+"))
      node = new_add(node, mul(), tok);
    else if (tok = consume("-"))
      node = new_sub(node, mul(), tok);
    else
      return node;
  }
}

// 目的：*と/の演算子をパースする
// mul : Node
// mul = unary ("*" unary | "/" unary)*
static Node *mul(void) {
  Node *node = unary();
  Token *tok;

  for (;;) {
    if (tok = consume("*"))
      node = new_binary(ND_MUL, node, unary(), tok);
    else if (tok = consume("/"))
      node = new_binary(ND_DIV, node, unary(), tok);
    else
      return node;
  }
}

// 目的：正負の記号をパースする
// unary : Node
// unary = ("+" | "-" | "*" | "&")? unary
//       | postfix
static Node *unary(void) {
  Token *tok;
  if (consume("+"))
    return unary();
  if (tok = consume("-"))
    return new_binary(ND_SUB, new_num(0, tok), unary(), tok);
  if (tok = consume("&"))
    return new_unary(ND_ADDR, unary(), tok);
  if (tok = consume ("*"))
    return new_unary(ND_DEREF, unary(), tok);
  return postfix();
}

// postfix = primary ("[" expr "]")*
static Node *postfix(void) {
  Node *node = primary();
  Token *tok;

  while (tok = consume("[")) {
    // x[y] は *(x+y) の短縮版
    Node *exp = new_add(node, expr(), tok);
    expect("]");
    node = new_unary(ND_DEREF, exp, tok);
  }
  return node;
}

// stmt-expr = "(" "{" stmt stmt* "}" ")"
// Statement expression is a GNU C extension
static Node *stmt_expr(Token *tok) {
  VarList *sc = scope;

  Node *node = new_node(ND_STMT_EXPR, tok);
  node->body = stmt();
  Node *cur = node->body;

  while (!consume("}")) {
    cur->next = stmt();
    cur = cur->next;
  }
  expect(")");

  scope = sc;

  if (cur->kind != ND_EXPR_STMT)
    error_tok(cur->tok, "stmt expr returning void is not supported");
  memcpy(cur, cur->lhs, sizeof(Node));
  return node;
}


// 目的：関数の引数をパースする
// func_args : void -> Node | NULL
// func_args = "(" (assign ("," assign)*)? ")"
static Node *func_args(void) {
  if (consume(")"))
    return NULL;
  
  Node *head = assign();
  Node *cur = head;
  while (consume(",")) {
    cur->next = assign();
    cur = cur->next;
  }
  expect(")");
  return head;
}

// 目的：(expr)、sizeof、ident、func-args, numをパースする
// primary : void -> Node
// primary = "(" "{" stmt-expr-tail
//         | "(" expr ")"
//         | "sizeof" unary
//         | ident func-args?
//         | str
//         | num
static Node *primary(void) {
  Token *tok;

  if (tok = consume("(")) {
    if (consume("{"))
      return stmt_expr(tok);
      
    Node *node = expr();
    expect(")");
    return node;
  }

  if (tok = consume("sizeof")) {
    Node *node = unary();
    add_type(node);
    return new_num(node->ty->size, tok);
  }

  if (tok = consume_ident()) {
    // Function Call
    if (consume("(")) {
      Node *node = new_node(ND_FUNCALL, tok);
      node->funcname = strndup(tok->str, tok->len);
      node->args = func_args();
      return node;
    }

    Var *var = find_var(tok);
    if (!var)
      error_tok(tok, "定義されていない変数です");
    return new_var_node(var, tok);
  }
  
  tok = token;
  if (tok->kind == TK_STR) {
    token = token->next;

    Type *ty = array_of(char_type, tok->cont_len);
    Var *var = new_gvar(new_label(), ty);
    var->contents = tok->contents;
    var->cont_len = tok->cont_len;
    return new_var_node(var, tok);
  }

  if (tok->kind != TK_NUM)
    error_tok(tok, "期待していた式です");
  return new_num(expect_number(), tok);
}