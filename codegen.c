#include "9cc.h"

// Nodeのアドレス分、スタックに領域を確保する
void gen_addr(Node *node) {
    if (node->kind == ND_LVAR) {
        // 'a'から数えて何番目のアルファベットか計算。1変数あたり8bit確保。
        int offset = (node->name - 'a' + 1) * 8;
        printf("    lea rax, [rbp-%d]\n", offset);
        printf("    push rax\n");
        return;
    }

    error("not an lvalue");
}

void load() {
    printf("    pop rax\n");
    printf("    mov rax, [rax]\n");
    printf("    push rax\n");
}

void store() {
    printf("    pop rdi\n");
    printf("    pop rax\n");
    printf("    mov [rax], rdi\n");
    printf("    push rdi\n");
}

// 抽象構文木をスタックマシンに乗せてエミュレートする
void gen(Node *node) {
    switch(node->kind) {
        case ND_NUM:
            printf("    push %d\n", node->val);
            return;
        case ND_EXPR_STMT:
            gen(node->lhs);
            printf("    add rsp, 8\n");
            return;
        case ND_LVAR:
            gen_addr(node);
            load();
            return;
        case ND_ASSIGN:
            gen_addr(node->lhs);
            gen(node->rhs);
            store();
            return;
        case ND_RETURN:
            gen(node->lhs);
            printf("    pop rax\n");
            printf("    jmp .Lreturn\n");
            return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind) {
        case ND_ADD:
            printf("    add rax, rdi\n");
            break;
        case ND_SUB:
            printf("    sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("    imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("    cqo\n");
            printf("    idiv rdi\n");
            break;
        case ND_EQ:
            printf("    cmp rax, rdi\n");
            printf("    sete al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_NE:
            printf("    cmp rax, rdi\n");
            printf("    setne al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_LT:
            printf("    cmp rax, rdi\n");
            printf("    setl al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_LE:
            printf("    cmp rax, rdi\n");
            printf("    setle al\n");
            printf("    movzb rax, al\n");
            break;
    }

    printf("    push rax\n");
}

void codegen(Node *node) {
    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // プロローグ
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, 208\n");

    // 抽象構文木を下りながらコード生成
    for(Node *n = node; n; n = n->next) {
        gen(n);
    }

    // エピローグ
    printf(".Lreturn:\n");
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
 
}