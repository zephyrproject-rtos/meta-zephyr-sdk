/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hw/sysbus.h"
#include "hw/hw.h"
#include "sysemu/sysemu.h"
#include "hw/devices.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "elf.h"
#include "exec/address-spaces.h"
#include "qapi/qmp/qerror.h"
#include "hw/nios2/altera.h"
#include "hw/nios2/zephyr/system.h"

static struct {
    uint32_t bootstrap_pc;
} boot_info_zephyr;

static void main_cpu_reset(void *opaque)
{
    Nios2CPU *cpu = opaque;
    CPUNios2State *env = &cpu->env;

    cpu_reset(CPU(cpu));
    env->regs[R_PC]   = boot_info_zephyr.bootstrap_pc;
}

static void cpu_irq_handler(void *opaque, int irq, int level)
{
    Nios2CPU *cpu = opaque;
    CPUState *cs = CPU(cpu);

    int type = irq ? CPU_INTERRUPT_NMI : CPU_INTERRUPT_HARD;

    if (level) {
        cpu_interrupt(cs, type);
    } else {
        cpu_reset_interrupt(cs, type);
    }
}

static inline DeviceState *altera_pic_init(Nios2CPU *cpu, qemu_irq cpu_irq)
{
    DeviceState *dev;
    SysBusDevice *d;

    dev = qdev_create(NULL, "altera,iic");
    qdev_prop_set_ptr(dev, "cpu", cpu);
    qdev_init_nofail(dev);
    d = SYS_BUS_DEVICE(dev);
    sysbus_connect_irq(d, 0, cpu_irq);

    return dev;
}

#define ROM_BASE RESET_REGION_BASE
#define ROM_SIZE RESET_REGION_SPAN

#define RAM_BASE ONCHIP_MEMORY2_0_BASE
#define RAM_SIZE ONCHIP_MEMORY2_0_SPAN

static void altera_10m50_zephyr_init( MachineState *machine)
{
    const char *kernel_filename;
    MemoryRegion *sysmem = get_system_memory();
    Nios2CPU *cpu;
    CPUNios2State *env;
    int i;
    QemuOpts *machine_opts;
    int kernel_size;
    qemu_irq *cpu_irq, irq[32];

    MemoryRegion *rom = g_new(MemoryRegion, 1);
    MemoryRegion *ram = g_new(MemoryRegion, 1);

    cpu = NIOS2_CPU(object_new(TYPE_NIOS2_CPU));
    cpu->mmu_present = false;
    env = &cpu->env;

    object_property_set_bool(OBJECT(cpu), true, "realized", &error_abort);
    machine_opts = qemu_get_machine_opts();
    kernel_filename = qemu_opt_get(machine_opts, "kernel");

    memory_region_init_ram(rom, NULL, "nios2.rom", ROM_SIZE);
    vmstate_register_ram_global(rom);
    memory_region_set_readonly(rom, true);
    memory_region_add_subregion(sysmem, ROM_BASE, rom);

    memory_region_init_ram(ram, NULL, "nios2.ram", RAM_SIZE);
    vmstate_register_ram_global(ram);
    memory_region_add_subregion(sysmem, RAM_BASE, ram);

     /* Create irq lines */
    cpu_irq = qemu_allocate_irqs(cpu_irq_handler, cpu, 2);
    env->pic_state = altera_pic_init(cpu,*cpu_irq);

    /* Nios2 IIC has 32 interrupt-request inputs*/
    for (i = 0; i < 32; i++) {
        irq[i] = qdev_get_gpio_in(env->pic_state, i);
    }

    altera_juart_create(0, JTAG_UART_0_BASE, irq[JTAG_UART_0_IRQ]);
    altera_uart_create(1, A_16550_UART_0_BASE, irq[A_16550_UART_0_IRQ]);
    altera_timer_create(TIMER_0_BASE, irq[TIMER_0_IRQ], TIMER_0_FREQ);

    cpu->env.reset_addr = ALT_CPU_RESET_ADDR;
    cpu->env.exception_addr = ALT_CPU_EXCEPTION_ADDR;
    cpu->env.fast_tlb_miss_addr = ALT_CPU_RESET_ADDR;

    if (kernel_filename) {
        uint64_t entry;

        /* Boots a kernel elf binary.  */
        kernel_size = load_elf(kernel_filename, NULL, NULL,
                               &entry, NULL, NULL,
                               0, EM_ALTERA_NIOS2, 0);

        boot_info_zephyr.bootstrap_pc = entry;

        /* Not an ELF image, try a RAW image. */
        if (kernel_size < 0) {
            kernel_size = load_image_targphys(kernel_filename, RAM_BASE,
                                              RAM_SIZE);
            boot_info_zephyr.bootstrap_pc = RAM_BASE;
        }

        if (kernel_size < 0) {
            fprintf(stderr, "qemu: could not load kernel '%s'\n",
                    kernel_filename);
            exit(1);
        }
    }

    qemu_register_reset(main_cpu_reset, cpu);
}

static QEMUMachine altera_10m50_zephyr_machine = {
    .name = "altera_10m50_zephyr",
    .desc = "Altera 10m50 for Zephyr.",
    .init = altera_10m50_zephyr_init,
    .is_default = 1,
};

static void altera_10m50_zephyr_machine_init(void)
{
    qemu_register_machine(&altera_10m50_zephyr_machine);
}

machine_init(altera_10m50_zephyr_machine_init);

