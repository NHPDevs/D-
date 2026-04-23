#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_TOKENS     65536
#define MAX_SRC        1048576
#define MAX_IDENT      256
#define MAX_STR        1024
#define MAX_VARS       4096
#define MAX_FUNCS      1024
#define MAX_LABELS     65536
#define MAX_ASM        2097152
#define MAX_PARAMS     64
#define MAX_SCOPE      256

typedef enum {
  tk_eof, tk_num, tk_float, tk_str, tk_ident,
  tk_plus, tk_minus, tk_star, tk_slash, tk_percent,
  tk_plusplus, tk_minusminus, tk_pluseq, tk_minuseq,
  tk_stareq, tk_slasheq,
  tk_eq, tk_eqeq, tk_bang, tk_bangeq,
  tk_lt, tk_gt, tk_lteq, tk_gteq,
  tk_and, tk_or, tk_amp, tk_pipe, tk_caret,
  tk_lshift, tk_rshift,
  tk_lparen, tk_rparen, tk_lbrace, tk_rbrace,
  tk_lbracket, tk_rbracket,
  tk_semi, tk_comma, tk_dot, tk_colon,
  tk_arrow, tk_hash, tk_amp_addr,
  tk_kw_int, tk_kw_float, tk_kw_char, tk_kw_void,
  tk_kw_long, tk_kw_short, tk_kw_unsigned, tk_kw_signed,
  tk_kw_if, tk_kw_else, tk_kw_elif,
  tk_kw_while, tk_kw_for, tk_kw_do,
  tk_kw_return, tk_kw_break, tk_kw_continue,
  tk_kw_include, tk_kw_define, tk_kw_main,
  tk_kw_struct, tk_kw_const, tk_kw_static,
  tk_kw_switch, tk_kw_case, tk_kw_default,
  tk_kw_printel, tk_kw_scanel,
  tk_kw_true, tk_kw_false, tk_kw_null,
  tk_kw_new, tk_kw_delete,
  tk_kw_any,
  tk_percent_d, tk_percent_f, tk_percent_s, tk_percent_c,
  tk_backslash_n, tk_backslash_t,
  tk_nl
} toktype;

typedef struct {
  toktype   type;
  char      sval[MAX_STR];
  long long ival;
  double    fval;
  int       line;
  int       col;
} token;

typedef struct {
  char name[MAX_IDENT];
  int  offset;
  int  is_global;
  int  is_ptr;
  int  arr_size;
  int  type_size;
  char type[32];
} var_entry;

typedef struct {
  char name[MAX_IDENT];
  int  label_id;
  int  param_count;
  char params[MAX_PARAMS][MAX_IDENT];
  char param_types[MAX_PARAMS][32];
} func_entry;

static char      src[MAX_SRC];
static token     toks[MAX_TOKENS];
static int       ntoks = 0;
static int       tpos  = 0;

static var_entry  vars[MAX_VARS];
static int        nvar = 0;
static func_entry funcs[MAX_FUNCS];
static int        nfunc = 0;

static char asm_buf[MAX_ASM];
static int  asm_pos = 0;

static int  label_counter = 0;
static int  str_counter   = 0;

static char str_literals[4096][MAX_STR];
static int  str_lit_count = 0;

static int  cur_stack_offset = 0;
static int  scope_depth      = 0;
static int  scope_var_start[MAX_SCOPE];

static int  loop_break_label[MAX_SCOPE];
static int  loop_cont_label[MAX_SCOPE];
static int  loop_depth = 0;

static int  in_function = 0;
static int  cur_func_idx = -1;

static int  has_float_ops = 0;

static void emit(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  asm_pos += vsnprintf(asm_buf + asm_pos, MAX_ASM - asm_pos, fmt, ap);
  va_end(ap);
}

static int new_label(void) {
  return label_counter++;
}

static int add_string_literal(const char *s) {
  int id = str_lit_count++;
  strncpy(str_literals[id], s, MAX_STR - 1);
  return id;
}

static var_entry *find_var(const char *name) {
  for (int i = nvar - 1; i >= 0; i--) {
    if (strcmp(vars[i].name, name) == 0) return &vars[i];
  }
  return NULL;
}

static func_entry *find_func(const char *name) {
  for (int i = 0; i < nfunc; i++) {
    if (strcmp(funcs[i].name, name) == 0) return &funcs[i];
  }
  return NULL;
}

static token *cur(void)      { return &toks[tpos]; }
static token *peek(int n)    { int p = tpos+n; if(p>=ntoks) return &toks[ntoks-1]; return &toks[p]; }
static token *advance(void)  { token *t = &toks[tpos]; if(tpos<ntoks-1) tpos++; return t; }
static int    check(toktype t){ return cur()->type == t; }

static token *expect(toktype t, const char *msg) {
  if (!check(t)) {
    fprintf(stderr, "[dpp error] line %d:%d => expected %s but got '%s'\n",
            cur()->line, cur()->col, msg, cur()->sval);
    exit(1);
  }
  return advance();
}

static int match(toktype t) {
  if (check(t)) { advance(); return 1; }
  return 0;
}

static void skip_semi(void) {
  while (check(tk_semi) || check(tk_nl)) advance();
}

static void lex_error(int line, int col, const char *msg) {
  fprintf(stderr, "[dpp lexer] line %d:%d => %s\n", line, col, msg);
  exit(1);
}

