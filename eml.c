/* ============================================================
 *  Emerald 语言解释器
 *  编译器: TDM-GCC / MinGW
 *  用法:
 *    eml -c <file.eml>       编译为 .elr
 *    eml run <file>          运行（不用写扩展名）
 *    eml wrapper <file.eml>  打包为独立 exe（自解压式）
 *    eml ide                 启动 Emerald IDE
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  include <shellapi.h>
#else
#  include <unistd.h>
#endif

#define MAX_TOKENS 10000
#define MAX_TEXT   256

/* 打包 footer 魔数 */
#define WRAP_MAGIC  "EMLWRAP1"
#define WRAP_FOOTER 20   /* 8 magic + 4 length + 8 magic */

/* ================= Token ================= */
typedef enum { T_EOF, T_IDENT, T_NUMBER, T_STRING, T_PUNCT } EmlTokenType;

typedef struct {
    EmlTokenType type;
    char text[MAX_TEXT];
    int line;
} Token;

Token tokens[MAX_TOKENS];
int token_count = 0;
int pos = 0;

/* ================= Value ================= */
typedef enum { V_INT, V_STRING, V_BOOL, V_LIST, V_NONE } VarType;
typedef struct List List;

typedef struct Value {
    VarType type;
    union {
        int i;
        int b;
        char *s;
        List *list;
    } v;
} Value;

struct List {
    Value *items;
    int count;
    int cap;
};

/* ================= 变量 ================= */
typedef struct Var {
    char name[64];
    Value val;
    struct Var *next;
} Var;
Var *vars = NULL;

/* ================= 函数 ================= */
typedef struct Function {
    char name[64];
    VarType ret_type;
    int body_start;
    int body_end;
    struct Function *next;
} Function;
Function *functions = NULL;

/* ================= 全局状态 ================= */
char g_subcmd[64] = "other";
Value g_return;
int g_has_return = 0;

/* 自身 exe 路径 */
char g_self_path[1024] = "";

/* ================= 工具函数 ================= */
char *xstrdup(const char *s) {
    char *p;
    p = (char*)malloc(strlen(s) + 1);
    strcpy(p, s);
    return p;
}

void get_self_path(void) {
#ifdef _WIN32
    DWORD n;
    n = GetModuleFileNameA(NULL, g_self_path, (DWORD)sizeof(g_self_path));
    if (n == 0) {
        strcpy(g_self_path, "eml.exe");
    }
#else
    {
        ssize_t n;
        n = readlink("/proc/self/exe", g_self_path, sizeof(g_self_path) - 1);
        if (n > 0) {
            g_self_path[n] = 0;
        } else {
            strcpy(g_self_path, "eml");
        }
    }
#endif
}

void add_token(EmlTokenType type, const char *text, int line) {
    if (token_count >= MAX_TOKENS) {
        fprintf(stderr, "Too many tokens\n");
        exit(1);
    }
    tokens[token_count].type = type;
    strncpy(tokens[token_count].text, text, MAX_TEXT - 1);
    tokens[token_count].text[MAX_TEXT - 1] = 0;
    tokens[token_count].line = line;
    token_count++;
}

Token *cur(void) { return &tokens[pos]; }

int match_punct(const char *s) {
    if (cur()->type == T_PUNCT && strcmp(cur()->text, s) == 0) {
        pos++;
        return 1;
    }
    return 0;
}

void expect_punct(const char *s) {
    if (!match_punct(s)) {
        fprintf(stderr, "Expected '%s' at line %d, got '%s'\n",
                s, cur()->line, cur()->text);
        exit(1);
    }
}

char *expect_ident(void) {
    char *s;
    if (cur()->type != T_IDENT) {
        fprintf(stderr, "Expected identifier at line %d\n", cur()->line);
        exit(1);
    }
    s = xstrdup(cur()->text);
    pos++;
    return s;
}

