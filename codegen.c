#include "9cc.h"

char *argreg1[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
char *argreg2[] = {"di", "si", "dx", "cx", "r8w", "r9w"};
char *argreg4[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
char *argreg8[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
int labelseq = 0;
char *funcname;

void gen(Node *node);

// Nodeのアドレス分、スタックに領域を確保する
void gen_addr(Node *node) {
    switch (node->kind) {
    case ND_VAR: {
        Var *var = node->var;
        if (var->is_local) {
            printf("    lea rax, [rbp-%d]\n", var->offset);
            printf("    push rax\n");
        } else {
            printf("    push offset %s\n", var->name);
        }
        return;
    }
    case ND_DEREF:
        gen(node->lhs);
        return;
    case ND_MEMBER:
        gen_addr(node->lhs);
        printf("    pop rax\n");
        printf("    add rax, %d\n", node->member->offset);
        printf("    push rax\n");
        return;
    }

    error_tok(node->tok, "not an lvalue");
}

void gen_lval(Node *node) {
    if (node->ty->kind == TY_ARRAY) 
        error_tok(node->tok, "not an lvalue");
    gen_addr(node);
}

void load(Type *ty) {
    printf("    pop rax\n");

    int sz = size_of(ty);

    if (sz == 1)
        printf("    movsx rax, byte ptr [rax]\n");
    else if (sz == 2) 
        printf("    movsx rax, word ptr [rax]\n");
    else if (sz == 4) 
        printf("    movsxd rax, dword ptr [rax]\n");
    else 
        printf("    mov rax, [rax]\n");
    
    printf("    push rax\n");
}

void store(Type *ty) {
    printf("    pop rdi\n");
    printf("    pop rax\n");

    if (ty->kind == TY_BOOL) {
        printf("    cmp rdi, 0\n");
        printf("    setne dil\n");
        printf("    movzb rdi, dil\n");
    }


    int sz = size_of(ty);

    if (sz == 1)
        printf("    mov [rax], dil\n");
    else if (sz == 2)
        printf("    mov [rax], di\n");
    else if (sz == 4)
        printf("    mov [rax], edi\n");
    else
        printf("    mov [rax], rdi\n");

    printf("    push rdi\n");
}

void inc(Type *ty) {
    printf("    pop rax\n");
    printf("    add rax, %d\n", ty->base ? size_of(ty->base) : 1);
    printf("    push rax\n");
}

void dec(Type *ty) {
    printf("    pop rax\n");
    printf("    sub rax, %d\n", ty->base ? size_of(ty->base) : 1);
    printf("    push rax\n");
}

// 抽象構文木をスタックマシンに乗せてエミュレートする
void gen(Node *node) {
    switch(node->kind) {
        case ND_NULL:
            return;
        case ND_NUM:
            printf("    push %d\n", node->val);
            return;
        case ND_EXPR_STMT:
            gen(node->lhs);
            printf("    add rsp, 8\n");
            return;
        case ND_VAR:
        case ND_MEMBER:
            gen_addr(node);
            if (node->ty->kind != TY_ARRAY)
                load(node->ty);
            return;
        case ND_ASSIGN:
            gen_lval(node->lhs);
            gen(node->rhs);
            store(node->ty);
            return;
        case ND_PRE_INC:
            gen_lval(node->lhs);
            printf("    push [rsp]\n");
            load(node->ty);
            inc(node->ty);
            store(node->ty);
            return;
        case ND_PRE_DEC:
            gen_lval(node->lhs);
            printf("    push [rsp]\n");
            load(node->ty);
            dec(node->ty);
            store(node->ty);
            return;
        case ND_POST_INC:
            gen_lval(node->lhs);
            printf("    push [rsp]\n");
            load(node->ty);
            inc(node->ty);
            store(node->ty);
            dec(node->ty);
            return;
        case ND_POST_DEC:
            gen_lval(node->lhs);
            printf("    push [rsp]\n");
            load(node->ty);
            dec(node->ty);
            store(node->ty);
            inc(node->ty);
            return;
        case ND_COMMA:
            gen(node->lhs);
            gen(node->rhs);
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
                printf("    pop %s\n", argreg8[i]);


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
            if (node->ty->base)  
                printf("    imul rdi, %d\n", size_of(node->ty->base));
            printf("    add rax, rdi\n");
            break;
        case ND_SUB:
            if (node->ty->base)  
                printf("    imul rdi, %d\n", size_of(node->ty->base));
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

void emit_data(Program *prog) {
    printf(".data\n");

    for (VarList *vl = prog->globals; vl; vl = vl->next) {
        Var *var = vl->var;
        printf("%s:\n", var->name);
        if (!var->contents) {
            printf("    .zero %d\n", size_of(var->ty));
            continue;
        }

        for (int i = 0; i < var->cont_len; i++)
            printf("    .byte %d\n", var->contents[i]);
    }
}

void load_arg(Var *var, int idx) {
    int sz = size_of(var->ty);
    if (sz == 1)
        printf("    mov [rbp-%d], %s\n", var->offset, argreg1[idx]);
    else if (sz == 2)
        printf("    mov [rbp-%d], %s\n", var->offset, argreg2[idx]);
    else if (sz == 4)
        printf("    mov [rbp-%d], %s\n", var->offset, argreg4[idx]);
    else
        printf("    mov [rbp-%d], %s\n", var->offset, argreg8[idx]);
}

void emit_text(Program *prog) {
    printf(".text\n");

    for (Function *fn = prog->fns; fn; fn = fn->next) {
        printf(".global %s\n", fn->name);
        printf("%s:\n", fn->name);
        funcname = fn->name;

        // プロローグ
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, %d\n", fn->stack_size);

        // 変数をスタックにpush
        int i = 0;
        for (VarList *vl = fn->params; vl; vl = vl->next) 
            load_arg(vl->var, i++);

        // Emit code
        for(Node *node = fn->node; node; node = node->next)
            gen(node);

        // エピローグ
        printf(".Lreturn.%s:\n", funcname);
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
    }
}

void codegen(Program *prog) {
    printf(".intel_syntax noprefix\n");
    emit_data(prog);
    emit_text(prog);
}