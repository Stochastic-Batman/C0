#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scanner.h"
#include "parser.h"
#include "scope.h"
#include "semantic.h"
#include "IR.h"
#include "codegen.h"


int main(int argc, char** argv) {
    int scan_mode = 0;
    int parse_mode = 0;
    int semantic_mode = 0;
    int ir_mode = 0;
    int codegen_mode = 0;
    const char* input_file = NULL;
    const char* output_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--scan") == 0) {
            scan_mode = 1;
        } else if (strcmp(argv[i], "--parse") == 0) {
            parse_mode = 1;
        } else if (strcmp(argv[i], "--semantic") == 0) {
            semantic_mode = 1;
        } else if (strcmp(argv[i], "--IR") == 0) {
            ir_mode = 1;
        } else if (strcmp(argv[i], "--codegen") == 0) {
            codegen_mode = 1;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (++i < argc) {
                output_file = argv[i];
            } else {
                fprintf(stderr, "Missing output file after -o\n");
                return 1;
            }
        } else if (!input_file) {
            input_file = argv[i];
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            fprintf(stderr, "Usage: %s [--scan|--parse|--semantic|--IR|--codegen] <input.c0> [-o <output>]\n", argv[0]);
            return 1;
        }
    }

    if (!input_file) {
        fprintf(stderr, "Missing input file\n");
        fprintf(stderr, "Usage: %s [--scan|--parse|--semantic|--IR|--codegen] <input.c0> [-o <output>]\n", argv[0]);
        return 1;
    }

    if (!scan_mode && !parse_mode && !semantic_mode && !ir_mode && !codegen_mode) {
        codegen_mode = 1;  // Default to full compilation if no mode flags
    }

    FILE* fp = fopen(input_file, "r");
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
        semantic_analyze(program);  // Will exit if errors
        printf("Semantic analysis passed for %s\n", input_file);
        free_decl(program);
    } else if (ir_mode) {
        decl_t* program = parse_program(fp);
        semantic_analyze(program);  // Ensure semantics pass first
        ir_program_t* ir = lower_to_ir(program);
        print_ir(ir);
        free_ir(ir);
        free_decl(program);
    } else if (codegen_mode) {
        decl_t* program = parse_program(fp);
        semantic_analyze(program);  // Ensure semantics pass first
        ir_program_t* ir = lower_to_ir(program);

        FILE* out = stdout;
        if (output_file) {
            if (strcmp(output_file, "-") != 0) {
                out = fopen(output_file, "w");
                if (!out) {
                    perror("fopen output");
                    return 1;
                }
            }
        } else {
            // Compute default: strip .c0, add _MIPS.s
            char default_out[256];
            strcpy(default_out, input_file);
            char* dot = strrchr(default_out, '.');
            if (dot && strcmp(dot, ".c0") == 0) *dot = '\0';
            strcat(default_out, "_MIPS.s");
            out = fopen(default_out, "w");
            if (!out) {
                perror("fopen default output");
                return 1;
            }
        }

        gen_code(ir, out);

        if (out != stdout) fclose(out);
        free_ir(ir);
        free_decl(program);
    }

    fclose(fp);

    return 0;
}
