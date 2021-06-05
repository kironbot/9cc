#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type Type;
typedef struct Member Member;

//
// tokenize.c
//

// トークンの種類
typedef enum {
    TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
    TK_STR,   // 文字列
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

    char *contents; // 文字列コンテンツ'\0'終わり
    char cont_len;  // 文字列の長さ
};

// グローバル変数
extern char *filename;
extern char *user_input;
extern Token *token;

// 関数定義
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
Token *peek(char *s);
Token *consume(char *op); 
char *strndup(char *p, int len);
Token *consume_ident();
void expect(char *s);
int expect_number();
char *expect_ident();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize();

//
// parse.c
//

// Variable
typedef struct Var Var;
struct Var {
    char *name;     // 変数名
    Type *ty;       // 型
    Token *tok;     // for error message
    bool is_local;  // ローカル変数フラグ

    // Local variable
    int offset;     // offset from RBP

    // Global variable
    char *contents;
    int cont_len;
};

typedef struct VarList VarList;
struct VarList {
    VarList *next;
    Var *var;
};

// 抽象構文木のノードの種類
typedef enum {
    ND_ADD,     // +
    ND_SUB,     // -
    ND_MUL,     // *
    ND_DIV,     // /
    ND_BITAND,  // &
    ND_BITOR,   // |
    ND_BITXOR,  // ^
    ND_EQ,      // ==
    ND_NE,      // !=
    ND_LT,      // <
    ND_LE,      // <=
    ND_NUM,     // 整数
    ND_CAST,    // Type cast
    ND_NULL,    // NULL
    ND_ASSIGN,  // =
    ND_PRE_INC, // pre ++
    ND_PRE_DEC, // pre --
    ND_POST_INC,// post ++
    ND_POST_DEC,// post --
    ND_A_ADD,   // +=
    ND_A_SUB,   // -=
    ND_A_MUL,   // *=
    ND_A_DIV,   // /=
    ND_COMMA,   // ,
    ND_MEMBER,  // . (struct member access)
    ND_ADDR,    // 単項 &
    ND_DEREF,   // 単項 *
    ND_NOT,     // !
    ND_BITNOT,  // ~
    ND_LOGAND,  // &&
    ND_LOGOR,   // ||
    ND_VAR,     // ローカル変数
    ND_RETURN,  // return
    ND_IF,      // if
    ND_WHILE,   // while
    ND_FOR,     // for
    ND_SIZEOF,  // sizeof
    ND_BLOCK,   // {...}
    ND_BREAK,   // break
    ND_FUNCALL, // 関数呼び出し
    ND_EXPR_STMT, // 宣言文
    ND_STMT_EXPR, // (...)
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind; // ノードの型
    Node *next;    // nextノード
    Type *ty;      // 型（例）int, intへのポインタ
    Token *tok;    // トークン

    Node *lhs;     // 左辺
    Node *rhs;     // 右辺

    // if, while, for
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;

    // ブロック {...} or ステートメント (...)
    Node *body;

    // Struct member access
    char *member_name;
    Member *member;

    // 関数呼び出し
    char *funcname;
    Node *args;

    int val;       // kindがND_NUMのとき使う
    Var *var;    // kindがND_VARのとき使う
};

typedef struct Function Function;
struct Function {
    Function *next;
    char *name;
    VarList *params;

    Node *node;
    VarList *locals;
    int stack_size;
};

typedef struct {
    VarList *globals;
    Function *fns;
} Program;

Program *program();

//
// typing.c
//

typedef enum {
    TY_VOID,
    TY_BOOL,
    TY_CHAR,
    TY_SHORT,
    TY_INT,
    TY_LONG,
    TY_ENUM,
    TY_PTR,
    TY_ARRAY,
    TY_STRUCT,
    TY_FUNC,
} TypeKind;

struct Type {
    TypeKind kind;
    bool is_typedef;    // typedef
    bool is_static;     // static
    bool is_incomplete; // incomplete array
    int align;          // alignment
    Type *base;         // pointer or array
    int array_size;     // array
    Member *members;    // struct
    Type *return_ty;    // function
};

struct Member {
    Member *next;
    Type *ty;
    Token *tok;
    char *name;
    int offset;
};

int align_to(int n, int align);
Type *void_type();
Type *bool_type();
Type *char_type();
Type *short_type();
Type *int_type();
Type *long_type();
Type *enum_type();
Type *struct_type();
Type *func_type(Type *return_ty);
Type *pointer_to(Type *base);
Type *array_of(Type *base, int size);
int size_of(Type *ty, Token *tok);

void add_type(Program *prog);

//
// codegen.c
//

void codegen(Program *prog);