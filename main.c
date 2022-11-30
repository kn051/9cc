#include "9cc.h"

// 目的：指定されたファイルの内容を返す
// read_file : char * -> char
static char *read_file(char *path) {
  // ファイルを開く
  FILE *fp = fopen(path, "r");
  if (!fp)
    error("cannot open %s: %s", path, strerror(errno));
  
  // ファイルを読み込む
  int filemax = 10 * 1024 * 1024;
  char *buf = malloc(filemax);
  // fread (格納先のバッファ、読み込むデータ１つのバイト数、読み込むデータの個数、ファイルポインタ)
  // 戻り値は、読み込んだデータの大きさ（個数）
  int size = fread(buf, 1, filemax - 2, fp);
  if (!feof(fp)) // feof : ファイルポインタの位置が EOF か判定する
    error("%s: file too large");
  
  // ファイルが必ず "\n\0" で終わっているようにする
  if (size == 0 || buf[size - 1] != '\n')
    buf[size++] = '\n';
  buf[size] = '\0';
  return buf;
}

int align_to(int n, int align) {
  return (n + align - 1) & ~(align - 1);
}

int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: 引数の個数が正しくありません", argv[0]);

  // Tokenize and parse
  filename = argv[1];
  user_input = read_file(argv[1]);
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
    fn->stack_size = align_to(offset, 8);
  }
  
  // ASTをトラバースして、アセンブリのコードを吐き出す
  codegen(prog);
  return 0;
}
