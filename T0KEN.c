#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define MAX_TOKENS 8192
#define MAX_SYMBOLS 2048
#define MAX_CODE 65536
#define HASH_SIZE 1024

typedef enum {
    TOK_EOF, TOK_INT, TOK_CHAR, TOK_IF, TOK_ELSE, TOK_WHILE, TOK_RETURN,
    TOK_IDENT, TOK_NUM, TOK_STRING, TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH,
    TOK_PERCENT, TOK_LT, TOK_GT, TOK_LE, TOK_GE, TOK_EQ, TOK_NE, TOK_ASSIGN,
    TOK_AMPERSAND, TOK_PIPE, TOK_CARET, TOK_LSHIFT, TOK_RSHIFT, TOK_LPAREN,
    TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_LBRACKET, TOK_RBRACKET, TOK_SEMICOLON,
    TOK_COMMA, TOK_AND, TOK_OR, TOK_NOT, TOK_TILDE
} TokenType;

typedef struct {
    TokenType type;
    char *text;
    int value;
    int line;
} Token;

typedef struct Symbol {
    char *name;
    int offset;
    int size;
    int is_param;
    struct Symbol *next;
} Symbol;

typedef struct {
    char *src;
    int pos;
    int line;
    Token tokens[MAX_TOKENS];
    int token_count;
    int current;
    Symbol *symbols[HASH_SIZE];
    int stack_offset;
    uint8_t code[MAX_CODE];
    int code_pos;
} Compiler;

static void error(Compiler *c, const char *msg) {
    fprintf(stderr, "Error line %d: %s\n", c->line, msg);
    exit(1);
}

static int hash(const char *str) {
    unsigned int h = 5381;
    int ch;
    while ((ch = *str++)) h = ((h << 5) + h) + ch;
    return h % HASH_SIZE;
}

static Symbol *find_symbol(Compiler *c, const char *name) {
    int h = hash(name);
    Symbol *s = c->symbols[h];
    while (s) {
        if (strcmp(s->name, name) == 0) return s;
        s = s->next;
    }
    return NULL;
}

static void add_symbol(Compiler *c, const char *name, int size, int is_param) {
    int h = hash(name);
    Symbol *s = malloc(sizeof(Symbol));
    s->name = strdup(name);
    s->size = size;
    s->is_param = is_param;
    s->offset = is_param ? 8 + c->stack_offset : -(c->stack_offset += size);
    s->next = c->symbols[h];
    c->symbols[h] = s;
}

static void emit(Compiler *c, uint8_t byte) {
    if (c->code_pos >= MAX_CODE) error(c, "Code buffer overflow");
    c->code[c->code_pos++] = byte;
}

static void emit32(Compiler *c, uint32_t val) {
    emit(c, val & 0xFF);
    emit(c, (val >> 8) & 0xFF);
    emit(c, (val >> 16) & 0xFF);
    emit(c, (val >> 24) & 0xFF);
}

