#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "IR.h"


static int temp_cnt = 0;
static int label_cnt = 0;

static void lower_stmt(stmt_t* s, ir_func_t* func, ir_instr_t** first, ir_instr_t** tail);
static char* lower_expr(expr_t* e, ir_func_t* func, ir_instr_t** first, ir_instr_t** tail);


static ir_instr_t* new_ir(ir_op_t op, char* dest, char* src1, char* src2, int imm) {
    ir_instr_t* i = malloc(sizeof(ir_instr_t));
    i->op = op;
    i->dest = dest ? strdup(dest) : NULL;
    i->src1 = src1 ? strdup(src1) : NULL;
    i->src2 = src2 ? strdup(src2) : NULL;
    i->imm = imm;
    i->next = NULL;
    return i;
}


static void append_ir(ir_instr_t** first, ir_instr_t** tail, ir_instr_t* instr) {
    if (*tail) {
        (*tail)->next = instr;
    } else {
        if (*first == NULL) *first = instr;
    }
    *tail = instr;
}


static char* new_temp(void) {
    char buf[16];
    snprintf(buf, sizeof(buf), "t%d", temp_cnt++);
    return strdup(buf);
}


static char* new_label(void) {
    char buf[16];
    snprintf(buf, sizeof(buf), "L%d", label_cnt++);
    return strdup(buf);
}


// Main lowering entry point
ir_program_t* lower_to_ir(decl_t* program_ast) {
    ir_program_t* ir = calloc(1, sizeof(ir_program_t));
    ir->globals = program_ast;  // keep globals/types as-is for now

    ir_func_t** func_tail = &ir->functions;

    for (decl_t* d = program_ast; d; d = d->next) {
        if (d->kind != DECL_FUNC) continue;

        ir_func_t* f = calloc(1, sizeof(ir_func_t));
        f->name = strdup(d->name);
        f->ret_type = d->type->subtype;
        f->params = d->type->params;

        ir_instr_t* body_head = NULL;
        ir_instr_t* body_tail = NULL;
        lower_stmt(d->code, f, &body_head, &body_tail);
        f->body = body_head;

        append_ir(&body_head, &body_tail, new_ir(IR_JR, NULL, "$ra", NULL, 0));

        *func_tail = f;
        func_tail = &f->next;
    }

    return ir;
}


// Lower statements
static void lower_stmt(stmt_t* s, ir_func_t* func, ir_instr_t** first, ir_instr_t** tail) {
    for (stmt_t* cur = s; cur; cur = cur->next_stmt) {
        switch (cur->kind) {
            case STMT_DECL: {
                // Local variable declaration - nothing to emit unless init
                if (cur->decl->value) {
                    char* val = lower_expr(cur->decl->value, func, first, tail);
                    append_ir(first, tail, new_ir(IR_MOVE, cur->decl->name, val, NULL, 0));
                }
                break;
            }
            case STMT_ASSIGN: {
                char* rhs = lower_expr(cur->cond, func, first, tail);  // rhs value
                char* lhs_addr = lower_expr(cur->init, func, first, tail);  // lvalue address
                append_ir(first, tail, new_ir(IR_SW, rhs, lhs_addr, NULL, 0));
                break;
            }
            case STMT_RETURN: {
                if (cur->cond) {
                    char* val = lower_expr(cur->cond, func, first, tail);
                    append_ir(first, tail, new_ir(IR_MOVE, "$v0", val, NULL, 0));
                }
                append_ir(first, tail, new_ir(IR_JR, NULL, "$ra", NULL, 0));
                break;
            }
            case STMT_IF: {
                char* cond = lower_expr(cur->cond, func, first, tail);
                char* else_l = new_label();
                char* end_l  = new_label();

                append_ir(first, tail, new_ir(IR_BEQ, cond, "$zero", else_l, 0));
                lower_stmt(cur->body, func, first, tail);
                append_ir(first, tail, new_ir(IR_J, end_l, NULL, NULL, 0));
                append_ir(first, tail, new_ir(IR_LABEL, else_l, NULL, NULL, 0));
                if (cur->else_body) lower_stmt(cur->else_body, func, first, tail);
                append_ir(first, tail, new_ir(IR_LABEL, end_l, NULL, NULL, 0));
                break;
            }
            case STMT_WHILE: {
                char* start = new_label();
                char* end   = new_label();

                append_ir(first, tail, new_ir(IR_LABEL, start, NULL, NULL, 0));
                char* cond = lower_expr(cur->cond, func, first, tail);
                append_ir(first, tail, new_ir(IR_BEQ, cond, "$zero", end, 0));
                lower_stmt(cur->body, func, first, tail);
                append_ir(first, tail, new_ir(IR_J, start, NULL, NULL, 0));
                append_ir(first, tail, new_ir(IR_LABEL, end, NULL, NULL, 0));
                break;
            }
            case STMT_BLOCK: {
                lower_stmt(cur->body, func, first, tail);
                break;
            }
            default:
                fprintf(stderr, "Unhandled stmt kind %d in IR lowering\n", cur->kind);
                exit(1);
        }
    }
}


