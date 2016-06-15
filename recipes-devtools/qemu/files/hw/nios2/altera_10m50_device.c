/*
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

#include "hw/sysbus.h"
#include "hw/hw.h"
#include "sysemu/sysemu.h"
#include "hw/devices.h"
#include "hw/boards.h"
#include "sysemu/device_tree.h"
#include "hw/loader.h"
#include "elf.h"
#include "qemu/log.h"
#include "exec/address-spaces.h"
#include "sysemu/blockdev.h"
#include "qapi/qmp/qerror.h"
#include "hw/nios2/altera.h"
#include <libfdt.h>

#define DEBUG

#ifdef DEBUG
# define DPRINTF(format, ...)     printf(format, ## __VA_ARGS__)
#else
# define DPRINTF(format, ...)     do { } while (0)
#endif

#define BINARY_DEVICE_TREE_FILE "altera_10m50_devboard.dtb"

static struct {
    uint32_t bootstrap_pc;
    uint32_t cmdline;
    uint32_t initrd_start;
    uint32_t initrd_end;
    uint32_t fdt;
} boot_info;

static void *altera_load_device_tree(int *fdt_size)
{
    char *path;
    void *fdt = NULL;
    const char *dtb_filename;

    dtb_filename = qemu_opt_get(qemu_get_machine_opts(), "dtb");
    if (dtb_filename) {
        fdt = load_device_tree(dtb_filename, fdt_size);
        if (!fdt) {
            error_report("Error while loading device tree file '%s'",
                dtb_filename);
        }
    } else {
        /* Try the local "altera.dtb" override.  */
        fdt = load_device_tree("altera.dtb", fdt_size);
        if (!fdt) {
            path = qemu_find_file(QEMU_FILE_TYPE_BIOS, BINARY_DEVICE_TREE_FILE);
            if (path) {
                fdt = load_device_tree(path, fdt_size);
                g_free(path);
            }
        }
    }

    return fdt;
}

static int altera_load_dtb(hwaddr addr, uint32_t ramsize, uint32_t initrd_start,
    uint32_t initrd_end, const char *kernel_cmdline, const char *dtb_filename)
{
    int fdt_size;
    void *fdt = NULL;
    int r;

    if (dtb_filename) {
        fdt = load_device_tree(dtb_filename, &fdt_size);
    }

    if (!fdt) {
        printf("Failed to load the DTB file %s\n",dtb_filename);
        return 0;
    }

    if (kernel_cmdline) {
        r = qemu_fdt_setprop_string(fdt, "/chosen", "bootargs",
                                    kernel_cmdline);
        if (r < 0) {
            fprintf(stderr, "couldn't set /chosen/bootargs\n");
        }
    }

    if (initrd_start) {
        qemu_fdt_setprop_cell(fdt, "/chosen", "linux,initrd-start",
                              initrd_start);

        qemu_fdt_setprop_cell(fdt, "/chosen", "linux,initrd-end",
                              initrd_end);
    }

    cpu_physical_memory_write(addr, fdt, fdt_size);
    return fdt_size;
}

/*
 * args passed from u-boot, called from head.S
 *
 * @r4: NIOS magic 0x534f494e
 * @r5: initrd start
 * @r6: initrd end or fdt
 * @r7: kernel command line
 */

static void main_cpu_reset(void *opaque)
{
    Nios2CPU *cpu = opaque;
    CPUNios2State *env = &cpu->env;

    cpu_reset(CPU(cpu));

    env->regs[4] = 0x534f494e;
    env->regs[5] = boot_info.initrd_start;
    env->regs[6] = boot_info.fdt;
    env->regs[7] = boot_info.cmdline;
    env->regs[R_PC] = boot_info.bootstrap_pc;
}

