#include "9cc.h"


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

Node *new_unary(NodeKind kind, Node *expr) {
  Node *node = new_node(kind);
  node->lhs = expr;
  return node;
}

// 目的：数値のノードを新しく作る
// new_num : int -> Node
Node *new_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

// 目的：ローカル変数のノードを新しく作る
// new_lvar : char -> Node
Node *new_lvar(char name) {
  Node *node = new_node(ND_LVAR);
  node->name = name;
  return node;
}

Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// 目的：
Node *program() {
  Node head;
  head.next = NULL;
  Node *cur = &head;

  while (!at_eof()) {
    cur->next = stmt();
    cur = cur->next;
  }
  return head.next;
}

// stmt : Node
// stmt ="return" expr ";"
//      | expr ";"
Node *stmt() {
  if (consume("return")) {
    Node *node = new_unary(ND_RETURN, expr());
    expect(";");
    return node;
  }

  Node *node = new_unary(ND_EXPR_STMT, expr());
  expect(";");
  return node;
}

// expr : Node
// expr = assign
Node *expr() {
  return assign();
}

// assign : Node
// assign = equality ("=" assign)?
Node *assign() {
    Node *node = equality();
    if (consume("="))
      node = new_binary(ND_ASSIGN, node, assign());
    return node;
}

// 目的：== と != をパースする
// equality : Node
// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
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
Node *relational() {
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
Node *add() {
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
Node *mul() {
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
Node *unary() {
  if (consume("+"))
    return unary();
  if (consume("-"))
    return new_binary(ND_SUB, new_num(0), unary());
  return primary();
}


// 目的：(expr)とnumをパースする
// primary : Node
// primary = "(" expr ")" | ident | num
Node *primary() {
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok = consume_ident();
  if (tok)
    return new_lvar(*tok->str);
  
  return new_num(expect_number());
}