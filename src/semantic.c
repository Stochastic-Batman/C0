#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic.h"
#include "scope.h"


// Forward declarations for recursive functions
static void declare_decls(env_t* env, decl_t* d);
static void resolve_decls(env_t* env, decl_t* d);
static type_t* resolve_expr(env_t* env, expr_t* e);
static void resolve_stmt(env_t* env, stmt_t* s);
static void resolve_type(env_t* env, type_t* t);
static int type_equal(env_t* env, type_t* a, type_t* b);


static type_t* get_base_type(env_t* env, type_t* t) {
    while (t && t->kind == TYPE_NAMED) {
        t = lookup_type(env, t->name);
        if (!t) {
            fprintf(stderr, "semantic error: undefined type '%s'\n", t->name);
            exit(1);
        }
    }
    return t;
}


void semantic_analyze(decl_t* program) {
    env_t* env = new_env();
    declare_decls(env, program);  // First pass: declare all top-level names
    resolve_decls(env, program);  // Second pass: resolve contents
    free_env(env);
}


static void declare_decls(env_t* env, decl_t* d) {
    for (decl_t* cur = d; cur; cur = cur->next) {
        switch (cur->kind) {
            case DECL_TYPE:
            case DECL_STRUCT:
                declare_type(env, cur->name, cur->type);
                break;
            case DECL_VAR:
            case DECL_FUNC:
                declare_value(env, cur->name, cur->type, (cur->kind == DECL_FUNC), (cur->kind == DECL_FUNC ? cur->type->params : NULL));
                break;
        }
    }
}


static void resolve_decls(env_t* env, decl_t* d) {
    for (decl_t* cur = d; cur; cur = cur->next) {
        switch (cur->kind) {
            case DECL_TYPE:
            case DECL_STRUCT:
                resolve_type(env, cur->type);
                break;
            case DECL_VAR:
                resolve_type(env, cur->type);
                if (cur->value) {
                    type_t* vt = resolve_expr(env, cur->value);
                    if (!type_equal(env, vt, cur->type)) {
                        fprintf(stderr, "semantic error: type mismatch in variable initialization\n");
                        exit(1);
                    }
                }
                break;
            case DECL_FUNC:
                resolve_type(env, cur->type);
                env->current_func = cur;
                push_scope(env);
                for (param_t* p = cur->type->params; p; p = p->next) {  // Declare parameters in function scope
                    declare_value(env, p->name, p->type, 0, NULL);
                }
                resolve_stmt(env, cur->code);
                pop_scope(env);
                env->current_func = NULL;
                break;
        }
    }
}


