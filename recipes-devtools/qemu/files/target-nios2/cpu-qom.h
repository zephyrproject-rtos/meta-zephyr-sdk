/*
 * QEMU Nios2 CPU
 *
 * Copyright (c) 2016 Intel Corporation
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
#ifndef QEMU_NIOS2_CPU_QOM_H
#define QEMU_NIOS2_CPU_QOM_H

#include "qom/cpu.h"

#define TYPE_NIOS2_CPU "nios2-cpu"
#define TYPE_NIOS2_CPU_NO_MMU "nios2-cpu-no-mmu"

#define NIOS2_CPU_CLASS(klass) \
    OBJECT_CLASS_CHECK(Nios2CPUClass, (klass), TYPE_NIOS2_CPU)
#define NIOS2_CPU(obj) \
    OBJECT_CHECK(Nios2CPU, (obj), TYPE_NIOS2_CPU)
#define NIOS2_CPU_GET_CLASS(obj) \
    OBJECT_GET_CLASS(Nios2CPUClass, (obj), TYPE_NIOS2_CPU)

/**
 * Nios2CPUClass:
 * @parent_realize: The parent class' realize handler.
 * @parent_reset: The parent class' reset handler.
 * @vr: Version Register value.
 *
 * A Nios2 CPU model.
 */
typedef struct Nios2CPUClass {
    /*< private >*/
    CPUClass parent_class;
    /*< public >*/

    DeviceRealize parent_realize;
    void (*parent_reset)(CPUState *cpu);

    uint32_t vr;
} Nios2CPUClass;

/**
 * Nios2CPU:
 * @env: #CPUNios2State
 *
 * A Nios2S CPU.
 */
typedef struct Nios2CPU {
    /*< private >*/
    CPUState parent_obj;
    /*< public >*/

    CPUNios2State env;
    bool mmu_present;
} Nios2CPU;

static inline Nios2CPU *nios2_env_get_cpu(CPUNios2State *env)
{
    return container_of(env, Nios2CPU, env);
}

#define ENV_GET_CPU(e) CPU(nios2_env_get_cpu(e))

#define ENV_OFFSET offsetof(Nios2CPU, env)

void nios2_cpu_do_interrupt(CPUState *cs);
void nios2_cpu_dump_state(CPUState *cpu, FILE *f, fprintf_function cpu_fprintf,
                          int flags);
hwaddr nios2_cpu_get_phys_page_debug(CPUState *cpu, vaddr addr);
int nios2_cpu_gdb_read_register(CPUState *cpu, uint8_t *buf, int reg);
int nios2_cpu_gdb_write_register(CPUState *cpu, uint8_t *buf, int reg);

#endif
