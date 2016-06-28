/*
 * QEMU model of the Altera timer.
 *
 * Copyright (c) 2016 Intel Corporation.
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

#include "hw/sysbus.h"
#include "sysemu/sysemu.h"
#include "hw/ptimer.h"
#include "hw/nios2/altera.h"

//#define DEBUG

#ifdef DEBUG
# define DPRINTF(format, ...)     printf(format, ## __VA_ARGS__)
#else
# define DPRINTF(format, ...)     do { } while (0)
#endif


#define DEFAULT_FREQUENCY 75000000

#define R_STATUS     0
#define R_CONTROL    1
#define R_PERIODL    2
#define R_PERIODH    3
#define R_SNAPL      4
#define R_SNAPH      5
#define R_MAX        6

#define STATUS_TO  0x0001
#define STATUS_RUN 0x0002

#define CONTROL_ITO   0x0001
#define CONTROL_CONT  0x0002
#define CONTROL_START 0x0004
#define CONTROL_STOP  0x0008

#define TYPE_ALTERA_TIMER "altera-timer"
#define ALTERA_TIMER(obj) \
    OBJECT_CHECK(AlteraTimerState, (obj), TYPE_ALTERA_TIMER)

typedef struct {
    SysBusDevice parent_obj;

    MemoryRegion  mmio;
    QEMUBH *bh;
    ptimer_state *ptimer;
    qemu_irq irq;
    uint32_t freq_hz;
    uint32_t regs[R_MAX];
} AlteraTimerState;


static uint64_t timer_read(void *opaque, hwaddr addr,
                           unsigned int size)
{
    AlteraTimerState *t = opaque;
    uint64_t r = 0;
    addr >>= 2;
    addr &= 0x7;

    switch (addr) {
    case R_STATUS:
        r = t->regs[R_STATUS];
        break;

    case R_CONTROL:
        r = t->regs[R_CONTROL];
        break;

    case R_SNAPL:
        r = t->regs[addr];
        break;

    default:
        if (addr < ARRAY_SIZE(t->regs)) {
            r = t->regs[addr];
        }
        break;
    }

    return r;
}

static void timer_start(AlteraTimerState *t)
{
    ptimer_stop(t->ptimer);
    ptimer_set_count(t->ptimer, (t->regs[R_PERIODH]<<16) | t->regs[R_PERIODL]);
    ptimer_run(t->ptimer, 1);
}

static inline int timer_irq_state(AlteraTimerState *t)
{
    return (t->regs[R_STATUS] & t->regs[R_CONTROL] & CONTROL_ITO) ? 1 : 0;
}

static void timer_write(void *opaque, hwaddr addr,
                        uint64_t val64, unsigned int size)
{
    AlteraTimerState *t = opaque;
    uint32_t value = val64;
    uint32_t count64 = 0;
    int irqState = timer_irq_state(t);

    addr >>= 2;
    addr &= 0x7;
    switch (addr) {
    case R_STATUS:
        /* Writing anything clears the timeout */
        t->regs[R_STATUS] &= ~STATUS_TO;
        break;

    case R_CONTROL:
        t->regs[R_CONTROL] = value & (CONTROL_ITO | CONTROL_CONT);
        if ((value & CONTROL_START) &&
            ((t->regs[R_STATUS] & STATUS_RUN) == 0)) {
            timer_start(t);
        }
        if ((value & CONTROL_STOP) && (t->regs[R_STATUS] & STATUS_RUN)) {
            ptimer_stop(t->ptimer);
        }
        break;

    case R_PERIODL:
    case R_PERIODH:
        t->regs[addr] = value & 0xFFFF;
        if (t->regs[R_STATUS] & STATUS_RUN) {
            timer_start(t);
        }

        break;

    case R_SNAPL:
    case R_SNAPH:
        /* A master peripheral may request a coherent snapshot of the
         * current internal counter by performing a write operation
         * (write-data ignored) to one of the snap_n registers.
         * When a write occurs, the value of the counter is copied to
         * snap_n registers.
         * In other words, a write request to any of the SNAPx registers
         * will latch the current value of the timer counter in both
         * SNAP registers.
         */

        count64 = ptimer_get_count(t->ptimer);
        t->regs[R_SNAPL] = count64 & 0xFFFF;
        t->regs[R_SNAPH] = (count64>>16) & 0xFFFF;
        break;
    }

    if (irqState != timer_irq_state(t)) {
        qemu_set_irq(t->irq, timer_irq_state(t));
    }
}

static const MemoryRegionOps timer_ops = {
    .read = timer_read,
    .write = timer_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 2,
        .max_access_size = 4
    }
};

static void timer_hit(void *opaque)
{
    AlteraTimerState *t = opaque;

    t->regs[R_STATUS] |= STATUS_TO;

    if (t->regs[R_CONTROL] & CONTROL_CONT) {
        timer_start(t);
    }

    qemu_set_irq(t->irq, timer_irq_state(t));
}

static int altera_timer_init(SysBusDevice *dev)
{
    AlteraTimerState *s = ALTERA_TIMER(dev);
    DPRINTF("TIMER: %s freq_hz:%d\n",__FUNCTION__,s->freq_hz);
    assert(s->freq_hz != 0);

    sysbus_init_irq(dev, &s->irq);

    s->bh = qemu_bh_new(timer_hit, s);
    s->ptimer = ptimer_init(s->bh);
    ptimer_set_freq(s->ptimer, s->freq_hz);

    memory_region_init_io(&s->mmio, OBJECT(s), &timer_ops, s,
                          TYPE_ALTERA_TIMER, R_MAX * sizeof(uint32_t));
    sysbus_init_mmio(dev, &s->mmio);
    return 0;
}

void altera_timer_create(const hwaddr addr, qemu_irq irq, uint32_t frequency)
{
    DeviceState *dev;
    SysBusDevice *bus;

    dev = qdev_create(NULL,TYPE_ALTERA_TIMER);

    qdev_prop_set_uint32(dev, "clock-frequency", frequency);
    bus = SYS_BUS_DEVICE(dev);
    qdev_init_nofail(dev);

    if (addr != (hwaddr)-1) {
        sysbus_mmio_map(bus, 0, addr);
    }

    sysbus_connect_irq(bus, 0, irq);
}

static const VMStateDescription vmstate_altera_timer = {
    .name = "altera-timer",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_PTIMER(ptimer, AlteraTimerState),
        VMSTATE_UINT32(freq_hz, AlteraTimerState),
        VMSTATE_UINT32_ARRAY(regs, AlteraTimerState, R_MAX),
        VMSTATE_END_OF_LIST()
    }
};

static Property altera_timer_properties[] = {
    DEFINE_PROP_UINT32("clock-frequency", AlteraTimerState, freq_hz, DEFAULT_FREQUENCY),
    DEFINE_PROP_END_OF_LIST(),
};

static void altera_timer_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);
    DPRINTF("TIMER: %s\n",__FUNCTION__);
    k->init = altera_timer_init;
    dc->vmsd = &vmstate_altera_timer;
    dc->props = altera_timer_properties;
}

static const TypeInfo altera_timer_info = {
    .name          = TYPE_ALTERA_TIMER,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(AlteraTimerState),
    .class_init    = altera_timer_class_init,
};

static void altera_timer_register(void)
{
    type_register_static(&altera_timer_info);
}

type_init(altera_timer_register)
