#include "9cc.h"


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


static Node *stmt(void);
static Node *expr(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);

// 目的：
Node *program() {
  Node head = {};
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
static Node *stmt(void) {
  if (consume("return")) {
    Node *node = new_unary(ND_RETURN, expr());
    expect(";");
    return node;
  }

  Node *node = expr();
  expect(";");
  return node;
}

// expr : Node
// expr = equality
static Node *expr(void) {
  return equality();
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


// 目的：(expr)とnumをパースする
// primary : Node
// primary = "(" expr ")" | num
static Node *primary(void) {
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }
  return new_num(expect_number());
}