#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"


static int type_size(type_t* t) {
    if (!t) return 0;

    switch (t->kind) {
        case TYPE_INT:
        case TYPE_BOOL:
        case TYPE_CHAR:
        case TYPE_UINT:
        case TYPE_POINTER: return 4;
        case TYPE_ARRAY: return t->size * type_size(t->subtype);
        case TYPE_STRUCT: {
            int size = 0;
            for (param_t* f = t->params; f; f = f->next) {
                size += type_size(f->type);
            }
            return size;
        }
        default: return 4;
    }
}


static int count_locals(stmt_t* s) {
    int count = 0;
    for (stmt_t* cur = s; cur; cur = cur->next_stmt) {
        switch (cur->kind) {
            case STMT_DECL: count += type_size(cur->decl->type); break;
            case STMT_IF:
                count += count_locals(cur->body);
                if (cur->else_body) count += count_locals(cur->else_body);
                break;
            case STMT_WHILE: count += count_locals(cur->body); break;
            case STMT_BLOCK: count += count_locals(cur->body); break;
            default: break;  // No locals in assign/return
        }
    }
    return count;
}


static void gen_globals(decl_t* globals, FILE* out) {  // types & structs are not stored
    fprintf(out, ".data\n");  // Start data section
    for (decl_t* g = globals; g; g = g->next) {
        if (g->kind == DECL_VAR) {
            fprintf(out, "%s: .word ", g->name);  // Label
            if (g->value) {
                fprintf(out, "%d\n", g->value->num_val);  // Assume simple int init for now
            } else fprintf(out, "0\n");  // Default 0
        }
    }
    fprintf(out, ".bss\n");  // Uninitialized if needed
    // For now, all in .data
}


static void gen_prologue(ir_func_t* f, FILE* out) {
    int frame_size = 8 + count_locals(f->ast->code);  // ra + fp + locals
    frame_size = (frame_size + 3) & ~3;  // Cool trick to align to 4 bytes
    fprintf(out, "addiu $sp, $sp, -%d\n", frame_size);  // Alloc frame
    fprintf(out, "sw $ra, %d($sp)\n", frame_size - 4);  // Save ra
    fprintf(out, "sw $fp, %d($sp)\n", frame_size - 8);  // Save fp
    fprintf(out, "move $fp, $sp\n");  // Set fp
}


static void gen_epilogue(ir_func_t* f, FILE* out) {
    int frame_size = 8 + count_locals(f->ast->code);  // Match prologue
    frame_size = (frame_size + 3) & ~3;
    fprintf(out, "lw $ra, %d($sp)\n", frame_size - 4);  // Restore ra
    fprintf(out, "lw $fp, %d($sp)\n", frame_size - 8);  // Restore fp
    fprintf(out, "addiu $sp, $sp, %d\n", frame_size);  // Dealloc
    fprintf(out, "jr $ra\n");  // Return
}


