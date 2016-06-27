/*
 * Copyright (C) 2010 Tobias Klauser <tklauser@distanz.ch>
 *  (Portions of this file that were originally from nios2sim-ng.)
 *
 * Copyright (C) 2012 Chris Wulff <crwulff@gmail.com>
 * Copyright (c) 2016 Intel Corporation.
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

#include <stdio.h>

#include "instruction.h"
#include "exec/exec-all.h"
#include "exec/helper-proto.h"
#include "exec/cpu_ldst.h"
#include "exec/helper-gen.h"

/* TODO: These local functions conflict with new macros in cpu_ldst.h,
   but the INSTRUCTION macro needs them to be named correctly... */
#undef ldw
#undef stb
#undef stw


static inline uint32_t get_opcode(uint32_t code)
{
    I_TYPE(instr, code);
    return instr->op;
}

static inline uint32_t get_opxcode(uint32_t code)
{
    R_TYPE(instr, code);
    return instr->opx6;
}

static inline void t_gen_helper_raise_exception(DisasContext *dc,
                                                uint32_t index)
{
    TCGv_i32 tmp = tcg_const_i32(index);

    tcg_gen_movi_tl(dc->cpu_R[R_PC], dc->pc);
    gen_helper_raise_exception(dc->cpu_env, tmp);
    tcg_temp_free_i32(tmp);
    dc->is_jmp = DISAS_UPDATE;
}

static inline void gen_goto_tb(DisasContext *dc, int n, uint32_t dest)
{
    TranslationBlock *tb = dc->tb;

    if ((tb->pc & TARGET_PAGE_MASK) == (dest & TARGET_PAGE_MASK)) {
        tcg_gen_goto_tb(n);
        tcg_gen_movi_tl(dc->cpu_R[R_PC], dest);
        tcg_gen_exit_tb((tcg_target_long)tb + n);
    } else {
        tcg_gen_movi_tl(dc->cpu_R[R_PC], dest);
        tcg_gen_exit_tb(0);
    }
}

/*
 * Instructions not implemented by the simulator.
 */
static void unimplemented(DisasContext *dc, uint32_t code)
{
    t_gen_helper_raise_exception(dc, EXCP_UNIMPL);
}

/*
 * Illegal instruction
 */
static void illegal_instruction(DisasContext *dc,
                                uint32_t code __attribute__((unused)))
{
    t_gen_helper_raise_exception(dc, EXCP_ILLEGAL);
}

/*
 * Used as a placeholder for all instructions which do not have an effect on the
 * simulator (e.g. flush, sync)
 */
static void nop(DisasContext *dc __attribute__((unused)),
                uint32_t code __attribute__((unused)))
{
    /* Nothing to do here */
}

/*
 * J-Type instructions
 */

/*
 * ra <- PC + 4
 * PC <- (PC(31..28) : IMM26 * 4)
 */
static void call(DisasContext *dc, uint32_t code)
{
    J_TYPE(instr, code);

#ifdef CALL_TRACING
    TCGv_i32 tmp = tcg_const_i32(dc->pc);
    TCGv_i32 tmp2 = tcg_const_i32((dc->pc & 0xF0000000) | (instr->imm26 * 4));
    gen_helper_call_status(tmp, tmp2);
    tcg_temp_free_i32(tmp);
    tcg_temp_free_i32(tmp2);
#endif

    tcg_gen_movi_tl(dc->cpu_R[R_RA], dc->pc + 4);

    gen_goto_tb(dc, 0, (dc->pc & 0xF0000000) | (instr->imm26 * 4));

    dc->is_jmp = DISAS_TB_JUMP;
}

/* PC <- (PC(31..28) : IMM26 * 4) */
static void jmpi(DisasContext *dc, uint32_t code)
{
    J_TYPE(instr, code);

    gen_goto_tb(dc, 0, (dc->pc & 0xF0000000) | (instr->imm26 * 4));

    dc->is_jmp = DISAS_TB_JUMP;
}

/*
 * I-Type instructions
 */

/* rB <- 0x000000 : Mem8[rA + @(IMM16)] */
static void ldbu(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv addr = tcg_temp_new();
    tcg_gen_addi_tl(addr, dc->cpu_R[instr->a],
                    (int32_t)((int16_t)instr->imm16));

    tcg_gen_qemu_ld8u(dc->cpu_R[instr->b], addr, dc->mem_idx);

    tcg_temp_free(addr);
}

/* rB <- rA + IMM16 */
static void addi(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv imm = tcg_temp_new();
    tcg_gen_addi_tl(dc->cpu_R[instr->b], dc->cpu_R[instr->a],
                    (int32_t)((int16_t)instr->imm16));
    tcg_temp_free(imm);
}

/* Mem8[rA + @(IMM16)] <- rB(7..0) */
static void stb(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv addr = tcg_temp_new();
    tcg_gen_addi_tl(addr, dc->cpu_R[instr->a],
                    (int32_t)((int16_t)instr->imm16));

    tcg_gen_qemu_st8(dc->cpu_R[instr->b], addr, dc->mem_idx);

    tcg_temp_free(addr);
}

/* PC <- PC + 4 + IMM16 */
static void br(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    gen_goto_tb(dc, 0, dc->pc + 4 + (int16_t)(instr->imm16 & 0xFFFC));

    dc->is_jmp = DISAS_TB_JUMP;
}

