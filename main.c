#include "9cc.h"

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
    Function *prog = program();

    // ローカル変数のオフセットを設定
    for (Function *fn = prog; fn; fn = fn->next) {
        int offset = 0;
        for (Var *var = prog->locals; var; var = var->next) {
            offset += 8;
            var->offset = offset;
        }
        fn->stack_size = offset;
    }

    // コード生成
    codegen(prog);

    return 0;
}