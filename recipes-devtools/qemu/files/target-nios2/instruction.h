/*
 * Copyright (C) 2010 Tobias Klauser <tklauser@distanz.ch>
 * Copyright (C) 2010 chysun2000@gmail.com
 *  (Portions of this file that were originally from nios2sim-ng.)
 *
 * Copyright (C) 2012 Chris Wulff <crwulff@gmail.com>
 * Copyright (C) 2016 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/lgpl-2.1.html>
 */

#ifndef _INSTRUCTION_H_
#define _INSTRUCTION_H_

#include <stdint.h>
#include "cpu.h"
#include "tcg-op.h"

/*
 * Instruction Word Formats
 */

/* I-Type instruction */
typedef struct Nios2IType {
    uint32_t op:6;
    uint32_t imm16:16;
    uint32_t b:5;
    uint32_t a:5;
} QEMU_PACKED Nios2IType;

union i_type_u {
    uint32_t      v;
    Nios2IType i;
};

#define I_TYPE(instr, op) \
    union i_type_u instr_u = { .v = op }; \
    Nios2IType *instr = &instr_u.i

/* R-Type instruction */
typedef struct Nios2RType {
    uint32_t op:6;
    /*
     * Some R-Type instructions embed a small immediate value in the
     * low-order bits of OPX.
     */
    uint32_t imm5:5;
    uint32_t opx6:6;
    uint32_t c:5;
    uint32_t b:5;
    uint32_t a:5;
} QEMU_PACKED Nios2RType;

union r_type_u {
    uint32_t      v;
    Nios2RType i;
};

#define R_TYPE(instr, op) \
    union r_type_u instr_u = { .v = op }; \
    Nios2RType *instr = &instr_u.i

/* J-Type instruction */
typedef struct Nios2JType {
    uint32_t op:6;
    uint32_t imm26:26;
} QEMU_PACKED Nios2JType;

#define J_TYPE(instr, op) \
    Nios2JType *instr = (Nios2JType *) &op

/*
 * Instruction Opcodes
 */

/*
 * OP Encodings for I-Type instructions (except for CALL and JMPI, which are
 * J-type instructions)
 */
enum {
    CALL    = 0x00,  /* J-type */
    JMPI    = 0x01,  /* J-type */
           /* 0x02 */
    LDBU    = 0x03,
    ADDI    = 0x04,
    STB     = 0x05,
    BR      = 0x06,
    LDB     = 0x07,
    CMPGEI  = 0x08,
           /* 0x09 */
           /* 0x0A */
    LDHU    = 0x0B,
    ANDI    = 0x0C,
    STH     = 0x0D,
    BGE     = 0x0E,
    LDH     = 0x0F,
    CMPLTI  = 0x10,
           /* 0x11 */
           /* 0x12 */
    INITDA  = 0x13,
    ORI     = 0x14,
    STW     = 0x15,
    BLT     = 0x16,
    LDW     = 0x17,
    CMPNEI  = 0x18,
           /* 0x19 */
           /* 0x1A */
    FLUSHDA = 0x1B,
    XORI    = 0x1C,
           /* 0x1D */
    BNE     = 0x1E,
           /* 0x1F */
    CMPEQI  = 0x20,
           /* 0x21 */
           /* 0x22 */
    LDBUIO  = 0x23,
    MULI    = 0x24,
    STBIO   = 0x25,
    BEQ     = 0x26,
    LDBIO   = 0x27,
    CMPGEUI = 0x28,
           /* 0x29 */
           /* 0x2A */
    LDHUIO  = 0x2B,
    ANDHI   = 0x2C,
    STHIO   = 0x2D,
    BGEU    = 0x2E,
    LDHIO   = 0x2F,
    CMPLTUI = 0x30,
           /* 0x31 */
    CUSTOM  = 0x32,
    INITD   = 0x33,
    ORHI    = 0x34,
    STWIO   = 0x35,
    BLTU    = 0x36,
    LDWIO   = 0x37,
    RDPRS   = 0x38,
           /* 0x39 */
    R_TYPE  = 0x3A,
    FLUSHD  = 0x3B,
    XORHI   = 0x3C,
           /* 0x3D */
           /* 0x3E */
           /* 0x3F */
};
#define I_TYPE_COUNT  0x40

