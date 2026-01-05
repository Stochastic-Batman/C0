#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scanner.h"
#include "parser.h"
#include "scope.h"
#include "semantic.h"
#include "IR.h"


int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s [--scan|--parse|--semantic|--IR] <input.c0>\n", argv[0]);
        return 1;
    }

    int arg_offset = 0;
    int scan_mode = 0;
    int parse_mode = 0;
    int semantic_mode = 0;
    int ir_mode = 0;

    if (argc == 3) {
        if (strcmp(argv[1], "--scan") == 0) {
            scan_mode = 1;
        } else if (strcmp(argv[1], "--parse") == 0) {
            parse_mode = 1;
        } else if (strcmp(argv[1], "--semantic") == 0) {
            semantic_mode = 1;
        } else if (strcmp(argv[1], "--IR") == 0) {
            ir_mode = 1;
        } else {
            fprintf(stderr, "Unknown flag: %s\n", argv[1]);
            return 1;
        }
        arg_offset = 1;
    } else {
        parse_mode = 1;  // Default to parse if no flag
    }

    const char* filename = argv[1 + arg_offset];

    FILE* fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    scanner_init(fp);

    if (scan_mode) {
        token_t* token;
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
    } else if (parse_mode) {
        decl_t* program = parse_program(fp);
        printf("Parsed program:\n");
        print_decl(program, 0);
        free_decl(program);
    } else if (semantic_mode) {
        decl_t* program = parse_program(fp);
        // Optionally print AST if you want to inspect
        // printf("Parsed program:\n");
        // print_decl(program, 0);
        semantic_analyze(program);  // Will exit if errors
        printf("Semantic analysis passed for %s\n", filename);
        free_decl(program);
    } else if (ir_mode) {
        decl_t* program = parse_program(fp);
        semantic_analyze(program);  // Ensure semantics pass first
        ir_program_t* ir = lower_to_ir(program);
        print_ir(ir);
        free_ir(ir);
        free_decl(program);
    }

    fclose(fp);

    return 0;
}