static void tokenize(void) {
  int i   = 0;
  int len = strlen(src);
  int line = 1, col = 1;

  while (i < len) {
    while (i < len && (src[i]==' '||src[i]=='\t'||src[i]=='\r')) { i++; col++; }
    if (i >= len) break;

    if (src[i] == '\n') {
      i++; line++; col = 1;
      continue;
    }

    if (src[i]=='/' && src[i+1]=='/') {
      while (i<len && src[i]!='\n') i++;
      continue;
    }
    if (src[i]=='/' && src[i+1]=='*') {
      i+=2;
      while (i<len-1 && !(src[i]=='*'&&src[i+1]=='/')) {
        if(src[i]=='\n'){line++;col=1;} i++;
      }
      i+=2; continue;
    }

    token *t = &toks[ntoks++];
    t->line = line; t->col = col;

    if (src[i]=='"' || src[i]=='\'') {
      char q = src[i++]; col++;
      int j = 0;
      while (i<len && src[i]!=q) {
        if (src[i]=='\\') {
          i++; col++;
          switch(src[i]) {
            case 'n': t->sval[j++]='\n'; break;
            case 't': t->sval[j++]='\t'; break;
            case '\\': t->sval[j++]='\\'; break;
            case '"':  t->sval[j++]='"';  break;
            case '\'': t->sval[j++]='\''; break;
            case '0':  t->sval[j++]='\0'; break;
            default:   t->sval[j++]=src[i]; break;
          }
        } else {
          if(src[i]=='\n'){line++;col=1;}
          t->sval[j++]=src[i];
        }
        i++; col++;
      }
      t->sval[j]=0; i++; col++;
      t->type = tk_str;
      continue;
    }

    if (isdigit(src[i]) || (src[i]=='.'&&isdigit(src[i+1]))) {
      char numbuf[64]; int j=0;
      int is_float=0, is_hex=0, is_bin=0;
      if (src[i]=='0'&&(src[i+1]=='x'||src[i+1]=='X')) {
        numbuf[j++]=src[i++]; numbuf[j++]=src[i++]; is_hex=1;
        while(i<len&&isxdigit(src[i])) numbuf[j++]=src[i++];
      } else if (src[i]=='0'&&(src[i+1]=='b'||src[i+1]=='B')) {
        i+=2; is_bin=1;
        while(i<len&&(src[i]=='0'||src[i]=='1')) numbuf[j++]=src[i++];
      } else {
        while(i<len&&isdigit(src[i])) numbuf[j++]=src[i++];
        if(i<len&&src[i]=='.'&&isdigit(src[i+1])){is_float=1;numbuf[j++]=src[i++];while(i<len&&isdigit(src[i]))numbuf[j++]=src[i++];}
        if(i<len&&(src[i]=='e'||src[i]=='E')){is_float=1;numbuf[j++]=src[i++];if(i<len&&(src[i]=='+'||src[i]=='-'))numbuf[j++]=src[i++];while(i<len&&isdigit(src[i]))numbuf[j++]=src[i++];}
      }
      numbuf[j]=0; col+=j;
      if (is_float) { t->type=tk_float; t->fval=atof(numbuf); sprintf(t->sval,"%s",numbuf); }
      else if (is_hex) { t->type=tk_num; t->ival=strtoll(numbuf,NULL,16); sprintf(t->sval,"%lld",t->ival); }
      else if (is_bin) { t->type=tk_num; t->ival=strtoll(numbuf,NULL,2); sprintf(t->sval,"%lld",t->ival); }
      else { t->type=tk_num; t->ival=atoll(numbuf); sprintf(t->sval,"%s",numbuf); }
      continue;
    }

    if (isalpha(src[i])||src[i]=='_') {
      int j=0;
      while(i<len&&(isalnum(src[i])||src[i]=='_')) { t->sval[j++]=src[i++]; col++; }
      t->sval[j]=0;
      if      (!strcmp(t->sval,"int"))       t->type=tk_kw_int;
      else if (!strcmp(t->sval,"float"))     t->type=tk_kw_float;
      else if (!strcmp(t->sval,"char"))      t->type=tk_kw_char;
      else if (!strcmp(t->sval,"void"))      t->type=tk_kw_void;
      else if (!strcmp(t->sval,"long"))      t->type=tk_kw_long;
      else if (!strcmp(t->sval,"short"))     t->type=tk_kw_short;
      else if (!strcmp(t->sval,"unsigned"))  t->type=tk_kw_unsigned;
      else if (!strcmp(t->sval,"signed"))    t->type=tk_kw_signed;
      else if (!strcmp(t->sval,"if"))        t->type=tk_kw_if;
      else if (!strcmp(t->sval,"else"))      t->type=tk_kw_else;
      else if (!strcmp(t->sval,"elif"))      t->type=tk_kw_elif;
      else if (!strcmp(t->sval,"while"))     t->type=tk_kw_while;
      else if (!strcmp(t->sval,"for"))       t->type=tk_kw_for;
      else if (!strcmp(t->sval,"do"))        t->type=tk_kw_do;
      else if (!strcmp(t->sval,"return"))    t->type=tk_kw_return;
      else if (!strcmp(t->sval,"break"))     t->type=tk_kw_break;
      else if (!strcmp(t->sval,"continue"))  t->type=tk_kw_continue;
      else if (!strcmp(t->sval,"struct"))    t->type=tk_kw_struct;
      else if (!strcmp(t->sval,"const"))     t->type=tk_kw_const;
      else if (!strcmp(t->sval,"static"))    t->type=tk_kw_static;
      else if (!strcmp(t->sval,"switch"))    t->type=tk_kw_switch;
      else if (!strcmp(t->sval,"case"))      t->type=tk_kw_case;
      else if (!strcmp(t->sval,"default"))   t->type=tk_kw_default;
      else if (!strcmp(t->sval,"main"))      t->type=tk_kw_main;
      else if (!strcmp(t->sval,"printel"))   t->type=tk_kw_printel;
      else if (!strcmp(t->sval,"scanel"))    t->type=tk_kw_scanel;
      else if (!strcmp(t->sval,"true"))      t->type=tk_kw_true;
      else if (!strcmp(t->sval,"false"))     t->type=tk_kw_false;
      else if (!strcmp(t->sval,"null"))      t->type=tk_kw_null;
      else if (!strcmp(t->sval,"new"))       t->type=tk_kw_new;
      else if (!strcmp(t->sval,"delete"))    t->type=tk_kw_delete;
      else if (!strcmp(t->sval,"any"))       t->type=tk_kw_any;
      else if (!strcmp(t->sval,"include"))   t->type=tk_kw_include;
      else if (!strcmp(t->sval,"define"))    t->type=tk_kw_define;
      else                                   t->type=tk_ident;
      continue;
    }

    col++;
    switch(src[i]) {
      case '#': t->type=tk_hash; strcpy(t->sval,"#"); i++; break;
      case ';': t->type=tk_semi; strcpy(t->sval,";"); i++; break;
      case ',': t->type=tk_comma; strcpy(t->sval,","); i++; break;
      case '.': t->type=tk_dot; strcpy(t->sval,"."); i++; break;
      case ':': t->type=tk_colon; strcpy(t->sval,":"); i++; break;
      case '{': t->type=tk_lbrace; strcpy(t->sval,"{"); i++; break;
      case '}': t->type=tk_rbrace; strcpy(t->sval,"}"); i++; break;
      case '(': t->type=tk_lparen; strcpy(t->sval,"("); i++; break;
      case ')': t->type=tk_rparen; strcpy(t->sval,")"); i++; break;
      case '[': t->type=tk_lbracket; strcpy(t->sval,"["); i++; break;
      case ']': t->type=tk_rbracket; strcpy(t->sval,"]"); i++; break;
      case '+':
        if(src[i+1]=='+'){t->type=tk_plusplus;strcpy(t->sval,"++");i+=2;}
        else if(src[i+1]=='='){t->type=tk_pluseq;strcpy(t->sval,"+=");i+=2;}
        else{t->type=tk_plus;strcpy(t->sval,"+");i++;}
        break;
      case '-':
        if(src[i+1]=='-'){t->type=tk_minusminus;strcpy(t->sval,"--");i+=2;}
        else if(src[i+1]=='='){t->type=tk_minuseq;strcpy(t->sval,"-=");i+=2;}
        else if(src[i+1]=='>'){t->type=tk_arrow;strcpy(t->sval,"->");i+=2;}
        else{t->type=tk_minus;strcpy(t->sval,"-");i++;}
        break;
      case '*':
        if(src[i+1]=='='){t->type=tk_stareq;strcpy(t->sval,"*=");i+=2;}
        else{t->type=tk_star;strcpy(t->sval,"*");i++;}
        break;
      case '/':
        if(src[i+1]=='='){t->type=tk_slasheq;strcpy(t->sval,"/=");i+=2;}
        else{t->type=tk_slash;strcpy(t->sval,"/");i++;}
        break;
      case '%':
        if(src[i+1]=='d'){t->type=tk_percent_d;strcpy(t->sval,"%d");i+=2;}
        else if(src[i+1]=='f'){t->type=tk_percent_f;strcpy(t->sval,"%f");i+=2;}
        else if(src[i+1]=='s'){t->type=tk_percent_s;strcpy(t->sval,"%s");i+=2;}
        else if(src[i+1]=='c'){t->type=tk_percent_c;strcpy(t->sval,"%c");i+=2;}
        else{t->type=tk_percent;strcpy(t->sval,"%");i++;}
        break;
      case '=':
        if(src[i+1]=='='){t->type=tk_eqeq;strcpy(t->sval,"==");i+=2;}
        else{t->type=tk_eq;strcpy(t->sval,"=");i++;}
        break;
      case '!':
        if(src[i+1]=='='){t->type=tk_bangeq;strcpy(t->sval,"!=");i+=2;}
        else{t->type=tk_bang;strcpy(t->sval,"!");i++;}
        break;
      case '<':
        if(src[i+1]=='='){t->type=tk_lteq;strcpy(t->sval,"<=");i+=2;}
        else if(src[i+1]=='<'){t->type=tk_lshift;strcpy(t->sval,"<<");i+=2;}
        else{t->type=tk_lt;strcpy(t->sval,"<");i++;}
        break;
      case '>':
        if(src[i+1]=='='){t->type=tk_gteq;strcpy(t->sval,">=");i+=2;}
        else if(src[i+1]=='>'){t->type=tk_rshift;strcpy(t->sval,">>");i+=2;}
        else{t->type=tk_gt;strcpy(t->sval,">");i++;}
        break;
      case '&':
        if(src[i+1]=='&'){t->type=tk_and;strcpy(t->sval,"&&");i+=2;}
        else{t->type=tk_amp;strcpy(t->sval,"&");i++;}
        break;
      case '|':
        if(src[i+1]=='|'){t->type=tk_or;strcpy(t->sval,"||");i+=2;}
        else{t->type=tk_pipe;strcpy(t->sval,"|");i++;}
        break;
      case '^': t->type=tk_caret;strcpy(t->sval,"^");i++; break;
      default:
        i++; ntoks--;
        break;
    }
  }
  token *eof = &toks[ntoks++];
  eof->type = tk_eof;
  strcpy(eof->sval, "EOF");
  eof->line = line; eof->col = col;
}

