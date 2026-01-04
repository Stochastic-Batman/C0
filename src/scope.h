#ifndef SCOPE_H
#define SCOPE_H

#include "parser.h"  // For decl_t, type_t, param_t


typedef enum {
    SYMBOL_VAR,
    SYMBOL_FUNC
} symbol_kind_t;


typedef struct symbol {
    symbol_kind_t kind;
    type_t* type;
    char* name;
    param_t* params;  // Only for functions
    struct symbol* next;
} symbol_t;


typedef struct scope {
    symbol_t* values;  // Linked list for value namespace
    symbol_t* types;   // Linked list for type namespace
    struct scope* parent;
} scope_t;


typedef struct env {
    scope_t* current;
    decl_t* current_func;  // Track current function for return type checks
} env_t;


// Create a new environment
env_t* new_env(void);


// Free the environment and all scopes
void free_env(env_t* env);


// Push a new scope onto the current
void push_scope(env_t* env);


// Pop the current scope
void pop_scope(env_t* env);


// Declare a symbol in the value namespace in the current scope
void declare_value(env_t* env, char* name, type_t* type, int is_func, param_t* params);  // Checks for duplicates in the current scope only


// Lookup a symbol in the value namespace, searching upwards through parents
symbol_t* lookup_value(env_t* env, const char* name);


// Declare a type in the type namespace in the current scope
void declare_type(env_t* env, char* name, type_t* type);  // Checks for duplicates in the current scope only


// Lookup a type in the type namespace, searching upwards through parents
type_t* lookup_type(env_t* env, const char* name);

#endif
