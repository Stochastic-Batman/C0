#include <ctype.h>  // isalpha, isdigit, isalnum
#include <stdlib.h>  // malloc, free, strtol
#include <string.h>  // strdup, strcmp, strcpy
#include "scanner.h"


static FILE *input;
static int cur_line = 1;
static int cur_col = 1;


static token_type check_keyword(const char* str) {
    if (strcmp(str, "int") == 0) return TOKEN_INT;
    else if (strcmp(str, "bool") == 0) return TOKEN_BOOL;
    else if (strcmp(str, "char") == 0) return TOKEN_CHAR_KW;
    else if (strcmp(str, "uint") == 0) return TOKEN_UINT;
    else if (strcmp(str, "true") == 0) return TOKEN_TRUE;
    else if (strcmp(str, "false") == 0) return TOKEN_FALSE;
    else if (strcmp(str, "null") == 0) return TOKEN_NULL;
    else if (strcmp(str, "if") == 0) return TOKEN_IF;
    else if (strcmp(str, "else") == 0) return TOKEN_ELSE;
    else if (strcmp(str, "while") == 0) return TOKEN_WHILE;
    else if (strcmp(str, "return") == 0) return TOKEN_RETURN;
    else if (strcmp(str, "typedef") == 0) return TOKEN_TYPEDEF;
    else if (strcmp(str, "struct") == 0) return TOKEN_STRUCT;
    else if (strcmp(str, "new") == 0) return TOKEN_NEW;
    return TOKEN_IDENTIFIER;
}


void scanner_init(FILE *fp) {
    input = fp;
    cur_line = 1;
    cur_col = 1;
}


