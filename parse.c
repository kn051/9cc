#include "9cc.h"

// ローカル変数の連結リスト。パース中に作られたローカル変数はここに格納される
Var *locals;

// 目的：トークン列を受け取り、名前でローカル変数を検索する。見つからなかったらNULLを返す。
// *find_var : *Token -> Var
static Var *find_var(Token *tok) {
  for (Var *var = locals; var; var = var->next)
    if (strlen(var->name) == tok->len && !strncmp(tok->str, var->name, tok->len))
      return var;
    return NULL;
}


// 目的：Nodeを新しく作る
// new_node : NodeKind -> Node
static Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

// 目的：右辺と左辺を受け取る2項演算子のNodeを作る
// new_binary : NodeKind -> Node -> Node -> Node
static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_unary(NodeKind kind, Node *expr) {
  Node *node = new_node(kind);
  node->lhs = expr;
  return node;
}

// 目的：数値のノードを新しく作る
// new_num : int -> Node
static Node *new_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

// 目的：Var 型のポインタを受け取り、変数のノードを新しく作る
// new_var_node : *Var -> Node
static Node *new_var_node(Var *var) {
  Node *node = new_node(ND_VAR);
  node->var = var;
  return node;
}

// 目的：char 型の *name を受け取り、*name の名前を持つ変数を作る
// *new_lvar : *char -> Var
static Var *new_lvar(char *name) {
  Var *var = calloc(1, sizeof(Var));
  var->next = locals;
  var->name = name;
  locals = var;
  return var;
}


static Node *stmt(void);
static Node *expr(void);
static Node *assign(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);


Function *program(void) {
  locals = NULL;

  Node head = {};
  Node *cur = &head;

  while (!at_eof()) {
    cur->next = stmt();
    cur = cur->next;
  }

  Function *prog = calloc(1, sizeof(Function));
  prog->node = head.next;
  prog->locals = locals;
  return prog;
  
}

static Node *read_expr_stmt(void) {
  return new_unary(ND_EXPR_STMT, expr());
}

// stmt : Node
// stmt ="return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "{" stmt* "}"
//      | expr ";"
static Node *stmt(void) {
  if (consume("return")) {
    Node *node = new_unary(ND_RETURN, expr());
    expect(";");
    return node;
  }
  
  // if 文のパース
  if (consume("if")) {
    Node *node = new_node(ND_IF);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    if (consume("else"))
      node->els = stmt();
    return node;
  }

  // while 文のパース
  if (consume("while")) {
    Node *node = new_node(ND_WHILE);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    return node;
  }

  // for 文のパース
  if (consume("for")) {
    Node *node = new_node(ND_FOR);
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
  if (consume("{")) {
    Node head = {};
    Node *cur = &head;

    while (!consume("}")) {
      cur->next = stmt();
      cur = cur->next;
    }

    Node *node = new_node(ND_BLOCK);
    node->body = head.next;
    return node;
  }

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
  if (consume("="))
    node = new_binary(ND_ASSIGN, node, assign());
  return node;
}

// 目的：== と != をパースする
// equality : Node
// equality = relational ("==" relational | "!=" relational)*
static Node *equality(void) {
  Node *node = relational();

  for (;;) {
    if (consume("=="))
      node = new_binary(ND_EQ, node, relational());
    else if (consume("!="))
      node = new_binary(ND_NE, node, relational());
    else
      return node;
  }
}

// 目的：<, <=, >, >= をパースする
// relational : Node
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(void) {
  Node *node = add();

  for (;;) {
    if (consume("<"))
      node = new_binary(ND_LT, node, add());
    else if (consume("<="))
      node = new_binary(ND_LE, node, add());
    else if (consume(">"))
      node = new_binary(ND_LT, add(), node);
    else if (consume(">="))
      node = new_binary(ND_LE, add(), node);
    else
      return node;
  }
}

// 目的：＋とーの演算子をパースする
// add : Node
// add = mul ("+" mul | "-" mul)*
static Node *add(void) {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_binary(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_binary(ND_SUB, node, mul());
    else
      return node;
  }
}

// 目的：*と/の演算子をパースする
// mul : Node
// mul = unary ("*" unary | "/" unary)*
static Node *mul(void) {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_binary(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_binary(ND_DIV, node, unary());
    else
      return node;
  }
}

// 目的：正負の記号をパースする
// unary : Node
// unary = ("+" | "-")? unary | primary
static Node *unary(void) {
  if (consume("+"))
    return unary();
  if (consume("-"))
    return new_binary(ND_SUB, new_num(0), unary());
  return primary();
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

// 目的：(expr)、ident、func-args, numをパースする
// primary : Node
// primary = "(" expr ")" | ident func-args? | num
static Node *primary(void) {
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok = consume_ident();
  if (tok) {
    // Function Call
    if (consume("(")) {
      Node *node = new_node(ND_FUNCALL);
      node->funcname = strndup(tok->str, tok->len);
      node->args = func_args();
      return node;
    }

    Var *var = find_var(tok);
    if (!var)
      var = new_lvar(strndup(tok->str, tok->len));
    return new_var_node(var);
  }
  
  return new_num(expect_number());
}