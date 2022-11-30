#include "9cc.h"

static char *argreg1[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
static char *argreg8[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static int labelseq = 1;
static char *funcname;

static void gen(Node *node);

// 目的：Nodeのポインタを受け取り、スタックにそのアドレスを push する
// gen_addr : *Node -> アセンブリコードの吐き出し
static void gen_addr(Node *node) {
  switch (node->kind) {
  case ND_VAR: {
    Var *var = node->var;
    // 変数がローカル変数の場合、変数用のアドレスを確保する
    // lea dest, [src] : [src]内のアドレス値がそのまま dest に読み出される。
    if (var->is_local) { 
    printf("  lea rax, [rbp-%d]\n", node->var->offset);
    printf("  push rax\n");
    } else {
      // 変数がグローバル変数の場合。
      printf("  push offset %s\n", var->name);
    }
    return;
  }
  case ND_DEREF:
    gen(node->lhs);
    return;
  }

  error_tok(node->tok, "ローカル変数ではありません");
}

// 目的：Node を受け取り、もし配列ならエラー、配列でないならスタックにそのアドレスを push する
// gen_lval : Node -> void
static void gen_lval(Node *node) {
  if (node->ty->kind == TY_ARRAY)
    error_tok(node->tok, "ローカル変数ではありません");
  gen_addr(node);
}

// 目的：メモリから値をロードしてスタックに push する
static void load(Type *ty) {
  printf("  pop rax\n");
  if (ty->size == 1)
    printf("  movsx rax, byte ptr [rax]\n");
  else
    printf("  mov rax, [rax]\n");
  printf("  push rax\n");
}

// 目的：メモリに値を格納する
static void store(Type *ty) {
  printf("  pop rdi\n");
  printf("  pop rax\n");

  if (ty->size == 1)
    printf("  mov [rax], dil\n");
  else
    printf("  mov [rax], rdi\n");

  printf("  push rdi\n");
}


// 目的：Node のポインタを受け取り、スタックマシンの要領でアセンブリコードを吐き出す
// gen : Node -> アセンブリコードの吐き出し
static void gen (Node *node) {
  switch (node->kind) {
  case ND_NULL:
    return;
  case ND_NUM:
    printf("  push %ld\n", node->val);
    return;
  case ND_EXPR_STMT:
    gen(node->lhs);
    printf("  add rsp, 8\n");
    return;
  case ND_VAR:
    gen_addr(node); // 変数のアドレスをスタックに push
    if (node->ty->kind != TY_ARRAY)
      load(node->ty); // 変数をスタックに push
    return;
  case ND_ASSIGN:
    gen_lval(node->lhs);  // 変数のアドレスをスタックにpush
    gen(node->rhs); // 式か何か
    store(node->ty);  // 変数のアドレスに式の評価結果をストアする＝代入
    return;
  case ND_ADDR:
    gen_addr(node->lhs);
    return;
  case ND_DEREF:
    gen(node->lhs);
    if (node->ty->kind != TY_ARRAY)
      load(node->ty);
    return;
  case ND_IF: {
    int seq = labelseq++;
    // もし else があれば if ... else、ないときは else のない if としてコンパイルする
    if (node->els) {
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .L.else.%d\n", seq);
      gen(node->then);
      printf("  jmp .L.end.%d\n", seq);
      printf(".L.else.%d:\n", seq);
      gen(node->els);
      printf(".L.end.%d:\n", seq);
    } else {
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .L.end.%d\n", seq);
      gen(node->then);
      printf(".L.end.%d:\n", seq);
    }
    return;
  }
  case ND_WHILE: {
    int seq = labelseq++;
    printf(".L.begin.%d:\n", seq);
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .L.end.%d\n", seq);
    gen(node->then);
    printf("  jmp .L.begin.%d\n", seq);
    printf(".L.end.%d:\n", seq);
    return;
  }
  case ND_FOR: {
    int seq = labelseq++;
    if (node->init)
      gen(node->init);
    printf(".L.begin.%d:\n", seq);
    if (node->cond) {
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .L.end.%d\n", seq);
    }
    gen(node->then);
    if (node->inc)
      gen(node->inc);
    printf("  jmp .L.begin.%d\n", seq);
    printf(".L.end.%d:\n", seq);
    return;
  }
  case ND_BLOCK:
  case ND_STMT_EXPR:
    for (Node *n = node->body; n; n = n->next)
      gen(n);
    return;
  case ND_FUNCALL: {
    int nargs = 0;
    for (Node *arg = node->args; arg; arg = arg->next) {
      gen(arg);
      nargs++;
    }

    for (int i = nargs - 1; i >= 0; i--)
      printf("  pop %s\n", argreg8[i]);
    
    // 関数を呼ぶ前に RSP を16バイトの倍数にする。ABI規約。
    // RAXは複数個の引数をとる関数用に 0 にセットする。
    int seq = labelseq++;
    printf("  mov rax, rsp\n");
    printf("  and rax, 15\n");
    printf("  jnz .L.call.%d\n", seq);
    printf("  mov rax, 0\n");
    printf("  call %s\n", node->funcname);
    printf("  jmp .L.end.%d\n", seq);
    printf(".L.call.%d:\n", seq);
    printf("  sub rsp, 8\n");
    printf("  mov rax, 0\n");
    printf("  call %s\n", node->funcname);
    printf("  add rsp, 8\n");
    printf(".L.end.%d:\n", seq);
    printf("  push rax\n");
    return;
  }
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  jmp .L.return.%s\n", funcname);
    return;
  }


  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:  // num + num
    printf("  add rax, rdi\n");
    break;
  case ND_PTR_ADD:  // ptr + num || num + ptr
    printf("  imul rdi, %d\n", node->ty->base->size);  // imul : 積
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:  // num - num
    printf("  sub rax, rdi\n");
    break;
  case ND_PTR_SUB:  // ptr - num
    printf("  imul rdi, %d\n", node->ty->base->size);
    printf("  sub rax, rdi\n");
    break;
  case ND_PTR_DIFF:
    printf("  sub rax, rdi\n");
    printf("  cqo\n");
    printf("  mov rdi, %d\n", node->lhs->ty->base->size);
    printf("  idiv rdi\n");
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

// 目的：グローバル変数を吐き出す
// emit_data : Program -> void
static void emit_data(Program *prog) {
  printf(".data\n");

  for (VarList *vl = prog->globals; vl; vl = vl->next) {
    Var *var = vl->var;
    printf("%s:\n", var->name);

    if (!var->contents) {
      printf("  .zero %d\n", var->ty->size);
      continue;
    }

    // 文字列の1文字ずつのバイトを確保する
    for (int i = 0; i < var->cont_len; i++)
      printf("  .byte %d\n", var->contents[i]);
  }
}

// 目的：変数とレジスタのインデックスを受け取り、変数のサイズに応じて引数を各レジスタに入れていく
// load_arg : Var -> int -> void
static void load_arg(Var *var, int idx) {
  int sz = var->ty->size;
  if (sz == 1) {
    printf("  mov [rbp-%d], %s\n", var->offset, argreg1[idx]);
  } else {
    assert(sz == 8);
    printf("  mov [rbp-%d], %s\n", var->offset, argreg8[idx]);
  }
}

// 目的：関数ごとのアセンブリコードを吐き出す
// emit_text : Program -> void
static void emit_text(Program *prog) {
  printf(".text\n");

  for (Function *fn = prog->fns; fn; fn = fn->next) {
    printf(".global %s\n", fn->name);
    printf("%s:\n", fn->name);
    funcname = fn->name;

    // プロローグ
    printf("  push rbp\n"); // 元のベースポインタをスタックに push し保存
    printf("  mov rbp, rsp\n"); // 保存されたベースポインタを指す rsp の位置にrbp を移動
    printf("  sub rsp, %d\n", fn->stack_size); // 変数分のメモリを確保

    // スタックに引数を push する
    int i = 0;
    for (VarList *vl = fn->params; vl; vl = vl->next)
      load_arg(vl->var, i++);

    // コードの吐き出し
    for (Node *node = fn->node; node; node = node->next)
      gen(node);

    // エピローグ
    printf(".L.return.%s:\n", funcname);
    printf("  mov rsp, rbp\n"); // rsp がリターンアドレスを指すようにする
    printf("  pop rbp\n"); // rbp に元のベースポインタを書き戻す（＝元のベースポイントを指す）
    printf("  ret\n"); // 呼び出し元の関数のリターンアドレスを pop し、そのアドレスにジャンプする
  }
}

void codegen(Program *prog) {
  printf(".intel_syntax noprefix\n");
  emit_data(prog);
  emit_text(prog);
}