/* rB <- @(Mem8[rA + @(IMM16)]) */
static void ldb(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv addr = tcg_temp_new();
    tcg_gen_addi_tl(addr, dc->cpu_R[instr->a],
                    (int32_t)((int16_t)instr->imm16));

    tcg_gen_qemu_ld8s(dc->cpu_R[instr->b], addr, dc->mem_idx);

    tcg_temp_free(addr);
}

/*
 * if ((signed) rA >= (signed) @(IMM16))
 *   rB <- 1
 * else
 *   rB <- 0
 */
static void cmpgei(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    tcg_gen_setcondi_tl(TCG_COND_GE, dc->cpu_R[instr->b], dc->cpu_R[instr->a],
                        (int32_t)((int16_t)instr->imm16));
}

/* rB <- 0x0000 : Mem16[rA + @IMM16)] */
static void ldhu(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv addr = tcg_temp_new();
    tcg_gen_addi_tl(addr, dc->cpu_R[instr->a],
                    (int32_t)((int16_t)instr->imm16));

    tcg_gen_qemu_ld16u(dc->cpu_R[instr->b], addr, dc->mem_idx);

    tcg_temp_free(addr);
}

/* rB <- rA & IMM16 */
static void andi(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    tcg_gen_andi_tl(dc->cpu_R[instr->b], dc->cpu_R[instr->a], instr->imm16);
}

/* Mem16[rA + @(IMM16)] <- rB(15..0) */
static void sth(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv addr = tcg_temp_new();
    tcg_gen_addi_tl(addr, dc->cpu_R[instr->a],
                    (int32_t)((int16_t)instr->imm16));

    tcg_gen_qemu_st16(dc->cpu_R[instr->b], addr, dc->mem_idx);

    tcg_temp_free(addr);
}

/*
 * if ((signed) rA >= (signed) rB)
 *   PC <- PC + 4 + @(IMM16)
 * else
 *   PC <- PC + 4
 */
static void bge(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    int l1 = gen_new_label();

    tcg_gen_brcond_tl(TCG_COND_GE, dc->cpu_R[instr->a],
                      dc->cpu_R[instr->b], l1);

    gen_goto_tb(dc, 0, dc->pc + 4);

    gen_set_label(l1);

    gen_goto_tb(dc, 1, dc->pc + 4 + (int16_t)(instr->imm16 & 0xFFFC));

    dc->is_jmp = DISAS_TB_JUMP;
}

/* rB <- Mem16[rA + @IMM16)] */
static void ldh(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv addr = tcg_temp_new();
    tcg_gen_addi_tl(addr, dc->cpu_R[instr->a],
                    (int32_t)((int16_t)instr->imm16));

    tcg_gen_qemu_ld16s(dc->cpu_R[instr->b], addr, dc->mem_idx);

    tcg_temp_free(addr);
}

/*
 * if ((signed) rA < (signed) @(IMM16))
 *   rB <- 1
 * else
 *   rB <- 0
 */
static void cmplti(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    tcg_gen_setcondi_tl(TCG_COND_LT, dc->cpu_R[instr->b], dc->cpu_R[instr->a],
                        (int32_t)((int16_t)instr->imm16));
}

/* Initializes the data cache line currently caching address rA + @(IMM16) */
static void initda(DisasContext *dc __attribute__((unused)),
                   uint32_t code __attribute__((unused)))
{
    /* TODO */
}

/* rB <- rA | IMM16 */
static void ori(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    tcg_gen_ori_tl(dc->cpu_R[instr->b], dc->cpu_R[instr->a], instr->imm16);
}

/* Mem32[rA + @(IMM16)] <- rB */
static void stw(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv addr = tcg_temp_new();
    tcg_gen_addi_tl(addr, dc->cpu_R[instr->a],
                    (int32_t)((int16_t)instr->imm16));

    tcg_gen_qemu_st32(dc->cpu_R[instr->b], addr, dc->mem_idx);

    tcg_temp_free(addr);
}

/*
 * if ((signed) rA < (signed) rB)
 *   PC <- PC + 4 + @(IMM16)
 * else
 *   PC <- PC + 4
 */
static void blt(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    int l1 = gen_new_label();

    tcg_gen_brcond_tl(TCG_COND_LT, dc->cpu_R[instr->a],
                      dc->cpu_R[instr->b], l1);

    gen_goto_tb(dc, 0, dc->pc + 4);

    gen_set_label(l1);

    gen_goto_tb(dc, 1, dc->pc + 4 + (int16_t)(instr->imm16 & 0xFFFC));

    dc->is_jmp = DISAS_TB_JUMP;
}

/* rB <- @(Mem32[rA + @(IMM16)]) */
static void ldw(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv addr = tcg_temp_new();
    tcg_gen_addi_tl(addr, dc->cpu_R[instr->a],
                    (int32_t)((int16_t)instr->imm16));

    tcg_gen_qemu_ld32u(dc->cpu_R[instr->b], addr, dc->mem_idx);

    tcg_temp_free(addr);
}

/*
 * if ((signed) rA != (signed) @(IMM16))
 *   rB <- 1
 * else
 *   rB <- 0
 */
static void cmpnei(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    tcg_gen_setcondi_tl(TCG_COND_NE, dc->cpu_R[instr->b], dc->cpu_R[instr->a],
                        (int32_t)((int16_t)instr->imm16));
}

