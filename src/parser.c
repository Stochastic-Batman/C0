#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"


static token_t* current = NULL;

static ast_t* parse_ty(void);
static void parse_te_prime(ast_t* node);
static ast_t* parse_te(void);
static ast_t* parse_va_d(void);
static ast_t* parse_va_ds(void);
static ast_t* parse_ty_d(void);
static ast_t* parse_ty_ds(void);
static ast_t* parse_tdso(void);
static ast_t* parse_lvalue_tail(ast_t* base);
static ast_t* parse_lvalue(void);
static ast_t* parse_primary(void);
static ast_t* parse_mul_expr_tail(ast_t* left);
static ast_t* parse_mul_expr(void);
static ast_t* parse_add_expr_tail(ast_t* left);
static ast_t* parse_add_expr(void);
static binop_t parse_rel_op(void);
static ast_t* parse_rel_expr_tail(ast_t* left);
static ast_t* parse_rel_expr(void);
static ast_t* parse_and_expr_tail(ast_t* left);
static ast_t* parse_and_expr(void);
static ast_t* parse_expr_tail(ast_t* left);
static ast_t* parse_expr(void);
static ast_t* parse_pa_s(void);
static ast_t* parse_pso(void);
static ast_t* parse_rhs(void);
static ast_t* parse_ep(void);
static ast_t* parse_st(void);
static ast_t* parse_st_s(void);
static ast_t* parse_r_st(void);
static ast_t* parse_sso(void);
static ast_t* parse_locals(void);
static ast_t* parse_body(void);
static ast_t* parse_pa_ds(void);
static ast_t* parse_pdso(void);
static ast_t* parse_gd(void);
static ast_t* parse_g_ds(void);


static void advance(void) {
    if (current != NULL) {
        free(current->lexeme);
        free(current);
    }
    current = next_token();
}


static void eat(token_type expected) {
    if (current->type == expected) {
        advance();
    } else {
        fprintf(stderr, "Error at line %d col %d: expected %d, got %d (%s)\n", current->line, current->col, expected, current->type, current->lexeme ? current->lexeme : "EOF");
        exit(1);
    }
}


