#include "9cc.h"

int align_to(int n, int align) {
    return (n + align - 1) & ~(align - 1);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        error("引数の個数が正しくありません");
        return 1;
    }

    // 入力プログラムを保存
    user_input = argv[1];
    // トークナイズする
    token = tokenize(argv[1]);
    // トークンを抽象構文木にパースする
    Program *prog = program();
    add_type(prog);

    // ローカル変数のオフセットを設定
    for (Function *fn = prog->fns; fn; fn = fn->next) {
        int offset = 0;
        for (VarList *vl = fn->locals; vl; vl = vl->next) {
            Var *var = vl->var;
            offset += size_of(var->ty);
            var->offset = offset;
        }
        fn->stack_size = align_to(offset, 8);
    }

    // コード生成
    codegen(prog);

    return 0;
}