/* rB <- rA ^ IMM16 */
static void xori(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    tcg_gen_xori_tl(dc->cpu_R[instr->b], dc->cpu_R[instr->a], instr->imm16);
}

/*
 * if (rA != rB)
 *   PC <- PC + 4 + @(IMM16)
 * else
 *   PC <- PC + 4
 */
static void bne(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    int l1 = gen_new_label();

    tcg_gen_brcond_tl(TCG_COND_NE, dc->cpu_R[instr->a],
                      dc->cpu_R[instr->b], l1);

    gen_goto_tb(dc, 0, dc->pc + 4);

    gen_set_label(l1);

    gen_goto_tb(dc, 1, dc->pc + 4 + (int16_t)(instr->imm16 & 0xFFFC));

    dc->is_jmp = DISAS_TB_JUMP;
}

/*
 * if ((signed) rA == (signed) @(IMM16))
 *   rB <- 1
 * else
 *   rB <- 0
 */
static void cmpeqi(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    tcg_gen_setcondi_tl(TCG_COND_EQ, dc->cpu_R[instr->b], dc->cpu_R[instr->a],
                        (int32_t)((int16_t)instr->imm16));
}

/* rB <- 0x000000 : Mem8[rA + @(IMM16)] (bypassing cache) */
static void ldbuio(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv addr = tcg_temp_new();
    tcg_gen_addi_tl(addr, dc->cpu_R[instr->a],
                    (int32_t)((int16_t)instr->imm16));

    tcg_gen_qemu_ld8u(dc->cpu_R[instr->b], addr, dc->mem_idx);

    tcg_temp_free(addr);
}

/* rB <- (rA * @(IMM16))(31..0) */
static void muli(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv imm = tcg_temp_new();
    tcg_gen_muli_i32(dc->cpu_R[instr->b], dc->cpu_R[instr->a],
                     (int32_t)((int16_t)instr->imm16));
    tcg_temp_free(imm);
}

/* Mem8[rA + @(IMM16)] <- rB(7..0) (bypassing cache) */
static void stbio(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv addr = tcg_temp_new();
    tcg_gen_addi_tl(addr, dc->cpu_R[instr->a],
                    (int32_t)((int16_t)instr->imm16));

    tcg_gen_qemu_st8(dc->cpu_R[instr->b], addr, dc->mem_idx);

    tcg_temp_free(addr);
}

/*
 * if (rA == rB)
 *   PC <- PC + 4 + @(IMM16)
 * else
 *   PC <- PC + 4
 */
static void beq(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    int l1 = gen_new_label();

    tcg_gen_brcond_tl(TCG_COND_EQ, dc->cpu_R[instr->a],
                      dc->cpu_R[instr->b], l1);

    gen_goto_tb(dc, 0, dc->pc + 4);

    gen_set_label(l1);

    gen_goto_tb(dc, 1, dc->pc + 4 + (int16_t)(instr->imm16 & 0xFFFC));

    dc->is_jmp = DISAS_TB_JUMP;
}

/* rB <- @(Mem8[rA + @(IMM16)]) (bypassing cache) */
static void ldbio(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv addr = tcg_temp_new();
    tcg_gen_addi_tl(addr, dc->cpu_R[instr->a],
                    (int32_t)((int16_t)instr->imm16));

    tcg_gen_qemu_ld8s(dc->cpu_R[instr->b], addr, dc->mem_idx);

    tcg_temp_free(addr);
}

/*
 * if (rA >= 0x0000 : @(IMM16))
 *   rB <- 1
 * else
 *   rB <- 0
 */
static void cmpgeui(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    tcg_gen_setcondi_tl(TCG_COND_GEU, dc->cpu_R[instr->b], dc->cpu_R[instr->a],
                        (int32_t)((int16_t)instr->imm16));
}

/* rB <- 0x0000 : Mem16[rA + @IMM16)] (bypassing cache) */
static void ldhuio(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv addr = tcg_temp_new();
    tcg_gen_addi_tl(addr, dc->cpu_R[instr->a],
                    (int32_t)((int16_t)instr->imm16));

    tcg_gen_qemu_ld16u(dc->cpu_R[instr->b], addr, dc->mem_idx);

    tcg_temp_free(addr);
}

/* rB <- rA & (IMM16 : 0x0000) */
static void andhi(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    tcg_gen_andi_tl(dc->cpu_R[instr->b], dc->cpu_R[instr->a],
                    instr->imm16 << 16);
}

/* Mem16[rA + @(IMM16)] <- rB(15..0) (bypassing cache) */
static void sthio(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv addr = tcg_temp_new();
    tcg_gen_addi_tl(addr, dc->cpu_R[instr->a],
                    (int32_t)((int16_t)instr->imm16));

    tcg_gen_qemu_st16(dc->cpu_R[instr->b], addr, dc->mem_idx);

    tcg_temp_free(addr);
}

/*
 * if (rA >= rB)
 *   PC <- PC + 4 + @(IMM16)
 * else
 *   PC <- PC + 4
 */
static void bgeu(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    int l1 = gen_new_label();

    tcg_gen_brcond_tl(TCG_COND_GEU, dc->cpu_R[instr->a],
                      dc->cpu_R[instr->b], l1);

    gen_goto_tb(dc, 0, dc->pc + 4);

    gen_set_label(l1);

    gen_goto_tb(dc, 1, dc->pc + 4 + (int16_t)(instr->imm16 & 0xFFFC));

    dc->is_jmp = DISAS_TB_JUMP;
}