/* ================= 词法分析 ================= */
void lex(const char *src) {
    int i = 0, line = 1;
    token_count = 0;

    while (src[i]) {
        if (isspace((unsigned char)src[i])) {
            if (src[i] == '\n') line++;
            i++;
            continue;
        }

        if (src[i] == '/' && src[i + 1] == '/') {
            while (src[i] && src[i] != '\n') i++;
            continue;
        }

        if (src[i] == '/' && src[i + 1] == '*') {
            i += 2;
            while (src[i] && !(src[i] == '*' && src[i + 1] == '/')) {
                if (src[i] == '\n') line++;
                i++;
            }
            if (src[i]) i += 2;
            continue;
        }

        /* 全角大括号兼容 */
        if ((unsigned char)src[i] == 0xEF &&
            (unsigned char)src[i + 1] == 0xBC &&
            (unsigned char)src[i + 2] == 0x9B) {
            add_token(T_PUNCT, "{", line);
            i += 3;
            continue;
        }
        if ((unsigned char)src[i] == 0xEF &&
            (unsigned char)src[i + 1] == 0xBC &&
            (unsigned char)src[i + 2] == 0x9D) {
            add_token(T_PUNCT, "}", line);
            i += 3;
            continue;
        }

        if (isalpha((unsigned char)src[i]) || src[i] == '_') {
            char buf[MAX_TEXT];
            int n = 0;
            while (isalnum((unsigned char)src[i]) || src[i] == '_') {
                if (n < MAX_TEXT - 1) buf[n++] = src[i];
                i++;
            }
            buf[n] = 0;
            add_token(T_IDENT, buf, line);
            continue;
        }

        if (isdigit((unsigned char)src[i])) {
            char buf[MAX_TEXT];
            int n = 0;
            while (isdigit((unsigned char)src[i])) {
                if (n < MAX_TEXT - 1) buf[n++] = src[i];
                i++;
            }
            buf[n] = 0;
            add_token(T_NUMBER, buf, line);
            continue;
        }

        if (src[i] == '"') {
            char buf[MAX_TEXT];
            int n = 0;
            i++;
            while (src[i] && src[i] != '"') {
                if (src[i] == '\\' && src[i + 1]) {
                    i++;
                    if (src[i] == 'n') buf[n++] = '\n';
                    else if (src[i] == 't') buf[n++] = '\t';
                    else buf[n++] = src[i];
                    i++;
                } else {
                    if (n < MAX_TEXT - 1) buf[n++] = src[i];
                    i++;
                }
            }
            buf[n] = 0;
            if (src[i] == '"') i++;
            add_token(T_STRING, buf, line);
            continue;
        }

        {
            char p[2];
            p[0] = src[i];
            p[1] = 0;
            add_token(T_PUNCT, p, line);
        }
        i++;
    }

    add_token(T_EOF, "", line);
}

/* ================= Value 构造 ================= */
Value make_int(int i) {
    Value v; v.type = V_INT; v.v.i = i; return v;
}
Value make_bool(int b) {
    Value v; v.type = V_BOOL; v.v.b = b; return v;
}
Value make_string(const char *s) {
    Value v; v.type = V_STRING; v.v.s = xstrdup(s); return v;
}
Value make_none(void) {
    Value v; v.type = V_NONE; v.v.i = 0; return v;
}

/* ================= List ================= */
List *list_new(void) {
    List *l;
    l = (List*)malloc(sizeof(List));
    l->count = 0;
    l->cap = 4;
    l->items = (Value*)malloc(sizeof(Value) * l->cap);
    return l;
}

void list_add(List *l, Value v) {
    if (l->count >= l->cap) {
        l->cap *= 2;
        l->items = (Value*)realloc(l->items, sizeof(Value) * l->cap);
    }
    l->items[l->count++] = v;
}

void list_del(List *l, int idx) {
    int i, j;
    if (idx < 1 || idx > l->count) return;
    i = idx - 1;
    for (j = i; j < l->count - 1; j++)
        l->items[j] = l->items[j + 1];
    l->count--;
}

void list_clean(List *l) {
    l->count = 0;
}

/* ================= 变量操作 ================= */
Var *find_var(const char *name) {
    Var *v;
    for (v = vars; v; v = v->next)
        if (strcmp(v->name, name) == 0) return v;
    return NULL;
}