static void gen_instr(ir_instr_t* i, FILE* out) {
    switch (i->op) {
        case IR_LW: {
          fprintf(out, "lw %s, %d(%s)\n", i->dest, i->imm, i->src1);
          break;
        }
        case IR_SW: {
          fprintf(out, "sw %s, %d(%s)\n", i->dest, i->imm, i->src1);
          break;
        }
        case IR_ADDI: {
          fprintf(out, "addi %s, %s, %d\n", i->dest, i->src1, i->imm);
          break;
        }
        case IR_ADDIU: {
          fprintf(out, "addiu %s, %s, %d\n", i->dest, i->src1, i->imm);
          break;
        }
        case IR_SLTI: {
          fprintf(out, "slti %s, %s, %d\n", i->dest, i->src1, i->imm);
          break;
        }
        case IR_SLTIU: {
          fprintf(out, "sltiu %s, %s, %d\n", i->dest, i->src1, i->imm);
          break;
        }
        case IR_ANDI: {
          fprintf(out, "andi %s, %s, %d\n", i->dest, i->src1, i->imm);
          break;
        }
        case IR_ORI: {
          fprintf(out, "ori %s, %s, %d\n", i->dest, i->src1, i->imm);
          break;
        }
        case IR_XORI: {
          fprintf(out, "xori %s, %s, %d\n", i->dest, i->src1, i->imm);
          break;
        }
        case IR_LUI: {
          fprintf(out, "lui %s, %d\n", i->dest, i->imm);
          break;
        }
        case IR_ADD: {
          fprintf(out, "add %s, %s, %s\n", i->dest, i->src1, i->src2);
          break;
        }
        case IR_ADDU: {
          fprintf(out, "addu %s, %s, %s\n", i->dest, i->src1, i->src2);
          break;
        }
        case IR_SUB: {
          fprintf(out, "sub %s, %s, %s\n", i->dest, i->src1, i->src2);
          break;
        }
        case IR_SUBU: {
          fprintf(out, "subu %s, %s, %s\n", i->dest, i->src1, i->src2);
          break;
        }
        case IR_AND: {
          fprintf(out, "and %s, %s, %s\n", i->dest, i->src1, i->src2);
          break;
        }
        case IR_OR: {
          fprintf(out, "or %s, %s, %s\n", i->dest, i->src1, i->src2);
          break;
        }
        case IR_XOR: {
          fprintf(out, "xor %s, %s, %s\n", i->dest, i->src1, i->src2);
          break;
        }
        case IR_NOR: {
          fprintf(out, "nor %s, %s, %s\n", i->dest, i->src1, i->src2);
          break;
        }
        case IR_SLT: {
          fprintf(out, "slt %s, %s, %s\n", i->dest, i->src1, i->src2);
          break;
        }
        case IR_SLTU: {
          fprintf(out, "sltu %s, %s, %s\n", i->dest, i->src1, i->src2);
          break;
        }
        case IR_SRL: {
          fprintf(out, "srl %s, %s, %d\n", i->dest, i->src1, i->imm);
          break;
        }
        case IR_BLTZ: {
          fprintf(out, "bltz %s, %s\n", i->src1, i->dest);
          break;
        }
        case IR_BGEZ: {
          fprintf(out, "bgez %s, %s\n", i->src1, i->dest);
          break;
        }
        case IR_BEQ: {
          fprintf(out, "beq %s, %s, %s\n", i->src1, i->src2, i->dest);
          break;
        }
        case IR_BNE: {
          fprintf(out, "bne %s, %s, %s\n", i->src1, i->src2, i->dest);
          break;
        }
        case IR_BLEZ: {
          fprintf(out, "blez %s, %s\n", i->src1, i->dest);
          break;
        }
        case IR_BGTZ: {
          fprintf(out, "bgtz %s, %s\n", i->src1, i->dest);
          break;
        }
        case IR_J: {
          fprintf(out, "j %s\n", i->dest);
          break;
        }
        case IR_JAL: {
          fprintf(out, "jal %s\n", i->dest);
          break;
        }
        case IR_JR: {
          fprintf(out, "jr %s\n", i->src1);
          break;
        }
        case IR_JALR: {
          fprintf(out, "jalr %s, %s\n", i->dest, i->src1);
          break;
        }
        case IR_SYSC: {
          fprintf(out, "syscall\n");
          break;
        }
        case IR_ERET: {
          fprintf(out, "eret\n");
          break;
        }
        case IR_MOVG2S: {
          fprintf(out, "movg2s %s, %s\n", i->dest, i->src1);
          break;
        }
        case IR_MOVS2G: {
          fprintf(out, "movs2g %s, %s\n", i->dest, i->src1);
          break;
        }
        case IR_LABEL: {
          fprintf(out, "%s:\n", i->dest);
          break;
        }
        case IR_LI: {
            if (i->imm >= -32768 && i->imm <= 32767) fprintf(out, "addi %s, $zero, %d\n", i->dest, i->imm);
            else {
                int hi = (i->imm >> 16) & 0xFFFF;  // 0xFFFF = 65535 = 2^16 - 1
                int lo = i->imm & 0xFFFF;
                fprintf(out, "lui %s, %d\n", i->dest, hi);
                fprintf(out, "ori %s, %s, %d\n", i->dest, i->dest, lo);
            }
            break;
        }
        case IR_LA: {
            fprintf(out, "lui %s, %%hi(%s)\n", i->dest, i->src1);
            fprintf(out, "addi %s, %s, %%lo(%s)\n", i->dest, i->dest, i->src1);
            break;
        }
        case IR_MOVE: {
          fprintf(out, "add %s, %s, $zero\n", i->dest, i->src1);
          break;
        }
        case IR_NOP: {
          fprintf(out, "nop\n");
          break;
        }
        default: {
          fprintf(out, "# unknown op %d\n", i->op);
          break;
        }
    }
}


void gen_code(ir_program_t* ir, FILE* out) {
    gen_globals(ir->globals, out);
    fprintf(out, ".text\n");

    for (ir_func_t* f = ir->functions; f; f = f->next) {
        fprintf(out, "%s:\n", f->name);
        gen_prologue(f, out);
        for (ir_instr_t* i = f->body; i; i = i->next) {
            gen_instr(i, out);
        }
        gen_epilogue(f, out);
    }
}
