#include "9cc.h"

char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
int labelseq = 0;
char *funcname;

// Nodeのアドレス分、スタックに領域を確保する
void gen_addr(Node *node) {
    if (node->kind == ND_VAR) {
        printf("    lea rax, [rbp-%d]\n", node->var->offset);
        printf("    push rax\n");
        return;
    }

    error_tok(node->tok, "not an lvalue");
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
        case ND_VAR:
            gen_addr(node);
            load();
            return;
        case ND_ASSIGN:
            gen_addr(node->lhs);
            gen(node->rhs);
            store();
            return;
        case ND_IF: {
            int seq = labelseq++;
            if (node->els) {
                // if (A) B else C
                gen(node->cond);
                printf("    pop rax\n");
                printf("    cmp rax, 0\n");
                printf("    je  .Lelse%d\n", seq);
                gen(node->then);
                printf("    jmp .Lend%d\n", seq);

                printf(".Lelse%d:\n", seq);
                gen(node->els);

                printf(".Lend%d:\n", seq);
            } else {
                // if (A) B
                gen(node->cond);
                printf("    pop rax\n");
                printf("    cmp rax, 0\n");
                printf("    je  .Lend%d\n", seq);
                gen(node->then);
                printf(".Lend%d:\n", seq);
            }
            return;
        }
        case ND_WHILE: {
            int seq = labelseq++;
            printf(".Lbegin%d:\n", seq);
            gen(node->cond);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je  .Lend%d\n", seq);
            gen(node->then);
            printf("    jmp .Lbegin%d\n", seq);
            printf(".Lend%d:\n", seq);
            return;
        }
        case ND_FOR: {
            int seq = labelseq++;

            // 初期条件
            if (node->init) gen(node->init);
            printf(".Lbegin%d:\n", seq);
            if (node->cond) {
                gen(node->cond);
                printf("    pop rax\n");
                printf("    cmp rax, 0\n");
                printf("    je  .Lend%d\n", seq);
            }
            // forループの中身
            gen(node->then);
            // インクリメント条件
            if (node->inc) gen(node->inc);
            printf("    jmp .Lbegin%d\n", seq);
            printf(".Lend%d:\n", seq);
            return;
        }
        case ND_BLOCK:
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
                printf("    pop %s\n", argreg[i]);


            // ABIにより関数呼び出し時はRSPが16バイトの倍数にある必要がある
            int seq = labelseq++;
            printf("    mov rax, rsp\n");
            printf("    and rax, 15\n");
            printf("    jnz .Lcall%d\n", seq);
            printf("    mov rax, 0\n");
            printf("    call %s\n", node->funcname);
            printf("    jmp .Lend%d\n", seq);
            printf(".Lcall%d:\n", seq);
            printf("    sub rsp, 8\n");
            printf("    mov rax, 0\n");
            printf("    call %s\n", node->funcname);
            printf("    add rsp, 8\n");
            printf(".Lend%d:\n", seq);
            printf("    push rax\n");
            return;
        }
        case ND_RETURN:
            gen(node->lhs);
            printf("    pop rax\n");
            printf("    jmp .Lreturn.%s\n", funcname);
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

void codegen(Function *prog) {
    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");

    for (Function *fn = prog; fn; fn = fn->next) {
        printf(".global %s\n", fn->name);
        printf("%s:\n", fn->name);
        funcname = fn->name;

        // プロローグ
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, %d\n", fn->stack_size);

        // 変数をスタックにpush
        int i = 0;
        for (VarList *vl = fn->params; vl; vl = vl->next) {
            Var *var = vl->var;
            printf("    mov [rbp-%d], %s\n", var->offset, argreg[i++]);
        }

        // 抽象構文木を下りながらコード生成
        for(Node *node = fn->node; node; node = node->next)
            gen(node);

        // エピローグ
        printf(".Lreturn.%s:\n", funcname);
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
    }
}