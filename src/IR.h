#ifndef IR_H
#define IR_H

#include <stdio.h>
#include "parser.h"

// IR opcodes
typedef enum {
    // Data Transfer
    IR_LW,  // lw rt, imm(rs)
    IR_SW,  // sw rt, imm(rs)

    // Arithmetic / Logical (I-type)
    IR_ADDI,  // addi rt, rs, imm
    IR_ADDIU,  // addiu rt, rs, imm
    IR_SLTI,  // slti rt, rs, imm
    IR_SLTIU,  // sltiu rt, rs, imm
    IR_ANDI,  // andi rt, rs, imm
    IR_ORI,  // ori rt, rs, imm
    IR_XORI,  // xori rt, rs, imm
    IR_LUI,  // lui rt, imm

    // Arithmetic / Logical (R-type)
    IR_ADD,  // add rd, rs, rt
    IR_ADDU,  // addu rd, rs, rt
    IR_SUB,  // sub rd, rs, rt
    IR_SUBU,  // subu rd, rs, rt
    IR_AND,  // and rd, rs, rt
    IR_OR,  // or rd, rs, rt
    IR_XOR,  // xor rd, rs, rt
    IR_NOR,  // nor rd, rs, rt
    IR_SLT,  // slt rd, rs, rt
    IR_SLTU,  // sltu rd, rs, rt

    // Shift (yeah, we only have one shift, though I think we designed a circuit for left shift or arithmetic right shift in operating systems class)
    IR_SRL,  // srl rd, rt, sa

    // Branches (I-type)
    IR_BLTZ,  // bltz rs, label
    IR_BGEZ,  // bgez rs, label
    IR_BEQ,  // beq rs, rt, label
    IR_BNE,  // bne rs, rt, label
    IR_BLEZ,  // blez rs, label
    IR_BGTZ,  // bgtz rs, label

    // Jumps
    IR_J,  // j label
    IR_JAL,  // jal label
    IR_JR,  // jr rs
    IR_JALR,  // jalr rd, rs

    // System / Coprocessor
    IR_SYSC,  // sysc
    IR_ERET,  // eret
    IR_MOVG2S,  // movg2s rd, rt   (coprocessor move)
    IR_MOVS2G,  // movs2g rd, rt

    // Pseudo / Helper (not real MIPS, but useful for lowering)
    IR_LABEL,  // label:
    IR_LI,  // li rt, imm      (pseudo: load immediate)
    IR_LA,  // la rt, label    (pseudo: load address)
    IR_MOVE,  // move rd, rs     (pseudo)
    IR_NOP  // nop
} ir_op_t;


typedef struct ir_instr {
    ir_op_t op;
    char* dest;  // rd / rt / label name
    char* src1;  // rs / rt / base
    char* src2;  // rt / imm (as string for labels) / sa
    int imm;  // immediate value (used when src2 is NULL)
    struct ir_instr* next;
} ir_instr_t;


// Function IR
typedef struct ir_func {
    char* name;
    type_t* ret_type;
    param_t* params;
    ir_instr_t* body;
    struct ir_func* next;
} ir_func_t;


// Whole program IR
typedef struct {
    decl_t* globals;     // Global variables / types (kept from AST)
    ir_func_t* functions;
} ir_program_t;


// Functions
ir_program_t* lower_to_ir(decl_t* program_ast);

void print_ir(const ir_program_t* ir);

void free_ir(ir_program_t* ir);

#endif
