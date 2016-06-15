/*
 * QEMU model of the Altera uart.
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

#include "hw/sysbus.h"
#include "sysemu/sysemu.h"
#include "sysemu/char.h"
#include "hw/nios2/altera.h"

#define R_RXDATA        0
#define R_TXDATA        1
#define R_STATUS        2
#define R_CONTROL       3
#define R_DIVISOR       4
#define R_ENDOFPACKET   5
#define R_MAX           6

#define STATUS_PE        0x0001
#define STATUS_FE        0x0002
#define STATUS_BRK       0x0004
#define STATUS_ROE       0x0008
#define STATUS_TOE       0x0010
#define STATUS_TMT       0x0020
#define STATUS_TRDY      0x0040
#define STATUS_RRDY      0x0080
#define STATUS_E         0x0100
#define STATUS_DTCS      0x0400
#define STATUS_CTS       0x0800
#define STATUS_EOP       0x1000

#define CONTROL_IPE      0x0001
#define CONTROL_IFE      0x0002
#define CONTROL_IBRK     0x0004
#define CONTROL_IROE     0x0008
#define CONTROL_ITOE     0x0010
#define CONTROL_ITMT     0x0020
#define CONTROL_ITRDY    0x0040
#define CONTROL_IRRDY    0x0080
#define CONTROL_IE       0x0100
#define CONTROL_TBRK     0x0200
#define CONTROL_IDTCS    0x0400
#define CONTROL_RTS      0x0800
#define CONTROL_IEOP     0x1000

#define TYPE_ALTERA_UART "altera-uart"
#define ALTERA_UART(obj) \
    OBJECT_CHECK(AlteraUARTState, (obj), TYPE_ALTERA_UART)

typedef struct AlteraUARTState {
    SysBusDevice busdev;
    MemoryRegion mmio;
    CharDriverState *chr;
    qemu_irq irq;

    uint32_t regs[R_MAX];
} AlteraUARTState;

static void uart_update_irq(AlteraUARTState *s)
{
    unsigned int irq;

    irq = (s->regs[R_STATUS] & s->regs[R_CONTROL] &
          (STATUS_PE | STATUS_FE | STATUS_BRK | STATUS_ROE | STATUS_TOE |
           STATUS_TMT | STATUS_TRDY | STATUS_RRDY | STATUS_E | STATUS_DTCS));
    irq = (irq == 0) ? 0 : 1;
    qemu_set_irq(s->irq, irq);
}

static uint64_t uart_read(void *opaque, hwaddr addr,
                          unsigned int size)
{
    AlteraUARTState *s = opaque;
    uint32_t r = 0;
    addr >>= 2;
    addr &= 0x7;
    switch (addr) {
    case R_RXDATA:
        r = s->regs[R_RXDATA];
        s->regs[R_STATUS] &= ~STATUS_RRDY;
        uart_update_irq(s);
        break;

    case R_STATUS:
        r = s->regs[R_STATUS];
        s->regs[R_STATUS] &= ~(STATUS_PE | STATUS_FE | STATUS_BRK |
                               STATUS_ROE | STATUS_TOE | STATUS_E |
                               STATUS_DTCS);
        uart_update_irq(s);
        break;

    default:
        if (addr < ARRAY_SIZE(s->regs)) {
            r = s->regs[addr];
        }
        break;
    }

    return r;
}

static void uart_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    AlteraUARTState *s = opaque;
    uint32_t value = val64;
    unsigned char ch = value;

    addr >>= 2;
    addr &= 0x7;

    switch (addr) {
    case R_TXDATA:
        if (s->chr) {
            qemu_chr_fe_write(s->chr, &ch, 1);
        }

        s->regs[addr] = value;
        break;

    case R_RXDATA:
    case R_STATUS:
        /* No writeable bits */
        break;

    default:
        s->regs[addr] = value;
        break;
    }
    uart_update_irq(s);
}

static void uart_rx(void *opaque, const uint8_t *buf, int size)
{
    AlteraUARTState *s = opaque;
    int i;

    s->regs[R_RXDATA] = *buf;
    s->regs[R_STATUS] |= STATUS_RRDY;

    uart_update_irq(s);
}

static int uart_can_rx(void *opaque)
{
    AlteraUARTState *s = opaque;
    return ((s->regs[R_STATUS] & STATUS_RRDY) == 0);
}

static void uart_event(void *opaque, int event)
{
}

static void uart_reset(DeviceState *d)
{
    int i;
    AlteraUARTState *s = ALTERA_UART(d);

    for (i = 0; i < R_MAX; i++) {
        s->regs[i] = 0;
    }

    /* Always ready to tx */
    s->regs[R_STATUS] = STATUS_TMT | STATUS_TRDY;
}

static const MemoryRegionOps uart_ops = {
    .read = uart_read,
    .write = uart_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 4
    }
};

static int altera_uart_init(SysBusDevice *dev)
{
    AlteraUARTState *s = ALTERA_UART(dev);

    sysbus_init_irq(dev, &s->irq);

    memory_region_init_io(&s->mmio, OBJECT(s), &uart_ops, s,
                          TYPE_ALTERA_UART, R_MAX * sizeof(uint32_t));
    sysbus_init_mmio(dev, &s->mmio);

    qemu_chr_add_handlers(s->chr, uart_can_rx, uart_rx, uart_event, s);

    return 0;
}

void altera_uart_create(int uart, const hwaddr addr, qemu_irq irq)
{
    DeviceState *dev;
    SysBusDevice *bus;
    CharDriverState *chr;
    const char chr_name[] = "uart";
    char label[ARRAY_SIZE(chr_name) + 1];

    dev = qdev_create(NULL, TYPE_ALTERA_UART);

    if (uart >= MAX_SERIAL_PORTS) {
        hw_error("Cannot assign uart %d: QEMU supports only %d ports\n",
                 uart, MAX_SERIAL_PORTS);
    }

    chr = serial_hds[uart];
    if (!chr) {
        snprintf(label, ARRAY_SIZE(label), "%s%d", chr_name, uart);
        chr = qemu_chr_new(label, "null", NULL);
        if (!(chr)) {
            hw_error("Can't assign serial port to altera uart%d.\n", uart);
        }
    }

    qdev_prop_set_chr(dev, "chardev", chr);
    bus = SYS_BUS_DEVICE(dev);
    qdev_init_nofail(dev);

    if (addr != (hwaddr)-1) {
        sysbus_mmio_map(bus, 0, addr);
    }

    sysbus_connect_irq(bus, 0, irq);
}

static Property altera_uart_properties[] = {
    DEFINE_PROP_CHR("chardev", AlteraUARTState, chr),
    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription vmstate_altera_uart = {
    .name = "altera-uart" ,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs,AlteraUARTState, R_MAX),
        VMSTATE_END_OF_LIST()
    }
};

static void altera_uart_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = altera_uart_init;
    dc->props = altera_uart_properties;
    dc->vmsd = &vmstate_altera_uart;
    dc->reset = uart_reset;
    dc->desc = "Altera UART";
}

static const TypeInfo altera_uart_info = {
    .name          = TYPE_ALTERA_UART,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(AlteraUARTState),
    .class_init    = altera_uart_class_init,
};

static void altera_uart_register(void)
{
    type_register_static(&altera_uart_info);
}

type_init(altera_uart_register)
