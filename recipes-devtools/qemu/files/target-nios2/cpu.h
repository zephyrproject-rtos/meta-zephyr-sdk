/*
 * Altera Nios II virtual CPU header
 *
 * Copyright (c) 2012 Chris Wulff <crwulff@gmail.com>
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
#ifndef CPU_NIOS2_H
#define CPU_NIOS2_H

#include "config.h"
#include "qemu-common.h"

#define TARGET_LONG_BITS 32
#define NB_MMU_MODES 2

#define CPUArchState struct CPUNios2State

#include "exec/cpu-defs.h"
#include "fpu/softfloat.h"

struct CPUNios2State;
typedef struct CPUNios2State CPUNios2State;

#if !defined(CONFIG_USER_ONLY)
#include "mmu.h"
#endif

/* GP regs + CR regs + PC */
#define NIOS2_NUM_CORE_REGS (32 + 32 + 1)

typedef struct CPUNios2State {
    uint32_t regs[NIOS2_NUM_CORE_REGS];

    /* Addresses that are hard-coded in the FPGA build settings */
    uint32_t reset_addr;
    uint32_t exception_addr;
    uint32_t fast_tlb_miss_addr;

#if !defined(CONFIG_USER_ONLY)
    Nios2MMU mmu;
#endif

    CPU_COMMON

    /* interrupt controller handle for callbacks */
    DeviceState *pic_state;
    /* JTAG UART handle for callbacks */
    DeviceState *juart_state;
} CPUNios2State;

#include "cpu-qom.h"

#define TARGET_HAS_ICE 1

/* Configuration options for Nios II */
#define RESET_ADDRESS         0x00000000
#define EXCEPTION_ADDRESS     0x00000004
#define FAST_TLB_MISS_ADDRESS 0x00000008

#define ELF_MACHINE EM_ALTERA_NIOS2

/* General purpose egister aliases */
#define R_ZERO   0
#define R_AT     1
#define R_RET0   2
#define R_RET1   3
#define R_ARG0   4
#define R_ARG1   5
#define R_ARG2   6
#define R_ARG3   7
#define R_ET     24
#define R_BT     25
#define R_GP     26
#define R_SP     27
#define R_FP     28
#define R_EA     29
#define R_BA     30
#define R_RA     31

/* Control register aliases */
#define CR_BASE  32
#define CR_STATUS    (CR_BASE + 0)
#define   CR_STATUS_PIE  (1<<0)

/* The state where both EH and U are one is illegal and causes
 * undefined results.
 */

#define   CR_STATUS_U        (1<<1)        /*reads as 0  if no MMU */
#define   CR_STATUS_EH       (1<<2)        /*reads as 0  if no MMU */
#define   CR_STATUS_EIC_IH   (1<<3)        /*reads as 0  if no EIC */
#define   CR_STATUS_EIC_IL   (63<<4)       /*reads as 0  if no EIC */
#define   CR_STATUS_CRS      (63<<10)
#define   CR_STATUS_EIC_PRS  (63<<16)      /*reads as 0  if no EIC */
#define   CR_STATUS_EIC_NMI  (1<<22)       /*reads as 0  if no EIC */
#define   CR_STATUS_EIC_RSIE (1<<23)

#define CR_ESTATUS   (CR_BASE + 1)
#define CR_BSTATUS   (CR_BASE + 2)
#define CR_IENABLE   (CR_BASE + 3)
#define CR_IPENDING  (CR_BASE + 4)
#define CR_CPUID     (CR_BASE + 5)
#define CR_EXCEPTION (CR_BASE + 7)
#define CR_PTEADDR   (CR_BASE + 8)
#define   CR_PTEADDR_PTBASE_SHIFT 22
#define   CR_PTEADDR_PTBASE_MASK  (0x3FF << CR_PTEADDR_PTBASE_SHIFT)
#define   CR_PTEADDR_VPN_SHIFT    2
#define   CR_PTEADDR_VPN_MASK     (0xFFFFF << CR_PTEADDR_VPN_SHIFT)
#define CR_TLBACC    (CR_BASE + 9)
#define   CR_TLBACC_IGN_SHIFT 25
#define   CR_TLBACC_IGN_MASK  (0x7F << CR_TLBACC_IGN_SHIFT)
#define   CR_TLBACC_C         (1<<24)
#define   CR_TLBACC_R         (1<<23)
#define   CR_TLBACC_W         (1<<22)
#define   CR_TLBACC_X         (1<<21)
#define   CR_TLBACC_G         (1<<20)
#define   CR_TLBACC_PFN_MASK  0x000FFFFF
#define CR_TLBMISC   (CR_BASE + 10)
#define   CR_TLBMISC_WAY_SHIFT 20
#define   CR_TLBMISC_WAY_MASK  (0xF << CR_TLBMISC_WAY_SHIFT)
#define   CR_TLBMISC_RD        (1<<19)
#define   CR_TLBMISC_WR        (1<<18)
#define   CR_TLBMISC_PID_SHIFT 4
#define   CR_TLBMISC_PID_MASK  (0x3FFF << CR_TLBMISC_PID_SHIFT)
#define   CR_TLBMISC_DBL       (1<<3)
#define   CR_TLBMISC_BAD       (1<<2)
#define   CR_TLBMISC_PERM      (1<<1)
#define   CR_TLBMISC_D         (1<<0)
#define CR_BADADDR   (CR_BASE + 12)
#define CR_CONFIG    (CR_BASE + 13)
#define CR_MPUBASE   (CR_BASE + 14)
#define CR_MPUACC    (CR_BASE + 15)

