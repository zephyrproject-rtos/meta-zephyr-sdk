/*
 * QEMU Nios II CPU
 *
 * Copyright (c) 2016 Intel Corporation
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

#include "cpu.h"
#include "qemu-common.h"
#include "hw/qdev-properties.h"

static void nios2_cpu_set_pc(CPUState *cs, vaddr value)
{
    Nios2CPU *cpu = NIOS2_CPU(cs);
    CPUNios2State *env = &cpu->env;

    env->regs[R_PC] = value;
}
static bool nios2_cpu_has_work(CPUState *cs)
{
    return cs->interrupt_request & (CPU_INTERRUPT_HARD | CPU_INTERRUPT_NMI);
}

/* CPUClass::reset() */
static void nios2_cpu_reset(CPUState *cs)
{
    Nios2CPU *cpu = NIOS2_CPU(cs);
    Nios2CPUClass *ncc = NIOS2_CPU_GET_CLASS(cpu);
    CPUNios2State *env = &cpu->env;

    if (qemu_loglevel_mask(CPU_LOG_RESET)) {
        qemu_log("CPU Reset (CPU %d)\n", cs->cpu_index);
        log_cpu_state(cs, 0);
    }

    ncc->parent_reset(cs);
    tlb_flush(cs, 1);
    memset(env->regs, 0, sizeof(uint32_t) * NIOS2_NUM_CORE_REGS);
    env->regs[R_PC] = env->reset_addr;

#if defined(CONFIG_USER_ONLY)
    /* start in user mode with interrupts enabled.  */
    env->regs[CR_STATUS] = CR_STATUS_U | CR_STATUS_PIE;
#else
    mmu_init(env);
#endif
}

static void nios2_cpu_realizefn(DeviceState *dev, Error **errp)
{
    CPUState *cs = CPU(dev);
    Nios2CPUClass *mcc = NIOS2_CPU_GET_CLASS(dev);

    cpu_reset(cs);
    qemu_init_vcpu(cs);

    mcc->parent_realize(dev, errp);
}

static void nios2_cpu_initfn(Object *obj)
{
    CPUState *cs = CPU(obj);
    Nios2CPU *cpu = NIOS2_CPU(obj);

    CPUNios2State *env = &cpu->env;
    static bool tcg_initialized;

    cpu->mmu_present = true;
    cs->env_ptr = env;
    cpu_exec_init(env);

    if (tcg_enabled() && !tcg_initialized) {
        tcg_initialized = true;
        nios2_translate_init(cpu);
  //      cpu_set_debug_excp_handler(nios2_debug_excp_handler);
    }
}

static Property nios2_properties[] = {
    DEFINE_PROP_BOOL("mmu_present", Nios2CPU, mmu_present, false),
    DEFINE_PROP_END_OF_LIST(),
};

static void nios2_cpu_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    CPUClass *cc = CPU_CLASS(oc);
    Nios2CPUClass *ncc = NIOS2_CPU_CLASS(oc);

    ncc->parent_realize = dc->realize;
    dc->realize = nios2_cpu_realizefn;
    dc->props = nios2_properties;
    ncc->parent_reset = cc->reset;
    cc->reset = nios2_cpu_reset;
    cc->has_work = nios2_cpu_has_work;
    cc->do_interrupt = nios2_cpu_do_interrupt;
    cc->dump_state = nios2_cpu_dump_state;
    cc->set_pc = nios2_cpu_set_pc;
    cc->gdb_read_register = nios2_cpu_gdb_read_register;
    cc->gdb_write_register = nios2_cpu_gdb_write_register;
#ifdef CONFIG_USER_ONLY
    cc->handle_mmu_fault = nios2_cpu_handle_mmu_fault;
#else
    cc->do_unassigned_access = nios2_cpu_unassigned_access;
    cc->get_phys_page_debug = nios2_cpu_get_phys_page_debug;
#endif
    cc->gdb_num_core_regs = 32;
}

static const TypeInfo nios2_cpu_type_info = {
    .name = TYPE_NIOS2_CPU,
    .parent = TYPE_CPU,
    .instance_size = sizeof(Nios2CPU),
    .instance_init = nios2_cpu_initfn,
    .abstract = false,
    .class_size = sizeof(Nios2CPUClass),
    .class_init = nios2_cpu_class_init,
};

static void nios2_cpu_register_types(void)
{
    type_register_static(&nios2_cpu_type_info);
}

type_init(nios2_cpu_register_types)
