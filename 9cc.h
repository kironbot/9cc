#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// トークンの種類
typedef enum {
    TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
    TK_NUM,      // 整数
    TK_EOF,      // 終端記号
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
    TokenKind kind; // トークンの型
    Token *next;    // 次の入力トークン
    int val;        // kindがTK_NUMの場合、その値
    char *str;      // トークン文字列
    int len;        // トークンの長さ
};

// 抽象構文木のノードの種類
typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_EQ,  // ==
    ND_NE,  // !=
    ND_LT,  // <
    ND_LE,  // <=
    ND_NUM, // 整数
    ND_ASSIGN, // =
    ND_LVAR,   // ローカル変数
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind; // ノードの型
    Node *next;    // nextノード
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    int val;       // kindがND_NUMの場合の数値
    int offset;    // kindがND_LVARの場合のみ使う。RBPからのオフセット値。
};


// parse.c
Token *token;
bool consume(char *op); 
bool expect(char *op);
int expect_number();
bool at_eof();

Token *new_token(TokenKind kind, Token *cur, char *str, int len);
bool startswith(char *p, char *q);
Token *tokenize(char *p);

Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);

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


// codegen.c
void gen(Node *node);
void codegen(Node *node);


// util.c
char *user_input;
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);