/* OPX Encodings for R-Type instructions */
enum {
          /* 0x00 */
    ERET   = 0x01,
    ROLI   = 0x02,
    ROL    = 0x03,
    FLUSHP = 0x04,
    RET    = 0x05,
    NOR    = 0x06,
    MULXUU = 0x07,
    CMPGE  = 0x08,
    BRET   = 0x09,
          /* 0x0A */
    ROR    = 0x0B,
    FLUSHI = 0x0C,
    JMP    = 0x0D,
    AND    = 0x0E,
          /* 0x0F */
    CMPLT  = 0x10,
          /* 0x11 */
    SLLI   = 0x12,
    SLL    = 0x13,
    WRPRS  = 0x14,
          /* 0x15 */
    OR     = 0x16,
    MULXSU = 0x17,
    CMPNE  = 0x18,
          /* 0x19 */
    SRLI   = 0x1A,
    SRL    = 0x1B,
    NEXTPC = 0x1C,
    CALLR  = 0x1D,
    XOR    = 0x1E,
    MULXSS = 0x1F,
    CMPEQ  = 0x20,
          /* 0x21 */
          /* 0x22 */
          /* 0x23 */
    DIVU   = 0x24,
    DIV    = 0x25,
    RDCTL  = 0x26,
    MUL    = 0x27,
    CMPGEU = 0x28,
    INITI  = 0x29,
          /* 0x2A */
          /* 0x2B */
          /* 0x2C */
    TRAP   = 0x2D,
    WRCTL  = 0x2E,
          /* 0x2F */
    CMPLTU = 0x30,
    ADD    = 0x31,
          /* 0x32 */
          /* 0x33 */
    BREAK  = 0x34,
          /* 0x35 */
    SYNC   = 0x36,
          /* 0x37 */
          /* 0x38 */
    SUB    = 0x39,
    SRAI   = 0x3A,
    SRA    = 0x3B,
          /* 0x3C */
          /* 0x3D */
          /* 0x3E */
          /* 0x3F */
};
#define R_TYPE_COUNT  0x40

/*
 * Return values for instruction handlers
 */
#define INSTR_UNIMPL     -2  /* Unimplemented instruction */
#define INSTR_ERR        -1  /* Error in instruction */
#define PC_INC_NORMAL     0  /* Normal PC increment after instruction */
#define PC_INC_BY_INSTR   1  /* PC got incremented by instruction */
#define INSTR_BREAK       2  /* Break encountered */
#define INSTR_EXCEPTION 255  /* Instruction generated an exception
                                (the exception cause will be stored
                                in struct nios2 */

#define EXCEPTION(cpu, cause)           \
    ({                                  \
        (cpu)->exception_cause = cause; \
        INSTR_EXCEPTION;                \
    })

typedef struct DisasContext {
    TCGv_ptr          cpu_env;
    TCGv             *cpu_R;
    int               is_jmp;
    target_ulong      pc;
    TranslationBlock *tb;
    int               mem_idx;
    bool              user;
} DisasContext;

#if defined(CONFIG_USER_ONLY)
#define IS_USER(s) true
#else
#define IS_USER(s) s->user
#endif

typedef void (*instruction_handler)(DisasContext *dc, uint32_t opcode);

typedef struct Nios2Instruction {
    const char         *name;
    instruction_handler handler;
} Nios2Instruction;

#define INSTRUCTION(name)    { stringify(name), name }
#define INSTRUCTION_NOP(name)    { stringify(name), nop }
#define INSTRUCTION_UNIMPLEMENTED(name)  { stringify(name), unimplemented }
#define INSTRUCTION_ILLEGAL()  { "", illegal_instruction }

extern void handle_instruction(DisasContext *dc, CPUNios2State *env);
extern const char *instruction_get_string(uint32_t code);

#define SIM_COMPAT 0
#define DISAS_GNU 1   /* Disassembly via GNU gdb derived routines */
#define DISAS_NIOS2 0 /* Disassembly via routines in instruction.c */
#if DISAS_NIOS2 && !SIM_COMPAT
#  define LOG_DIS(...) qemu_log_mask(CPU_LOG_TB_IN_ASM, ## __VA_ARGS__)
#else
#  define LOG_DIS(...) do { } while (0)
#endif

#endif /* _INSTRUCTION_H_ */