/* rB <- Mem16[rA + @IMM16)] (bypassing cache) */
static void ldhio(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv addr = tcg_temp_new();
    tcg_gen_addi_tl(addr, dc->cpu_R[instr->a],
                    (int32_t)((int16_t)instr->imm16));

    tcg_gen_qemu_ld16s(dc->cpu_R[instr->b], addr, dc->mem_idx);

    tcg_temp_free(addr);
}

/*
 * if (rA < 0x0000 : @(IMM16))
 *   rB <- 1
 * else
 *   rB <- 0
 */
static void cmpltui(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    tcg_gen_setcondi_tl(TCG_COND_LTU, dc->cpu_R[instr->b], dc->cpu_R[instr->a],
                        (int32_t)((int16_t)instr->imm16));
}

/* */
static void initd(DisasContext *dc __attribute__((unused)),
                  uint32_t code __attribute((unused)))
{
    /* TODO */
}

/* rB <- rA | (IMM16 : 0x0000) */
static void orhi(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    tcg_gen_ori_tl(dc->cpu_R[instr->b], dc->cpu_R[instr->a],
                   instr->imm16 << 16);
}

/* Mem32[rA + @(IMM16)] <- rB (bypassing cache) */
static void stwio(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv addr = tcg_temp_new();
    tcg_gen_addi_tl(addr, dc->cpu_R[instr->a],
                    (int32_t)((int16_t)instr->imm16));

    tcg_gen_qemu_st32(dc->cpu_R[instr->b], addr, dc->mem_idx);

    tcg_temp_free(addr);
}

/*
 * if ((unsigned) rA < (unsigned) rB)
 *   PC <- PC + 4 + @(IMM16)
 * else
 *   PC <- PC + 4
 */
static void bltu(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    int l1 = gen_new_label();

    tcg_gen_brcond_tl(TCG_COND_LTU, dc->cpu_R[instr->a],
                      dc->cpu_R[instr->b], l1);

    gen_goto_tb(dc, 0, dc->pc + 4);

    gen_set_label(l1);

    gen_goto_tb(dc, 1, dc->pc + 4 + (int16_t)(instr->imm16 & 0xFFFC));

    dc->is_jmp = DISAS_TB_JUMP;
}

/* rB <- @(Mem32[rA + @(IMM16)]) (bypassing cache) */
static void ldwio(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    TCGv addr = tcg_temp_new();
    tcg_gen_addi_tl(addr, dc->cpu_R[instr->a],
                    (int32_t)((int16_t)instr->imm16));

    tcg_gen_qemu_ld32u(dc->cpu_R[instr->b], addr, dc->mem_idx);

    tcg_temp_free(addr);
}

/* Prototype only, defined below */
static void handle_r_type_instr(DisasContext *dc, uint32_t code);

/* rB <- rA ^ (IMM16 : 0x0000) */
static void xorhi(DisasContext *dc, uint32_t code)
{
    I_TYPE(instr, code);

    tcg_gen_xori_tl(dc->cpu_R[instr->b], dc->cpu_R[instr->a],
                    instr->imm16 << 16);
}

static const Nios2Instruction i_type_instructions[I_TYPE_COUNT] = {
    [CALL]    = INSTRUCTION(call),
    [JMPI]    = INSTRUCTION(jmpi),
    [0x02]    = INSTRUCTION_ILLEGAL(),
    [LDBU]    = INSTRUCTION(ldbu),
    [ADDI]    = INSTRUCTION(addi),
    [STB]     = INSTRUCTION(stb),
    [BR]      = INSTRUCTION(br),
    [LDB]     = INSTRUCTION(ldb),
    [CMPGEI]  = INSTRUCTION(cmpgei),
    [0x09]    = INSTRUCTION_ILLEGAL(),
    [0x0a]    = INSTRUCTION_ILLEGAL(),
    [LDHU]    = INSTRUCTION(ldhu),
    [ANDI]    = INSTRUCTION(andi),
    [STH]     = INSTRUCTION(sth),
    [BGE]     = INSTRUCTION(bge),
    [LDH]     = INSTRUCTION(ldh),
    [CMPLTI]  = INSTRUCTION(cmplti),
    [0x11]    = INSTRUCTION_ILLEGAL(),
    [0x12]    = INSTRUCTION_ILLEGAL(),
    [INITDA]  = INSTRUCTION(initda),
    [ORI]     = INSTRUCTION(ori),
    [STW]     = INSTRUCTION(stw),
    [BLT]     = INSTRUCTION(blt),
    [LDW]     = INSTRUCTION(ldw),
    [CMPNEI]  = INSTRUCTION(cmpnei),
    [0x19]    = INSTRUCTION_ILLEGAL(),
    [0x1a]    = INSTRUCTION_ILLEGAL(),
    [FLUSHDA] = INSTRUCTION_NOP(flushda),
    [XORI]    = INSTRUCTION(xori),
    [0x1d]    = INSTRUCTION_ILLEGAL(),
    [BNE]     = INSTRUCTION(bne),
    [0x1f]    = INSTRUCTION_ILLEGAL(),
    [CMPEQI]  = INSTRUCTION(cmpeqi),
    [0x21]    = INSTRUCTION_ILLEGAL(),
    [0x22]    = INSTRUCTION_ILLEGAL(),
    [LDBUIO]  = INSTRUCTION(ldbuio),
    [MULI]    = INSTRUCTION(muli),
    [STBIO]   = INSTRUCTION(stbio),
    [BEQ]     = INSTRUCTION(beq),
    [LDBIO]   = INSTRUCTION(ldbio),
    [CMPGEUI] = INSTRUCTION(cmpgeui),
    [0x29]    = INSTRUCTION_ILLEGAL(),
    [0x2a]    = INSTRUCTION_ILLEGAL(),
    [LDHUIO]  = INSTRUCTION(ldhuio),
    [ANDHI]   = INSTRUCTION(andhi),
    [STHIO]   = INSTRUCTION(sthio),
    [BGEU]    = INSTRUCTION(bgeu),
    [LDHIO]   = INSTRUCTION(ldhio),
    [CMPLTUI] = INSTRUCTION(cmpltui),
    [0x31]    = INSTRUCTION_ILLEGAL(),
    [CUSTOM]  = INSTRUCTION_UNIMPLEMENTED(custom),
    [INITD]   = INSTRUCTION(initd),
    [ORHI]    = INSTRUCTION(orhi),
    [STWIO]   = INSTRUCTION(stwio),
    [BLTU]    = INSTRUCTION(bltu),
    [LDWIO]   = INSTRUCTION(ldwio),
    [RDPRS]   = INSTRUCTION_UNIMPLEMENTED(rdprs),
    [0x39]    = INSTRUCTION_ILLEGAL(),
    [R_TYPE]  = { "<R-type instruction>", handle_r_type_instr },
    [FLUSHD]  = INSTRUCTION_NOP(flushd),
    [XORHI]   = INSTRUCTION(xorhi),
    [0x3d]    = INSTRUCTION_ILLEGAL(),
    [0x3e]    = INSTRUCTION_ILLEGAL(),
    [0x3f]    = INSTRUCTION_ILLEGAL(),
};