void set_var(const char *name, Value val) {
    Var *v;
    v = find_var(name);
    if (v) {
        v->val = val;
        return;
    }
    v = (Var*)malloc(sizeof(Var));
    strncpy(v->name, name, 63);
    v->name[63] = 0;
    v->val = val;
    v->next = vars;
    vars = v;
}

Value get_var(const char *name) {
    Var *v;
    v = find_var(name);
    if (!v) {
        fprintf(stderr, "Undefined variable: %s\n", name);
        exit(1);
    }
    return v->val;
}

/* ================= 函数操作 ================= */
Function *find_func(const char *name) {
    Function *f;
    for (f = functions; f; f = f->next)
        if (strcmp(f->name, name) == 0) return f;
    return NULL;
}

void add_func(const char *name, VarType ret, int start, int end) {
    Function *f;
    f = (Function*)malloc(sizeof(Function));
    strncpy(f->name, name, 63);
    f->name[63] = 0;
    f->ret_type = ret;
    f->body_start = start;
    f->body_end = end;
    f->next = functions;
    functions = f;
}

/* ================= 打印 ================= */
void print_value(Value v) {
    int i;
    switch (v.type) {
        case V_INT:
            printf("%d", v.v.i);
            break;
        case V_BOOL:
            printf("%s", v.v.b ? "true" : "false");
            break;
        case V_STRING:
            printf("%s", v.v.s);
            break;
        case V_LIST:
            printf("[");
            for (i = 0; i < v.v.list->count; i++) {
                if (i) printf(", ");
                print_value(v.v.list->items[i]);
            }
            printf("]");
            break;
        default:
            break;
    }
}

/* ================= 前向声明 ================= */
Value eval_expr(void);
Value call_func(const char *name);
void exec_block(int start, int end);
void exec_statement(void);

/* ================= 表达式求值 ================= */
Value eval_expr(void) {
    Token *t;
    t = cur();

    if (t->type == T_NUMBER) {
        pos++;
        return make_int(atoi(t->text));
    }

    if (t->type == T_STRING) {
        pos++;
        return make_string(t->text);
    }

    if (t->type == T_IDENT) {
        char name[64];
        strncpy(name, t->text, 63);
        name[63] = 0;
        pos++;

        if (strcmp(name, "true") == 0)  return make_bool(1);
        if (strcmp(name, "false") == 0) return make_bool(0);

        if (match_punct("(")) {
            expect_punct(")");
            return call_func(name);
        }

        return get_var(name);
    }

    fprintf(stderr, "Bad expression at line %d\n", t->line);
    exit(1);
}

/* ================= 函数调用 ================= */
Value call_func(const char *name) {
    Function *f;
    int old_pos, old_has_return;
    Value old_return, ret;

    f = find_func(name);
    if (!f) {
        fprintf(stderr, "Function not found: %s\n", name);
        exit(1);
    }

    old_pos = pos;
    old_has_return = g_has_return;
    old_return = g_return;

    g_has_return = 0;
    pos = f->body_start;
    exec_block(f->body_start, f->body_end);

    ret = g_has_return ? g_return : make_none();

    pos = old_pos;
    g_has_return = old_has_return;
    g_return = old_return;

    return ret;
}