static void parse_includes(void) {
  while (check(tk_hash)) {
    advance();
    if (check(tk_kw_include)) {
      advance();
      while (!check(tk_nl) && !check(tk_eof) && !check(tk_semi)) advance();
    } else if (check(tk_kw_define)) {
      advance();
      while (!check(tk_nl) && !check(tk_eof) && !check(tk_semi)) advance();
    }
    skip_semi();
  }
}

static void compile_expr(void);
static void compile_stmt(void);
static void compile_block(void);

static void compile_primary(void) {
  token *t = cur();

  if (t->type == tk_num) {
    advance();
    emit("    mov rax, %lld\n", t->ival);
    emit("    push rax\n");
    return;
  }

  if (t->type == tk_float) {
    advance();
    has_float_ops = 1;
    union { double d; unsigned long long u; } cvt;
    cvt.d = t->fval;
    emit("    mov rax, 0x%llxULL\n", (unsigned long long)cvt.u);
    emit("    push rax\n");
    return;
  }

  if (t->type == tk_str) {
    advance();
    int id = add_string_literal(t->sval);
    emit("    lea rax, [rel _str%d]\n", id);
    emit("    push rax\n");
    return;
  }

  if (t->type == tk_kw_true) {
    advance();
    emit("    push 1\n");
    return;
  }

  if (t->type == tk_kw_false || t->type == tk_kw_null) {
    advance();
    emit("    push 0\n");
    return;
  }

  if (t->type == tk_amp) {
    advance();
    token *id = expect(tk_ident, "identifier after &");
    var_entry *v = find_var(id->sval);
    if (!v) {
      fprintf(stderr, "[dpp error] line %d => unknown variable '%s'\n", id->line, id->sval);
      exit(1);
    }
    if (v->is_global) {
      emit("    lea rax, [rel _%s]\n", v->name);
    } else {
      emit("    lea rax, [rbp - %d]\n", v->offset);
    }
    emit("    push rax\n");
    return;
  }

  if (t->type == tk_bang) {
    advance();
    compile_primary();
    emit("    pop rax\n");
    emit("    test rax, rax\n");
    emit("    sete al\n");
    emit("    movzx rax, al\n");
    emit("    push rax\n");
    return;
  }

  if (t->type == tk_minus) {
    advance();
    compile_primary();
    emit("    pop rax\n");
    emit("    neg rax\n");
    emit("    push rax\n");
    return;
  }

  if (t->type == tk_plusplus || t->type == tk_minusminus) {
    int is_inc = t->type == tk_plusplus;
    advance();
    token *id = expect(tk_ident, "identifier");
    var_entry *v = find_var(id->sval);
    if (!v) { fprintf(stderr,"[dpp error] unknown var '%s'\n",id->sval); exit(1); }
    if (v->is_global) {
      emit("    mov rax, [rel _%s]\n", v->name);
      if(is_inc) emit("    inc rax\n"); else emit("    dec rax\n");
      emit("    mov [rel _%s], rax\n", v->name);
    } else {
      emit("    mov rax, [rbp - %d]\n", v->offset);
      if(is_inc) emit("    inc rax\n"); else emit("    dec rax\n");
      emit("    mov [rbp - %d], rax\n", v->offset);
    }
    emit("    push rax\n");
    return;
  }

  if (t->type == tk_lparen) {
    advance();
    compile_expr();
    expect(tk_rparen, ")");
    return;
  }

  if (t->type == tk_ident || t->type == tk_kw_main) {
    advance();
    if (check(tk_lparen)) {
      advance();
      int argc = 0;
      while (!check(tk_rparen) && !check(tk_eof)) {
        compile_expr();
        argc++;
        if (!match(tk_comma)) break;
      }
      expect(tk_rparen, ")");
      static const char *arg_regs[] = {"rdi","rsi","rdx","rcx","r8","r9"};
      for (int i = argc-1; i >= 0; i--) {
        if (i < 6) emit("    pop %s\n", arg_regs[i]);
        else emit("    ");
      }
      emit("    xor eax, eax\n");
      emit("    call _%s\n", t->sval);
      emit("    push rax\n");
    } else {
      var_entry *v = find_var(t->sval);
      if (v) {
        if (v->is_global) {
          emit("    mov rax, [rel _%s]\n", v->name);
        } else {
          emit("    mov rax, [rbp - %d]\n", v->offset);
        }
        emit("    push rax\n");
        if (check(tk_plusplus)) {
          advance();
          if (v->is_global) {
            emit("    mov rcx, [rel _%s]\n", v->name);
            emit("    inc rcx\n");
            emit("    mov [rel _%s], rcx\n", v->name);
          } else {
            emit("    mov rcx, [rbp - %d]\n", v->offset);
            emit("    inc rcx\n");
            emit("    mov [rbp - %d], rcx\n", v->offset);
          }
        } else if (check(tk_minusminus)) {
          advance();
          if (v->is_global) {
            emit("    mov rcx, [rel _%s]\n", v->name);
            emit("    dec rcx\n");
            emit("    mov [rel _%s], rcx\n", v->name);
          } else {
            emit("    mov rcx, [rbp - %d]\n", v->offset);
            emit("    dec rcx\n");
            emit("    mov [rbp - %d], rcx\n", v->offset);
          }
        }
      } else {
        emit("    xor rax, rax\n");
        emit("    push rax\n");
      }
    }
    return;
  }

  advance();
  emit("    push 0\n");
}

