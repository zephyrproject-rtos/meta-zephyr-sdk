/*
 * Altera Nios II emulation for qemu: main translation routines.
 *
 * Copyright (C) 2016 Intel Corporation.
 * Copyright (C) 2012 Chris Wulff <crwulff@gmail.com>
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

#include "cpu.h"
#include "disas/disas.h"
#include "tcg-op.h"
#include "exec/helper-proto.h"
#include "instruction.h"
#include "exec/cpu_ldst.h"
#include "exec/helper-gen.h"

static const char *regnames[] = {
    "zero",     "at",       "r2",       "r3",
    "r4",       "r5",       "r6",       "r7",
    "r8",       "r9",       "r10",      "r11",
    "r12",      "r13",      "r14",      "r15",
    "r16",      "r17",      "r18",      "r19",
    "r20",      "r21",      "r22",      "r23",
    "et",       "bt",       "gp",       "sp",
    "fp",       "ea",       "ba",       "ra",
    "status",   "estatus",  "bstatus",  "ienable",
    "ipending", "cpuid",    "reserved", "exception",
    "pteaddr",  "tlbacc",   "tlbmisc",  "reserved",
    "badaddr",  "config",   "mpubase",  "mpuacc",
    "reserved", "reserved", "reserved", "reserved",
    "reserved", "reserved", "reserved", "reserved",
    "reserved", "reserved", "reserved", "reserved",
    "reserved", "reserved", "reserved", "reserved",
    "rpc"
};

static TCGv_ptr cpu_env;
static TCGv cpu_R[NIOS2_NUM_CORE_REGS];

#include "exec/gen-icount.h"

static inline void t_gen_raise_exception(uint32_t index)
{
    TCGv_i32 tmp = tcg_const_i32(index);
    gen_helper_raise_exception(cpu_env, tmp);
    tcg_temp_free_i32(tmp);
}

static void check_breakpoint(CPUNios2State *env, DisasContext *dc)
{
    CPUState *cs = CPU(nios2_env_get_cpu(env));
    CPUBreakpoint *bp;

    if (unlikely(!QTAILQ_EMPTY(&cs->breakpoints))) {
        QTAILQ_FOREACH(bp, &cs->breakpoints, entry) {
            if (bp->pc == dc->pc) {
                tcg_gen_movi_tl(cpu_R[R_PC], dc->pc);
                t_gen_raise_exception(EXCP_DEBUG);
                dc->is_jmp = DISAS_UPDATE;
             }
        }
    }
}

/* generate intermediate code for basic block 'tb'.  */
static void gen_intermediate_code_internal(
    CPUNios2State *env, TranslationBlock *tb, int search_pc)
{
    DisasContext dc1, *dc = &dc1;
    CPUState *cs = CPU(env);
    int num_insns;
    int max_insns;
    uint32_t next_page_start;

    int j, lj = -1;
    uint16_t *gen_opc_end = tcg_ctx.gen_opc_buf + OPC_MAX_SIZE;

    /* Initialize DC */
    dc->cpu_env = cpu_env;
    dc->cpu_R   = cpu_R;
    dc->is_jmp  = DISAS_NEXT;
    dc->pc      = tb->pc;
    dc->tb      = tb;
    dc->mem_idx = cpu_mmu_index(env);

#ifndef CONFIG_USER_ONLY
    if ((env->regs[CR_STATUS] & CR_STATUS_U) == CR_STATUS_U) {
        dc->user = true;
    } else {
        dc->user = false;
    }
#endif

    /* Dump the CPU state to the log */
    if (qemu_loglevel_mask(CPU_LOG_TB_IN_ASM)) {
        qemu_log("--------------\n");
        log_cpu_state(CPU(env), 0);
    }

    /* Set up instruction counts */
    num_insns = 0;
    max_insns = tb->cflags & CF_COUNT_MASK;
    if (max_insns == 0) {
        max_insns = CF_COUNT_MASK;
    }
    next_page_start = (tb->pc & TARGET_PAGE_MASK) + TARGET_PAGE_SIZE;

    gen_tb_start();
    do {
        check_breakpoint(env, dc);

        /* Mark instruction start with associated PC */
        if (search_pc) {
            j = tcg_ctx.gen_opc_ptr  - tcg_ctx.gen_opc_buf;
            if (lj < j) {
                lj++;
                while (lj < j) {
                    tcg_ctx.gen_opc_instr_start[lj++] = 0;
                }
            }
            tcg_ctx.gen_opc_pc[lj] = dc->pc;
            tcg_ctx.gen_opc_instr_start[lj] = 1;
            tcg_ctx.gen_opc_icount[lj] = num_insns;
        }

        LOG_DIS("%8.8x:\t", dc->pc);

        if (num_insns + 1 == max_insns && (tb->cflags & CF_LAST_IO)) {
            gen_io_start();
        }

        /* Decode an instruction */
        handle_instruction(dc, env);

        dc->pc += 4;
        num_insns++;

        /* Translation stops when a conditional branch is encountered.
         * Otherwise the subsequent code could get translated several times.
         * Also stop translation when a page boundary is reached.  This
         * ensures prefetch aborts occur at the right place.  */
    } while (!dc->is_jmp && tcg_ctx.gen_opc_ptr < gen_opc_end &&
             !cs->singlestep_enabled &&
             !singlestep &&
             dc->pc < next_page_start &&
             num_insns < max_insns);

    if (tb->cflags & CF_LAST_IO) {
        gen_io_end();
    }

    /* Indicate where the next block should start */
    switch (dc->is_jmp) {
    case DISAS_NEXT:
        /* Save the current PC back into the CPU register */
        tcg_gen_movi_tl(cpu_R[R_PC], dc->pc);
        tcg_gen_exit_tb(0);
        break;

    default:
    case DISAS_JUMP:
    case DISAS_UPDATE:
        /* The jump will already have updated the PC register */
        tcg_gen_exit_tb(0);
        break;

    case DISAS_TB_JUMP:
        /* nothing more to generate */
        break;
    }

    /* End off the block */
    gen_tb_end(tb, num_insns);
    *tcg_ctx.gen_opc_ptr = INDEX_op_end;
    /* Mark instruction starts for the final generated instruction */
    if (search_pc) {
        j = tcg_ctx.gen_opc_ptr - tcg_ctx.gen_opc_buf;
        lj++;
        while (lj <= j) {
            tcg_ctx.gen_opc_instr_start[lj++] = 0;
        }
    } else {
        tb->size = dc->pc - tb->pc;
        tb->icount = num_insns;
    }

#ifdef DEBUG_DISAS
    if (qemu_loglevel_mask(CPU_LOG_TB_IN_ASM)) {
        qemu_log("----------------\n");
        qemu_log("IN: %s\n", lookup_symbol(tb->pc));
        log_target_disas(env, tb->pc, dc->pc - tb->pc, 0);
        qemu_log("\nisize=%d osize=%td\n",
                    dc->pc - tb->pc, tcg_ctx.gen_opc_ptr - tcg_ctx.gen_opc_buf);
    }
#endif
}