/* ================= 执行语句 ================= */
void exec_statement(void) {
    Token *t;
    t = cur();

    /* import */
    if (t->type == T_IDENT && strcmp(t->text, "import") == 0) {
        pos++;
        while (cur()->type != T_EOF &&
               !(cur()->type == T_PUNCT && strcmp(cur()->text, ";") == 0))
            pos++;
        match_punct(";");
        return;
    }

    /* eml() 入口函数定义（定义后自动调用） */
    if (t->type == T_IDENT && strcmp(t->text, "eml") == 0) {
        pos++;
        if (match_punct("(")) {
            int body_start, body_end, depth;
            expect_punct(")");
            expect_punct("{");
            body_start = pos;
            depth = 1;
            while (depth > 0 && cur()->type != T_EOF) {
                if (cur()->type == T_PUNCT && strcmp(cur()->text, "{") == 0) depth++;
                else if (cur()->type == T_PUNCT && strcmp(cur()->text, "}") == 0) depth--;
                pos++;
            }
            body_end = pos - 1;
            add_func("eml", V_NONE, body_start, body_end);
            call_func("eml");
            return;
        }
    }

    /* print */
    if (t->type == T_IDENT && strcmp(t->text, "print") == 0) {
        Value v;
        pos++;
        expect_punct("(");
        v = eval_expr();
        expect_punct(")");
        expect_punct(";");
        print_value(v);
        fflush(stdout);
        return;
    }

    /* newline */
    if (t->type == T_IDENT && strcmp(t->text, "newline") == 0) {
        pos++;
        expect_punct("(");
        expect_punct(")");
        expect_punct(";");
        printf("\n");
        fflush(stdout);
        return;
    }

    /* input */
    if (t->type == T_IDENT && strcmp(t->text, "input") == 0) {
        char *name;
        char buf[1024];
        pos++;
        expect_punct("(");
        name = expect_ident();
        expect_punct(")");
        expect_punct(";");

        if (fgets(buf, sizeof(buf), stdin)) {
            buf[strcspn(buf, "\n")] = 0;
            set_var(name, make_string(buf));
        }
        free(name);
        return;
    }

    /* speak */
    if (t->type == T_IDENT && strcmp(t->text, "speak") == 0) {
        Value v;
        pos++;
        expect_punct("(");
        v = eval_expr();
        expect_punct(")");
        expect_punct(";");
        printf("[讲述人] ");
        print_value(v);
        printf("\n");
        fflush(stdout);
        return;
    }

    /* format(a, list) */
    if (t->type == T_IDENT && strcmp(t->text, "format") == 0) {
        char *name, *type;
        Value v;
        pos++;
        expect_punct("(");
        name = expect_ident();
        expect_punct(",");
        type = expect_ident();
        expect_punct(")");
        expect_punct(";");

        if (strcmp(type, "list") == 0) {
            v.type = V_LIST;
            v.v.list = list_new();
            set_var(name, v);
        }
        free(name);
        free(type);
        return;
    }

    /* list a(add, ...) / list a(del, ...) / list a(clean) */
    if (t->type == T_IDENT && strcmp(t->text, "list") == 0) {
        char *name, *op;
        pos++;
        name = expect_ident();
        expect_punct("(");
        op = expect_ident();

        if (strcmp(op, "add") == 0) {
            Value v;
            Var *var;
            expect_punct(",");
            v = eval_expr();
            expect_punct(")");
            expect_punct(";");
            var = find_var(name);
            if (var && var->val.type == V_LIST)
                list_add(var->val.v.list, v);
        } else if (strcmp(op, "del") == 0) {
            Value v;
            Var *var;
            expect_punct(",");
            v = eval_expr();
            expect_punct(")");
            expect_punct(";");
            var = find_var(name);
            if (var && var->val.type == V_LIST)
                list_del(var->val.v.list, v.v.i);
        } else if (strcmp(op, "clean") == 0) {
            Var *var;
            expect_punct(")");
            expect_punct(";");
            var = find_var(name);
            if (var && var->val.type == V_LIST)
                list_clean(var->val.v.list);
        }
        free(name);
        free(op);
        return;
    }

    /* 类型声明 / 函数定义 */
    if (t->type == T_IDENT &&
        (strcmp(t->text, "int") == 0 ||
         strcmp(t->text, "string") == 0 ||
         strcmp(t->text, "bool") == 0 ||
         strcmp(t->text, "function") == 0)) {

        VarType type;
        char *name;

        type = V_NONE;
        if (strcmp(t->text, "int") == 0) type = V_INT;
        else if (strcmp(t->text, "string") == 0) type = V_STRING;
        else if (strcmp(t->text, "bool") == 0) type = V_BOOL;

        pos++;
        name = expect_ident();

        /* 函数定义 */
        if (match_punct("(")) {
            int body_start, body_end, depth;
            expect_punct(")");
            expect_punct("{");
            body_start = pos;
            depth = 1;
            while (depth > 0 && cur()->type != T_EOF) {
                if (cur()->type == T_PUNCT && strcmp(cur()->text, "{") == 0) depth++;
                else if (cur()->type == T_PUNCT && strcmp(cur()->text, "}") == 0) depth--;
                pos++;
            }
            body_end = pos - 1;
            add_func(name, type, body_start, body_end);
            free(name);
            return;
        }

        /* 变量声明 */
        if (match_punct(";")) {
            Value v;
            if (type == V_INT) v = make_int(0);
            else if (type == V_STRING) v = make_string("");
            else v = make_bool(0);
            set_var(name, v);
            free(name);
            return;
        }

        /* 变量声明并赋值 */
        if (match_punct("=")) {
            Value v;
            v = eval_expr();
            expect_punct(";");
            set_var(name, v);
            free(name);
            return;
        }
    }

    /* 赋值或函数调用 */
    if (t->type == T_IDENT) {
        char name[64];
        strncpy(name, t->text, 63);
        name[63] = 0;
        pos++;

        if (match_punct("=")) {
            Value v;
            v = eval_expr();
            expect_punct(";");
            set_var(name, v);
            return;
        }

        if (match_punct("(")) {
            expect_punct(")");
            expect_punct(";");
            call_func(name);
            return;
        }
    }

    /* subcommand */
    if (t->type == T_IDENT && strcmp(t->text, "subcommand") == 0) {
        char arg[64];
        int body_start, body_end, depth;
        pos++;
        expect_punct("(");
        if (cur()->type == T_STRING || cur()->type == T_IDENT) {
            strncpy(arg, cur()->text, 63);
            arg[63] = 0;
            pos++;
        } else {
            strcpy(arg, "other");
        }
        expect_punct(")");
        expect_punct("{");

        body_start = pos;
        depth = 1;
        while (depth > 0 && cur()->type != T_EOF) {
            if (cur()->type == T_PUNCT && strcmp(cur()->text, "{") == 0) depth++;
            else if (cur()->type == T_PUNCT && strcmp(cur()->text, "}") == 0) depth--;
            pos++;
        }
        body_end = pos - 1;

        if (strcmp(arg, g_subcmd) == 0 ||
            (strcmp(arg, "other") == 0 && strcmp(g_subcmd, "other") == 0)) {
            exec_block(body_start, body_end);
        }
        return;
    }

    /* return */
    if (t->type == T_IDENT && strcmp(t->text, "return") == 0) {
        pos++;
        g_return = eval_expr();
        g_has_return = 1;
        expect_punct(";");
        return;
    }

    /* 未知 token，跳过 */
    pos++;
}