static void compile_mul_div(void) {
  compile_primary();
  while (check(tk_star)||check(tk_slash)||check(tk_percent)) {
    toktype op = cur()->type;
    advance();
    compile_primary();
    emit("    pop rcx\n");
    emit("    pop rax\n");
    if (op == tk_star) {
      emit("    imul rax, rcx\n");
    } else if (op == tk_slash) {
      emit("    cqo\n");
      emit("    idiv rcx\n");
    } else {
      emit("    cqo\n");
      emit("    idiv rcx\n");
      emit("    mov rax, rdx\n");
    }
    emit("    push rax\n");
  }
}

static void compile_add_sub(void) {
  compile_mul_div();
  while (check(tk_plus)||check(tk_minus)) {
    toktype op = cur()->type;
    advance();
    compile_mul_div();
    emit("    pop rcx\n");
    emit("    pop rax\n");
    if (op == tk_plus) emit("    add rax, rcx\n");
    else               emit("    sub rax, rcx\n");
    emit("    push rax\n");
  }
}

static void compile_shift(void) {
  compile_add_sub();
  while (check(tk_lshift)||check(tk_rshift)) {
    toktype op = cur()->type;
    advance();
    compile_add_sub();
    emit("    pop rcx\n");
    emit("    pop rax\n");
    if (op == tk_lshift) emit("    shl rax, cl\n");
    else                 emit("    sar rax, cl\n");
    emit("    push rax\n");
  }
}

static void compile_cmp(void) {
  compile_shift();
  while (check(tk_lt)||check(tk_gt)||check(tk_lteq)||check(tk_gteq)) {
    toktype op = cur()->type;
    advance();
    compile_shift();
    emit("    pop rcx\n");
    emit("    pop rax\n");
    emit("    cmp rax, rcx\n");
    if      (op==tk_lt)  emit("    setl al\n");
    else if (op==tk_gt)  emit("    setg al\n");
    else if (op==tk_lteq)emit("    setle al\n");
    else                 emit("    setge al\n");
    emit("    movzx rax, al\n");
    emit("    push rax\n");
  }
}

static void compile_eq_neq(void) {
  compile_cmp();
  while (check(tk_eqeq)||check(tk_bangeq)) {
    toktype op = cur()->type;
    advance();
    compile_cmp();
    emit("    pop rcx\n");
    emit("    pop rax\n");
    emit("    cmp rax, rcx\n");
    if (op==tk_eqeq) emit("    sete al\n");
    else             emit("    setne al\n");
    emit("    movzx rax, al\n");
    emit("    push rax\n");
  }
}