/*
 * R-Type instructions
 */

/*
 * status <- estatus
 * PC <- ea
 */
static void eret(DisasContext *dc, uint32_t code __attribute__((unused)))
{
#ifdef CALL_TRACING
    TCGv_i32 tmp = tcg_const_i32(dc->pc);
    gen_helper_eret_status(tmp);
    tcg_temp_free_i32(tmp);
#endif

    tcg_gen_mov_tl(dc->cpu_R[CR_STATUS], dc->cpu_R[CR_ESTATUS]);
    tcg_gen_mov_tl(dc->cpu_R[R_PC], dc->cpu_R[R_EA]);

//    dc->is_jmp = DISAS_JUMP;
//printf("JB: ERET %x\n",dc->cpu_R[CR_ESTATUS]);
    dc->is_jmp = DISAS_UPDATE;
}

/* rC <- rA rotated left IMM5 bit positions */
static void roli(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_rotli_tl(dc->cpu_R[instr->c], dc->cpu_R[instr->a], instr->imm5);
}

/* rC <- rA rotated left rB(4..0) bit positions */
static void rol(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    TCGv t0 = tcg_temp_new();

    tcg_gen_andi_tl(t0, dc->cpu_R[instr->b], 31);
    tcg_gen_rotl_tl(dc->cpu_R[instr->c], dc->cpu_R[instr->a], t0);

    tcg_temp_free(t0);
}

/* */
static void flushp(DisasContext *dc __attribute__((unused)),
                   uint32_t code __attribute__((unused)))
{
    /* TODO */
}

/* PC <- ra */
static void ret(DisasContext *dc, uint32_t code __attribute__((unused)))
{
#ifdef CALL_TRACING
    TCGv_i32 tmp = tcg_const_i32(dc->pc);
    gen_helper_ret_status(tmp);
    tcg_temp_free_i32(tmp);
#endif

    tcg_gen_mov_tl(dc->cpu_R[R_PC], dc->cpu_R[R_RA]);

    dc->is_jmp = DISAS_JUMP;
}

/* rC <- ~(A | rB) */
static void nor(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_nor_tl(dc->cpu_R[instr->c], dc->cpu_R[instr->a],
                   dc->cpu_R[instr->b]);
}

/* rC <- ((unsigned)rA * (unsigned)rB))(31..0) */
static void mulxuu(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    TCGv_i64 t0, t1;

    t0 = tcg_temp_new_i64();
    t1 = tcg_temp_new_i64();

    tcg_gen_extu_i32_i64(t0, dc->cpu_R[instr->a]);
    tcg_gen_extu_i32_i64(t1, dc->cpu_R[instr->b]);
    tcg_gen_mul_i64(t0, t0, t1);

    tcg_gen_shri_i64(t0, t0, 32);
    tcg_gen_trunc_i64_i32(dc->cpu_R[instr->c], t0);

    tcg_temp_free_i64(t0);
    tcg_temp_free_i64(t1);
}

/*
 * if (rA >= rB)
 *   rC <- 1
 * else
 *   rC <- 0
 */
static void cmpge(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_setcond_tl(TCG_COND_GE, dc->cpu_R[instr->c], dc->cpu_R[instr->a],
                       dc->cpu_R[instr->b]);
}

/* PC <- ba */
static void bret(DisasContext *dc, uint32_t code __attribute__((unused)))
{
    tcg_gen_mov_tl(dc->cpu_R[R_PC], dc->cpu_R[R_BA]);
    dc->is_jmp = DISAS_JUMP;
}