void exec_block(int start, int end) {
    pos = start;
    while (pos < end && cur()->type != T_EOF) {
        if (g_has_return) break;
        exec_statement();
    }
}

/* ================= 编译 ================= */
void compile_file(const char *filename) {
    FILE *f, *o;
    long len;
    char *src, *dot;
    char base[512];
    char out[512];

    f = fopen(filename, "rb");
    if (!f) { fprintf(stderr, "Cannot open %s\n", filename); exit(1); }

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);

    src = (char*)malloc(len + 1);
    fread(src, 1, len, f);
    src[len] = 0;
    fclose(f);

    strcpy(base, filename);
    dot = strrchr(base, '.');
    if (dot) *dot = 0;

    sprintf(out, "%s.elr", base);

    o = fopen(out, "wb");
    if (!o) { fprintf(stderr, "Cannot write %s\n", out); exit(1); }
    fputs("EMRL1\n", o);
    fputs(src, o);
    fclose(o);

    free(src);
    printf("Compiled to %s\n", out);
}

/* ================= 运行 ================= */
void run_file(const char *filename) {
    char path[512];
    FILE *f = NULL;
    long len;
    char *src;

    f = fopen(filename, "rb");
    if (!f) {
        sprintf(path, "%s.elr", filename);
        f = fopen(path, "rb");
    }
    if (!f) {
        sprintf(path, "%s.eml", filename);
        f = fopen(path, "rb");
    }
    if (!f) {
        fprintf(stderr, "Cannot open %s\n", filename);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);

    src = (char*)malloc(len + 1);
    fread(src, 1, len, f);
    src[len] = 0;
    fclose(f);

    if (strncmp(src, "EMRL1\n", 6) == 0) {
        memmove(src, src + 6, len - 5);
    }

    lex(src);

    pos = 0;
    g_has_return = 0;
    exec_block(0, token_count);

    free(src);
}