static void lex(Compiler *c) {
    while (c->src[c->pos]) {
        char ch = c->src[c->pos];
        
        if (isspace(ch)) {
            if (ch == '\n') c->line++;
            c->pos++;
            continue;
        }
        
        if (ch == '/' && c->src[c->pos + 1] == '/') {
            while (c->src[c->pos] && c->src[c->pos] != '\n') c->pos++;
            continue;
        }
        
        Token *t = &c->tokens[c->token_count++];
        t->line = c->line;
        
        if (isalpha(ch) || ch == '_') {
            int start = c->pos;
            while (isalnum(c->src[c->pos]) || c->src[c->pos] == '_') c->pos++;
            int len = c->pos - start;
            t->text = strndup(&c->src[start], len);
            
            if (strcmp(t->text, "int") == 0) t->type = TOK_INT;
            else if (strcmp(t->text, "char") == 0) t->type = TOK_CHAR;
            else if (strcmp(t->text, "if") == 0) t->type = TOK_IF;
            else if (strcmp(t->text, "else") == 0) t->type = TOK_ELSE;
            else if (strcmp(t->text, "while") == 0) t->type = TOK_WHILE;
            else if (strcmp(t->text, "return") == 0) t->type = TOK_RETURN;
            else t->type = TOK_IDENT;
            continue;
        }
        
        if (isdigit(ch)) {
            t->type = TOK_NUM;
            t->value = 0;
            while (isdigit(c->src[c->pos])) {
                t->value = t->value * 10 + (c->src[c->pos++] - '0');
            }
            continue;
        }
        
        c->pos++;
        switch (ch) {
            case '+': t->type = TOK_PLUS; break;
            case '-': t->type = TOK_MINUS; break;
            case '*': t->type = TOK_STAR; break;
            case '/': t->type = TOK_SLASH; break;
            case '%': t->type = TOK_PERCENT; break;
            case '&': t->type = c->src[c->pos] == '&' ? (c->pos++, TOK_AND) : TOK_AMPERSAND; break;
            case '|': t->type = c->src[c->pos] == '|' ? (c->pos++, TOK_OR) : TOK_PIPE; break;
            case '^': t->type = TOK_CARET; break;
            case '~': t->type = TOK_TILDE; break;
            case '!': t->type = c->src[c->pos] == '=' ? (c->pos++, TOK_NE) : TOK_NOT; break;
            case '<': t->type = c->src[c->pos] == '=' ? (c->pos++, TOK_LE) : 
                                c->src[c->pos] == '<' ? (c->pos++, TOK_LSHIFT) : TOK_LT; break;
            case '>': t->type = c->src[c->pos] == '=' ? (c->pos++, TOK_GE) :
                                c->src[c->pos] == '>' ? (c->pos++, TOK_RSHIFT) : TOK_GT; break;
            case '=': t->type = c->src[c->pos] == '=' ? (c->pos++, TOK_EQ) : TOK_ASSIGN; break;
            case '(': t->type = TOK_LPAREN; break;
            case ')': t->type = TOK_RPAREN; break;
            case '{': t->type = TOK_LBRACE; break;
            case '}': t->type = TOK_RBRACE; break;
            case '[': t->type = TOK_LBRACKET; break;
            case ']': t->type = TOK_RBRACKET; break;
            case ';': t->type = TOK_SEMICOLON; break;
            case ',': t->type = TOK_COMMA; break;
            default: error(c, "Unknown character");
        }
    }
    c->tokens[c->token_count++].type = TOK_EOF;
}

static Token *peek(Compiler *c) {
    return &c->tokens[c->current];
}

static Token *consume(Compiler *c) {
    return &c->tokens[c->current++];
}

static int match(Compiler *c, TokenType type) {
    if (peek(c)->type == type) {
        consume(c);
        return 1;
    }
    return 0;
}

static void expect(Compiler *c, TokenType type) {
    if (!match(c, type)) error(c, "Unexpected token");
}

static void expr(Compiler *c);

static void primary(Compiler *c) {
    Token *t = peek(c);
    
    if (t->type == TOK_NUM) {
        consume(c);
        emit(c, 0x48); emit(c, 0xC7); emit(c, 0xC0);
        emit32(c, t->value);
        return;
    }
    
    if (t->type == TOK_IDENT) {
        consume(c);
        Symbol *s = find_symbol(c, t->text);
        if (!s) error(c, "Undefined variable");
        emit(c, 0x48); emit(c, 0x8B); emit(c, 0x45);
        emit(c, s->offset & 0xFF);
        return;
    }
    
    if (match(c, TOK_LPAREN)) {
        expr(c);
        expect(c, TOK_RPAREN);
        return;
    }
    
    error(c, "Expected expression");
}

static void unary(Compiler *c) {
    if (match(c, TOK_MINUS)) {
        unary(c);
        emit(c, 0x48); emit(c, 0xF7); emit(c, 0xD8);
        return;
    }
    if (match(c, TOK_TILDE)) {
        unary(c);
        emit(c, 0x48); emit(c, 0xF7); emit(c, 0xD0);
        return;
    }
    if (match(c, TOK_NOT)) {
        unary(c);
        emit(c, 0x48); emit(c, 0x83); emit(c, 0xF8); emit(c, 0x00);
        emit(c, 0x0F); emit(c, 0x94); emit(c, 0xC0);
        emit(c, 0x48); emit(c, 0x0F); emit(c, 0xB6); emit(c, 0xC0);
        return;
    }
    primary(c);
}

