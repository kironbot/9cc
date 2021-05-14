#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// tokenize.c
//

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

// グローバル変数
extern char *user_input;
extern Token *token;

// 関数定義
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(char *op); 
Token *consume_ident();
bool expect(char *op);
int expect_number();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize(char *p);

//
// parse.c
//

// 抽象構文木のノードの種類
typedef enum {
    ND_ADD,     // +
    ND_SUB,     // -
    ND_MUL,     // *
    ND_DIV,     // /
    ND_EQ,      // ==
    ND_NE,      // !=
    ND_LT,      // <
    ND_LE,      // <=
    ND_NUM,     // 整数
    ND_ASSIGN,  // =
    ND_LVAR,    // ローカル変数
    ND_RETURN,  // return
    ND_EXPR_STMT, // 宣言文
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind; // ノードの型
    Node *next;    // nextノード
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    int val;       // kindがND_NUMの場合の数値
    char name;    // kindがND_LVARの場合のみ使う。RBPからのオフセット値。
};

Node *program();

//
// codegen.c
//

void codegen(Node *node);