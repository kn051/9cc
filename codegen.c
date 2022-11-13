#include "9cc.h"

// 目的：Nodeのポインタを受け取り、スタックにそのアドレスを push する
// gen_addr : *Node -> アセンブリコードの吐き出し
static void gen_addr(Node *node) {
  if (node->kind == ND_VAR) {
    // 変数用のアドレスを確保する
    // lea dest, [src] : [src]内のアドレス値がそのまま dest に読み出される。 
    printf("  lea rax, [rbp-%d]\n", node->var->offset);
    printf("  push rax\n");
    return;
  }

  error("ローカル変数ではありません");
}

// 目的：メモリから値をロードしてスタックに push する
static void load(void) {
  printf("  pop rax\n");
  printf("  mov rax, [rax]\n");
  printf("  push rax\n");
}

// 目的：メモリに値を格納する
static void store(void) {
  printf("  pop rdi\n");
  printf("  pop rax\n");
  printf("  mov [rax], rdi\n");
  printf("  push rdi\n");
}


// 目的：Node のポインタを受け取り、スタックマシンの要領でアセンブリコードを吐き出す
// gen : Node -> アセンブリコードの吐き出し
static void gen (Node *node) {
  switch (node->kind) {
  case ND_NUM:
    printf("  push %ld\n", node->val);
    return;
  case ND_EXPR_STMT:
    gen(node->lhs);
    printf("  add rsp, 8\n");
    return;
  case ND_VAR:
    gen_addr(node); // 変数のアドレスをスタックに push
    load(); // 変数をスタックに push
    return;
  case ND_ASSIGN:
    gen_addr(node->lhs);  // 変数のアドレスをスタックにpush
    gen(node->rhs); // 式か何か
    store();  // 変数のアドレスに式の評価結果をストアする＝代入
    return;
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  jmp .L.return\n");
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

void codegen(Function *prog) {
  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // プロローグ
  printf("  push rbp\n"); // 元のベースポインタをスタックに push し保存
  printf("  mov rbp, rsp\n"); // 保存されたベースポインタを指す rsp の位置にrbp を移動
  printf("  sub rsp, %d\n", prog->stack_size); // 変数分のメモリを確保

  for (Node *node = prog->node; node; node = node->next)
    gen(node);

  // エピローグ
  printf(".L.return:\n");
  printf("  mov rsp, rbp\n"); // rsp がリターンアドレスを指すようにする
  printf("  pop rbp\n"); // rbp に元のベースポインタを書き戻す（＝元のベースポイントを指す）
  printf("  ret\n"); // 呼び出し元の関数のリターンアドレスを pop し、そのアドレスにジャンプする
}