void gen_intermediate_code(CPUNios2State *env, TranslationBlock *tb)
{
    gen_intermediate_code_internal(env, tb, 0);
}

void gen_intermediate_code_pc(CPUNios2State *env, TranslationBlock *tb)
{
    gen_intermediate_code_internal(env, tb, 1);
}

void nios2_cpu_dump_state(CPUState *cs, FILE *f, fprintf_function cpu_fprintf,
                    int flags)
{
    Nios2CPU *cpu = NIOS2_CPU(cs);
    CPUNios2State *env = &cpu->env;
    int i;

    if (!env || !f) {
        return;
    }

    cpu_fprintf(f, "IN: PC=%x %s\n",
                env->regs[R_PC], lookup_symbol(env->regs[R_PC]));

    for (i = 0; i < NIOS2_NUM_CORE_REGS; i++) {
        cpu_fprintf(f, "%9s=%8.8x ", regnames[i], env->regs[i]);
        if ((i + 1) % 4 == 0) {
            cpu_fprintf(f, "\n");
        }
    }
    cpu_fprintf(f, " mmu write: VPN=%05X PID %02X TLBACC %08X\n",
                env->mmu.pteaddr_wr & CR_PTEADDR_VPN_MASK,
                (env->mmu.tlbmisc_wr & CR_TLBMISC_PID_MASK) >> 4,
                env->mmu.tlbacc_wr);
    cpu_fprintf(f, "\n\n");
}


void nios2_translate_init(Nios2CPU *cpu)
{
    int i;

    cpu_reset(CPU(cpu));
    qemu_init_vcpu(CPU(cpu));

    cpu_env = tcg_global_reg_new_ptr(TCG_AREG0, "env");

    for (i = 0; i < NIOS2_NUM_CORE_REGS; i++) {
        cpu_R[i] = tcg_global_mem_new(TCG_AREG0,
                                      offsetof(CPUNios2State, regs[i]),
                                      regnames[i]);
    }
}

void restore_state_to_opc(CPUNios2State *env, TranslationBlock *tb, int pc_pos)
{
    env->regs[R_PC] = tcg_ctx.gen_opc_pc[pc_pos];
}