static void compile_bitand(void) {
  compile_eq_neq();
  while (check(tk_amp)) {
    advance(); compile_eq_neq();
    emit("    pop rcx\n"); emit("    pop rax\n");
    emit("    and rax, rcx\n"); emit("    push rax\n");
  }
}

static void compile_bitxor(void) {
  compile_bitand();
  while (check(tk_caret)) {
    advance(); compile_bitand();
    emit("    pop rcx\n"); emit("    pop rax\n");
    emit("    xor rax, rcx\n"); emit("    push rax\n");
  }
}

static void compile_bitor(void) {
  compile_bitxor();
  while (check(tk_pipe)) {
    advance(); compile_bitxor();
    emit("    pop rcx\n"); emit("    pop rax\n");
    emit("    or rax, rcx\n"); emit("    push rax\n");
  }
}

static void compile_logand(void) {
  compile_bitor();
  while (check(tk_and)) {
    advance(); compile_bitor();
    emit("    pop rcx\n"); emit("    pop rax\n");
    emit("    test rax, rax\n"); emit("    setne al\n");
    emit("    test rcx, rcx\n"); emit("    setne cl\n");
    emit("    and al, cl\n");
    emit("    movzx rax, al\n"); emit("    push rax\n");
  }
}

static void compile_logor(void) {
  compile_logand();
  while (check(tk_or)) {
    advance(); compile_logand();
    emit("    pop rcx\n"); emit("    pop rax\n");
    emit("    or rax, rcx\n");
    emit("    setne al\n");
    emit("    movzx rax, al\n"); emit("    push rax\n");
  }
}

static void compile_expr(void) {
  compile_logor();

  if (check(tk_eq)||check(tk_pluseq)||check(tk_minuseq)||check(tk_stareq)||check(tk_slasheq)) {
    fprintf(stderr, "[dpp warning] assignment inside expression\n");
  }
}

static int parse_type(void) {
  int sz = 8;
  if (check(tk_kw_int)||check(tk_kw_long)||check(tk_kw_unsigned)||check(tk_kw_signed)||check(tk_kw_short)) {
    if (check(tk_kw_short)) sz=2; else sz=8;
    advance();
    if (check(tk_kw_int)) advance();
  } else if (check(tk_kw_char)) {
    sz=1; advance();
  } else if (check(tk_kw_float)) {
    sz=8; advance();
  } else if (check(tk_kw_void)) {
    sz=0; advance();
  } else if (check(tk_kw_any)) {
    sz=8; advance();
  }
  while (check(tk_star)) { advance(); sz=8; }
  return sz;
}

static void compile_printel(void) {
  advance();
  expect(tk_lparen, "(");

  if (!check(tk_str)) {
    if (!check(tk_rparen)) compile_expr();
    expect(tk_rparen, ")");
    skip_semi();
    return;
  }

  token *fmt_tok = cur(); advance();

  char fmt[MAX_STR*2];
  char nasm_fmt[MAX_STR*2];
  strcpy(fmt, fmt_tok->sval);

  int args[64];
  int nargs = 0;
  int fmt_args_start = str_lit_count;

  while (check(tk_comma)) {
    advance();
    compile_expr();
    args[nargs++] = 1;
  }
  expect(tk_rparen, ")");
  skip_semi();

  char *p = fmt;
  char *q = nasm_fmt;
  while (*p) {
    if (*p == '\n') { *q++ = '\\'; *q++ = 'n'; p++; }
    else if (*p == '\t') { *q++ = '\\'; *q++ = 't'; p++; }
    else { *q++ = *p++; }
  }
  *q = 0;

  int fmt_id = add_string_literal(fmt_tok->sval);

  static const char *arg_regs[] = {"rsi","rdx","rcx","r8","r9"};
  for (int i = nargs-1; i >= 0; i--) {
    if (i < 5) emit("    pop %s\n", arg_regs[i]);
    else { emit("    pop rax\n"); }
  }
  emit("    lea rdi, [rel _str%d]\n", fmt_id);
  emit("    xor eax, eax\n");
  emit("    call _printf\n");
}

static void compile_scanel(void) {
  advance();
  expect(tk_lparen, "(");

  token *fmt_tok = NULL;
  if (check(tk_str)) { fmt_tok = cur(); advance(); }

  int fmt_id = -1;
  if (fmt_tok) fmt_id = add_string_literal(fmt_tok->sval);

  static const char *arg_regs[] = {"rsi","rdx","rcx","r8","r9"};
  int nargs = 0;
  while (check(tk_comma)) {
    advance();
    if (check(tk_amp)) {
      advance();
      token *id = expect(tk_ident, "variable name");
      var_entry *v = find_var(id->sval);
      if (v) {
        if (v->is_global) emit("    lea %s, [rel _%s]\n", arg_regs[nargs], v->name);
        else              emit("    lea %s, [rbp - %d]\n", arg_regs[nargs], v->offset);
      } else {
        emit("    xor %s, %s\n", arg_regs[nargs], arg_regs[nargs]);
      }
      nargs++;
    } else {
      compile_expr();
      if (nargs < 5) emit("    pop %s\n", arg_regs[nargs]);
      nargs++;
    }
  }
  expect(tk_rparen, ")");
  skip_semi();

  if (fmt_id >= 0) emit("    lea rdi, [rel _str%d]\n", fmt_id);
  emit("    xor eax, eax\n");
  emit("    call _scanf\n");
}

static void compile_var_decl(void) {
  int sz = parse_type();
  if (sz == 0) sz = 8;

  int is_arr = 0;
  int arr_sz = 0;

  token *name_tok = expect(tk_ident, "variable name");
  char vname[MAX_IDENT];
  strcpy(vname, name_tok->sval);

  if (check(tk_lbracket)) {
    advance();
    is_arr = 1;
    if (check(tk_num)) { arr_sz = (int)cur()->ival; advance(); }
    else arr_sz = 64;
    expect(tk_rbracket, "]");
  }

  var_entry *existing = find_var(vname);
  if (!existing) {
    var_entry *v = &vars[nvar++];
    strcpy(v->name, vname);
    v->is_global = !in_function;
    v->arr_size  = is_arr ? arr_sz : 0;
    v->type_size = sz;
    if (!in_function) {
      v->offset = 0;
    } else {
      int total = is_arr ? arr_sz * sz : sz;
      if (total < 8) total = 8;
      total = (total + 7) & ~7;
      cur_stack_offset += total;
      v->offset = cur_stack_offset;
      emit("    sub rsp, %d\n", total);
    }
  }

  var_entry *v = find_var(vname);

  if (check(tk_eq)) {
    advance();
    compile_expr();
    emit("    pop rax\n");
    if (v) {
      if (v->is_global) emit("    mov [rel _%s], rax\n", v->name);
      else              emit("    mov [rbp - %d], rax\n", v->offset);
    }
  }

  skip_semi();
}

