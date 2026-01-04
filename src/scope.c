#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "scope.h"


static scope_t* new_scope(scope_t* parent) {
    scope_t* s = malloc(sizeof(scope_t));
    if (!s) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    s->values = NULL;
    s->types = NULL;
    s->parent = parent;
    return s;
}


env_t* new_env(void) {
    env_t* env = malloc(sizeof(env_t));
    if (!env) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    env->current = new_scope(NULL);
    env->current_func = NULL;
    return env;
}


void free_env(env_t* env) {
    while (env->current) {
        pop_scope(env);
    }
    free(env);
}


void push_scope(env_t* env) {
    env->current = new_scope(env->current);
}


void pop_scope(env_t* env) {
    scope_t* old = env->current;
    env->current = old->parent;

    symbol_t* v = old->values;
    while (v) {
        symbol_t* next = v->next;
        free(v->name);
        free(v);  // Must not free type/params here; they are owned by the AST
        v = next;
    }

    symbol_t* t = old->types;
    while (t) {
        symbol_t* next = t->next;
        free(t->name);
        free(t);  // Must not free type here; owned by AST
        t = next;
    }

    free(old);
}


static symbol_t* new_symbol(const char* name) {
    symbol_t* sym = malloc(sizeof(symbol_t));
    if (!sym) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    sym->name = strdup(name);
    sym->type = NULL;
    sym->params = NULL;
    sym->kind = SYMBOL_VAR;
    sym->next = NULL;
    return sym;
}


static int name_exists_in_list(symbol_t* list, const char* name) {
    for (symbol_t* cur = list; cur; cur = cur->next) {
        if (strcmp(cur->name, name) == 0) {
            return 1;
        }
    }
    return 0;
}


void declare_value(env_t* env, char* name, type_t* type, int is_func, param_t* params) {
    if (name_exists_in_list(env->current->values, name)) {
        fprintf(stderr, "semantic error: duplicate value declaration '%s'\n", name);
        exit(1);
    }
    symbol_t* sym = new_symbol(name);
    sym->kind = is_func ? SYMBOL_FUNC : SYMBOL_VAR;
    sym->type = type;
    sym->params = params;
    sym->next = env->current->values;
    env->current->values = sym;
}


symbol_t* lookup_value(env_t* env, const char* name) {
    for (scope_t* s = env->current; s; s = s->parent) {
        for (symbol_t* sym = s->values; sym; sym = sym->next) {
            if (strcmp(sym->name, name) == 0) {
                return sym;
            }
        }
    }
    return NULL;
}


void declare_type(env_t* env, char* name, type_t* type) {
    if (name_exists_in_list(env->current->types, name)) {
        fprintf(stderr, "semantic error: duplicate type declaration '%s'\n", name);
        exit(1);
    }
    symbol_t* sym = new_symbol(name);
    sym->type = type;
    sym->next = env->current->types;
    env->current->types = sym;
}


type_t* lookup_type(env_t* env, const char* name) {
    for (scope_t* s = env->current; s; s = s->parent) {
        for (symbol_t* sym = s->types; sym; sym = sym->next) {
            if (strcmp(sym->name, name) == 0) {
                return sym->type;
            }
        }
    }
    return NULL;
}