static void multiplicative(Compiler *c) {
    unary(c);
    while (1) {
        if (match(c, TOK_STAR)) {
            emit(c, 0x50);
            unary(c);
            emit(c, 0x48); emit(c, 0x89); emit(c, 0xC3);
            emit(c, 0x58);
            emit(c, 0x48); emit(c, 0x0F); emit(c, 0xAF); emit(c, 0xC3);
        } else if (match(c, TOK_SLASH)) {
            emit(c, 0x50);
            unary(c);
            emit(c, 0x48); emit(c, 0x89); emit(c, 0xC3);
            emit(c, 0x58);
            emit(c, 0x48); emit(c, 0x99);
            emit(c, 0x48); emit(c, 0xF7); emit(c, 0xFB);
        } else if (match(c, TOK_PERCENT)) {
            emit(c, 0x50);
            unary(c);
            emit(c, 0x48); emit(c, 0x89); emit(c, 0xC3);
            emit(c, 0x58);
            emit(c, 0x48); emit(c, 0x99);
            emit(c, 0x48); emit(c, 0xF7); emit(c, 0xFB);
            emit(c, 0x48); emit(c, 0x89); emit(c, 0xD0);
        } else break;
    }
}

static void additive(Compiler *c) {
    multiplicative(c);
    while (1) {
        if (match(c, TOK_PLUS)) {
            emit(c, 0x50);
            multiplicative(c);
            emit(c, 0x48); emit(c, 0x89); emit(c, 0xC3);
            emit(c, 0x58);
            emit(c, 0x48); emit(c, 0x01); emit(c, 0xD8);
        } else if (match(c, TOK_MINUS)) {
            emit(c, 0x50);
            multiplicative(c);
            emit(c, 0x48); emit(c, 0x89); emit(c, 0xC3);
            emit(c, 0x58);
            emit(c, 0x48); emit(c, 0x29); emit(c, 0xD8);
        } else break;
    }
}

static void shift(Compiler *c) {
    additive(c);
    while (1) {
        if (match(c, TOK_LSHIFT)) {
            emit(c, 0x50);
            additive(c);
            emit(c, 0x48); emit(c, 0x89); emit(c, 0xC1);
            emit(c, 0x58);
            emit(c, 0x48); emit(c, 0xD3); emit(c, 0xE0);
        } else if (match(c, TOK_RSHIFT)) {
            emit(c, 0x50);
            additive(c);
            emit(c, 0x48); emit(c, 0x89); emit(c, 0xC1);
            emit(c, 0x58);
            emit(c, 0x48); emit(c, 0xD3); emit(c, 0xE8);
        } else break;
    }
}

static void relational(Compiler *c) {
    shift(c);
    while (1) {
        TokenType op = peek(c)->type;
        if (op == TOK_LT || op == TOK_GT || op == TOK_LE || op == TOK_GE) {
            consume(c);
            emit(c, 0x50);
            shift(c);
            emit(c, 0x48); emit(c, 0x89); emit(c, 0xC3);
            emit(c, 0x58);
            emit(c, 0x48); emit(c, 0x39); emit(c, 0xD8);
            if (op == TOK_LT) { emit(c, 0x0F); emit(c, 0x9C); emit(c, 0xC0); }
            else if (op == TOK_GT) { emit(c, 0x0F); emit(c, 0x9F); emit(c, 0xC0); }
            else if (op == TOK_LE) { emit(c, 0x0F); emit(c, 0x9E); emit(c, 0xC0); }
            else if (op == TOK_GE) { emit(c, 0x0F); emit(c, 0x9D); emit(c, 0xC0); }
            emit(c, 0x48); emit(c, 0x0F); emit(c, 0xB6); emit(c, 0xC0);
        } else break;
    }
}