/*  rC <- rA rotated right rb(4..0) bit positions */
static void ror(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    TCGv t0 = tcg_temp_new();

    tcg_gen_andi_tl(t0, dc->cpu_R[instr->b], 31);
    tcg_gen_rotr_tl(dc->cpu_R[instr->c], dc->cpu_R[instr->a], t0);

    tcg_temp_free(t0);
}

/* */
static void flushi(DisasContext *dc __attribute__((unused)),
                   uint32_t code __attribute__((unused)))
{
    /* TODO */
}

/* PC <- rA */
static void jmp(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_mov_tl(dc->cpu_R[R_PC], dc->cpu_R[instr->a]);

    dc->is_jmp = DISAS_JUMP;
}

/* rC <- rA & rB */
static void and(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_and_tl(dc->cpu_R[instr->c], dc->cpu_R[instr->a],
                   dc->cpu_R[instr->b]);
}

/*
 * if ((signed) rA < (signed) rB)
 *   rC <- 1
 * else
 *   rC <- 0
 */
static void cmplt(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_setcond_tl(TCG_COND_LT, dc->cpu_R[instr->c], dc->cpu_R[instr->a],
                       dc->cpu_R[instr->b]);
}

/* rC <- rA << IMM5 */
static void slli(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_shli_tl(dc->cpu_R[instr->c], dc->cpu_R[instr->a], instr->imm5);
}

/* rC <- rA << rB(4..0) */
static void sll(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    TCGv t0 = tcg_temp_new();

    tcg_gen_andi_tl(t0, dc->cpu_R[instr->b], 31);
    tcg_gen_shl_tl(dc->cpu_R[instr->c], dc->cpu_R[instr->a], t0);

    tcg_temp_free(t0);
}

/* rC <- rA | rB */
static void or(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_or_tl(dc->cpu_R[instr->c], dc->cpu_R[instr->a],
                  dc->cpu_R[instr->b]);
}

/* rC <- ((signed)rA * (unsigned)rB))(31..0) */
static void mulxsu(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    TCGv_i64 t0, t1;

    t0 = tcg_temp_new_i64();
    t1 = tcg_temp_new_i64();

    tcg_gen_ext_i32_i64(t0, dc->cpu_R[instr->a]);
    tcg_gen_extu_i32_i64(t1, dc->cpu_R[instr->b]);
    tcg_gen_mul_i64(t0, t0, t1);

    tcg_gen_shri_i64(t0, t0, 32);
    tcg_gen_trunc_i64_i32(dc->cpu_R[instr->c], t0);

    tcg_temp_free_i64(t0);
    tcg_temp_free_i64(t1);
}

/*
 * if (rA != rB)
 *   rC <- 1
 * else
 *   rC <- 0
 */
static void cmpne(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_setcond_tl(TCG_COND_NE, dc->cpu_R[instr->c], dc->cpu_R[instr->a],
                       dc->cpu_R[instr->b]);
}

/* rC <- (unsigned) rA >> ((unsigned) IMM5)*/
static void srli(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_shri_tl(dc->cpu_R[instr->c], dc->cpu_R[instr->a], instr->imm5);
}

/* rC <- (unsigned) rA >> ((unsigned) rB(4..0))*/
static void srl(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    TCGv t0 = tcg_temp_new();

    tcg_gen_andi_tl(t0, dc->cpu_R[instr->b], 31);
    tcg_gen_shr_tl(dc->cpu_R[instr->c], dc->cpu_R[instr->a], t0);

    tcg_temp_free(t0);
}

/* rC <- PC + 4 */
static void nextpc(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_movi_tl(dc->cpu_R[instr->c], dc->pc + 4);
}

/*
 * ra <- PC + 4
 * PC <- rA
 */
static void callr(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

#ifdef CALL_TRACING
    TCGv_i32 tmp = tcg_const_i32(dc->pc);
    gen_helper_call_status(tmp, dc->cpu_R[instr->a]);
    tcg_temp_free_i32(tmp);
#endif

    tcg_gen_movi_tl(dc->cpu_R[R_RA], dc->pc + 4);
    tcg_gen_mov_tl(dc->cpu_R[R_PC], dc->cpu_R[instr->a]);

    dc->is_jmp = DISAS_JUMP;
}

/* rC <- rA ^ rB */
static void xor(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_xor_tl(dc->cpu_R[instr->c], dc->cpu_R[instr->a],
                   dc->cpu_R[instr->b]);
}

/* rC <- ((signed)rA * (signed)rB))(31..0) */
static void mulxss(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    TCGv_i64 t0, t1;

    t0 = tcg_temp_new_i64();
    t1 = tcg_temp_new_i64();

    tcg_gen_ext_i32_i64(t0, dc->cpu_R[instr->a]);
    tcg_gen_ext_i32_i64(t1, dc->cpu_R[instr->b]);
    tcg_gen_mul_i64(t0, t0, t1);

    tcg_gen_shri_i64(t0, t0, 32);
    tcg_gen_trunc_i64_i32(dc->cpu_R[instr->c], t0);

    tcg_temp_free_i64(t0);
    tcg_temp_free_i64(t1);
}

/*
 * if (rA == rB)
 *   rC <- 1
 * else
 *   rC <- 0
 */
static void cmpeq(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_setcond_tl(TCG_COND_EQ, dc->cpu_R[instr->c], dc->cpu_R[instr->a],
                       dc->cpu_R[instr->b]);
}

