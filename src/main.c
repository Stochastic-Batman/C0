#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input.c0>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    scanner_init(fp);

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

    fclose(fp);
    return 0;
}