static void equality(Compiler *c) {
    relational(c);
    while (1) {
        if (match(c, TOK_EQ)) {
            emit(c, 0x50);
            relational(c);
            emit(c, 0x48); emit(c, 0x89); emit(c, 0xC3);
            emit(c, 0x58);
            emit(c, 0x48); emit(c, 0x39); emit(c, 0xD8);
            emit(c, 0x0F); emit(c, 0x94); emit(c, 0xC0);
            emit(c, 0x48); emit(c, 0x0F); emit(c, 0xB6); emit(c, 0xC0);
        } else if (match(c, TOK_NE)) {
            emit(c, 0x50);
            relational(c);
            emit(c, 0x48); emit(c, 0x89); emit(c, 0xC3);
            emit(c, 0x58);
            emit(c, 0x48); emit(c, 0x39); emit(c, 0xD8);
            emit(c, 0x0F); emit(c, 0x95); emit(c, 0xC0);
            emit(c, 0x48); emit(c, 0x0F); emit(c, 0xB6); emit(c, 0xC0);
        } else break;
    }
}

static void bitwise_and(Compiler *c) {
    equality(c);
    while (match(c, TOK_AMPERSAND)) {
        emit(c, 0x50);
        equality(c);
        emit(c, 0x48); emit(c, 0x89); emit(c, 0xC3);
        emit(c, 0x58);
        emit(c, 0x48); emit(c, 0x21); emit(c, 0xD8);
    }
}

static void bitwise_xor(Compiler *c) {
    bitwise_and(c);
    while (match(c, TOK_CARET)) {
        emit(c, 0x50);
        bitwise_and(c);
        emit(c, 0x48); emit(c, 0x89); emit(c, 0xC3);
        emit(c, 0x58);
        emit(c, 0x48); emit(c, 0x31); emit(c, 0xD8);
    }
}

static void bitwise_or(Compiler *c) {
    bitwise_xor(c);
    while (match(c, TOK_PIPE)) {
        emit(c, 0x50);
        bitwise_xor(c);
        emit(c, 0x48); emit(c, 0x89); emit(c, 0xC3);
        emit(c, 0x58);
        emit(c, 0x48); emit(c, 0x09); emit(c, 0xD8);
    }
}

static void logical_and(Compiler *c) {
    bitwise_or(c);
    while (match(c, TOK_AND)) {
        emit(c, 0x50);
        bitwise_or(c);
        emit(c, 0x48); emit(c, 0x89); emit(c, 0xC3);
        emit(c, 0x58);
        emit(c, 0x48); emit(c, 0x85); emit(c, 0xC0);
        emit(c, 0x0F); emit(c, 0x95); emit(c, 0xC0);
        emit(c, 0x48); emit(c, 0x85); emit(c, 0xDB);
        emit(c, 0x0F); emit(c, 0x95); emit(c, 0xC3);
        emit(c, 0x48); emit(c, 0x21); emit(c, 0xD8);
    }
}

static void logical_or(Compiler *c) {
    logical_and(c);
    while (match(c, TOK_OR)) {
        emit(c, 0x50);
        logical_and(c);
        emit(c, 0x48); emit(c, 0x89); emit(c, 0xC3);
        emit(c, 0x58);
        emit(c, 0x48); emit(c, 0x09); emit(c, 0xD8);
        emit(c, 0x48); emit(c, 0x85); emit(c, 0xC0);
        emit(c, 0x0F); emit(c, 0x95); emit(c, 0xC0);
        emit(c, 0x48); emit(c, 0x0F); emit(c, 0xB6); emit(c, 0xC0);
    }
}

static void expr(Compiler *c) {
    logical_or(c);
}

static void stmt(Compiler *c);

static void block(Compiler *c) {
    expect(c, TOK_LBRACE);
    while (!match(c, TOK_RBRACE)) {
        if (peek(c)->type == TOK_EOF) error(c, "Unexpected EOF");
        stmt(c);
    }
}