static type_t* resolve_expr(env_t* env, expr_t* e) {
    if (!e) return NULL;

    switch (e->kind) {
        case EXPR_ID:
            {
                symbol_t* sym = lookup_value(env, e->name);
                if (!sym) {
                    fprintf(stderr, "semantic error: undefined identifier '%s'\n", e->name);
                    exit(1);
                }
                if (sym->kind != SYMBOL_VAR) {
                    fprintf(stderr, "semantic error: '%s' is not a variable\n", e->name);
                    exit(1);
                }
                return sym->type;
            }
        case EXPR_NUM:
            // Assume int type for numbers (adjust if uint based on suffix, but simplified)
            return type_create(TYPE_INT, NULL, NULL);
        case EXPR_CHAR:
            return type_create(TYPE_CHAR, NULL, NULL);
        case EXPR_BOOL:
            return type_create(TYPE_BOOL, NULL, NULL);
        case EXPR_NULL:
            // Null is pointer type, but in C0 it's compatible with any pointer
            return type_create(TYPE_POINTER, NULL, NULL);  // Void pointer-like
        case EXPR_CALL:
            {
                symbol_t* sym = lookup_value(env, e->name);
                if (!sym || sym->kind != SYMBOL_FUNC) {
                    fprintf(stderr, "semantic error: '%s' is not a function\n", e->name);
                    exit(1);
                }
                // Check argument types match parameters
                expr_t* arg = e->left;
                param_t* par = sym->params;
                while (arg && par) {
                    type_t* at = resolve_expr(env, arg);
                    if (!type_equal(env, at, par->type)) {
                        fprintf(stderr, "semantic error: argument type mismatch\n");
                        exit(1);
                    }
                    arg = arg->next;
                    par = par->next;
                }
                if (arg || par) {
                    fprintf(stderr, "semantic error: incorrect number of arguments\n");
                    exit(1);
                }
                return sym->type->subtype;  // Return type
            }
        case EXPR_ADD:
        case EXPR_SUB:
        case EXPR_MUL:
        case EXPR_DIV:
            {
                type_t* lt = resolve_expr(env, e->left);
                type_t* rt = resolve_expr(env, e->right);
                if (!type_equal(env, lt, rt) || (lt->kind != TYPE_INT && lt->kind != TYPE_UINT)) {
                    fprintf(stderr, "semantic error: type mismatch in arithmetic operation\n");
                    exit(1);
                }
                return lt;
            }
        case EXPR_AND:
        case EXPR_OR:
            {
                type_t* lt = resolve_expr(env, e->left);
                type_t* rt = resolve_expr(env, e->right);
                if (lt->kind != TYPE_BOOL || rt->kind != TYPE_BOOL) {
                    fprintf(stderr, "semantic error: logical operation requires bool types\n");
                    exit(1);
                }
                return type_create(TYPE_BOOL, NULL, NULL);
            }
        case EXPR_EQ:
        case EXPR_NEQ:
        case EXPR_LT:
        case EXPR_GT:
        case EXPR_LEQ:
        case EXPR_GEQ:
            {
                type_t* lt = resolve_expr(env, e->left);
                type_t* rt = resolve_expr(env, e->right);
                if (!type_equal(env, lt, rt)) {
                    fprintf(stderr, "semantic error: type mismatch in comparison\n");
                    exit(1);
                }
                return type_create(TYPE_BOOL, NULL, NULL);
            }
        case EXPR_NEG:
        case EXPR_NOT:
            {
                type_t* lt = resolve_expr(env, e->left);
                if ((e->kind == EXPR_NEG && (lt->kind != TYPE_INT && lt->kind != TYPE_UINT)) || (e->kind == EXPR_NOT && lt->kind != TYPE_BOOL)) {
                    fprintf(stderr, "semantic error: invalid type for unary operator\n");
                    exit(1);
                }
                return lt;
            }
        case EXPR_ALLOC:
            {
                // new ID@ allocates pointer to ID
                type_t* base = lookup_type(env, e->name);
                if (!base) {
                    fprintf(stderr, "semantic error: undefined type '%s' in allocation\n", e->name);
                    exit(1);
                }
                return type_create(TYPE_POINTER, base, NULL);
            }
        case EXPR_FIELD:
            {
                type_t* lt = resolve_expr(env, e->left);
                type_t* base = get_base_type(env, lt);
                if (base->kind != TYPE_STRUCT) {
                    fprintf(stderr, "semantic error: field access on non-struct\n");
                    exit(1);
                }
                // Find field by name (e->name) in struct params (fields)
                param_t* field = base->params;
                while (field) {
                    if (strcmp(field->name, e->name) == 0) {
                        return field->type;
                    }
                    field = field->next;
                }
                fprintf(stderr, "semantic error: undefined field '%s'\n", e->name);
                exit(1);
            }
        case EXPR_INDEX:
            {
                type_t* t = resolve_expr(env, e->left);
                type_t* base = get_base_type(env, t);
                if (base->kind != TYPE_ARRAY) {
                    fprintf(stderr, "semantic error: invalid types in array index\n");
                    exit(1);
                }
                type_t* it = resolve_expr(env, e->right);
                if (it->kind != TYPE_INT && it->kind != TYPE_UINT) {
                    fprintf(stderr, "semantic error: array index must be integer\n");
                    exit(1);
                }
                return base->subtype;
            }
        case EXPR_DEREF:
            {
                type_t* pt = resolve_expr(env, e->left);
                type_t* base = get_base_type(env, pt);
                if (base->kind != TYPE_POINTER) {
                    fprintf(stderr, "semantic error: dereference on non-pointer\n");
                    exit(1);
                }
                return base->subtype;
            }
        case EXPR_ADDR:
            {
                type_t* vt = resolve_expr(env, e->left);
                return type_create(TYPE_POINTER, vt, NULL);
            }
        default:
            fprintf(stderr, "semantic error: unhandled expression kind\n");
            exit(1);
    }
    return NULL;  // Unreachable
}