static void compile_assign(const char *vname) {
  toktype op = cur()->type;
  advance();
  compile_expr();
  emit("    pop rax\n");
  var_entry *v = find_var(vname);
  if (!v) { fprintf(stderr,"[dpp error] unknown var '%s'\n", vname); exit(1); }

  if (op != tk_eq) {
    if (v->is_global) emit("    mov rcx, [rel _%s]\n", v->name);
    else              emit("    mov rcx, [rbp - %d]\n", v->offset);
    if      (op==tk_pluseq)  emit("    add rcx, rax\n");
    else if (op==tk_minuseq) emit("    sub rcx, rax\n");
    else if (op==tk_stareq)  emit("    imul rcx, rax\n");
    else if (op==tk_slasheq) {
      emit("    xchg rax, rcx\n");
      emit("    cqo\n");
      emit("    idiv rcx\n");
      emit("    mov rcx, rax\n");
    }
    emit("    mov rax, rcx\n");
  }

  if (v->is_global) emit("    mov [rel _%s], rax\n", v->name);
  else              emit("    mov [rbp - %d], rax\n", v->offset);

  skip_semi();
}

static void compile_if(void) {
  advance();
  int lbl_else = new_label();
  int lbl_end  = new_label();

  expect(tk_lparen, "(");
  compile_expr();
  expect(tk_rparen, ")");

  emit("    pop rax\n");
  emit("    test rax, rax\n");
  emit("    jz .L%d\n", lbl_else);

  compile_block();

  int has_else = 0;
  if (check(tk_kw_elif)) {
    emit("    jmp .L%d\n", lbl_end);
    emit(".L%d:\n", lbl_else);
    while (check(tk_kw_elif)) {
      advance();
      int lbl_next = new_label();
      expect(tk_lparen, "(");
      compile_expr();
      expect(tk_rparen, ")");
      emit("    pop rax\n");
      emit("    test rax, rax\n");
      emit("    jz .L%d\n", lbl_next);
      compile_block();
      emit("    jmp .L%d\n", lbl_end);
      emit(".L%d:\n", lbl_next);
    }
    has_else = 1;
  }
  if (check(tk_kw_else)) {
    if (!has_else) {
      emit("    jmp .L%d\n", lbl_end);
      emit(".L%d:\n", lbl_else);
    }
    advance();
    compile_block();
    emit(".L%d:\n", lbl_end);
  } else {
    if (!has_else) emit(".L%d:\n", lbl_else);
    emit(".L%d:\n", lbl_end);
  }
}

static void compile_while(void) {
  advance();
  int lbl_start = new_label();
  int lbl_end   = new_label();

  loop_break_label[loop_depth] = lbl_end;
  loop_cont_label[loop_depth]  = lbl_start;
  loop_depth++;

  emit(".L%d:\n", lbl_start);
  expect(tk_lparen, "(");
  compile_expr();
  expect(tk_rparen, ")");
  emit("    pop rax\n");
  emit("    test rax, rax\n");
  emit("    jz .L%d\n", lbl_end);

  compile_block();

  emit("    jmp .L%d\n", lbl_start);
  emit(".L%d:\n", lbl_end);
  loop_depth--;
}

static void compile_for(void) {
  advance();
  expect(tk_lparen, "(");

  int lbl_cond  = new_label();
  int lbl_body  = new_label();
  int lbl_incr  = new_label();
  int lbl_end   = new_label();

  loop_break_label[loop_depth] = lbl_end;
  loop_cont_label[loop_depth]  = lbl_incr;
  loop_depth++;

  int saved_tpos = tpos;
  int semi_count = 0;
  int depth = 1;
  int incr_start = -1;
  int i2 = tpos;
  while (i2 < ntoks && semi_count < 2) {
    if (toks[i2].type == tk_semi) semi_count++;
    i2++;
  }
  incr_start = i2;

  if (!check(tk_semi)) {
    if (check(tk_kw_int)||check(tk_kw_float)||check(tk_kw_char)||check(tk_kw_void)||check(tk_kw_long)||check(tk_kw_any)) {
      compile_var_decl();
    } else {
      compile_expr();
      emit("    pop rax\n");
      skip_semi();
    }
  } else { advance(); }

  emit(".L%d:\n", lbl_cond);
  if (!check(tk_semi)) {
    compile_expr();
    emit("    pop rax\n");
    emit("    test rax, rax\n");
    emit("    jz .L%d\n", lbl_end);
    skip_semi();
  } else { advance(); }

  int saved2 = tpos;
  while (!check(tk_rparen) && !check(tk_eof)) { advance(); }
  int incr_end = tpos;
  expect(tk_rparen, ")");

  emit("    jmp .L%d\n", lbl_body);
  emit(".L%d:\n", lbl_incr);

  int after_body = tpos;
  tpos = saved2;
  while (tpos < incr_end) {
    compile_expr();
    emit("    pop rax\n");
    if (check(tk_comma)) advance();
  }
  tpos = incr_end;
  expect(tk_rparen, ")");
  emit("    jmp .L%d\n", lbl_cond);

  emit(".L%d:\n", lbl_body);
  compile_block();
  emit("    jmp .L%d\n", lbl_incr);
  emit(".L%d:\n", lbl_end);
  loop_depth--;
}

