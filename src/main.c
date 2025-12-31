#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scanner.h"
#include "parser.h"


int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s [--scan] <input.c0>\n", argv[0]);
        return 1;
    }

    int arg_offset = 0;
    int scan_mode = 0;
    if (argc == 3 && strcmp(argv[1], "--scan") == 0) {
        scan_mode = 1;
        arg_offset = 1;
    }

    const char *filename = argv[1 + arg_offset];

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    scanner_init(fp);

    if (scan_mode) {
        token_t *token;

        while ((token = next_token())->type != TOKEN_EOF) {
            printf("Type: %d, Lexeme: %s, Line: %d, Col: %d", token->type, token->lexeme ? token->lexeme : "(none)", token->line, token->col);
            if (token->type == TOKEN_NUMBER) printf(", Value: %d", token->value.num_value);
            else if (token->type == TOKEN_CHAR) printf(", Value: '%c'", token->value.char_value);
            else if (token->type == TOKEN_TRUE || token->type == TOKEN_FALSE) printf(", Value: %s", token->value.bool_value ? "true" : "false");
            printf("\n");
            free(token->lexeme);
            free(token);
        }

        free(token);  // EOF token
    } else {
        ast_t *ast = parse_program(fp);
        printf("Parsed AST:\n");
        print_ast(ast, 0);
        free_ast(ast);
    }

    fclose(fp);

    return 0;
}