static void stmt(Compiler *c) {
    if (match(c, TOK_RETURN)) {
        expr(c);
        expect(c, TOK_SEMICOLON);
        emit(c, 0x48); emit(c, 0x89); emit(c, 0xEC);
        emit(c, 0x5D);
        emit(c, 0xC3);
        return;
    }
    
    if (match(c, TOK_IF)) {
        expect(c, TOK_LPAREN);
        expr(c);
        expect(c, TOK_RPAREN);
        
        emit(c, 0x48); emit(c, 0x83); emit(c, 0xF8); emit(c, 0x00);
        emit(c, 0x0F); emit(c, 0x84);
        int jz_pos = c->code_pos;
        emit32(c, 0);
        
        stmt(c);
        
        if (match(c, TOK_ELSE)) {
            emit(c, 0xE9);
            int jmp_pos = c->code_pos;
            emit32(c, 0);
            
            int else_pos = c->code_pos;
            *(uint32_t*)&c->code[jz_pos] = else_pos - (jz_pos + 4);
            
            stmt(c);
            
            int end_pos = c->code_pos;
            *(uint32_t*)&c->code[jmp_pos] = end_pos - (jmp_pos + 4);
        } else {
            int end_pos = c->code_pos;
            *(uint32_t*)&c->code[jz_pos] = end_pos - (jz_pos + 4);
        }
        return;
    }
    
    if (match(c, TOK_WHILE)) {
        int loop_start = c->code_pos;
        expect(c, TOK_LPAREN);
        expr(c);
        expect(c, TOK_RPAREN);
        
        emit(c, 0x48); emit(c, 0x83); emit(c, 0xF8); emit(c, 0x00);
        emit(c, 0x0F); emit(c, 0x84);
        int jz_pos = c->code_pos;
        emit32(c, 0);
        
        stmt(c);
        
        emit(c, 0xE9);
        emit32(c, loop_start - (c->code_pos + 4));
        
        int end_pos = c->code_pos;
        *(uint32_t*)&c->code[jz_pos] = end_pos - (jz_pos + 4);
        return;
    }
    
    if (peek(c)->type == TOK_INT || peek(c)->type == TOK_CHAR) {
        consume(c);
        Token *name = consume(c);
        if (name->type != TOK_IDENT) error(c, "Expected identifier");
        add_symbol(c, name->text, 8, 0);
        
        if (match(c, TOK_ASSIGN)) {
            expr(c);
            Symbol *s = find_symbol(c, name->text);
            emit(c, 0x48); emit(c, 0x89); emit(c, 0x45);
            emit(c, s->offset & 0xFF);
        }
        expect(c, TOK_SEMICOLON);
        return;
    }
    
    if (peek(c)->type == TOK_IDENT) {
        Token *name = consume(c);
        if (match(c, TOK_ASSIGN)) {
            expr(c);
            Symbol *s = find_symbol(c, name->text);
            if (!s) error(c, "Undefined variable");
            emit(c, 0x48); emit(c, 0x89); emit(c, 0x45);
            emit(c, s->offset & 0xFF);
            expect(c, TOK_SEMICOLON);
            return;
        }
    }
    
    if (match(c, TOK_LBRACE)) {
        while (!match(c, TOK_RBRACE)) {
            if (peek(c)->type == TOK_EOF) error(c, "Unexpected EOF");
            stmt(c);
        }
        return;
    }
    
    expr(c);
    expect(c, TOK_SEMICOLON);
}

static void compile_function(Compiler *c) {
    expect(c, TOK_INT);
    Token *name = consume(c);
    if (name->type != TOK_IDENT) error(c, "Expected function name");
    
    expect(c, TOK_LPAREN);
    expect(c, TOK_RPAREN);
    
    emit(c, 0x55);
    emit(c, 0x48); emit(c, 0x89); emit(c, 0xE5);
    
    block(c);
    
    emit(c, 0x48); emit(c, 0x31); emit(c, 0xC0);
    emit(c, 0x48); emit(c, 0x89); emit(c, 0xEC);
    emit(c, 0x5D);
    emit(c, 0xC3);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source.c>\n", argv[0]);
        return 1;
    }
    
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *src = malloc(size + 1);
    fread(src, 1, size, f);
    src[size] = 0;
    fclose(f);
    
    Compiler c = {0};
    c.src = src;
    c.line = 1;
    
    lex(&c);
    compile_function(&c);
    
    FILE *out = fopen("out.bin", "wb");
    fwrite(c.code, 1, c.code_pos, out);
    fclose(out);
    
    printf("Compiled %d bytes to out.bin\n", c.code_pos);
    return 0;
}