static void compile_do_while(void) {
  advance();
  int lbl_start = new_label();
  int lbl_end   = new_label();
  loop_break_label[loop_depth] = lbl_end;
  loop_cont_label[loop_depth]  = lbl_start;
  loop_depth++;

  emit(".L%d:\n", lbl_start);
  compile_block();
  expect(tk_kw_while, "while");
  expect(tk_lparen, "(");
  compile_expr();
  expect(tk_rparen, ")");
  skip_semi();
  emit("    pop rax\n");
  emit("    test rax, rax\n");
  emit("    jnz .L%d\n", lbl_start);
  emit(".L%d:\n", lbl_end);
  loop_depth--;
}

static void compile_switch(void) {
  advance();
  expect(tk_lparen, "(");
  compile_expr();
  expect(tk_rparen, ")");
  emit("    pop rax\n");
  emit("    mov r15, rax\n");
  expect(tk_lbrace, "{");

  int lbl_end = new_label();
  int lbl_default = -1;
  loop_break_label[loop_depth] = lbl_end;
  loop_depth++;

  while (!check(tk_rbrace) && !check(tk_eof)) {
    skip_semi();
    if (check(tk_kw_case)) {
      advance();
      compile_expr();
      expect(tk_colon, ":");
      emit("    pop rcx\n");
      emit("    cmp r15, rcx\n");
      int lbl_skip = new_label();
      emit("    jne .L%d\n", lbl_skip);
      while (!check(tk_kw_case) && !check(tk_kw_default) && !check(tk_rbrace) && !check(tk_eof)) {
        compile_stmt();
      }
      emit(".L%d:\n", lbl_skip);
    } else if (check(tk_kw_default)) {
      advance();
      expect(tk_colon, ":");
      lbl_default = new_label();
      emit(".L%d:\n", lbl_default);
      while (!check(tk_kw_case) && !check(tk_rbrace) && !check(tk_eof)) {
        compile_stmt();
      }
    } else {
      compile_stmt();
    }
  }
  expect(tk_rbrace, "}");
  emit(".L%d:\n", lbl_end);
  loop_depth--;
}

static void compile_return(void) {
  advance();
  if (!check(tk_semi) && !check(tk_rbrace) && !check(tk_eof)) {
    compile_expr();
    emit("    pop rax\n");
  } else {
    emit("    xor eax, eax\n");
  }
  emit("    mov rsp, rbp\n");
  emit("    pop rbp\n");
  emit("    ret\n");
  skip_semi();
}

static void compile_stmt(void) {
  skip_semi();
  if (check(tk_eof)) return;

  if (check(tk_kw_printel)) { compile_printel(); return; }
  if (check(tk_kw_scanel))  { compile_scanel();  return; }
  if (check(tk_kw_if))      { compile_if();      return; }
  if (check(tk_kw_while))   { compile_while();   return; }
  if (check(tk_kw_for))     { compile_for();     return; }
  if (check(tk_kw_do))      { compile_do_while();return; }
  if (check(tk_kw_switch))  { compile_switch();  return; }
  if (check(tk_kw_return))  { compile_return();  return; }

  if (check(tk_kw_break)) {
    advance(); skip_semi();
    if (loop_depth > 0) emit("    jmp .L%d\n", loop_break_label[loop_depth-1]);
    return;
  }
  if (check(tk_kw_continue)) {
    advance(); skip_semi();
    if (loop_depth > 0) emit("    jmp .L%d\n", loop_cont_label[loop_depth-1]);
    return;
  }

  if (check(tk_kw_int)||check(tk_kw_float)||check(tk_kw_char)||
      check(tk_kw_void)||check(tk_kw_long)||check(tk_kw_short)||
      check(tk_kw_unsigned)||check(tk_kw_signed)||check(tk_kw_any)||
      check(tk_kw_const)||check(tk_kw_static)) {
    if (check(tk_kw_const)||check(tk_kw_static)) advance();
    compile_var_decl();
    return;
  }

  if ((check(tk_ident)||check(tk_kw_main)) && (
      peek(1)->type==tk_eq ||
      peek(1)->type==tk_pluseq ||
      peek(1)->type==tk_minuseq ||
      peek(1)->type==tk_stareq ||
      peek(1)->type==tk_slasheq)) {
    char nm[MAX_IDENT];
    strcpy(nm, cur()->sval);
    advance();
    compile_assign(nm);
    return;
  }

  if ((check(tk_ident)||check(tk_kw_main)) &&
      (peek(1)->type==tk_plusplus||peek(1)->type==tk_minusminus)) {
    char nm[MAX_IDENT];
    strcpy(nm, cur()->sval);
    advance();
    int is_inc = cur()->type == tk_plusplus;
    advance();
    var_entry *v = find_var(nm);
    if (v) {
      if (v->is_global) {
        emit("    mov rax, [rel _%s]\n", v->name);
        if(is_inc) emit("    inc rax\n"); else emit("    dec rax\n");
        emit("    mov [rel _%s], rax\n", v->name);
      } else {
        emit("    mov rax, [rbp - %d]\n", v->offset);
        if(is_inc) emit("    inc rax\n"); else emit("    dec rax\n");
        emit("    mov [rbp - %d], rax\n", v->offset);
      }
    }
    skip_semi();
    return;
  }

  if (check(tk_lbrace)) { compile_block(); return; }

  compile_expr();
  emit("    pop rax\n");
  skip_semi();
}

static void compile_block(void) {
  if (check(tk_lbrace)) {
    advance();
    scope_var_start[scope_depth++] = nvar;
    while (!check(tk_rbrace) && !check(tk_eof)) compile_stmt();
    expect(tk_rbrace, "}");
    nvar = scope_var_start[--scope_depth];
  } else {
    compile_stmt();
  }
}