/* rC <- rA / rB */
static void divu(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    gen_helper_divu(dc->cpu_R[instr->c], dc->cpu_R[instr->a],
                    dc->cpu_R[instr->b]);
}

/* rC <- rA / rB */
static void _div(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    gen_helper_divs(dc->cpu_R[instr->c], dc->cpu_R[instr->a],
                    dc->cpu_R[instr->b]);
}

/* rC <- ctlN */
static void rdctl(DisasContext *dc, uint32_t code)
{
    if (IS_USER(dc)) {
        illegal_instruction(dc,0);
    } else {
        R_TYPE(instr, code);

        switch (instr->imm5 + 32) {
        case CR_PTEADDR:
        case CR_TLBACC:
        case CR_TLBMISC:
        {
            TCGv_i32 tmp = tcg_const_i32(instr->imm5 + 32);
            gen_helper_mmu_read(dc->cpu_R[instr->c], dc->cpu_env, tmp);
            tcg_temp_free_i32(tmp);
            break;
        }

        default:
            tcg_gen_mov_tl(dc->cpu_R[instr->c], dc->cpu_R[instr->imm5 + 32]);
            break;
        }
    }
}


/* rC <- (rA * rB))(31..0) */
static void mul(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_mul_i32(dc->cpu_R[instr->c], dc->cpu_R[instr->a],
                    dc->cpu_R[instr->b]);
}

/*
 * if (rA >= rB)
 *   rC <- 1
 * else
 *   rC <- 0
 */
static void cmpgeu(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_setcond_tl(TCG_COND_GEU, dc->cpu_R[instr->c], dc->cpu_R[instr->a],
                       dc->cpu_R[instr->b]);
}

/* */
static void initi(DisasContext *dc __attribute__((unused)),
                  uint32_t code __attribute__((unused)))
{
    /* TODO */
}

/*
 * estatus <- status
 * PIE <- 0
 * U <- 0
 * ea <- PC + 4
 * PC <- exception handler address
 */
static void trap(DisasContext *dc, uint32_t code __attribute__((unused)))
{
    t_gen_helper_raise_exception(dc, EXCP_TRAP);
}

/* ctlN <- rA */
static void wrctl(DisasContext *dc, uint32_t code)
{
    if (IS_USER(dc)) {
        illegal_instruction(dc,0);
    } else {
        R_TYPE(instr, code);

        switch (instr->imm5 + 32) {
        case CR_PTEADDR:
        case CR_TLBACC:
        case CR_TLBMISC:
        {
            TCGv_i32 tmp = tcg_const_i32(instr->imm5 + 32);
            gen_helper_mmu_write(dc->cpu_env, tmp, dc->cpu_R[instr->a]);
            tcg_temp_free_i32(tmp);
            break;
        }

       case CR_IPENDING: /* read only, ignore writes */
            break;

        case CR_IENABLE:
            gen_helper_cr_ienable_write(dc->cpu_env, dc->cpu_R[instr->a]);
            tcg_gen_movi_tl(dc->cpu_R[R_PC], dc->pc + 4);
            dc->is_jmp = DISAS_UPDATE;
            break;

        case CR_STATUS:
            gen_helper_cr_status_write(dc->cpu_env, dc->cpu_R[instr->a]);
            tcg_gen_movi_tl(dc->cpu_R[R_PC], dc->pc + 4);
            dc->is_jmp = DISAS_UPDATE;
            break;

        default:
            tcg_gen_mov_tl(dc->cpu_R[instr->imm5 + 32], dc->cpu_R[instr->a]);
            break;
        }
    }
}

/*
 * if (rA < rB)
 *   rC <- 1
 * else
 *   rC <- 0
 */
static void cmpltu(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_setcond_tl(TCG_COND_LTU, dc->cpu_R[instr->c], dc->cpu_R[instr->a],
                       dc->cpu_R[instr->b]);
}

/* rC <- rA + rB */
static void add(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_add_tl(dc->cpu_R[instr->c], dc->cpu_R[instr->a],
                   dc->cpu_R[instr->b]);
}

/*
 * bstatus ← status
 * PIE <- 0
 * U <- 0
 * ba <- PC + 4
 * PC <- break handler address
 */
static void __break(DisasContext *dc, uint32_t code __attribute__((unused)))
{
    t_gen_helper_raise_exception(dc, EXCP_BREAK);
}

/* rC <- rA - rB */
static void sub(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_sub_tl(dc->cpu_R[instr->c], dc->cpu_R[instr->a],
                   dc->cpu_R[instr->b]);
}

/* rC <- (signed) rA >> ((unsigned) IMM5) */
static void srai(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    tcg_gen_sari_tl(dc->cpu_R[instr->c], dc->cpu_R[instr->a], instr->imm5);
}

/* rC <- (signed) rA >> ((unsigned) rB(4..0)) */
static void sra(DisasContext *dc, uint32_t code)
{
    R_TYPE(instr, code);

    TCGv t0 = tcg_temp_new();

    tcg_gen_andi_tl(t0, dc->cpu_R[instr->b], 31);
    tcg_gen_sar_tl(dc->cpu_R[instr->c], dc->cpu_R[instr->a], t0);

    tcg_temp_free(t0);
}

