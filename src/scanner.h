#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>   // for FILE
#include <stdbool.h> // for bool


typedef enum {
    TOKEN_EOF, // End of file
    TOKEN_IDENTIFIER,  // Name: letter followed by alphanums/_
    TOKEN_NUMBER,  // Digit sequence, optional 'u' suffix (positive only)
    TOKEN_CHAR,  // 'printable_ascii'
    // Keywords (separate for quick parser checks)
    TOKEN_INT, TOKEN_BOOL, TOKEN_CHAR_KW, TOKEN_UINT, // Types
    TOKEN_TRUE, TOKEN_FALSE,  // Bool constants
    TOKEN_NULL,  // null (pointer constant)
    TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE,  // Control
    TOKEN_RETURN,  // Return
    TOKEN_TYPEDEF, TOKEN_STRUCT, TOKEN_NEW,  // Types/alloc
    // Operators and punctuation
    TOKEN_PLUS,  // +
    TOKEN_MINUS,  // - (parser distinguishes unary/binary)
    TOKEN_STAR,  // * (multiplication only, since deref is @)
    TOKEN_DIV,  // /
    TOKEN_ASSIGN,  // =
    TOKEN_EQ,  // ==
    TOKEN_NEQ,  // !=
    TOKEN_LT,  // <
    TOKEN_GT,  // >
    TOKEN_LEQ,  // <=
    TOKEN_GEQ,  // >=
    TOKEN_AND,  // &&
    TOKEN_OR,  // ||
    TOKEN_NOT,  // !
    TOKEN_COMMA,  // ,
    TOKEN_SEMI,  // ;
    TOKEN_LBRACE,  // {
    TOKEN_RBRACE,  // }
    TOKEN_LPAREN,  // (
    TOKEN_RPAREN,  // )
    TOKEN_LBRACKET,  // [
    TOKEN_RBRACKET,  // ]
    TOKEN_DOT,  // .
    TOKEN_AMP,  // & (address-of)
    TOKEN_AT,  // @ (dereference)
    TOKEN_ERROR  // Invalid char/sequence
} token_type;


typedef union {
    int num_value;  // For TOKEN_NUMBER (parsed with strtol)
    char char_value;  // For TOKEN_CHAR
    bool bool_value;  // For TOKEN_TRUE/FALSE
} token_value;


typedef struct {
    token_type type;
    char *lexeme;  // Duplicated string of the token (must free after use)
    token_value value;  // Semantic value (num/char/bool)
    int line;  // Line number (starts at 1)
    int col;  // Column number (starts at 1)
} token_t;


// Initialize the scanner with input file
void scanner_init(FILE *fp);


// Get the next token (caller must free token->lexeme)
token_t *next_token();

#endif
