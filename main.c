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
    Node *node = program();

    // コード生成
    codegen(node);

    return 0;
}