static ast_t* new_ast(ast_kind_t kind) {
    ast_t* node = malloc(sizeof(ast_t));
    if (node == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    node->kind = kind;
    node->token = NULL;
    node->children = NULL;
    node->child_count = 0;
    return node;
}


static void add_child(ast_t* parent, ast_t* child) {
    parent->children = realloc(parent->children, (parent->child_count + 1) * sizeof(ast_t*));
    if (parent->children == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    parent->children[parent->child_count] = child;
    parent->child_count++;
}


static void add_children(ast_t* parent, ast_t** new_children, int new_count) {
    if (new_count == 0) return;
    parent->children = realloc(parent->children, (parent->child_count + new_count) * sizeof(ast_t*));
    if (parent->children == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    for (int i = 0; i < new_count; i++) {
        parent->children[parent->child_count + i] = new_children[i];
    }
    parent->child_count += new_count;
}


static ast_t* parse_ty(void) {
    ast_t* node = new_ast(AST_BASIC_TYPE);
    switch (current->type) {
        case TOKEN_INT:
        case TOKEN_BOOL:
        case TOKEN_CHAR_KW:
        case TOKEN_UINT:
        case TOKEN_IDENTIFIER:
            node->type_name = strdup(current->lexeme);
            advance();
            break;
        default:
            fprintf(stderr, "Expected type at %d:%d\n", current->line, current->col);
            exit(1);
    }
    return node;
}


static void parse_te_prime(ast_t* node) {
    switch (current->type) {
        case TOKEN_LBRACKET:
            advance();
            if (current->type == TOKEN_NUMBER) {
                node->array_size = current->value.num_value;
                advance();
            } else {
                node->array_size = 0;
            }
            eat(TOKEN_RBRACKET);
            break;
        case TOKEN_AT:
            advance();
            node->array_size = -1;
            break;
        default:
            node->array_size = 0;
            break;
    }
}


static ast_t* parse_te(void) {
    ast_t* node = new_ast(AST_TYPE_EXPR);
    if (current->type == TOKEN_STRUCT) {
        advance();
        eat(TOKEN_LBRACE);
        ast_t* fields = parse_va_ds();
        add_children(node, fields->children, fields->child_count);
        free(fields);
        eat(TOKEN_RBRACE);
        parse_te_prime(node);
        node->kind = AST_STRUCT_DECL;
    } else {
        ast_t* base = parse_ty();
        add_child(node, base);
        parse_te_prime(node);
    }
    return node;
}


static ast_t* parse_va_d(void) {
    ast_t* node = new_ast(AST_VAR_DECL);
    ast_t* ty = parse_ty();
    node->var_type = ty->type_name;
    free(ty);
    if (current->type == TOKEN_IDENTIFIER) {
        node->var_name = strdup(current->lexeme);
        advance();
    } else {
        eat(TOKEN_IDENTIFIER);
    }
    if (current->type == TOKEN_ASSIGN) {
        advance();
        node->init = parse_rhs();
    } else {
        node->init = NULL;
    }
    return node;
}


static ast_t* parse_va_ds(void) {
    ast_t* seq = new_ast(AST_VAR_DECL_SEQ);
    while (current->type == TOKEN_INT || current->type == TOKEN_BOOL || current->type == TOKEN_CHAR_KW || current->type == TOKEN_UINT || current->type == TOKEN_IDENTIFIER) {
        ast_t* decl = parse_va_d();
        add_child(seq, decl);
        eat(TOKEN_SEMI);
    }
    return seq;
}


static ast_t* parse_ty_d(void) {
    eat(TOKEN_TYPEDEF);
    ast_t* node = new_ast(AST_TYPE_DECL);
    node->type_expr = parse_te();
    if (current->type == TOKEN_IDENTIFIER) {
        node->decl_name = strdup(current->lexeme);
        advance();
    } else {
        eat(TOKEN_IDENTIFIER);
    }
    return node;
}


static ast_t* parse_ty_ds(void) {
    ast_t* seq = new_ast(AST_TYPE_DECL_SEQ);
    while (current->type == TOKEN_TYPEDEF) {
        ast_t* decl = parse_ty_d();
        add_child(seq, decl);
        eat(TOKEN_SEMI);
    }
    return seq;
}


static ast_t* parse_tdso(void) {
    if (current->type == TOKEN_TYPEDEF) {
        return parse_ty_ds();
    }
    return new_ast(AST_TYPE_DECL_SEQ);
}


static ast_t* parse_lvalue_tail(ast_t* base) {
    if (current->type == TOKEN_DOT) {
        advance();
        ast_t* node = new_ast(AST_FIELD_ACCESS);
        node->base = base;
        if (current->type == TOKEN_IDENTIFIER) {
            node->field_name = strdup(current->lexeme);
            advance();
        } else {
            eat(TOKEN_IDENTIFIER);
        }
        return parse_lvalue_tail(node);
    } else if (current->type == TOKEN_LBRACKET) {
        advance();
        ast_t* node = new_ast(AST_ARRAY_INDEX);
        node->base = base;
        node->index_expr = parse_expr();
        eat(TOKEN_RBRACKET);
        return parse_lvalue_tail(node);
    } else if (current->type == TOKEN_AT) {
        advance();
        ast_t* node = new_ast(AST_DEREF);
        node->child = base;
        return parse_lvalue_tail(node);
    }
    return base;
}


static ast_t* parse_lvalue(void) {
    if (current->type != TOKEN_IDENTIFIER) {
        eat(TOKEN_IDENTIFIER);
    }
    char* id = strdup(current->lexeme);
    advance();
    ast_t* base = new_ast(AST_ID);
    base->type_name = id;
    return parse_lvalue_tail(base);
}


static ast_t* parse_primary(void) {
    ast_t* node;
    switch (current->type) {
        case TOKEN_IDENTIFIER:
            char* id = strdup(current->lexeme);
            advance();
            if (current->type == TOKEN_LPAREN) {
                advance();
                node = new_ast(AST_CALL);
                node->call_name = id;
                ast_t* params = parse_pso();
                add_children(node, params->children, params->child_count);
                free(params);
                eat(TOKEN_RPAREN);
            } else {
                ast_t* base = new_ast(AST_ID);
                base->type_name = id;
                node = parse_lvalue_tail(base);
            }
            break;
        case TOKEN_MINUS:
            advance();
            node = new_ast(AST_UNARY_EXPR);
            node->un_op = OP_NEG;
            node->child = parse_primary();
            break;
        case TOKEN_NOT:
            advance();
            node = new_ast(AST_UNARY_EXPR);
            node->un_op = OP_NOT;
            node->child = parse_primary();
            break;
        case TOKEN_AMP:
            advance();
            node = new_ast(AST_ADDR_OF);
            node->child = parse_primary();
            break;
        case TOKEN_AT:
            advance();
            node = new_ast(AST_DEREF);
            node->child = parse_primary();
            break;
        case TOKEN_LPAREN:
            advance();
            node = parse_expr();
            eat(TOKEN_RPAREN);
            break;
        case TOKEN_NUMBER:
            node = new_ast(AST_PRIMARY);
            node->num_val = current->value.num_value;
            advance();
            break;
        case TOKEN_CHAR:
            node = new_ast(AST_PRIMARY);
            node->char_val = current->value.char_value;
            advance();
            break;
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            node = new_ast(AST_PRIMARY);
            node->bool_val = current->type == TOKEN_TRUE;
            advance();
            break;
        case TOKEN_NULL:
            node = new_ast(AST_PRIMARY);
            node->num_val = 0;
            advance();
            break;
        default:
            fprintf(stderr, "Unexpected token %d at %d:%d\n", current->type, current->line, current->col);
            exit(1);
    }
    return node;
}


static ast_t* parse_mul_expr_tail(ast_t* left) {
    while (current->type == TOKEN_STAR || current->type == TOKEN_DIV) {
        binop_t op = (current->type == TOKEN_STAR) ? OP_MUL : OP_DIV;
        advance();
        ast_t* right = parse_primary();
        ast_t* node = new_ast(AST_BIN_EXPR);
        node->left = left;
        node->bin_op = op;
        node->right = right;
        left = node;
    }
    return left;
}


static ast_t* parse_mul_expr(void) {
    ast_t* left = parse_primary();
    return parse_mul_expr_tail(left);
}


static ast_t* parse_add_expr_tail(ast_t* left) {
    while (current->type == TOKEN_PLUS || current->type == TOKEN_MINUS) {
        binop_t op = (current->type == TOKEN_PLUS) ? OP_ADD : OP_SUB;
        advance();
        ast_t* right = parse_mul_expr();
        ast_t* node = new_ast(AST_BIN_EXPR);
        node->left = left;
        node->bin_op = op;
        node->right = right;
        left = node;
    }
    return left;
}


static ast_t* parse_add_expr(void) {
    ast_t* left = parse_mul_expr();
    return parse_add_expr_tail(left);
}


static binop_t parse_rel_op(void) {
    switch (current->type) {
        case TOKEN_EQ: return OP_EQ;
        case TOKEN_NEQ: return OP_NEQ;
        case TOKEN_LT: return OP_LT;
        case TOKEN_GT: return OP_GT;
        case TOKEN_LEQ: return OP_LEQ;
        case TOKEN_GEQ: return OP_GEQ;
        default:
            fprintf(stderr, "Expected rel op at %d:%d\n", current->line, current->col);
            exit(1);
    }
}


static ast_t* parse_rel_expr_tail(ast_t* left) {
    while (current->type == TOKEN_EQ || current->type == TOKEN_NEQ || current->type == TOKEN_LT || current->type == TOKEN_GT || current->type == TOKEN_LEQ || current->type == TOKEN_GEQ) {
        binop_t op = parse_rel_op();
        advance();
        ast_t* right = parse_add_expr();
        ast_t* node = new_ast(AST_BIN_EXPR);
        node->left = left;
        node->bin_op = op;
        node->right = right;
        left = node;
    }
    return left;
}


static ast_t* parse_rel_expr(void) {
    ast_t* left = parse_add_expr();
    return parse_rel_expr_tail(left);
}


static ast_t* parse_and_expr_tail(ast_t* left) {
    while (current->type == TOKEN_AND) {
        advance();
        ast_t* right = parse_rel_expr();
        ast_t* node = new_ast(AST_BIN_EXPR);
        node->left = left;
        node->bin_op = OP_AND;
        node->right = right;
        left = node;
    }
    return left;
}


static ast_t* parse_and_expr(void) {
    ast_t* left = parse_rel_expr();
    return parse_and_expr_tail(left);
}


static ast_t* parse_expr_tail(ast_t* left) {
    while (current->type == TOKEN_OR) {
        advance();
        ast_t* right = parse_and_expr();
        ast_t* node = new_ast(AST_BIN_EXPR);
        node->left = left;
        node->bin_op = OP_OR;
        node->right = right;
        left = node;
    }
    return left;
}


static ast_t* parse_expr(void) {
    ast_t* left = parse_and_expr();
    return parse_expr_tail(left);
}


static ast_t* parse_pa_s(void) {
    ast_t* seq = new_ast(AST_PARAM_DECL_SEQ);
    ast_t* expr = parse_expr();
    add_child(seq, expr);
    while (current->type == TOKEN_COMMA) {
        advance();
        expr = parse_expr();
        add_child(seq, expr);
    }
    return seq;
}


static ast_t* parse_pso(void) {
    if (current->type == TOKEN_RPAREN) return new_ast(AST_PARAM_DECL_SEQ);
    return parse_pa_s();
}


static ast_t* parse_rhs(void) {
    if (current->type == TOKEN_NEW) {
        advance();
        ast_t* node = new_ast(AST_ALLOC);
        if (current->type == TOKEN_IDENTIFIER) {
            node->alloc_type = strdup(current->lexeme);
            advance();
        } else {
            eat(TOKEN_IDENTIFIER);
        }
        eat(TOKEN_AT);
        return node;
    } else {
        return parse_expr();
    }
}


static ast_t* parse_ep(void) {
    if (current->type == TOKEN_ELSE) {
        advance();
        eat(TOKEN_LBRACE);
        ast_t* else_body = parse_st_s();
        eat(TOKEN_RBRACE);
        return else_body;
    }
    return NULL;
}


static ast_t* parse_st(void) {
    ast_t* node;
    switch (current->type) {
        case TOKEN_IF:
            advance();
            node = new_ast(AST_IF);
            node->if_cond = parse_expr();
            eat(TOKEN_LBRACE);
            node->then_stmt = parse_st_s();
            eat(TOKEN_RBRACE);
            node->else_stmt = parse_ep();
            break;
        case TOKEN_WHILE:
            advance();
            node = new_ast(AST_WHILE);
            node->while_cond = parse_expr();
            eat(TOKEN_LBRACE);
            node->body = parse_st_s();
            eat(TOKEN_RBRACE);
            break;
        default:  // Assign
            ast_t* lhs = parse_lvalue();
            eat(TOKEN_ASSIGN);
            ast_t* rhs = parse_rhs();
            node = new_ast(AST_ASSIGN);
            node->lhs = lhs;
            node->rhs = rhs;
            break;
    }
    return node;
}


static ast_t* parse_st_s(void) {
    ast_t* seq = new_ast(AST_STMT_SEQ);
    if (current->type == TOKEN_RBRACE || current->type == TOKEN_RETURN) {
        return seq;  // Empty
    }
    ast_t* st = parse_st();
    add_child(seq, st);
    while (current->type == TOKEN_SEMI) {
        advance();
        if (current->type == TOKEN_RBRACE || current->type == TOKEN_RETURN) {
            break;
        }
        st = parse_st();
        add_child(seq, st);
    }
    return seq;
}


static ast_t* parse_r_st(void) {
    eat(TOKEN_RETURN);
    ast_t* node = new_ast(AST_RETURN);
    node->ret_expr = parse_expr();
    eat(TOKEN_SEMI);
    return node;
}


static ast_t* parse_sso(void) {
    return parse_st_s();
}


static ast_t* parse_locals(void) {
    if (current->type == TOKEN_INT || current->type == TOKEN_BOOL || current->type == TOKEN_CHAR_KW || current->type == TOKEN_UINT || current->type == TOKEN_IDENTIFIER) return parse_va_ds();
    return new_ast(AST_VAR_DECL_SEQ);
}


static ast_t* parse_body(void) {
    ast_t* node = new_ast(AST_STMT_SEQ);
    ast_t* stmts = parse_sso();
    add_children(node, stmts->children, stmts->child_count);
    free(stmts);
    ast_t* ret = parse_r_st();
    add_child(node, ret);
    return node;
}


static ast_t* parse_pa_ds(void) {
    ast_t* seq = new_ast(AST_PARAM_DECL_SEQ);
    do {
        ast_t* param = parse_va_d();
        add_child(seq, param);
        if (current->type == TOKEN_COMMA) {
            advance();
        } else {
            break;
        }
    } while (1);
    return seq;
}


static ast_t* parse_pdso(void) {
    if (current->type == TOKEN_RPAREN) return new_ast(AST_PARAM_DECL_SEQ);
    return parse_pa_ds();
}


static ast_t* parse_gd(void) {
    ast_t* ty = parse_ty();
    char* name = NULL;
    if (current->type == TOKEN_IDENTIFIER) {
        name = strdup(current->lexeme);
        advance();
    } else {
        eat(TOKEN_IDENTIFIER);
    }
    if (current->type == TOKEN_SEMI) {
        ast_t* var = new_ast(AST_GLOBAL_VAR);
        var->var_type = ty->type_name;
        var->var_name = name;
        free(ty);
        advance();
        return var;
    } else {
        ast_t* func = new_ast(AST_FUNC_DECL);
        func->ret_type = ty->type_name;
        func->func_name = name;
        free(ty);
        eat(TOKEN_LPAREN);
        ast_t* params = parse_pdso();
        func->param_count = params->child_count;
        func->children = params->children;
        free(params);
        eat(TOKEN_RPAREN);
        eat(TOKEN_LBRACE);
        ast_t* locals = parse_locals();
        func->local_count = locals->child_count;
        add_children(func, locals->children, locals->child_count);
        free(locals);
        ast_t* body = parse_body();
        add_child(func, body);
        eat(TOKEN_RBRACE);
        return func;
    }
}


static ast_t* parse_g_ds(void) {
    ast_t* seq = new_ast(AST_VAR_DECL_SEQ);
    while (current->type != TOKEN_EOF) {
        ast_t* gd = parse_gd();
        add_child(seq, gd);
    }
    return seq;
}


ast_t* parse_program(FILE* fp) {
    scanner_init(fp);
    advance();
    ast_t* prog = new_ast(AST_PROGRAM);
    ast_t* types = parse_tdso();
    prog->type_count = types->child_count;
    prog->children = types->children;
    free(types);
    ast_t* globals = parse_g_ds();
    add_children(prog, globals->children, globals->child_count);
    free(globals);
    return prog;
}


void free_ast(ast_t* node) {
    if (node == NULL) return;
    for (int i = 0; i < node->child_count; i++) {
        free_ast(node->children[i]);
    }
    free(node->children);
    if (node->kind == AST_BASIC_TYPE || node->kind == AST_ID) {
        free(node->type_name);
    } else if (node->kind == AST_TYPE_DECL) {
        free_ast(node->type_expr);
        free(node->decl_name);
    } else if (node->kind == AST_VAR_DECL) {
        free(node->var_type);
        free(node->var_name);
        if (node->init) free_ast(node->init);
    } else if (node->kind == AST_FIELD_ACCESS) {
        free_ast(node->base);
        free(node->field_name);
    } else if (node->kind == AST_ARRAY_INDEX) {
        free_ast(node->base);
        free_ast(node->index_expr);
    } else if (node->kind == AST_UNARY_EXPR || node->kind == AST_DEREF || node->kind == AST_ADDR_OF) {
        free_ast(node->child);
    } else if (node->kind == AST_BIN_EXPR) {
        free_ast(node->left);
        free_ast(node->right);
    } else if (node->kind == AST_CALL) {
        free(node->call_name);
    } else if (node->kind == AST_ALLOC) {
        free(node->alloc_type);
    } else if (node->kind == AST_ASSIGN) {
        free_ast(node->lhs);
        free_ast(node->rhs);
    } else if (node->kind == AST_IF) {
        free_ast(node->if_cond);
        free_ast(node->then_stmt);
        free_ast(node->else_stmt);
    } else if (node->kind == AST_WHILE) {
        free_ast(node->while_cond);
        free_ast(node->body);
    } else if (node->kind == AST_RETURN) {
        free_ast(node->ret_expr);
    } else if (node->kind == AST_FUNC_DECL) {
        free(node->ret_type);
        free(node->func_name);
    }
    free(node->token);
    free(node);
}


void print_ast(ast_t* node, int indent) {
    if (node == NULL) return;
    for (int i = 0; i < indent; i++) printf(" ");
    printf("Kind: %d\n", node->kind);
    if (node->type_name) {
        for (int i = 0; i < indent; i++) printf(" ");
        printf("Name: %s\n", node->type_name);
    } else if (node->kind == AST_VAR_DECL) {
        for (int i = 0; i < indent; i++) printf(" ");
        printf("Var: %s type %s\n", node->var_name, node->var_type);
        if (node->init) {
            print_ast(node->init, indent + 2);
        }
    } // Add more cases for print as needed
    for (int i = 0; i < node->child_count; i++) {
        print_ast(node->children[i], indent + 2);
    }
}
