#include "9cc.h"

// 入力ファイル
char *filename;
// 入力文字列のトークンを保存するグローバル変数
Token *token;
// 入力プログラムを保存するグローバル変数
char *user_input;
// パース結果のノード置き場
Node *code[100];

// エラー出力
void error(char *fmt) {
    fprintf(stderr, fmt, NULL);
    fprintf(stderr, "\n");
    exit(1);
}

void error1(char *fmt, char *v1) {
    fprintf(stderr, fmt, v1);
    fprintf(stderr, "\n");
    exit(1);
}

void error2(char *fmt, char *v1, char *v2) {
    fprintf(stderr, fmt, v1, v2);
    fprintf(stderr, "\n");
    exit(1);
}

// エラー箇所を報告する
void verror_at(char *loc, char *fmt, char *v) {
    // Find a line containing 'loc'
    char *line = loc;
    while (user_input < line && line[-1] != '\n')
        line--;
    
    char *end = loc;
    while (*end != '\n')
        end++;
    
    // Get a line number
    int line_num = 1;
    for (char *p = user_input; p < line; p++)
        if (*p == '\n')
            line_num++;
    
    // Print out the line
    int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
    fprintf(stderr, "%.*s\n", (end - line), line);

    int pos = loc - line + indent;
    fprintf(stderr, "%*s", pos, ""); // 空白 x pos
    fprintf(stderr, "^ ");
    fprintf(stderr, fmt, v);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt) {
    verror_at(loc, fmt, NULL);
}

void error_at_s(char *loc, char *fmt, char *v) {
    verror_at(loc, fmt, v);
}

void error_tok(Token *tok, char *fmt) {
    if (tok)
        verror_at(tok->str, fmt, NULL);

    fprintf(stderr, fmt, NULL);
    fprintf(stderr, "\n");
    exit(1);
}

void error_tok_s(Token *tok, char *fmt, char *v) {
    if (tok)
        verror_at(tok->str, fmt, v);

    fprintf(stderr, fmt, v);
    fprintf(stderr, "\n");
    exit(1);
}

char *strndup(char *p, int len) {
    char *buf = malloc(len + 1);
    strncpy(buf, p, len);
    buf[len] = '\0';
    return buf;
}

// 与えたstringとトークンが一致したらtrueを返す
Token *peek(char *s) {
    if (token->kind != TK_RESERVED || strlen(s) != token->len
        || memcmp(token->str, s, token->len))
        return NULL;
    return token;
}

// 次のトークンが期待している記号であればトークンを返してトークンを1つすすめる
Token *consume(char *s) {
    if (!peek(s))
        return NULL;
    Token *t = token;
    token = token->next;
    return t;
}

// 次のトークンが変数であれば消費して次のトークンを返す
Token *consume_ident() {
    if (token->kind != TK_IDENT)
        return NULL;
    Token *t = token;
    token = token->next;
    return t;
}

// 次のトークンが期待している記号であれば、トークンを1つ進める。それ以外はエラーを報告する。
void expect(char *s) {
    if (!peek(s))
        error_tok_s(token, "expected \"%s\"", s);
    token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
    if (token->kind != TK_NUM)
        error_tok(token, "expected a number");
    int val = token->val;
    token = token->next;
    return val;
}

// トークンが識別子か判定してトークンを1つ進め文字列を返す
char *expect_ident() {
    if (token->kind != TK_IDENT)
        error_tok(token, "expected an identifier");
    char *s = strndup(token->str, token->len);
    token = token->next;
    return s;
}


