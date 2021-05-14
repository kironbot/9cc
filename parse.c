#include "9cc.h"

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

Node *new_node_lvar(char name) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    node->name = name;
    return node;
}

// 先に宣言しておく
Node *program();
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
Node *program() {
    Node head;
    head.next = NULL;
    Node *cur = &head;

    while(!at_eof()) {
        cur->next = stmt();
        cur = cur->next;
    }
    return head.next;
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

// primary = "(" expr ")" | ident | num
Node *primary() {
    // 次のトークンが'('なら'(' expr ')'のはず
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok = consume_ident();
    if (tok) return new_node_lvar(*tok->str);

    // そうでなければ数値
    return new_node_num(expect_number());
}

Node *unary() {
    if (consume("+"))
        return primary();
    if (consume("-"))
        return new_node(ND_SUB, new_node_num(0), primary());
    
    return primary();
}