/* ================= 打包（自解压式） ================= */
/*
 * 输出结构：
 *   [ 原 eml.exe 的全部二进制 ]
 *   [ 嵌入的 .eml 源码 ]
 *   [ "EMLWRAP1" (8B) ][ 源码长度 (4B, little-endian) ][ "EMLWRAP1" (8B) ]
 */
void wrapper_file(const char *filename) {
    FILE *f, *me, *cfg, *out;
    long src_len, me_len;
    char *src = NULL, *me_buf = NULL;
    char pack_name[64];
    char pack_version[64];
    char icon[64];
    char line[256];
    char out_path[512];
    char base[512];
    char *dot, *slash;
    unsigned char footer[WRAP_FOOTER];
    unsigned int L;

    /* ---------- 0) 默认 pack_name 由 .eml 文件名派生 ---------- */
    strncpy(base, filename, sizeof(base) - 1);
    base[sizeof(base) - 1] = 0;

    slash = strrchr(base, '\\');
    if (!slash) slash = strrchr(base, '/');
    if (slash) {
        memmove(base, slash + 1, strlen(slash + 1) + 1);
    }

    dot = strrchr(base, '.');
    if (dot) *dot = 0;

    if (base[0] == 0) {
        strcpy(base, "example");
    }

    strncpy(pack_name, base, sizeof(pack_name) - 1);
    pack_name[sizeof(pack_name) - 1] = 0;

    strcpy(pack_version, "1.0.0");
    icon[0] = 0;

    /* ---------- 1) 读 .eml 源码 ---------- */
    f = fopen(filename, "rb");
    if (!f) { fprintf(stderr, "Cannot open %s\n", filename); exit(1); }
    fseek(f, 0, SEEK_END);
    src_len = ftell(f);
    fseek(f, 0, SEEK_SET);
    src = (char*)malloc(src_len + 1);
    fread(src, 1, src_len, f);
    src[src_len] = 0;
    fclose(f);

    /* ---------- 2) 读 wrapper.ewd（可选，存在则覆盖） ---------- */
    cfg = fopen("wrapper.ewd", "r");
    if (cfg) {
        while (fgets(line, sizeof(line), cfg)) {
            char *q;
            if ((q = strstr(line, "pack_name="))) {
                char tmp[64];
                if (sscanf(q + 10, "\"%63[^\"]\"", tmp) == 1 && tmp[0]) {
                    strncpy(pack_name, tmp, sizeof(pack_name) - 1);
                    pack_name[sizeof(pack_name) - 1] = 0;
                }
            } else if ((q = strstr(line, "pack_version="))) {
                char tmp[64];
                if (sscanf(q + 13, "\"%63[^\"]\"", tmp) == 1 && tmp[0]) {
                    strncpy(pack_version, tmp, sizeof(pack_version) - 1);
                    pack_version[sizeof(pack_version) - 1] = 0;
                }
            } else if ((q = strstr(line, "icon="))) {
                char tmp[64];
                if (sscanf(q + 5, "\"%63[^\"]\"", tmp) == 1 && tmp[0]) {
                    strncpy(icon, tmp, sizeof(icon) - 1);
                    icon[sizeof(icon) - 1] = 0;
                }
            }
        }
        fclose(cfg);
    }

    /* ---------- 3) 读自身 exe ---------- */
    me = fopen(g_self_path, "rb");
    if (!me) {
        fprintf(stderr, "Cannot open self exe: %s\n", g_self_path);
        free(src);
        exit(1);
    }
    fseek(me, 0, SEEK_END);
    me_len = ftell(me);
    fseek(me, 0, SEEK_SET);
    me_buf = (char*)malloc(me_len);
    if (fread(me_buf, 1, me_len, me) != (size_t)me_len) {
        fprintf(stderr, "Read self exe failed\n");
        fclose(me); free(src); free(me_buf); exit(1);
    }
    fclose(me);

    /* ---------- 4) 输出：exe + 源码 + footer ---------- */
    sprintf(out_path, "%s.exe", pack_name);
    out = fopen(out_path, "wb");
    if (!out) {
        fprintf(stderr, "Cannot write %s\n", out_path);
        free(src); free(me_buf); exit(1);
    }

    fwrite(me_buf, 1, me_len, out);
    fwrite(src, 1, src_len, out);

    memcpy(footer, WRAP_MAGIC, 8);
    L = (unsigned int)src_len;
    footer[8]  = (unsigned char)(L & 0xFF);
    footer[9]  = (unsigned char)((L >> 8)  & 0xFF);
    footer[10] = (unsigned char)((L >> 16) & 0xFF);
    footer[11] = (unsigned char)((L >> 24) & 0xFF);
    memcpy(footer + 12, WRAP_MAGIC, 8);

    fwrite(footer, 1, WRAP_FOOTER, out);
    fclose(out);

    printf("Wrapper generated: %s\n", out_path);
    printf("Pack name   : %s\n", pack_name);
    printf("Pack version: %s\n", pack_version);
    if (icon[0]) printf("Icon        : %s (metadata only)\n", icon);

    free(src);
    free(me_buf);
}

