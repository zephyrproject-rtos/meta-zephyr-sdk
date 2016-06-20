/*
 * Nios2 gdb server stub
 *
 * Copyright (c) 2016 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "qemu-common.h"
#include "exec/gdbstub.h"

/*
    The Nios II architecture supports a flat register file, consisting of
    thirty-two 32-bit general-purpose integer registers, and up to thirty-two
    32-bit control registers.

    However, GDB expects exactly (and in that order):
         32 general registers
         1  PC
         16 control registers

     In particular, this is the order expected by GDB:

     0  "zero"
     1  "at"
     2  "r2"
     3  "r3"
     4  "r4"
     5  "r5"
     6  "r6"
     7  "r7"
     8  "r8"
     9  "r9"
    10  "r10"
    11  "r11"
    12  "r12"
    13  "r13"
    14  "r14"
    15  "r15"
    16  "r16"
    17  "r17"
    18  "r18"
    19  "r19"
    20  "r20"
    21  "r21"
    22  "r22"
    23  "r23"
    24  "et"
    25  "bt"
    26  "gp"
    27  "sp"
    28  "fp"
    29  "ea"
    30  "sstatus"
    31  "ra"
    32  "pc"
    33  "status"
    34  "estatus"
    35  "bstatus"
    36  "ienable"
    37  "ipending"
    38  "cpuid"
    39  "ctl6"
    40  "exception"
    41  "pteaddr"
    42  "tlbacc"
    43  "tlbmisc"
    44  "eccinj"
    45  "badaddr"
    46  "config"
    47  "mpubase"
    48  "mpuacc"

*/

int nios2_cpu_gdb_read_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    Nios2CPU *cpu = NIOS2_CPU(cs);
    CPUNios2State *env = &cpu->env;

    if (n < 32) {
        return gdb_get_reg32(mem_buf, env->regs[n]);
    } else {
        switch (n) {
        case 32:
            return gdb_get_reg32(mem_buf, env->regs[R_PC]);
        case 33:
            return gdb_get_reg32(mem_buf, env->regs[CR_STATUS]);
        case 34:
            return gdb_get_reg32(mem_buf, env->regs[CR_ESTATUS]);
        case 35:
            return gdb_get_reg32(mem_buf, env->regs[CR_BSTATUS]);
        case 36:
            return gdb_get_reg32(mem_buf, env->regs[CR_IENABLE]);
        case 37:
            return gdb_get_reg32(mem_buf, env->regs[CR_IPENDING]);
        case 38:
            return gdb_get_reg32(mem_buf, env->regs[CR_CPUID]);
        case 39:
            return gdb_get_reg32(mem_buf, env->regs[CR_CTL6]);
        case 40:
            return gdb_get_reg32(mem_buf, env->regs[CR_EXCEPTION]);
        case 41:
            return gdb_get_reg32(mem_buf, env->regs[CR_PTEADDR]);
        case 42:
            return gdb_get_reg32(mem_buf, env->regs[CR_TLBACC]);
        case 43:
            return gdb_get_reg32(mem_buf, env->regs[CR_TLBMISC]);
        case 44:
            return gdb_get_reg32(mem_buf, env->regs[CR_ECCINJ]);
        case 45:
            return gdb_get_reg32(mem_buf, env->regs[CR_BADADDR]);
        case 46:
            return gdb_get_reg32(mem_buf, env->regs[CR_CONFIG]);
        case 47:
            return gdb_get_reg32(mem_buf, env->regs[CR_MPUBASE]);
        case 48:
            return gdb_get_reg32(mem_buf, env->regs[CR_MPUACC]);
        default:
            return 0;
        }
    }
}


int nios2_cpu_gdb_write_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    Nios2CPU *cpu = NIOS2_CPU(cs);
    CPUNios2State *env = &cpu->env;
    CPUClass *cc = CPU_GET_CLASS(cs);
    uint32_t tmp;

    if (n > cc->gdb_num_core_regs) {
        return 0;
    }

    tmp = ldl_p(mem_buf);

    if (n < 32) {
        env->regs[n] = tmp;
    } else {
        switch (n) {
        case 32:
            env->regs[R_PC] = tmp;
            break;
        case 33:
            env->regs[CR_STATUS] = tmp;
            break;
        case 34:
            env->regs[CR_ESTATUS] = tmp;
            break;
        case 35:
            env->regs[CR_BSTATUS] = tmp;
            break;
        case 36:
            env->regs[CR_IENABLE] = tmp;
            break;
        case 37:
            env->regs[CR_IPENDING] = tmp;
            break;
        case 38:
            env->regs[CR_CPUID] = tmp;
            break;
        case 39:
            env->regs[CR_CTL6] = tmp;
            break;
        case 40:
            env->regs[CR_EXCEPTION] = tmp;
            break;
        case 41:
            env->regs[CR_PTEADDR] = tmp;
            break;
        case 42:
            env->regs[CR_TLBACC] = tmp;
            break;
        case 43:
            env->regs[CR_TLBMISC] = tmp;
            break;
        case 44:
            env->regs[CR_ECCINJ] = tmp;
            break;
        case 45:
            env->regs[CR_BADADDR] = tmp;
            break;
        case 46:
            env->regs[CR_CONFIG] = tmp;
            break;
        case 47:
            env->regs[CR_MPUBASE] = tmp;
            break;
        case 48:
            env->regs[CR_MPUACC] = tmp;
            break;
        }
    }

    return 4;
}