token_t* next_token() {
    token_t* token = malloc(sizeof(token_t));
    if (token == NULL) return NULL;
    token->lexeme = NULL;

    char buf[256];
    int idx = 0;
    int c;
    int start_line, start_col;

    while (true) {
        c = fgetc(input);

        if (isspace(c)) {
            if (c == '\n') {
                cur_line++;
                cur_col = 1;
            } else cur_col++;

            continue;
        } 

        if (c == EOF) {
            token->type = TOKEN_EOF;
            token->lexeme = NULL;
            token->line = cur_line;
            token->col = cur_col;
            return token;
        }

        start_line = cur_line;
        start_col = cur_col;
        cur_col++;

        if (isalpha(c) || c == '_') {
            buf[idx++] = c;
            while ((c = fgetc(input)) != EOF && (isalnum(c) || c == '_')) {
                buf[idx++] = c;
                cur_col++;
            }

            if (c != EOF) ungetc(c, input);

            buf[idx] = '\0';
            token->type = check_keyword(buf);
            token->lexeme = strdup(buf);

            if (token->type == TOKEN_TRUE) token->value.bool_value = true;
            else if (token->type == TOKEN_FALSE) token->value.bool_value = false;
            else if (token->type == TOKEN_NULL) token->value.num_value = 0;

            break;
        } else if (isdigit(c)) {
            buf[idx++] = c;
            while ((c = fgetc(input)) != EOF && isdigit(c)) {
                buf[idx++] = c;
                cur_col++;
            }

            if (c == 'u') {
                buf[idx++] = 'u';
                cur_col++;
            } else if (c != EOF) ungetc(c, input);

            buf[idx] = '\0';
            token->type = TOKEN_NUMBER;
            token->lexeme = strdup(buf);
            token->value.num_value = strtol(buf, NULL, 10);

            break;
        } else if (c == '\'') {
            int ch = fgetc(input);
            
            if (ch == EOF || ch < 32 || ch > 126) {
                token->type = TOKEN_ERROR;
                token->lexeme = strdup("invalid char");
            } else {
                cur_col++;
            
                if (fgetc(input) != '\'') {
                    token->type = TOKEN_ERROR;
                    token->lexeme = strdup("unclosed char");
                } else {
                    cur_col++;
                    token->type = TOKEN_CHAR;
                    token->value.char_value = ch;
                    token->lexeme = malloc(4);
                    sprintf(token->lexeme, "'%c'", ch);
                }
            }

            break;
        } else {
            switch (c) {
                case '+':
                    token->type = TOKEN_PLUS;
                    token->lexeme = strdup("+");
                    break;
                case '-':
                    token->type = TOKEN_MINUS;
                    token->lexeme = strdup("-");
                    break;
                case '*':
                    token->type = TOKEN_STAR;
                    token->lexeme = strdup("*");
                    break;
                case '/':
                    token->type = TOKEN_DIV;
                    token->lexeme = strdup("/");
                    break;
                case '=':
                    c = fgetc(input);
                    if (c == '=') {
                        token->type = TOKEN_EQ;
                        token->lexeme = strdup("==");
                        cur_col++;
                    } else {
                        if (c != EOF) ungetc(c, input);
                        token->type = TOKEN_ASSIGN;
                        token->lexeme = strdup("=");
                    }
                    break;
                case '!':
                    c = fgetc(input);
                    if (c == '=') {
                        token->type = TOKEN_NEQ;
                        token->lexeme = strdup("!=");
                        cur_col++;
                    } else {
                        if (c != EOF) ungetc(c, input);
                        token->type = TOKEN_NOT;
                        token->lexeme = strdup("!");
                    }
                    break;
                case '<':
                    c = fgetc(input);
                    if (c == '=') {
                        token->type = TOKEN_LEQ;
                        token->lexeme = strdup("<=");
                        cur_col++;
                    } else {
                        if (c != EOF) ungetc(c, input);
                        token->type = TOKEN_LT;
                        token->lexeme = strdup("<");
                    }
                    break;
                case '>':
                    c = fgetc(input);
                    if (c == '=') {
                        token->type = TOKEN_GEQ;
                        token->lexeme = strdup(">=");
                        cur_col++;
                    } else {
                        if (c != EOF) ungetc(c, input);
                        token->type = TOKEN_GT;
                        token->lexeme = strdup(">");
                    }
                    break;
                case '&':
                    c = fgetc(input);
                    if (c == '&') {
                        token->type = TOKEN_AND;
                        token->lexeme = strdup("&&");
                        cur_col++;
                    } else {
                        if (c != EOF) ungetc(c, input);
                        token->type = TOKEN_AMP;
                        token->lexeme = strdup("&");
                    }
                    break;
                case '|':
                    c = fgetc(input);
                    if (c == '|') {
                        token->type = TOKEN_OR;
                        token->lexeme = strdup("||");
                        cur_col++;
                    } else {
                        if (c != EOF) ungetc(c, input);
                        token->type = TOKEN_ERROR;
                        token->lexeme = strdup("|");
                    }
                    break;
                case ',':
                    token->type = TOKEN_COMMA;
                    token->lexeme = strdup(",");
                    break;
                case ';':
                    token->type = TOKEN_SEMI;
                    token->lexeme = strdup(";");
                    break;
                case '{':
                    token->type = TOKEN_LBRACE;
                    token->lexeme = strdup("{");
                    break;
                case '}':
                    token->type = TOKEN_RBRACE;
                    token->lexeme = strdup("}");
                    break;
                case '(':
                    token->type = TOKEN_LPAREN;
                    token->lexeme = strdup("(");
                    break;
                case ')':
                    token->type = TOKEN_RPAREN;
                    token->lexeme = strdup(")");
                    break;
                case '[':
                    token->type = TOKEN_LBRACKET;
                    token->lexeme = strdup("[");
                    break;
                case ']':
                    token->type = TOKEN_RBRACKET;
                    token->lexeme = strdup("]");
                    break;
                case '.':
                    token->type = TOKEN_DOT;
                    token->lexeme = strdup(".");
                    break;
                case '@':
                    token->type = TOKEN_AT;
                    token->lexeme = strdup("@");
                    break;
                default:
                    token->type = TOKEN_ERROR;
                    token->lexeme = malloc(2);
                    token->lexeme[0] = c;
                    token->lexeme[1] = '\0';
                    break;
            }

            break;
        }
    }

    token->line = start_line;
    token->col = start_col;

    return token;
}