static const Nios2Instruction r_type_instructions[R_TYPE_COUNT] = {
    [0x00]   = INSTRUCTION_ILLEGAL(),
    [ERET]   = INSTRUCTION(eret),
    [ROLI]   = INSTRUCTION(roli),
    [ROL]    = INSTRUCTION(rol),
    [FLUSHP] = INSTRUCTION(flushp),
    [RET]    = INSTRUCTION(ret),
    [NOR]    = INSTRUCTION(nor),
    [MULXUU] = INSTRUCTION(mulxuu),
    [CMPGE]  = INSTRUCTION(cmpge),
    [BRET]   = INSTRUCTION(bret),
    [0x0a]   = INSTRUCTION_ILLEGAL(),
    [ROR]    = INSTRUCTION(ror),
    [FLUSHI] = INSTRUCTION(flushi),
    [JMP]    = INSTRUCTION(jmp),
    [AND]    = INSTRUCTION(and),
    [0x0f]   = INSTRUCTION_ILLEGAL(),
    [CMPLT]  = INSTRUCTION(cmplt),
    [0x11]   = INSTRUCTION_ILLEGAL(),
    [SLLI]   = INSTRUCTION(slli),
    [SLL]    = INSTRUCTION(sll),
    [WRPRS]  = INSTRUCTION_UNIMPLEMENTED(wrprs),
    [0x15]   = INSTRUCTION_ILLEGAL(),
    [OR]     = INSTRUCTION(or),
    [MULXSU] = INSTRUCTION(mulxsu),
    [CMPNE]  = INSTRUCTION(cmpne),
    [0x19]   = INSTRUCTION_ILLEGAL(),
    [SRLI]   = INSTRUCTION(srli),
    [SRL]    = INSTRUCTION(srl),
    [NEXTPC] = INSTRUCTION(nextpc),
    [CALLR]  = INSTRUCTION(callr),
    [XOR]    = INSTRUCTION(xor),
    [MULXSS] = INSTRUCTION(mulxss),
    [CMPEQ]  = INSTRUCTION(cmpeq),
    [0x21]   = INSTRUCTION_ILLEGAL(),
    [0x22]   = INSTRUCTION_ILLEGAL(),
    [0x23]   = INSTRUCTION_ILLEGAL(),
    [DIVU]   = INSTRUCTION(divu),
    [DIV]    = { "div", _div },
    [RDCTL]  = INSTRUCTION(rdctl),
    [MUL]    = INSTRUCTION(mul),
    [CMPGEU] = INSTRUCTION(cmpgeu),
    [INITI]  = INSTRUCTION(initi),
    [0x2a]   = INSTRUCTION_ILLEGAL(),
    [0x2b]   = INSTRUCTION_ILLEGAL(),
    [0x2c]   = INSTRUCTION_ILLEGAL(),
    [TRAP]   = INSTRUCTION(trap),
    [WRCTL]  = INSTRUCTION(wrctl),
    [0x2f]   = INSTRUCTION_ILLEGAL(),
    [CMPLTU] = INSTRUCTION(cmpltu),
    [ADD]    = INSTRUCTION(add),
    [0x32]   = INSTRUCTION_ILLEGAL(),
    [0x33]   = INSTRUCTION_ILLEGAL(),
    [BREAK]  = { "break", __break },
    [0x35]   = INSTRUCTION_ILLEGAL(),
    [SYNC]   = INSTRUCTION(nop),
    [0x37]   = INSTRUCTION_ILLEGAL(),
    [0x38]   = INSTRUCTION_ILLEGAL(),
    [SUB]    = INSTRUCTION(sub),
    [SRAI]   = INSTRUCTION(srai),
    [SRA]    = INSTRUCTION(sra),
    [0x3c]   = INSTRUCTION_ILLEGAL(),
    [0x3d]   = INSTRUCTION_ILLEGAL(),
    [0x3e]   = INSTRUCTION_ILLEGAL(),
    [0x3f]   = INSTRUCTION_ILLEGAL(),
};

static void handle_r_type_instr(DisasContext *dc, uint32_t code)
{
    uint32_t opx;
    instruction_handler handle_instr;

    opx = get_opxcode(code);
    if (unlikely(opx >= R_TYPE_COUNT)) {
        goto illegal_op;
    }

    LOG_DIS("R: %s (%08x)\n", r_type_instructions[opx].name, code);
    handle_instr = r_type_instructions[opx].handler;

    handle_instr(dc, code);

    return;

illegal_op:
    t_gen_helper_raise_exception(dc, EXCP_ILLEGAL);
}


void handle_instruction(DisasContext *dc, CPUNios2State *env)
{
    uint32_t insn = cpu_ldl_code(env, dc->pc);
    uint32_t op = get_opcode(insn);

    LOG_DIS("%8.8x\t", insn);

    if (unlikely(op >= I_TYPE_COUNT)) {
        goto illegal_op;
    }

    if (op != R_TYPE) {
        LOG_DIS("I: %s (%08x)\n", i_type_instructions[op].name, insn);
    }
    i_type_instructions[op].handler(dc, insn);

    return;

illegal_op:
    t_gen_helper_raise_exception(dc, EXCP_ILLEGAL);
}

const char *instruction_get_string(uint32_t code)
{
    uint32_t op = get_opcode(code);

    if (unlikely(op >= I_TYPE_COUNT)) {
        return "";
    } else if (op == R_TYPE) {
        uint32_t opx = get_opxcode(code);
        if (unlikely(opx >= R_TYPE_COUNT)) {
            return "";
        }
        return r_type_instructions[opx].name;
    } else {
        return i_type_instructions[op].name;
    }
}

