#include "9cc.h"

// ローカル変数列
Var *locals;

// ローカル変数を名前から見つける
Var *find_var(Token *tok) {
    for (Var *var = locals; var; var = var->next) {
        if (strlen(var->name) == tok->len && !memcmp(tok->str, var->name, tok->len))
            return var;
    }
    return NULL;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

Node *new_node_unary(NodeKind kind, Node *expr) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = expr;
    return node;
}

Node *new_node_var(Var *var) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_VAR;
    node->var = var;
    return node;
}

Var *push_var(char *name) {
    Var *var = calloc(1, sizeof(Var));
    var->next = locals;
    var->name = name;
    locals = var;
    return var;
}

// 先に宣言しておく
Program *program();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// program = stmt*
Program *program() {
    locals = NULL;

    Node head;
    head.next = NULL;
    Node *cur = &head;

    while(!at_eof()) {
        cur->next = stmt();
        cur = cur->next;
    }

    Program *prog = calloc(1, sizeof(Program));
    prog->node = head.next;
    prog->locals = locals;
    return prog;
}

// stmt = "return" expr ";" | expr ";"
Node *stmt() {
    if (consume("return")) {
        Node *node = new_node_unary(ND_RETURN, expr());
        expect(";");
        return node;
    }

    Node *node = new_node_unary(ND_EXPR_STMT, expr());
    expect(";");
    return node; 
}

// expr = assign
Node *expr() {
    return assign();
}

Node *assign() {
    Node *node = equality();
    if(consume("="))
        node = new_node(ND_ASSIGN, node, assign());
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
    Node *node = relational();

    for (;;) {
        if (consume("==")) 
            node = new_node(ND_EQ, node, relational());
        else if (consume("!="))
            node = new_node(ND_NE, node, relational());
        else
            return node;
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
    Node *node = add();

    for(;;) {
        if (consume("<"))
            node = new_node(ND_LT, node, add());
        else if (consume("<="))
            node = new_node(ND_LE, node, add());
        else if (consume(">"))
            node = new_node(ND_LT, add(), node);
        else if (consume(">="))
            node = new_node(ND_LE, add(), node);
        else
            return node;
    }
}

// add = mul ("+" mul | "-" mul)*
Node *add() {
    Node *node = mul();

    for(;;) {
        if (consume("+"))
            node = new_node(ND_ADD, node, mul());
        if (consume("-"))
            node = new_node(ND_SUB, node, mul());
        else
            return node;
    }
}

Node *mul() {
    Node *node = unary();

    for (;;) {
        if (consume("*"))
            node = new_node(ND_MUL, node, unary());
        else if (consume("/"))
            node = new_node(ND_DIV, node, unary());
        else
            return node;
    }
}

Node *unary() {
    if (consume("+"))
        return primary();
    if (consume("-"))
        return new_node(ND_SUB, new_node_num(0), primary());
    
    return primary();
}

// primary = "(" expr ")" | ident | num
Node *primary() {
    // 次のトークンが'('なら'(' expr ')'のはず
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok = consume_ident();
    if (tok) {
        Var *var = find_var(tok);
        if (!var) 
            var = push_var(strndup(tok->str, tok->len));
        return new_node_var(var);
    }

    // そうでなければ数値
    return new_node_num(expect_number());
}

