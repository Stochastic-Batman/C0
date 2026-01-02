#ifndef PARSER_H
#define PARSER_H

#include "scanner.h"  // token_t


typedef struct decl decl_t;
typedef struct stmt stmt_t;
typedef struct expr expr_t;
typedef struct type type_t;
typedef struct param param_t;


typedef enum {
    DECL_VAR,  // int x = ...
    DECL_FUNC,  // function definition
    DECL_TYPE,  // typedef
    DECL_STRUCT
} decl_kind_t;


// Decl struct (name, type, value expr/init, code stmt/body, next)
struct decl {
    decl_kind_t kind;
    char* name;
    type_t* type;
    expr_t* value;  // Init for variables, NULL for functions
    stmt_t* code;  // Body for functions
    decl_t* next;
};


// Statement kinds
typedef enum {
    STMT_ASSIGN,
    STMT_IF,
    STMT_WHILE,
    STMT_RETURN,
    STMT_BLOCK,  // { stmts }
    STMT_DECL  // local decl (var/type)
} stmt_kind_t;


// Stmt struct (kind, decl for local, expr/cond/init/next, body/else, next)
struct stmt {
    stmt_kind_t kind;
    decl_t* decl;  // For local decl
    expr_t* init;  // Unused in C0 (we don't have a for loop)
    expr_t* cond;
    expr_t* next;  // Unused in C0 (we don't have a for loop)
    stmt_t* body;
    stmt_t* else_body;
    stmt_t* next_stmt;
};


// Expr kinds (C0: binary/unary/literal/id/call/alloc/postfix like field/index/deref/addr)
typedef enum {
    EXPR_ADD, EXPR_SUB, EXPR_MUL, EXPR_DIV,
    EXPR_AND, EXPR_OR,
    EXPR_EQ, EXPR_NEQ, EXPR_LT, EXPR_GT, EXPR_LEQ, EXPR_GEQ,
    EXPR_NEG, EXPR_NOT,
    EXPR_ID,
    EXPR_NUM, EXPR_CHAR, EXPR_BOOL, EXPR_NULL,
    EXPR_CALL,
    EXPR_ALLOC,  // new ID@
    EXPR_FIELD,  // . ID
    EXPR_INDEX,  // [ expr ]
    EXPR_DEREF,  // @
    EXPR_ADDR  // &
} expr_kind_t;


// Expr struct (kind, left/right for bin/postfix, name/value for leaf)
struct expr {
    expr_kind_t kind;
    expr_t* left;
    expr_t* right;
    char* name;  // ID/field
    int num_val;
    char char_val;
    bool bool_val;
    expr_t* next;
};


// Type kinds (C0: primitive/array/pointer/struct/function)
typedef enum {
    TYPE_INT, TYPE_BOOL, TYPE_CHAR, TYPE_UINT,
    TYPE_STRUCT,
    TYPE_ARRAY,
    TYPE_POINTER,
    TYPE_FUNC,
    TYPE_NAMED
} type_kind_t;


// Type struct (kind, subtype for compound, params for function)
struct type {
    type_kind_t kind;
    type_t* subtype;  // Array element/pointer to/function return
    param_t* params;  // Function params
    char* name;
    int size;
};


// Param (name, type, next for list)
struct param {
    char* name;
    type_t* type;
    param_t* next;
};


// Factories (malloc/init)
decl_t* decl_create(decl_kind_t kind, char* name, type_t* type, expr_t* value, stmt_t* code, decl_t* next);
stmt_t* stmt_create(stmt_kind_t kind, decl_t* decl, expr_t* init, expr_t* cond, expr_t* next, stmt_t* body, stmt_t* else_body, stmt_t* next_stmt);
expr_t* expr_create(expr_kind_t kind, expr_t* left, expr_t* right);
expr_t* expr_create_id(char* name);
expr_t* expr_create_num(int val);
expr_t* expr_create_char(char val);
expr_t* expr_create_bool(bool val);
expr_t* expr_create_null(void);
type_t* type_create(type_kind_t kind, type_t* subtype, param_t* params);
param_t* param_create(char* name, type_t* type, param_t* next);


// Parse program (returns decl head)
decl_t* parse_program(FILE* fp);


// Free
void free_decl(decl_t* d);
void free_stmt(stmt_t* s);
void free_expr(expr_t* e);
void free_type(type_t* t);
void free_param(param_t* p);


// For debugging
void print_decl(decl_t* d, int indent);
void print_stmt(stmt_t* s, int indent);
void print_expr(expr_t* e, int indent);
void print_type(type_t* t, int indent);
void print_param(param_t* p, int indent);

#endif
