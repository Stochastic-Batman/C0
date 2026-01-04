#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"


static token_t* current = NULL;


static expr_t* parse_expr(void);
static expr_t* parse_pso(void);
static stmt_t* parse_st_s(void);
static expr_t* parse_postfix(void);


static void advance(void) {
    if (current != NULL) {
        free(current->lexeme);
        free(current);
    }
    next_token();
}


static void eat(token_type expected) {
    if (current->type == expected) {
        advance();
    } else {
        fprintf(stderr, "Error at line %d col %d: expected %d, got %d (%s)\n", current->line, current->col, expected, current->type, current->lexeme ? current->lexeme : "EOF");
        exit(1);
    }
}


static bool is_type_start(token_type tt) {
    return tt == TOKEN_INT || tt == TOKEN_BOOL || tt == TOKEN_CHAR_KW || tt == TOKEN_UINT || tt == TOKEN_IDENTIFIER;
}


decl_t* decl_create(decl_kind_t kind, char* name, type_t* type, expr_t* value, stmt_t* code, decl_t* next) {
    decl_t* d = malloc(sizeof(decl_t));
    if (d == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    d->kind = kind;
    d->name = name;
    d->type = type;
    d->value = value;
    d->code = code;
    d->next = next;
    return d;
}


stmt_t* stmt_create(stmt_kind_t kind, decl_t* decl, expr_t* init, expr_t* cond, expr_t* next_expr, stmt_t* body, stmt_t* else_body, stmt_t* next_stmt) {
    stmt_t* s = malloc(sizeof(stmt_t));
    if (s == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    s->kind = kind;
    s->decl = decl;
    s->init = init;
    s->cond = cond;
    s->next = next_expr;
    s->body = body;
    s->else_body = else_body;
    s->next_stmt = next_stmt;
    return s;
}


expr_t* expr_create(expr_kind_t kind, expr_t* left, expr_t* right) {
    expr_t* e = malloc(sizeof(expr_t));
    if (e == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    e->kind = kind;
    e->left = left;
    e->right = right;
    e->name = NULL;
    e->num_val = 0;
    e->char_val = 0;
    e->bool_val = false;
    e->next = NULL;
    return e;
}


expr_t* expr_create_id(char* name) {
    expr_t* e = expr_create(EXPR_ID, NULL, NULL);
    e->name = name;
    return e;
}


expr_t* expr_create_num(int val) {
    expr_t* e = expr_create(EXPR_NUM, NULL, NULL);
    e->num_val = val;
    return e;
}


expr_t* expr_create_char(char val) {
    expr_t* e = expr_create(EXPR_CHAR, NULL, NULL);
    e->char_val = val;
    return e;
}


expr_t* expr_create_bool(bool val) {
    expr_t* e = expr_create(EXPR_BOOL, NULL, NULL);
    e->bool_val = val;
    return e;
}


expr_t* expr_create_null(void) {
    expr_t* e = expr_create(EXPR_NULL, NULL, NULL);
    return e;
}


type_t* type_create(type_kind_t kind, type_t* subtype, param_t* params) {
    type_t* t = malloc(sizeof(type_t));
    if (t == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    t->kind = kind;
    t->subtype = subtype;
    t->params = params;
    t->size = 0;
    t->name = NULL;
    return t;
}


param_t* param_create(char* name, type_t* type, param_t* next) {
    param_t* p = malloc(sizeof(param_t));
    if (p == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    p->name = name;
    p->type = type;
    p->next = next;
    return p;
}


static type_t* parse_ty(void) {
    type_t* t;
    if (current->type == TOKEN_IDENTIFIER) {
        t = type_create(TYPE_NAMED, NULL, NULL);
        t->name = strdup(current->lexeme);
        advance();
    } else {
        type_kind_t k;
        switch (current->type) {
            case TOKEN_INT: k = TYPE_INT; break;
            case TOKEN_BOOL: k = TYPE_BOOL; break;
            case TOKEN_CHAR_KW: k = TYPE_CHAR; break;
            case TOKEN_UINT: k = TYPE_UINT; break;
            default:
                fprintf(stderr, "Expected type at %d:%d\n", current->line, current->col);
                exit(1);
        }
        advance();
        t = type_create(k, NULL, NULL);
    }
    return t;
}


static type_t* parse_te_prime(type_t* base) {
    if (current->type == TOKEN_LBRACKET) {
        advance();
        type_t* arr = type_create(TYPE_ARRAY, base, NULL);
        arr->size = current->value.num_value;
        eat(TOKEN_NUMBER);
        eat(TOKEN_RBRACKET);
        return arr;
    } else if (current->type == TOKEN_AT) {
        advance();
        return type_create(TYPE_POINTER, base, NULL);
    }
    return base;
}


static param_t* parse_fields(void) {
    param_t* head = NULL;
    param_t* tail = NULL;
    if (!is_type_start(current->type)) {
        return NULL;  // Allow empty struct
    }
    do {
        type_t* ty = parse_ty();
        char* name = strdup(current->lexeme);
        eat(TOKEN_IDENTIFIER);
        param_t* p = param_create(name, ty, NULL);
        if (tail) {
            tail->next = p;
        } else {
            head = p;
        }
        tail = p;
        if (current->type == TOKEN_SEMI) {
            advance();
        } else {
            break;
        }
    } while (is_type_start(current->type));
    return head;
}


static type_t* parse_te(void) {
    if (current->type == TOKEN_STRUCT) {
        advance();
        eat(TOKEN_LBRACE);
        param_t* fields = parse_fields();
        eat(TOKEN_RBRACE);
        return type_create(TYPE_STRUCT, NULL, fields);
    } else {
        type_t* base = parse_ty();
        return parse_te_prime(base);
    }
}


static decl_t* parse_tyd(void) {
    eat(TOKEN_TYPEDEF);
    type_t* te = parse_te();
    char* name = strdup(current->lexeme);
    eat(TOKEN_IDENTIFIER);
    return decl_create(DECL_TYPE, name, te, NULL, NULL, NULL);
}


static decl_t* parse_tyds(void) {
    decl_t* head = parse_tyd();
    decl_t* tail = head;
    while (current->type == TOKEN_SEMI) {
        advance();
        if (current->type != TOKEN_TYPEDEF) {
            break;
        }
        decl_t* next = parse_tyd();
        tail->next = next;
        tail = next;
    }
    return head;
}


static decl_t* parse_tdso(void) {
    if (current->type == TOKEN_TYPEDEF) {
        return parse_tyds();
    }
    return NULL;
}


static param_t* parse_va_d_as_param(void) {
    type_t* ty = parse_te();
    char* name = strdup(current->lexeme);
    eat(TOKEN_IDENTIFIER);
    return param_create(name, ty, NULL);
}


static param_t* parse_pa_ds(void) {
    param_t* head = parse_va_d_as_param();
    param_t* tail = head;
    while (current->type == TOKEN_COMMA) {
        advance();
        param_t* next = parse_va_d_as_param();
        tail->next = next;
        tail = next;
    }
    return head;
}


static param_t* parse_pdso(void) {
    if (current->type == TOKEN_RPAREN) {
        return NULL;
    }
    return parse_pa_ds();
}


static decl_t* parse_va_d_as_decl(void) {
    type_t* ty = parse_te();
    char* name = strdup(current->lexeme);
    eat(TOKEN_IDENTIFIER);
    return decl_create(DECL_VAR, name, ty, NULL, NULL, NULL);
}


static decl_t* parse_va_ds(void) {
    decl_t* head = parse_va_d_as_decl();
    decl_t* tail = head;
    while (current->type == TOKEN_SEMI) {
        advance();
        if (current->type != TOKEN_INT && current->type != TOKEN_BOOL && current->type != TOKEN_CHAR_KW && current->type != TOKEN_UINT) {
            break;
        }
        decl_t* next = parse_va_d_as_decl();
        tail->next = next;
        tail = next;
    }
    return head;
}


static stmt_t* parse_locals(void) {
    if (current->type == TOKEN_INT || current->type == TOKEN_BOOL || current->type == TOKEN_CHAR_KW || current->type == TOKEN_UINT) {
        decl_t* vars = parse_va_ds();
        stmt_t* head = NULL;
        stmt_t* tail = NULL;
        for (decl_t* v = vars; v != NULL; ) {
            decl_t* next_v = v->next;
            v->next = NULL;
            stmt_t* s = stmt_create(STMT_DECL, v, NULL, NULL, NULL, NULL, NULL, NULL);
            if (tail) {
                tail->next_stmt = s;
            } else {
                head = s;
            }
            tail = s;
            v = next_v;
        }
        return head;
    }
    return NULL;
}


static expr_t* parse_lvalue_tail(expr_t* base) {
    while (true) {
        if (current->type == TOKEN_DOT) {
            advance();
            expr_t* node = expr_create(EXPR_FIELD, base, NULL);
            node->name = strdup(current->lexeme);
            eat(TOKEN_IDENTIFIER);
            base = node;
        } else if (current->type == TOKEN_LBRACKET) {
            advance();
            expr_t* node = expr_create(EXPR_INDEX, base, parse_expr());
            eat(TOKEN_RBRACKET);
            base = node;
        } else if (current->type == TOKEN_AT) {
            advance();
            expr_t* node = expr_create(EXPR_DEREF, base, NULL);
            base = node;
        } else if (current->type == TOKEN_AMP) {
            advance();
            expr_t* node = expr_create(EXPR_ADDR, base, NULL);
            base = node;
        } else {
            break;
        }
    }
    return base;
}


static expr_t* parse_lvalue(void) {
    char* name = strdup(current->lexeme);
    eat(TOKEN_IDENTIFIER);
    expr_t* base = expr_create_id(name);
    return parse_lvalue_tail(base);
}


static expr_t* parse_primary(void) {
    expr_t* node;
    switch (current->type) {
        case TOKEN_IDENTIFIER:
            {
                char* name = strdup(current->lexeme);
                advance();
                if (current->type == TOKEN_LPAREN) {
                    advance();
                    expr_t* args = parse_pso();
                    node = expr_create(EXPR_CALL, args, NULL);
                    node->name = name;
                    eat(TOKEN_RPAREN);
                } else {
                    expr_t* id = expr_create_id(name);
                    node = parse_lvalue_tail(id);
                }
            }
            break;
        case TOKEN_MINUS:
            advance();
            node = expr_create(EXPR_NEG, parse_primary(), NULL);
            break;
        case TOKEN_NOT:
            advance();
            node = expr_create(EXPR_NOT, parse_primary(), NULL);
            break;
        case TOKEN_LPAREN:
            advance();
            node = parse_expr();
            eat(TOKEN_RPAREN);
            break;
        case TOKEN_NUMBER:
            node = expr_create_num(current->value.num_value);
            advance();
            break;
        case TOKEN_CHAR:
            node = expr_create_char(current->value.char_value);
            advance();
            break;
        case TOKEN_TRUE:
            node = expr_create_bool(true);
            advance();
            break;
        case TOKEN_FALSE:
            node = expr_create_bool(false);
            advance();
            break;
        case TOKEN_NULL:
            node = expr_create_null();
            advance();
            break;
        default:
            fprintf(stderr, "Unexpected token %d at %d:%d\n", current->type, current->line, current->col);
            exit(1);
    }
    return node;
}


static expr_t* parse_mul_expr_tail(expr_t* left) {
    while (current->type == TOKEN_STAR || current->type == TOKEN_DIV) {
        expr_kind_t op = (current->type == TOKEN_STAR) ? EXPR_MUL : EXPR_DIV;
        advance();
        expr_t* right = parse_primary();
        left = expr_create(op, left, right);
    }
    return left;
}


static expr_t* parse_postfix(void) {
    expr_t* base = parse_primary();
    return parse_lvalue_tail(base);
}


static expr_t* parse_mul_expr(void) {
    expr_t* left = parse_postfix();
    return parse_mul_expr_tail(left);
}


static expr_t* parse_add_expr_tail(expr_t* left) {
    while (current->type == TOKEN_PLUS || current->type == TOKEN_MINUS) {
        expr_kind_t op = (current->type == TOKEN_PLUS) ? EXPR_ADD : EXPR_SUB;
        advance();
        expr_t* right = parse_mul_expr();
        left = expr_create(op, left, right);
    }
    return left;
}


static expr_t* parse_add_expr(void) {
    expr_t* left = parse_mul_expr();
    return parse_add_expr_tail(left);
}


static expr_kind_t parse_rel_op(void) {
    switch (current->type) {
        case TOKEN_EQ: return EXPR_EQ;
        case TOKEN_NEQ: return EXPR_NEQ;
        case TOKEN_LT: return EXPR_LT;
        case TOKEN_GT: return EXPR_GT;
        case TOKEN_LEQ: return EXPR_LEQ;
        case TOKEN_GEQ: return EXPR_GEQ;
        default:
            fprintf(stderr, "Expected rel op at %d:%d\n", current->line, current->col);
            exit(1);
    }
    return EXPR_EQ;  // Unreachable
}


static expr_t* parse_rel_expr_tail(expr_t* left) {
    while (current->type == TOKEN_EQ || current->type == TOKEN_NEQ || current->type == TOKEN_LT || current->type == TOKEN_GT || current->type == TOKEN_LEQ || current->type == TOKEN_GEQ) {
        expr_kind_t op = parse_rel_op();
        advance();
        expr_t* right = parse_add_expr();
        left = expr_create(op, left, right);
    }
    return left;
}


static expr_t* parse_rel_expr(void) {
    expr_t* left = parse_add_expr();
    return parse_rel_expr_tail(left);
}


static expr_t* parse_and_expr_tail(expr_t* left) {
    while (current->type == TOKEN_AND) {
        advance();
        expr_t* right = parse_rel_expr();
        left = expr_create(EXPR_AND, left, right);
    }
    return left;
}


static expr_t* parse_and_expr(void) {
    expr_t* left = parse_rel_expr();
    return parse_and_expr_tail(left);
}


static expr_t* parse_expr_tail(expr_t* left) {
    while (current->type == TOKEN_OR) {
        advance();
        expr_t* right = parse_and_expr();
        left = expr_create(EXPR_OR, left, right);
    }
    return left;
}


static expr_t* parse_expr(void) {
    expr_t* left = parse_and_expr();
    return parse_expr_tail(left);
}


static expr_t* parse_pa_s(void) {
    expr_t* head = parse_expr();
    expr_t* tail = head;
    while (current->type == TOKEN_COMMA) {
        advance();
        expr_t* next = parse_expr();
        tail->next = next;
        tail = next;
    }
    return head;
}


static expr_t* parse_pso(void) {
    if (current->type == TOKEN_RPAREN) return NULL;
    return parse_pa_s();
}


static expr_t* parse_rhs(void) {
    if (current->type == TOKEN_NEW) {
        advance();
        expr_t* node = expr_create(EXPR_ALLOC, NULL, NULL);
        node->name = strdup(current->lexeme);
        eat(TOKEN_IDENTIFIER);
        eat(TOKEN_AT);
        return node;
    } else {
        return parse_expr();
    }
}


static stmt_t* parse_ep(void) {
    if (current->type == TOKEN_ELSE) {
        advance();
        eat(TOKEN_LBRACE);
        stmt_t* else_body = parse_st_s();
        eat(TOKEN_RBRACE);
        return else_body;
    }
    return NULL;
}


static stmt_t* parse_st(void) {
    stmt_t* node;
    if (current->type == TOKEN_IF) {
        advance();
        expr_t* cond = parse_expr();
        eat(TOKEN_LBRACE);
        stmt_t* body = parse_st_s();
        eat(TOKEN_RBRACE);
        stmt_t* else_body = parse_ep();
        node = stmt_create(STMT_IF, NULL, NULL, cond, NULL, body, else_body, NULL);
    } else if (current->type == TOKEN_WHILE) {
        advance();
        expr_t* cond = parse_expr();
        eat(TOKEN_LBRACE);
        stmt_t* body = parse_st_s();
        eat(TOKEN_RBRACE);
        node = stmt_create(STMT_WHILE, NULL, NULL, cond, NULL, body, NULL, NULL);
    } else {
        expr_t* lhs = parse_lvalue();
        eat(TOKEN_ASSIGN);
        expr_t* rhs = parse_rhs();
        node = stmt_create(STMT_ASSIGN, NULL, lhs, rhs, NULL, NULL, NULL, NULL);
    }
    return node;
}


static stmt_t* parse_st_s(void) {
    stmt_t* head = NULL;
    stmt_t* tail = NULL;
    if (current->type != TOKEN_RBRACE && current->type != TOKEN_RETURN) {
        head = parse_st();
        tail = head;
        while (current->type == TOKEN_SEMI) {
            advance();
            if (current->type == TOKEN_RBRACE || current->type == TOKEN_RETURN) {
                break;
            }
            stmt_t* next = parse_st();
            tail->next_stmt = next;
            tail = next;
        }
    }
    return head;
}


static stmt_t* parse_r_st(void) {
    eat(TOKEN_RETURN);
    expr_t* expr = parse_expr();
    eat(TOKEN_SEMI);
    return stmt_create(STMT_RETURN, NULL, NULL, expr, NULL, NULL, NULL, NULL);
}


static stmt_t* parse_sso(void) {
    if (current->type == TOKEN_RBRACE || current->type == TOKEN_RETURN) return NULL;
    return parse_st_s();
}


static stmt_t* parse_body(void) {
    stmt_t* stmts = parse_sso();
    stmt_t* ret = parse_r_st();
    if (stmts == NULL) return ret;
    else {
        stmt_t* tail = stmts;
        while (tail->next_stmt != NULL) {
            tail = tail->next_stmt;
        }
        tail->next_stmt = ret;
        return stmts;
    }
}


static decl_t* parse_gd(void) {
    type_t* ty = parse_te();
    char* name = strdup(current->lexeme);
    eat(TOKEN_IDENTIFIER);
    if (current->type == TOKEN_SEMI) {
        advance();
        return decl_create(DECL_VAR, name, ty, NULL, NULL, NULL);
    } else if (current->type == TOKEN_ASSIGN) {
        advance();
        expr_t* init_expr = parse_rhs();
        eat(TOKEN_SEMI);
        return decl_create(DECL_VAR, name, ty, init_expr, NULL, NULL);
    } else {
        eat(TOKEN_LPAREN);
        param_t* params = parse_pdso();
        eat(TOKEN_RPAREN);
        eat(TOKEN_LBRACE);
        stmt_t* locals = parse_locals();
        stmt_t* body_stmts = parse_body();
        if (locals != NULL) {
            stmt_t* local_tail = locals;
            while (local_tail->next_stmt != NULL) {
                local_tail = local_tail->next_stmt;
            }
            local_tail->next_stmt = body_stmts;
            body_stmts = locals;
        }
        eat(TOKEN_RBRACE);
        type_t* func_ty = type_create(TYPE_FUNC, ty, params);
        return decl_create(DECL_FUNC, name, func_ty, NULL, body_stmts, NULL);
    }
}


static decl_t* parse_g_ds(void) {
    decl_t* head = NULL;
    decl_t* tail = NULL;
    while (current->type != TOKEN_EOF) {
        decl_t* gd = parse_gd();
        if (tail) {
            tail->next = gd;
        } else {
            head = gd;
        }
        tail = gd;
    }
    return head;
}


decl_t* parse_program(FILE* fp) {
    scanner_init(fp);
    advance();
    decl_t* types = parse_tdso();
    decl_t* globals = parse_g_ds();
    if (types == NULL) return globals;
    else {
        decl_t* tail = types;
        while (tail->next != NULL) {
            tail = tail->next;
        }
        tail->next = globals;
        return types;
    }
}


void free_decl(decl_t* d) {
    while (d != NULL) {
        decl_t* next = d->next;
        free(d->name);
        free_type(d->type);
        free_expr(d->value);
        free_stmt(d->code);
        free(d);
        d = next;
    }
}


void free_stmt(stmt_t* s) {
    while (s != NULL) {
        stmt_t* next = s->next_stmt;
        free_decl(s->decl);
        free_expr(s->init);
        free_expr(s->cond);
        free_expr(s->next);
        free_stmt(s->body);
        free_stmt(s->else_body);
        free(s);
        s = next;
    }
}


void free_expr(expr_t* e) {
    if (e == NULL) return;
    free_expr(e->left);
    free_expr(e->right);
    free(e->name);
    free_expr(e->next);
    free(e);
}


void free_type(type_t* t) {
    if (t == NULL) return;
    free(t->name);
    free_type(t->subtype);
    free_param(t->params);
    free(t);
}


void free_param(param_t* p) {
    while (p != NULL) {
        param_t* next = p->next;
        free(p->name);
        free_type(p->type);
        free(p);
        p = next;
    }
}


void print_decl(decl_t* d, int indent) {
    for (decl_t* cur = d; cur != NULL; cur = cur->next) {
        for (int i = 0; i < indent; i++) printf(" ");
        printf("Decl kind: %d", cur->kind);
        if (cur->name) printf(", name: %s", cur->name);
        printf("\n");
        print_type(cur->type, indent + 2);
        print_expr(cur->value, indent + 2);
        print_stmt(cur->code, indent + 2);
    }
}


void print_stmt(stmt_t* s, int indent) {
    for (stmt_t* cur = s; cur != NULL; cur = cur->next_stmt) {
        for (int i = 0; i < indent; i++) printf(" ");
        printf("Stmt kind: %d\n", cur->kind);
        print_decl(cur->decl, indent + 2);
        print_expr(cur->init, indent + 2);
        print_expr(cur->cond, indent + 2);
        print_expr(cur->next, indent + 2);
        print_stmt(cur->body, indent + 2);
        print_stmt(cur->else_body, indent + 2);
    }
}


void print_expr(expr_t* e, int indent) {
    if (e == NULL) return;
    for (int i = 0; i < indent; i++) printf(" ");
    printf("Expr kind: %d", e->kind);
    if (e->name) printf(", name: %s", e->name);
    if (e->kind == EXPR_NUM) printf(", val: %d", e->num_val);
    else if (e->kind == EXPR_CHAR) printf(", val: '%c'", e->char_val);
    else if (e->kind == EXPR_BOOL) printf(", val: %s", e->bool_val ? "true" : "false");
    printf("\n");
    print_expr(e->left, indent + 2);
    print_expr(e->right, indent + 2);
    print_expr(e->next, indent + 2);
}


void print_type(type_t* t, int indent) {
    if (t == NULL) return;
    for (int i = 0; i < indent; i++) printf(" ");
    printf("Type kind: %d", t->kind);
    if (t->name) printf(", name: %s", t->name);
    if (t->size > 0) printf(", size: %d", t->size);
    printf("\n");
    print_type(t->subtype, indent + 2);
    print_param(t->params, indent + 2);
}


void print_param(param_t* p, int indent) {
    for (param_t* cur = p; cur != NULL; cur = cur->next) {
        for (int i = 0; i < indent; i++) printf(" ");
        printf("Param");
        if (cur->name) printf(", name: %s", cur->name);
        printf("\n");
        print_type(cur->type, indent + 2);
    }
}