static void compile_function(void) {
  parse_type();
  char fname[MAX_IDENT];
  if (check(tk_kw_main)) { strcpy(fname,"main"); advance(); }
  else { strcpy(fname, cur()->sval); advance(); }

  func_entry *f = &funcs[nfunc++];
  strcpy(f->name, fname);
  f->label_id = new_label();

  expect(tk_lparen, "(");
  f->param_count = 0;
  static const char *param_regs[] = {"rdi","rsi","rdx","rcx","r8","r9"};
  while (!check(tk_rparen) && !check(tk_eof)) {
    parse_type();
    if (check(tk_ident)) {
      strcpy(f->params[f->param_count], cur()->sval);
      f->param_count++;
      advance();
    }
    if (!match(tk_comma)) break;
  }
  expect(tk_rparen, ")");

  if (check(tk_semi)) { advance(); nfunc--; return; }

  emit("\n_%s:\n", fname);
  emit("    push rbp\n");
  emit("    mov rbp, rsp\n");

  int saved_offset = cur_stack_offset;
  cur_stack_offset = 0;
  in_function = 1;
  cur_func_idx = nfunc - 1;

  int var_start = nvar;
  for (int i = 0; i < f->param_count && i < 6; i++) {
    var_entry *v = &vars[nvar++];
    strcpy(v->name, f->params[i]);
    v->is_global = 0;
    cur_stack_offset += 8;
    v->offset = cur_stack_offset;
    v->type_size = 8;
    emit("    sub rsp, 8\n");
    emit("    mov [rbp - %d], %s\n", v->offset, param_regs[i]);
  }

  if (!check(tk_lbrace)) {
    fprintf(stderr, "[dpp error] expected '{' for function body\n"); exit(1);
  }
  advance();
  scope_var_start[scope_depth++] = nvar;
  while (!check(tk_rbrace) && !check(tk_eof)) compile_stmt();
  expect(tk_rbrace, "}");
  nvar = scope_var_start[--scope_depth];

  emit("    xor eax, eax\n");
  emit("    mov rsp, rbp\n");
  emit("    pop rbp\n");
  emit("    ret\n");

  in_function = 0;
  cur_stack_offset = saved_offset;
  nvar = var_start;
}

static void compile_global_var(void) {
  parse_type();
  token *name_tok = expect(tk_ident, "variable name");
  var_entry *v = &vars[nvar++];
  strcpy(v->name, name_tok->sval);
  v->is_global = 1;
  v->offset = 0;
  v->type_size = 8;
  v->arr_size = 0;

  long long init_val = 0;
  if (check(tk_lbracket)) {
    advance();
    if (check(tk_num)) { v->arr_size = (int)cur()->ival; advance(); }
    else v->arr_size = 64;
    expect(tk_rbracket, "]");
  }
  if (check(tk_eq)) {
    advance();
    if (check(tk_num)) { init_val = cur()->ival; advance(); }
    else if (check(tk_minus)) { advance(); init_val = -cur()->ival; advance(); }
  }
  v->ival = init_val;
  skip_semi();
}

static int is_type_token(void) {
  return check(tk_kw_int)||check(tk_kw_float)||check(tk_kw_char)||
         check(tk_kw_void)||check(tk_kw_long)||check(tk_kw_short)||
         check(tk_kw_unsigned)||check(tk_kw_signed)||check(tk_kw_any)||
         check(tk_kw_const)||check(tk_kw_static);
}

static int is_func_decl(void) {
  int saved = tpos;
  while (is_type_token()) advance();
  if (!check(tk_ident) && !check(tk_kw_main)) { tpos = saved; return 0; }
  advance();
  int r = check(tk_lparen);
  tpos = saved;
  return r;
}

static void compile_toplevel(void) {
  parse_includes();
  while (!check(tk_eof)) {
    skip_semi();
    if (check(tk_eof)) break;
    if (is_func_decl()) {
      compile_function();
    } else if (is_type_token()) {
      compile_global_var();
    } else {
      advance();
    }
  }
}

static void generate_asm_header(FILE *out) {
  fprintf(out, "bits 64\n");
  fprintf(out, "default rel\n\n");
  fprintf(out, "section .data\n");

  for (int i = 0; i < str_lit_count; i++) {
    fprintf(out, "_str%d: db ", i);
    unsigned char *s = (unsigned char*)str_literals[i];
    int first = 1;
    while (*s) {
      if (!first) fprintf(out, ",");
      if (*s == '\n') fprintf(out, "10");
      else if (*s == '\t') fprintf(out, "9");
      else if (*s == '\r') fprintf(out, "13");
      else if (*s == '\\') fprintf(out, "92");
      else if (*s == '"') fprintf(out, "34");
      else fprintf(out, "%d", (int)*s);
      s++; first = 0;
    }
    if (!first) fprintf(out, ",");
    fprintf(out, "0\n");
  }
  fprintf(out, "\n");

  fprintf(out, "section .bss\n");
  for (int i = 0; i < nvar; i++) {
    var_entry *v = &vars[i];
    if (v->is_global) {
      int sz = v->arr_size > 0 ? v->arr_size * v->type_size : v->type_size;
      if (sz < 8) sz = 8;
      fprintf(out, "_%s: resq %d\n", v->name, (sz+7)/8);
    }
  }
  fprintf(out, "\n");

  fprintf(out, "section .text\n");
  fprintf(out, "global _main\n");
  fprintf(out, "extern _printf\n");
  fprintf(out, "extern _scanf\n");
  fprintf(out, "extern _malloc\n");
  fprintf(out, "extern _free\n");
  fprintf(out, "extern _exit\n");
  fprintf(out, "\n");
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "dpp compiler v2.0\n");
    fprintf(stderr, "usage: dpp <source.dpp> [-o output.asm]\n");
    return 1;
  }

  const char *infile  = argv[1];
  const char *outfile = argc >= 4 && !strcmp(argv[2],"-o") ? argv[3] : "out.asm";

  FILE *f = fopen(infile, "r");
  if (!f) { fprintf(stderr, "[dpp] cannot open '%s'\n", infile); return 1; }
  int n = fread(src, 1, MAX_SRC-1, f);
  src[n] = 0;
  fclose(f);

  clock_t t0 = clock();

  tokenize();
  compile_toplevel();

  clock_t t1 = clock();
  double ms = (double)(t1-t0)*1000.0/CLOCKS_PER_SEC;

  FILE *out = fopen(outfile, "w");
  if (!out) { fprintf(stderr, "[dpp] cannot write '%s'\n", outfile); return 1; }

  generate_asm_header(out);
  fwrite(asm_buf, 1, asm_pos, out);
  fclose(out);

  fprintf(stderr, "[dpp] compiled in %.2fms => %s\n", ms, outfile);
  fprintf(stderr, "[dpp] tokens: %d | strings: %d | vars: %d | funcs: %d\n",
          ntoks, str_lit_count, nvar, nfunc);

  return 0;
}