// Lower expression - returns name of temp holding result
static char* lower_expr(expr_t* e, ir_func_t* func, ir_instr_t** first, ir_instr_t** tail) {
    if (!e) return NULL;

    switch (e->kind) {
        case EXPR_NUM: {
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_LI, t, NULL, NULL, e->num_val));  // Uses LUI/ORI if >16-bit
            return t;
        }
        case EXPR_CHAR: {
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_LI, t, NULL, NULL, (int)e->char_val));
            return t;
        }
        case EXPR_BOOL: {
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_LI, t, NULL, NULL, e->bool_val ? 1 : 0));
            return t;
        }
        case EXPR_NULL: {
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_LI, t, NULL, NULL, 0));  // Null as 0
            return t;
        }
        case EXPR_ID: {
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_LA, t, e->name, NULL, 0));  // Load address if global/var
            append_ir(first, tail, new_ir(IR_LW, t, t, NULL, 0));  // Then load value
            return t;
        }
        case EXPR_CALL: {
            // Params: push to stack or regs (simple: use SW to $sp offsets)
            expr_t* arg = e->left;
            int offset = 0;
            while (arg) {
                char* a = lower_expr(arg, func, first, tail);
                append_ir(first, tail, new_ir(IR_SW, a, "$sp", NULL, offset));
                offset -= 4;  // Stack grows down
                arg = arg->next;
            }
            append_ir(first, tail, new_ir(IR_JAL, e->name, NULL, NULL, 0));
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_MOVE, t, "$v0", NULL, 0));  // Return in $v0
            return t;
        }
        case EXPR_ADD: {
            char* l = lower_expr(e->left, func, first, tail);
            char* r = lower_expr(e->right, func, first, tail);
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_ADD, t, l, r, 0));  // or ADDU for unsigned
            return t;
        }
        case EXPR_SUB: {
            char* l = lower_expr(e->left, func, first, tail);
            char* r = lower_expr(e->right, func, first, tail);
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_SUB, t, l, r, 0));  // or SUBU
            return t;
        }
        case EXPR_MUL: {
            // Placeholder: call Paul's mult routine (e.g., JAL "mult")
            char* l = lower_expr(e->left, func, first, tail);
            char* r = lower_expr(e->right, func, first, tail);
            append_ir(first, tail, new_ir(IR_MOVE, "$a0", l, NULL, 0));
            append_ir(first, tail, new_ir(IR_MOVE, "$a1", r, NULL, 0));
            append_ir(first, tail, new_ir(IR_JAL, "mult", NULL, NULL, 0));  // Assume mult func
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_MOVE, t, "$v0", NULL, 0));
            return t;
        }
        case EXPR_DIV: {
            // Placeholder: call Paul's div routine
            char* l = lower_expr(e->left, func, first, tail);
            char* r = lower_expr(e->right, func, first, tail);
            append_ir(first, tail, new_ir(IR_MOVE, "$a0", l, NULL, 0));
            append_ir(first, tail, new_ir(IR_MOVE, "$a1", r, NULL, 0));
            append_ir(first, tail, new_ir(IR_JAL, "div", NULL, NULL, 0));
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_MOVE, t, "$v0", NULL, 0));
            return t;
        }
        case EXPR_AND: {
            char* l = lower_expr(e->left, func, first, tail);
            char* r = lower_expr(e->right, func, first, tail);
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_AND, t, l, r, 0));
            return t;
        }
        case EXPR_OR: {
            char* l = lower_expr(e->left, func, first, tail);
            char* r = lower_expr(e->right, func, first, tail);
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_OR, t, l, r, 0));
            return t;
        }
        case EXPR_EQ: {
            char* l = lower_expr(e->left, func, first, tail);
            char* r = lower_expr(e->right, func, first, tail);
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_SUB, t, l, r, 0));
            append_ir(first, tail, new_ir(IR_SLTIU, t, t, NULL, 1));  // t = (diff < 1) i.e. ==0
            append_ir(first, tail, new_ir(IR_XORI, t, t, NULL, 1));  // Invert for EQ
            return t;
        }
        case EXPR_NEQ: {
            char* l = lower_expr(e->left, func, first, tail);
            char* r = lower_expr(e->right, func, first, tail);
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_SUB, t, l, r, 0));
            append_ir(first, tail, new_ir(IR_SLTIU, t, t, NULL, 1));  // 1 if !=0
            return t;
        }
        case EXPR_LT: {
            char* l = lower_expr(e->left, func, first, tail);
            char* r = lower_expr(e->right, func, first, tail);
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_SLT, t, l, r, 0));
            return t;
        }
        case EXPR_GT: {
            char* l = lower_expr(e->left, func, first, tail);
            char* r = lower_expr(e->right, func, first, tail);
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_SLT, t, r, l, 0));  // Swap for GT
            return t;
        }
        case EXPR_LEQ: {
            char* l = lower_expr(e->left, func, first, tail);
            char* r = lower_expr(e->right, func, first, tail);
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_SLT, t, r, l, 0));  // ! (r < l)
            append_ir(first, tail, new_ir(IR_XORI, t, t, NULL, 1));
            return t;
        }
        case EXPR_GEQ: {
            char* l = lower_expr(e->left, func, first, tail);
            char* r = lower_expr(e->right, func, first, tail);
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_SLT, t, l, r, 0));  // ! (l < r)
            append_ir(first, tail, new_ir(IR_XORI, t, t, NULL, 1));
            return t;
        }
        case EXPR_NEG: {
            char* op = lower_expr(e->left, func, first, tail);
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_SUB, t, "$zero", op, 0));
            return t;
        }
        case EXPR_NOT: {
            char* op = lower_expr(e->left, func, first, tail);
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_XORI, t, op, NULL, 1));  // Flip bool
            return t;
        }
        case EXPR_ALLOC: {
            // For new T@, use syscall or heap routine (OS dev implements alloc via SYSC)
            // Placeholder: assume "malloc" routine
            append_ir(first, tail, new_ir(IR_LI, "$a0", NULL, NULL, 4));  // Size from type
            append_ir(first, tail, new_ir(IR_SYSC, NULL, NULL, NULL, 9));  // sbrk syscall code 9
            // Note: SYSC for alloc is to be implemented by OS developer
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_MOVE, t, "$v0", NULL, 0));
            return t;
        }
        case EXPR_FIELD: {
            char* base = lower_expr(e->left, func, first, tail);
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_ADDI, t, base, NULL, 0));  // Offset from type
            return t;
        }
        case EXPR_INDEX: {
            char* base = lower_expr(e->left, func, first, tail);
            char* idx = lower_expr(e->right, func, first, tail);
            char* scaled = new_temp();
            append_ir(first, tail, new_ir(IR_ADD, scaled, idx, idx, 0));  // *2
            char* scaled4 = new_temp();
            append_ir(first, tail, new_ir(IR_ADD, scaled4, scaled, scaled, 0));  // *4
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_ADD, t, base, scaled4, 0));
            return t;
        }
        case EXPR_DEREF: {
            char* ptr = lower_expr(e->left, func, first, tail);
            char* t = new_temp();
            append_ir(first, tail, new_ir(IR_LW, t, ptr, NULL, 0));
            return t;
        }
        case EXPR_ADDR: {
            char* val = lower_expr(e->left, func, first, tail);
            return val;  // Addr is base (use LA if global)
        }
        default:
            fprintf(stderr, "Unhandled expr kind %d\n", e->kind);
            exit(1);
    }
 
    return NULL;
}


