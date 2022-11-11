#include "9cc.h"


// 目的：Node を受け取り、スタックマシンの要領でアセンブリコードを吐き出す
// gen : Node -> アセンブリコードの吐き出し
static void gen (Node *node) {
  switch (node->kind) {
  case ND_NUM:
    printf("  push %ld\n", node->val);
    return;
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  ret\n");
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
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;
  }

  printf("  push rax\n");
}

void codegen(Node *node) {
  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  for (Node *n = node; n; n = n->next) {
    gen (n);
    printf("  pop rax\n");
  }

  printf("  ret\n");
}