/* ================= 检查自身是否为打包过的 exe ================= */
int try_run_wrapped(void) {
    FILE *me;
    long total;
    unsigned char footer[WRAP_FOOTER];
    unsigned int src_len;
    char *src;

    me = fopen(g_self_path, "rb");
    if (!me) return 0;

    fseek(me, 0, SEEK_END);
    total = ftell(me);

    if (total < (long)WRAP_FOOTER + 1) { fclose(me); return 0; }

    fseek(me, total - WRAP_FOOTER, SEEK_SET);
    if (fread(footer, 1, WRAP_FOOTER, me) != WRAP_FOOTER) { fclose(me); return 0; }

    if (memcmp(footer, WRAP_MAGIC, 8) != 0) { fclose(me); return 0; }
    if (memcmp(footer + 12, WRAP_MAGIC, 8) != 0) { fclose(me); return 0; }

    src_len = ((unsigned int)footer[8])
            | ((unsigned int)footer[9]  << 8)
            | ((unsigned int)footer[10] << 16)
            | ((unsigned int)footer[11] << 24);

    if ((long)src_len + WRAP_FOOTER >= total) { fclose(me); return 0; }

    fseek(me, total - WRAP_FOOTER - (long)src_len, SEEK_SET);
    src = (char*)malloc(src_len + 1);
    if (fread(src, 1, src_len, me) != src_len) {
        free(src); fclose(me); return 0;
    }
    src[src_len] = 0;
    fclose(me);

    lex(src);

    pos = 0;
    g_has_return = 0;
    exec_block(0, token_count);

    free(src);
    return 1;
}