// Debug print
void print_ir(const ir_program_t* ir) {
    printf(".data\n");
    // globals... (emit labels/init)
    for (decl_t* g = ir->globals; g; g = g->next) {
        if (g->kind == DECL_VAR) {
            printf("%s: .word 0\n", g->name);  // Simple init
        }
    }

    printf(".text\n");
    for (ir_func_t* f = ir->functions; f; f = f->next) {
        printf("%s:\n", f->name);
        for (ir_instr_t* i = f->body; i; i = i->next) {
            printf("  ");
            if (i->op == IR_LABEL) {
                printf("%s:", i->dest);
            } else {
                const char* mn = "";
                switch (i->op) {
                    case IR_LW: mn = "lw"; break;
                    case IR_SW: mn = "sw"; break;
                    case IR_ADDI: mn = "addi"; break;
                    case IR_ADDIU: mn = "addiu"; break;
                    case IR_SLTI: mn = "slti"; break;
                    case IR_SLTIU: mn = "sltiu"; break;
                    case IR_ANDI: mn = "andi"; break;
                    case IR_ORI: mn = "ori"; break;
                    case IR_XORI: mn = "xori"; break;
                    case IR_LUI: mn = "lui"; break;
                    case IR_ADD: mn = "add"; break;
                    case IR_ADDU: mn = "addu"; break;
                    case IR_SUB: mn = "sub"; break;
                    case IR_SUBU: mn = "subu"; break;
                    case IR_AND: mn = "and"; break;
                    case IR_OR: mn = "or"; break;
                    case IR_XOR: mn = "xor"; break;
                    case IR_NOR: mn = "nor"; break;
                    case IR_SLT: mn = "slt"; break;
                    case IR_SLTU: mn = "sltu"; break;
                    case IR_SRL: mn = "srl"; break;
                    case IR_BLTZ: mn = "bltz"; break;
                    case IR_BGEZ: mn = "bgez"; break;
                    case IR_BEQ: mn = "beq"; break;
                    case IR_BNE: mn = "bne"; break;
                    case IR_BLEZ: mn = "blez"; break;
                    case IR_BGTZ: mn = "bgtz"; break;
                    case IR_J: mn = "j"; break;
                    case IR_JAL: mn = "jal"; break;
                    case IR_JR: mn = "jr"; break;
                    case IR_JALR: mn = "jalr"; break;
                    case IR_SYSC: {
                        mn = "syscall";
                        // Note: SYSC is to be implemented by OS developer
                        break;
                    }
                    case IR_ERET: {
                        mn = "eret";
                        // Note: ERET is to be implemented by OS developer
                        break;
                    }
                    case IR_MOVG2S: {
                        mn = "movg2s";
                        // Note: MOVG2S is to be implemented by OS developer
                        break;
                    }
                    case IR_MOVS2G: {
                        mn = "movs2g";
                        // Note: MOVS2G is to be implemented by OS developer
                        break;
                    }
                    case IR_LI: mn = "li"; break;
                    case IR_LA: mn = "la"; break;
                    case IR_MOVE: mn = "move"; break;
                    case IR_NOP: mn = "nop"; break;
                    default: mn = "unknown";
                }
                printf("%s ", mn);
                if (i->dest) printf("%s, ", i->dest);
                if (i->src1) printf("%s, ", i->src1);
                if (i->src2) printf("%s", i->src2);
                else if (i->imm) printf("%d", i->imm);
            }
            printf("\n");
        }
    }
}


// Free
void free_ir(ir_program_t* ir) {
    ir_func_t* f = ir->functions;
    while (f) {
        ir_func_t* next_f = f->next;
        free(f->name);
        ir_instr_t* i = f->body;
        while (i) {
            ir_instr_t* next_i = i->next;
            free(i->dest);
            free(i->src1);
            free(i->src2);
            free(i);
            i = next_i;
        }
        free(f);
        f = next_f;
    }
    free(ir);
}