// トークンが終端記号かどうか判定する
bool at_eof() {
    return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurにつなげる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

// 入力文字列pの最初がqと一致しているか判定
bool startswith(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}

// 入力がアルファベット1文字か判定
bool is_alpha(char c) {
    return ('a' <= c && c<= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

// 入力がアルファベット1文字、アンダースコア、数字 か判定
bool is_alnum(char c) {
    return is_alpha(c) || ('0' <= c && c <= '9');
}

char *starts_with_reserved(char *p) {
    // キーワード
    static char *kw[] = {"return", "if", "else", "while", "for", "int", 
                         "char", "sizeof", "struct", "typedef", "short",
                         "long", "void", "bool", "_Bool", "FILE", "enum", "static",
                         "break", "continue", "goto", "switch", "case", "default"};

    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        int len = strlen(kw[i]);
        if (startswith(p, kw[i]) && !is_alnum(p[len]))
            return kw[i];
    }

    // 長さ2の記号トークン
    // 前から順に解釈するので順番に注意
    static char *ops[] = {"<<=", ">>=",  "==", "!=", "<=", ">=", "->",
                          "++", "--", "<<", ">>", "+=", "-=", "*=", "/=", "&&", "||"};

    for (int i = 0; i < sizeof(ops) / sizeof(*ops); i++) {
        if (startswith(p, ops[i]))
            return ops[i];
    }

    return NULL;
}

char get_escape_char(char c) {
    switch(c) {
    case 'a': return '\a';
    case 'b': return '\b';
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'v': return '\v';
    case 'e': return 27;
    case '0': return 0;
    default: return c;
    }
}

Token *read_string_literal(Token *cur, char *start) {
    char *p = start + 1;
    char buf[1024];
    int len = 0;

    for (;;) {
        if (len == sizeof(buf))
            error_at(start, "string literal too large");
        if (*p == '\0')
            error_at(start, "unclosed string literal");
        if (*p == '"')
            break;

        if (*p == '\\') {
            p++;
            buf[len++] = get_escape_char(*p++);
        } else {
            buf[len++] = *p++;
        }
    }

    Token *tok = new_token(TK_STR, cur, start, p - start + 1);
    tok->contents = malloc(len + 1);
    memcpy(tok->contents, buf, len);
    tok->contents[len] = '\0';
    tok->cont_len = len + 1;
    return tok;
}

Token *read_char_literal(Token *cur, char *start) {
    char *p = start + 1;
    if (*p == '\0')
        error_at(start, "unclosed char literal");
    
    char c;
    if (*p == '\\') {
        p++;
        c = get_escape_char(*p++);
    } else {
        c = *p++;
    }

    if (*p != '\'')
        error_at(start, "char literal too long");
    p++;

    Token *tok = new_token(TK_NUM, cur, start, p - start);
    tok->val = c;
    return tok;
}


// 入力文字列pをトークナイズしてそれを返す
Token *tokenize() {
    char *p = user_input;
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while(*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        // #include行をスキップ
        if (startswith(p, "#include")) {
            while (*p != '\n')
                p++;
            continue;
        }

        // #extern行をスキップ
        if (startswith(p, "extern")) {
            while (*p != '\n')
                p++;
            continue;
        }

        // ラインコメントをスキップ
        if (startswith(p, "//")) {
            p += 2;
            while (*p != '\n')
                p++;
            continue;
        }

        // ブロックコメントをスキップ
        if (startswith(p, "/*")) {
            char *q = strstr(p + 2, "*/");
            if (!q) 
                error_at(p, "unclosed block comment");
            p = q + 2;
            continue;
        }

        // キーワード、2文字以上の記号トークン
        char *kw = starts_with_reserved(p);
        if (kw) {
            int len = strlen(kw);
            cur = new_token(TK_RESERVED, cur, p, len);
            p += len;
            continue;
        }

        // 長さ1の記号トークン
        if (strchr("+-*/()<>;={},&[].,!~|^:?", *p)) {
            cur = new_token(TK_RESERVED, cur, p, 1);
            p++;
            continue;
        }

        // Identity
        if (is_alpha(*p)) {
            char *q = p++;
            while (is_alnum(*p)) p++;
            cur = new_token(TK_IDENT, cur, q, p - q);
            continue;
        }

        // String
        if (*p == '"') {
            cur = read_string_literal(cur, p);
            p += cur->len;
            continue;
        }

        // Char
        if (*p == '\'') {
            cur = read_char_literal(cur, p);
            p += cur->len;
            continue;
        }

        // Int
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        error_at(token->str, "cannot tokenize");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}