static uint64_t translate_kernel_address(void *opaque, uint64_t addr)
{
    return addr - 0xC0000000LL;
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

static inline DeviceState *altera_pic_init(Nios2CPU *cpu,
        qemu_irq cpu_irq)
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

static int find_compatible_controller(void *fdt, int node,
            const char *compat, hwaddr *addr, int *irq, uint32_t * freq)
{
    node = fdt_node_offset_by_compatible(fdt, node, compat);
    if (node >= 0) {
        int len;
        #ifdef DEBUG
        const char *name = fdt_get_name(fdt, node, &len);
        DPRINTF("controller: %s\n", name);
        #endif
        const int *cell = fdt_getprop_w(fdt, node, "interrupts", &len);
        if (cell && len >= sizeof(int)) {
           *irq = fdt32_to_cpu(cell[0]);
           DPRINTF("irq: %d\n", *irq);
        } else {
            goto Barf;
        }

        const int *reg = fdt_getprop(fdt, node, "reg", &len);
        if (reg && len >= sizeof(int)) {
            *addr = fdt32_to_cpu(reg[0]);
            DPRINTF("reg: %llx [len:%d]\n",*addr,len);
        } else {
            goto Barf;
        }

        if (freq != NULL) {
            const int *fr = fdt_getprop(fdt, node, "clock-frequency", &len);
            if (fr && len >= sizeof(int)) {
                *freq = fdt32_to_cpu(fr[0]);
            DPRINTF("frequency: %d [len:%d]\n",*freq,len);
            } else {
                goto Barf;
            }
        }
    }
    return node;
Barf:
    fprintf(stderr,"Error: failed to parse \"%s\"\n",compat);
    exit(1);
}

static void init_cpu_from_devtree(void *fdt, Nios2CPU *cpu)
{
    cpu->env.reset_addr =
        qemu_fdt_getprop_cell(fdt, "/cpus/cpu", "altr,reset-addr");

    cpu->env.exception_addr =
        qemu_fdt_getprop_cell(fdt, "/cpus/cpu", "altr,exception-addr");

    cpu->env.fast_tlb_miss_addr =
        qemu_fdt_getprop_cell(fdt, "/cpus/cpu", "altr,fast-tlb-miss-addr");

    DPRINTF("\tcpu->env.reset_addr: \t\t%0x\n", cpu->env.reset_addr);
    DPRINTF("\tcpu->env.exception_addr: \t%0x\n", cpu->env.exception_addr);
    DPRINTF("\tcpu->env.fast_tlb_miss_addr: \t%0x\n",cpu->env.fast_tlb_miss_addr);

    cpu->env.mmu.pid_bits =
        qemu_fdt_getprop_cell(fdt, "/cpus/cpu", "altr,pid-num-bits");
    cpu->env.mmu.tlb_num_ways =
        qemu_fdt_getprop_cell(fdt, "/cpus/cpu", "altr,tlb-num-ways");
    cpu->env.mmu.tlb_num_entries =
        qemu_fdt_getprop_cell(fdt, "/cpus/cpu", "altr,tlb-num-entries");
    cpu->env.mmu.tlb_entry_mask =
        (cpu->env.mmu.tlb_num_entries/cpu->env.mmu.tlb_num_ways) - 1;
}

#define MEMORY_BASEADDR 0x08000000
#define LMB_BRAM_SIZE   (0x1024*4)

static void altera_10m50_init_common(Nios2CPU *cpu, MachineState *machine)
{
    ram_addr_t ram_size = machine->ram_size;
    const char *kernel_filename;
    const char *kernel_cmdline;
    MemoryRegion *address_space_mem = get_system_memory();
    MemoryRegion *phys_ram = g_new(MemoryRegion, 1);
    CPUNios2State *env = &cpu->env;
    const char *dtb_arg;
    int i, kernel_size, node, interrupt,initrd_size = 0;
    hwaddr base, ddr_base = MEMORY_BASEADDR,initrd_base = 0;
    qemu_irq *cpu_irq, irq[32];
    QemuOpts *machine_opts;
    int fdt_size;
    void *fdt = altera_load_device_tree(&fdt_size);

    if (!fdt)
        return;

    object_property_set_bool(OBJECT(cpu), true, "realized", &error_abort);

    machine_opts = qemu_get_machine_opts();
    kernel_filename = qemu_opt_get(machine_opts, "kernel");
    kernel_cmdline = qemu_opt_get(machine_opts, "append");

    const char * dtb_filename = BINARY_DEVICE_TREE_FILE;
    dtb_arg = qemu_opt_get(machine_opts, "dtb");
    if (dtb_arg) { /* Preference a -dtb argument */
        dtb_filename = dtb_arg;
    } else { /* default to pcbios dtb as passed by machine_init */
        dtb_filename = qemu_find_file(QEMU_FILE_TYPE_BIOS, dtb_filename);
    }

    DPRINTF("\n\tkernel_filename: %s\n", kernel_filename);
    DPRINTF("\tinitrd: %s\n",machine->initrd_filename);
    DPRINTF("\tkernel_cmdline:  %s\n", kernel_cmdline);
    DPRINTF("\tdtb filename:  %s\n", dtb_filename);
    DPRINTF("\tmmu_present:  %d\n\n", cpu->mmu_present);

    MemoryRegion *phys_lmb_bram = g_new(MemoryRegion, 1);
    MemoryRegion *phys_lmb_bram_alias = g_new(MemoryRegion, 1);
    MemoryRegion *phys_ram_alias = g_new(MemoryRegion, 1);

    memory_region_init_ram(phys_lmb_bram, NULL, "nios2.lmb_bram", LMB_BRAM_SIZE);
    memory_region_init_alias(phys_lmb_bram_alias, NULL, "nios2.lmb_bram.alias",
                             phys_lmb_bram, 0, LMB_BRAM_SIZE);
    vmstate_register_ram_global(phys_lmb_bram);
    memory_region_add_subregion(address_space_mem, 0x00000000, phys_lmb_bram);
    memory_region_add_subregion(address_space_mem, 0xc0000000, phys_lmb_bram_alias);

    memory_region_init_ram(phys_ram, NULL, "nios2.ram", ram_size);
    vmstate_register_ram_global(phys_ram);
    memory_region_add_subregion(address_space_mem, ddr_base, phys_ram);
    memory_region_init_alias(phys_ram_alias, NULL, "nios2.ram.mirror",
                             phys_ram, 0, ram_size);
    memory_region_add_subregion(address_space_mem, ddr_base + 0xc0000000,
                             phys_ram_alias);

     /* Create irq lines */
    cpu_irq = qemu_allocate_irqs(cpu_irq_handler, cpu, 2);
    env->pic_state = altera_pic_init(cpu,*cpu_irq);

    /* Nios2 IIC has 32 interrupt-request inputs*/
    for (i = 0; i < 32; i++) {
        irq[i] = qdev_get_gpio_in(env->pic_state, i);
    }

#if 1
    find_compatible_controller(fdt, -1, "altr,juart-1.0", &base,
                                                    &interrupt, NULL);
    altera_juart_create(0, base, irq[interrupt]);
    find_compatible_controller(fdt, -1, "altr,uart-1.0", &base,
                                                    &interrupt, NULL);
    altera_uart_create(1, base, irq[interrupt]);
#else
    /* Register: Altera 16550 UART */
    serial_mm_init(address_space_mem, 0xf8001600, 2, irq[1], 115200,
                                serial_hds[0], DEVICE_NATIVE_ENDIAN);
#endif

    node = -1;
    do {
        uint32_t frequency;
        node = find_compatible_controller(fdt, node, "altr,timer-1.0",
                                        &base, &interrupt, &frequency);
        if (node >= 0) {
            altera_timer_create(base, irq[interrupt],frequency);
        }
    } while (node > 0);

    init_cpu_from_devtree(fdt, cpu);

    if (kernel_filename) {
        uint64_t entry = 0, low = 0, high = 0;
        uint32_t base32 = 0;

        /* Boots a kernel elf binary.  */
        kernel_size = load_elf(kernel_filename, NULL, NULL,
                               &entry, &low, &high,
                               0, EM_ALTERA_NIOS2, 0);
        base32 = entry;
        if (base32 == 0xc8000000) {
            kernel_size = load_elf(kernel_filename, translate_kernel_address,
                                   NULL, &entry, NULL, NULL,
                                   0, EM_ALTERA_NIOS2, 0);
            DPRINTF("(ELF image) \n  kernel_size:%x\n",kernel_size);
        }
        /* Always boot into physical ram.  */
        boot_info.bootstrap_pc = ddr_base + 0xc0000000 + (entry & 0x07ffffff);

        /* If it wasn't an ELF image, try an u-boot image.  */
        if (kernel_size < 0) {
            hwaddr uentry, loadaddr;

            kernel_size = load_uimage(kernel_filename, &uentry, &loadaddr, 0,
                                      NULL, NULL);
            if (kernel_size > 0) {
                DPRINTF("(u-boot image) \n  load_addr:%llx \n  uentry:%llx \n  kernel_size:%x\n",
                    loadaddr, uentry, kernel_size);
                boot_info.bootstrap_pc = uentry;
                high = (loadaddr + kernel_size + 3) & ~3;
            }
        }

        /* Not an ELF image nor an u-boot image, try a RAW image.  */
        if (kernel_size < 0) {
            kernel_size = load_image_targphys(kernel_filename, ddr_base,
                                              ram_size);
            DPRINTF("(RAW binary image) \n  kernel_size:%x\n",
                kernel_size);
            boot_info.bootstrap_pc = ddr_base;
            high = (ddr_base + kernel_size + 3) & ~3;
        }

        /* Load initrd. */
        if (machine->initrd_filename) {
            initrd_base = high = ROUND_UP(high, 4);
            initrd_size = load_image_targphys(machine->initrd_filename,
                                              high, ram_size - high);

            if (initrd_size < 0) {
                error_report("couldn't load ram disk '%s'",
                             machine->initrd_filename);
                exit(1);
            }
            high = ROUND_UP(high + initrd_size, 4);
            boot_info.initrd_start = initrd_base;
            boot_info.initrd_end = initrd_base + initrd_size;
        }

        boot_info.cmdline = high + 4096;
        if (machine->kernel_cmdline && strlen(machine->kernel_cmdline)) {
            pstrcpy_targphys("cmdline", boot_info.cmdline, 256,
                     machine->kernel_cmdline);
        }

        /* Provide a device-tree.  */
        boot_info.fdt = boot_info.cmdline + 4096;
        altera_load_dtb(boot_info.fdt, ram_size,
                            boot_info.initrd_start,
                            boot_info.initrd_end,
                            kernel_cmdline,
                            dtb_filename);
    }
    qemu_register_reset(main_cpu_reset, cpu);
}

static void altera_10m50_init_no_mmu(MachineState *machine)
{
    /* Init CPU */
    Nios2CPU *cpu = NIOS2_CPU(object_new(TYPE_NIOS2_CPU));
    object_property_set_bool(OBJECT(cpu), false, "mmu_present", &error_abort);
    altera_10m50_init_common(cpu,machine);
}

static void altera_10m50_init_mmu(MachineState *machine)
{
    /* Init CPU */
    Nios2CPU *cpu = NIOS2_CPU(object_new(TYPE_NIOS2_CPU));
    object_property_set_bool(OBJECT(cpu), true, "mmu_present", &error_abort);
    altera_10m50_init_common(cpu,machine);
}

static QEMUMachine altera_10m50_mmu_machine = {
    .name = "altera_10m50_mmu",
    .desc = "Altera 10m50 device with MMU.",
    .init = altera_10m50_init_mmu,
    .is_default = 0,
};

static QEMUMachine altera_10m50_no_mmu_machine = {
    .name = "altera_10m50_no_mmu",
    .desc = "Altera 10m50 device without MMU.",
    .init = altera_10m50_init_no_mmu,
    .is_default = 0,
};

static void altera_10m50_machine_init(void)
{
    qemu_register_machine(&altera_10m50_mmu_machine);
    qemu_register_machine(&altera_10m50_no_mmu_machine);
}

machine_init(altera_10m50_machine_init);

