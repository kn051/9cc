#include "9cc.h"


int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: 引数の個数が正しくありません", argv[0]);

  
  user_input = argv[1];
  token = tokenize();     // トークン列の連結リストを返す。(head.next)
  Program *prog = program();

  // 関数ごとにオフセットをローカル変数に割り当てる
  for (Function *fn = prog->fns; fn; fn = fn->next) {
    int offset = 0;
    for (VarList *vl = fn->locals; vl; vl = vl->next) {
      Var *var = vl->var;
      offset += var->ty->size;
      var->offset = offset;
    }
    fn->stack_size = offset;
  }
  
  // ASTをトラバースして、アセンブリのコードを吐き出す
  codegen(prog);
  return 0;
}
