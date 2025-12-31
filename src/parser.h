#ifndef PARSER_H
#define PARSER_H

#include "scanner.h"


// AST node kinds (match grammar non-terminals loosely)
typedef enum {
    // Types
    AST_BASIC_TYPE,  // <Ty>: int/bool/char/uint/ID
    AST_TYPE_EXPR,  // <TE>: array/pointer/struct
    AST_STRUCT_DECL,
    AST_TYPE_DECL,

    // Declarations
    AST_VAR_DECL,  // <VaD>: type ID
    AST_VAR_DECL_SEQ,
    AST_TYPE_DECL_SEQ,
    AST_PARAM_DECL_SEQ,
    AST_LOCAL_DECLS,  // <locals>: local <VaDS> or empty

    // L-values and postfix
    AST_LVALUE,  // Base ID with postfix chain
    AST_ID,  // Simple identifier
    AST_FIELD_ACCESS,  // expr . ID
    AST_ARRAY_INDEX,  // expr [ expr ]
    AST_DEREF,
    AST_ADDR_OF,

    // Expressions
    AST_PRIMARY,  // ID (with tail), unary, (expr)
    AST_BIN_EXPR,  // left op right (arith/logic/rel)
    AST_UNARY_EXPR,  // op child (- or !)
    AST_CALL,  // ID ( params )
    AST_ALLOC,  // new ID @

    // Statements
    AST_ASSIGN,  // lvalue = rhs
    AST_IF,  // if expr { stmts } [else { stmts }]
    AST_WHILE,
    AST_RETURN,
    AST_STMT_SEQ,  // list of stmts

    // Functions
    AST_FUNC_DECL,  // type ID (params) { locals body }

    // Program
    AST_GLOBAL_VAR,  // Global var: type ID ;
    AST_PROGRAM  // <prog>: typedefs + globals (vars/funcs)
} ast_kind_t;


typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV,
    OP_AND, OP_OR,
    OP_EQ, OP_NEQ, OP_LT, OP_GT, OP_LEQ, OP_GEQ
} binop_t;


typedef enum {
    OP_NEG, OP_NOT, OP_DEREF, OP_ADDR
} unop_t;


// AST node (recursive, with union for variants)
typedef struct ast_node {
    ast_kind_t kind;
    token_t* token;  // Source token for loc/lexeme (copy lexeme if needed)
    struct ast_node** children;  // For sequences/lists/postfix chains
    int child_count;
    union {
    	char* type_name;  // Basic type: "int" etc. or ID name
    	int array_size;  // For [NUM], -1 if none
	
    	// Struct: children = fields (VAR_DECLs)
    	// Type decl: type_expr, name
    	struct { struct ast_node* type_expr; char* decl_name; };
    	// Var decl: type_name, var_name
    	struct { char* var_type; char* var_name; struct ast_node* init; };
    	// Lvalue/postfix: base + tail (chained as children[0] = base, children[1] = next postfix if chain)
    	struct ast_node* base;
    	// Field: field_name
    	char* field_name;
    	// Index: index_expr
    	struct ast_node* index_expr;
        // For Unary/Deref
        struct ast_node* child;
    	// Primary lit: value from token
    	int num_val;
    	char char_val;
    	bool bool_val;
    	// Bin expr: left, op, right
    	struct { struct ast_node* left; binop_t bin_op; struct ast_node* right; };
    	// Unary (uses ast_node* child above)
    	struct { unop_t un_op; };
    	// Call: func_name, params (children)
    	char* call_name;
    	// Alloc: alloc_type (ID)
    	char* alloc_type;
    	// Assign: lhs (lvalue), rhs (expr/alloc)
    	struct { struct ast_node* lhs; struct ast_node* rhs; };
    	// If: cond, then_stmt (stmt_seq), else_stmt (or NULL)
    	struct { struct ast_node* if_cond; struct ast_node* then_stmt; struct ast_node* else_stmt; };
    	// While: cond, body (stmt_seq)
    	struct { struct ast_node* while_cond; struct ast_node* body; };
    	// Return: expr
    	struct ast_node* ret_expr;
    	// Func decl: ret_type, name, params (children[0...param_count - 1]), locals (children[param_count...]), body (children[last])
    	struct { char* ret_type; char* func_name; int param_count; int local_count; };
    	// Program: type_decls (children[0...type_count-1]), globals (children[type_count...]) - vars and funcs mixed
    	int type_count;
    };
} ast_t;


// Parse the entire program from input file
ast_t *parse_program(FILE* fp);


// Should be recursive
void free_ast(ast_t* node);


// For Debuggin
void print_ast(ast_t* node, int indent);

#endif