/* Other registers */
#define R_PC         64

/* Exceptions (cause) */
#define EXCP_BREAK    -1
#define EXCP_RESET    0
#define EXCP_PRESET   1
#define EXCP_IRQ      2
#define EXCP_TRAP     3
#define EXCP_UNIMPL   4
#define EXCP_ILLEGAL  5
#define EXCP_UNALIGN  6
#define EXCP_UNALIGND 7
#define EXCP_DIV      8
#define EXCP_SUPERA   9
#define EXCP_SUPERI   10
#define EXCP_SUPERD   11
#define EXCP_TLBD     12
#define EXCP_TLBX     13
#define EXCP_TLBR     14
#define EXCP_TLBW     15
#define EXCP_MPUI     16
#define EXCP_MPUD     17

#define CPU_INTERRUPT_NMI       CPU_INTERRUPT_TGT_EXT_3

#include "cpu-qom.h"

void nios2_translate_init(Nios2CPU *cpu);
Nios2CPU *cpu_nios2_init(const char *cpu_model);
int cpu_nios2_exec(CPUNios2State *s);
void cpu_nios2_close(CPUNios2State *s);
int cpu_nios2_signal_handler(int host_signum, void *pinfo, void *puc);

void dump_mmu(FILE *f, fprintf_function cpu_fprintf, CPUNios2State *env);

#define TARGET_PHYS_ADDR_SPACE_BITS 32
#define TARGET_VIRT_ADDR_SPACE_BITS 32

static inline CPUNios2State *cpu_init(const char *cpu_model)
{
    Nios2CPU *cpu = cpu_nios2_init(cpu_model);
    if (cpu == NULL) {
        return NULL;
    }
    return &cpu->env;
}

#define cpu_exec cpu_nios2_exec
#define cpu_gen_code cpu_nios2_gen_code
#define cpu_signal_handler cpu_nios2_signal_handler

#define CPU_SAVE_VERSION 1

#define TARGET_PAGE_BITS 12

/* MMU modes definitions */
#define MMU_MODE0_SUFFIX _kernel
#define MMU_MODE1_SUFFIX _user
#define MMU_SUPERVISOR_IDX  0
#define MMU_USER_IDX        1

static inline int cpu_mmu_index(CPUNios2State *env)
{
    return (env->regs[CR_STATUS] & CR_STATUS_U) ? MMU_USER_IDX :
                                                  MMU_SUPERVISOR_IDX;
}

int cpu_nios2_handle_mmu_fault(CPUState *cs, target_ulong address,
                               int rw, int mmu_idx);

#if defined(CONFIG_USER_ONLY)
static inline void cpu_clone_regs(CPUNios2State *env, target_ulong newsp)
{
    if (newsp) {
        env->regs[R_SP] = newsp;
    }
    env->regs[R_RET0] = 0;
}
#endif

static inline void cpu_set_tls(CPUNios2State *env, target_ulong newtls)
{
}

static inline int cpu_interrupts_enabled(CPUNios2State *env)
{
    return env->regs[CR_STATUS] & CR_STATUS_PIE;
}

#include "exec/cpu-all.h"

static inline target_ulong cpu_get_pc(CPUNios2State *env)
{
    return env->regs[R_PC];
}

static inline void cpu_get_tb_cpu_state(CPUNios2State *env, target_ulong *pc,
                                        target_ulong *cs_base, int *flags)
{
    *pc = env->regs[R_PC];
    *cs_base = 0;
    *flags = (env->regs[CR_STATUS] & (CR_STATUS_EH | CR_STATUS_U));
}

#if !defined(CONFIG_USER_ONLY)
void nios2_cpu_unassigned_access(CPUState *cpu, hwaddr addr,
                              bool is_write, bool is_exec, int is_asi,
                              unsigned size);
#endif

#include "exec/exec-all.h"

static inline void cpu_pc_from_tb(CPUNios2State *env, TranslationBlock *tb)
{
    env->regs[R_PC] = tb->pc;
}

#endif /* CPU_NIOS2_H */

