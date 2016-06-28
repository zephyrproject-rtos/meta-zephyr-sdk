/*
 * QEMU Altera Internal Interrupt Controller.
 *
 * Copyright (c) 2012 Chris Wulff <crwulff@gmail.com>
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
#include "cpu.h"
#include "hw/nios2/altera_iic.h"

//#define DEBUG

#ifdef DEBUG
# define DPRINTF(format, ...) printf(format , ## __VA_ARGS__)
#else
# define DPRINTF(format, ...) do { } while (0)
#endif

#define TYPE_ALTERA_IIC "altera,iic"
#define ALTERA_IIC(obj) \
    OBJECT_CHECK(AlteraIIC, (obj), TYPE_ALTERA_IIC)

typedef struct AlteraIIC {
    SysBusDevice busdev;
    void        *cpu;
    qemu_irq    parent_irq;
    uint32_t    irqs;
} AlteraIIC;

/*
 * When the internal interrupt controller is implemented, a peripheral
 * device can request a hardware interrupt by asserting one of the Nios II
 * processor’s 32 interrupt-request inputs, irq0 through irq31. A hardware
 * interrupt is generated if and only if all three of these conditions are
 * true:
 *  1.    The PIE bit of the status control register is one.
 *  2.    An interrupt-request input, irq n, is asserted.
 *  3.    The corresponding bit n of the ienable control register is one
*/

static void update_irq(AlteraIIC *pv)
{
    CPUNios2State *env = &((Nios2CPU*)(pv->cpu))->env;

    if ((env->regs[CR_STATUS] & CR_STATUS_PIE) == 0) {
        qemu_irq_lower(pv->parent_irq);
        return;
    }

    if (env->regs[CR_IPENDING]) {
        qemu_irq_raise(pv->parent_irq);
    } else {
        qemu_irq_lower(pv->parent_irq);
    }
}

static void irq_handler(void *opaque, int irq, int level)
{
    AlteraIIC *s = opaque;
    CPUNios2State *env = &((Nios2CPU*)(s->cpu))->env;
    s->irqs &= ~(1 << irq);
    s->irqs |= level << irq;
    env->regs[CR_IPENDING] = env->regs[CR_IENABLE] & s->irqs;
    update_irq(s);
}

void altera_iic_update_cr_status(DeviceState *d)
{
    AlteraIIC *s = ALTERA_IIC(d);
    update_irq(s);
}

/*
 * ipending register
 * A value of one in bit n means that the corresponding irq n input is
 * asserted and enabled in the ienable register. Writing a value to the
 * ipending register has no effect.

 * The ipending register is present only when the internal interrupt
 * controller is implemented.
 */
void altera_iic_update_cr_ienable(DeviceState *d)
{
    /* Modify the IPENDING register */
    AlteraIIC *s = ALTERA_IIC(d);
    CPUNios2State *env = &((Nios2CPU*)(s->cpu))->env;
    env->regs[CR_IPENDING] = env->regs[CR_IENABLE] & s->irqs;
    update_irq(s);
}

static int altera_iic_init(SysBusDevice *obj)
{
    AlteraIIC *pv = ALTERA_IIC(obj);
    DPRINTF("IIC: %s\n",__FUNCTION__);
    qdev_init_gpio_in(DEVICE(pv), irq_handler, 32);
    sysbus_init_irq(SYS_BUS_DEVICE(obj), &pv->parent_irq);
    return 0;
}

static Property altera_iic_properties[] = {
    DEFINE_PROP_PTR("cpu", AlteraIIC, cpu),
    DEFINE_PROP_END_OF_LIST(),
};

static void altera_iic_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    DPRINTF("IIC: %s\n",__FUNCTION__);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = altera_iic_init;
    dc->props = altera_iic_properties;
}

static TypeInfo altera_iic_info = {
    .name          = "altera,iic",
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(AlteraIIC),
    .class_init    = altera_iic_class_init,
};

static void altera_iic_register(void)
{
    type_register_static(&altera_iic_info);
}

type_init(altera_iic_register)

