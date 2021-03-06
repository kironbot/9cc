#include "9cc.h"

// Return the contents of a given file
char *read_file(char *path) {
    // open and read the file
    FILE *fp = fopen(path, "r");

    if (!fp)
        error2("cannot open %s: %s", path, strerror(errno));

    int filemax = 10 * 1024 * 1024;
    char *buf = malloc(filemax);
    int size = fread(buf, 1, filemax - 2, fp);
    if (!feof(fp))
        error("file too large");

    // make sure that the string ends with "\n\0"
    if (size == 0 || buf[size-1] != '\n')
        buf[size++] = '\n';
    buf[size] = '\0';
    return buf;
}

int main(int argc, char **argv) {
    // Tokenize and parse
    // 複数ファイルのコンパイルに対応
    for (int i = 1; i < argc; i++) {
        filename = argv[i];
        user_input = read_file(filename);
        Token *t = tokenize();
        if (!token) {
            token = t;
        } else {
            Token *tt = token;
            for (;;) {
                if (tt->next->kind == TK_EOF) {
                    tt->next = t;
                    break;
                }
                tt = tt->next;
            }
        }
    }
    Program *prog = program();
    add_type(prog);

    // ローカル変数のオフセットを設定
    for (Function *fn = prog->fns; fn; fn = fn->next) {
        int offset = 0;
        for (VarList *vl = fn->locals; vl; vl = vl->next) {
            Var *var = vl->var;
            offset = align_to(offset, var->ty->align);
            offset += size_of(var->ty, var->tok);
            var->offset = offset;
        }
        fn->stack_size = align_to(offset, 8);
    }

    // コード生成
    codegen(prog);

    return 0;
}