/* ================= 启动 Emerald IDE ================= */
#ifdef _WIN32
int launch_ide(void) {
    char self_path[MAX_PATH];
    char candidates[6][MAX_PATH];
    int i, found = -1;
    DWORD n;

    /* 1) 获取 eml.exe 自身路径 */
    n = GetModuleFileNameA(NULL, self_path, MAX_PATH);
    if (n == 0) {
        fprintf(stderr, "无法获取 eml.exe 路径\n");
        return 1;
    }

    /* 2) 去掉文件名，得到所在目录 */
    {
        char *p = strrchr(self_path, '\\');
        if (p) *p = 0;
    }

    /* 3) 拼多个候选路径（相对 eml.exe 所在目录） */
    /*    1. 同目录 */
    snprintf(candidates[0], MAX_PATH,
        "%s\\EmeraldIDE.exe", self_path);

    /*    2. 同级 IDE 目录 */
    snprintf(candidates[1], MAX_PATH,
        "%s\\..\\emerald-ide-dist\\win-unpacked\\EmeraldIDE.exe", self_path);

    /*    3. 下级目录 */
    snprintf(candidates[2], MAX_PATH,
        "%s\\emerald-ide-dist\\win-unpacked\\EmeraldIDE.exe", self_path);

    /*    4. 另一个可能的目录 */
    snprintf(candidates[3], MAX_PATH,
        "%s\\..\\eml-ide-dist\\win-unpacked\\EmeraldIDE.exe", self_path);

    /*    5. electron-builder 默认输出 */
    snprintf(candidates[4], MAX_PATH,
        "%s\\..\\eml-ide\\dist\\win-unpacked\\EmeraldIDE.exe", self_path);

    /*    6. 上级目录的 IDE 源码目录下的 dist */
    snprintf(candidates[5], MAX_PATH,
        "%s\\..\\emerald-ide\\dist\\win-unpacked\\EmeraldIDE.exe", self_path);

    /* 4) 依次查找第一个存在的 */
    for (i = 0; i < 6; i++) {
        if (GetFileAttributesA(candidates[i]) != INVALID_FILE_ATTRIBUTES) {
            found = i;
            break;
        }
    }

    if (found < 0) {
        fprintf(stderr, "找不到 EmeraldIDE.exe。\n\n");
        fprintf(stderr, "已尝试以下路径：\n");
        for (i = 0; i < 6; i++) {
            fprintf(stderr, "  %s\n", candidates[i]);
        }
        fprintf(stderr, "\n请确保 IDE 已经打包，或者把 EmeraldIDE.exe\n");
        fprintf(stderr, "放到 eml.exe 同目录下。\n");
        return 1;
    }

    /* 5) 用 ShellExecute 启动 IDE */
    {
        HINSTANCE h = ShellExecuteA(
            NULL,
            "open",
            candidates[found],
            NULL,
            NULL,
            SW_SHOWNORMAL
        );

        if ((INT_PTR)h <= 32) {
            fprintf(stderr, "启动 IDE 失败（ShellExecute 错误码 %d）\n",
                    (int)(INT_PTR)h);
            return 1;
        }
    }

    printf("已启动 Emerald IDE: %s\n", candidates[found]);
    return 0;
}
#else
int launch_ide(void) {
    fprintf(stderr, "eml ide 目前仅支持 Windows。\n");
    return 1;
}
#endif

/* ================= main ================= */
int main(int argc, char **argv) {
    get_self_path();

    /* 优先检查是否被打包过 —— 是的话直接执行嵌入的源码 */
    if (try_run_wrapped()) {
        return 0;
    }

    if (argc < 2) {
        printf("Emerald 语言解释器\n");
        printf("用法:\n");
        printf("  eml -c <file.eml>       编译为 .elr\n");
        printf("  eml run <file>          运行（不用写扩展名）\n");
        printf("  eml wrapper <file.eml>  打包为独立 exe\n");
        printf("  eml ide                 启动 Emerald IDE\n");
        printf("  eml --help              显示此帮助\n");
        return 1;
    }

    /* eml ide / eml -ide / eml --ide 都启动 IDE */
    if (strcmp(argv[1], "ide") == 0 ||
        strcmp(argv[1], "-ide") == 0 ||
        strcmp(argv[1], "--ide") == 0) {
        return launch_ide();
    }

    if (strcmp(argv[1], "-c") == 0) {
        if (argc < 3) { fprintf(stderr, "缺少文件\n"); return 1; }
        compile_file(argv[2]);
    } else if (strcmp(argv[1], "run") == 0) {
        if (argc < 3) { fprintf(stderr, "缺少文件\n"); return 1; }
        run_file(argv[2]);
    } else if (strcmp(argv[1], "wrapper") == 0) {
        if (argc < 3) { fprintf(stderr, "缺少文件\n"); return 1; }
        wrapper_file(argv[2]);
    } else if (strcmp(argv[1], "--help") == 0 ||
               strcmp(argv[1], "-h") == 0) {
        printf("Emerald 语言解释器\n");
        printf("用法:\n");
        printf("  eml -c <file.eml>       编译为 .elr\n");
        printf("  eml run <file>          运行（不用写扩展名）\n");
        printf("  eml wrapper <file.eml>  打包为独立 exe\n");
        printf("  eml ide                 启动 Emerald IDE\n");
        return 0;
    } else {
        run_file(argv[1]);
    }

    return 0;
}