static void resolve_stmt(env_t* env, stmt_t* s) {
    for (stmt_t* cur = s; cur; cur = cur->next_stmt) {
        switch (cur->kind) {
            case STMT_DECL:
                // Local declarations: declare then resolve
                declare_decls(env, cur->decl);
                resolve_decls(env, cur->decl);
                break;
            case STMT_ASSIGN:
                {
                    type_t* lt = resolve_expr(env, cur->init); // Left (lvalue)
                    type_t* rt = resolve_expr(env, cur->cond); // Right (rhs)
                    if (!type_equal(env, lt, rt)) {
                        fprintf(stderr, "semantic error: type mismatch in assignment\n");
                        exit(1);
                    }
                    // Check init is lvalue (e.g., ID, field, index, deref)
                    bool is_lval = false;
                    switch (cur->init->kind) {
                        case EXPR_ID:
                        case EXPR_FIELD:
                        case EXPR_INDEX:
                        case EXPR_DEREF:
                            is_lval = true;
                            break;
                        default:
                            is_lval = false;
                            break;
                    }
                    if (!is_lval) {
                        fprintf(stderr, "semantic error: assignment to non-lvalue\n");
                        exit(1);
                    }
                }
                break;
            case STMT_IF:
                {
                    type_t* ct = resolve_expr(env, cur->cond);
                    if (ct->kind != TYPE_BOOL) {
                        fprintf(stderr, "semantic error: if condition must be bool\n");
                        exit(1);
                    }
                    resolve_stmt(env, cur->body);
                    if (cur->else_body) {
                        resolve_stmt(env, cur->else_body);
                    }
                }
                break;
            case STMT_WHILE:
                {
                    type_t* ct = resolve_expr(env, cur->cond);
                    if (ct->kind != TYPE_BOOL) {
                        fprintf(stderr, "semantic error: while condition must be bool\n");
                        exit(1);
                    }
                    resolve_stmt(env, cur->body);
                }
                break;
            case STMT_RETURN:
                {
                    type_t* rt = resolve_expr(env, cur->cond); // Return expr
                    if (!env->current_func) {
                        fprintf(stderr, "semantic error: return outside function\n");
                        exit(1);
                    }
                    type_t* ft = env->current_func->type->subtype; // Function return type
                    if (!type_equal(env, rt, ft)) {
                        fprintf(stderr, "semantic error: return type mismatch\n");
                        exit(1);
                    }
                }
                break;
            case STMT_BLOCK:
                push_scope(env);
                resolve_stmt(env, cur->body);
                pop_scope(env);
                break;
            default:
                fprintf(stderr, "semantic error: unhandled statement kind\n");
                exit(1);
        }
    }
}


static void resolve_type(env_t* env, type_t* t) {
    if (!t) return;

    switch (t->kind) {
        case TYPE_NAMED:
            {
                type_t* base = lookup_type(env, t->name);
                if (!base) {
                    fprintf(stderr, "semantic error: undefined type '%s'\n", t->name);
                    exit(1);
                }
            }
            break;
        case TYPE_ARRAY:
            resolve_type(env, t->subtype);
            // Size is already set from parse (NUM)
            if (t->size <= 0) {
                fprintf(stderr, "semantic error: invalid array size\n");
                exit(1);
            }
            break;
        case TYPE_POINTER:
            resolve_type(env, t->subtype);
            break;
        case TYPE_FUNC:
            resolve_type(env, t->subtype);  // Return type
            for (param_t* p = t->params; p; p = p->next) {
                resolve_type(env, p->type);
            }
            break;
        case TYPE_STRUCT:
            // Fields are params; declare them in a temp scope to check duplicates
            push_scope(env);
            for (param_t* f = t->params; f; f = f->next) {
                resolve_type(env, f->type);
                declare_value(env, f->name, f->type, 0, NULL);  // Check no duplicate fields
            }
            pop_scope(env);
            break;
        // Primitives need no resolution
        default:
            break;
    }
}


static int type_equal(env_t* env, type_t* a, type_t* b) {
    if (!a || !b) return a == b;
    a = get_base_type(env, a);
    b = get_base_type(env, b);
    if (a->kind != b->kind) return 0;

    switch (a->kind) {
        case TYPE_INT:
        case TYPE_BOOL:
        case TYPE_CHAR:
        case TYPE_UINT:
            return 1;
        case TYPE_ARRAY:
            return a->size == b->size && type_equal(env, a->subtype, b->subtype);
        case TYPE_POINTER:
            return type_equal(env, a->subtype, b->subtype);
        case TYPE_FUNC:
            if (!type_equal(env, a->subtype, b->subtype)) return 0;
            param_t* pa = a->params;
            param_t* pb = b->params;
            while (pa && pb) {
                if (!type_equal(env, pa->type, pb->type)) return 0;
                pa = pa->next;
                pb = pb->next;
            }
            return pa == NULL && pb == NULL;
        case TYPE_STRUCT:
            // Compare fields (assume order matters, deep equal)
            param_t* fa = a->params;
            param_t* fb = b->params;
            while (fa && fb) {
                if (strcmp(fa->name, fb->name) != 0 || !type_equal(env, fa->type, fb->type)) return 0;
                fa = fa->next;
                fb = fb->next;
            }
            return fa == NULL && fb == NULL;
        case TYPE_NAMED:
            // For named types, equality after resolution (but simplified: name match)
            return strcmp(a->name, b->name) == 0;
        default:
            return 0;
    }
}
