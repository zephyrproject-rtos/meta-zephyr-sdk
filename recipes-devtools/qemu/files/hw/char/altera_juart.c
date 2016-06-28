/*
 * QEMU model of the Altera JTAG uart.
 *
 * Copyright (c) 2016 Intel Corporation.
 *
 * https://www.altera.com/content/dam/altera-www/global/en_US/pdfs/literature/ug/ug_embedded_ip.pdf
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

#define R_DATA        0
    /* If RVALID=1, the DATAfield is valid, otherwise DATA is undefined */
#define DATA_RVALID     0x00008000
    /*The number of characters remaining in the read FIFO (after the
      current read).*/
#define DATA_RAVAIL     0xFFFF0000

#define R_CONTROL     1
#define CONTROL_RE              0x00000001
#define CONTROL_WE              0x00000002
#define CONTROL_RI              0x00000100
#define CONTROL_WI              0x00000200
#define CONTROL_AC              0x00000400
#define CONTROL_WSPACE          0xFFFF0000

#define CONTROL_WMASK (CONTROL_RE | CONTROL_WE | CONTROL_AC)

/* For Nios II processor systems, read and write IRQ thresholds of 8 are
 * an appropriate default. */

/* The read and write FIFO depths can be set from 8 to 32,768 bytes.
 * Only powers of two are allowed. A depth of 64 is generally optimal for
 * performance, and larger values are rarely necessary. */

#define FIFO_LENGTH 64

#define TYPE_ALTERA_JUART "altera-juart"
#define ALTERA_JUART(obj) \
    OBJECT_CHECK(AlteraJUARTState, (obj), TYPE_ALTERA_JUART)

typedef struct AlteraJUARTState {
    SysBusDevice busdev;
    MemoryRegion mmio;
    CharDriverState *chr;
    qemu_irq irq;

    uint8_t rx_fifo[FIFO_LENGTH];
    unsigned int rx_fifo_pos;
    unsigned int rx_fifo_len;
    uint32_t jdata;
    uint32_t jcontrol;
} AlteraJUARTState;

/* The JTAG UART core generates an interrupt when either of the individual
 * interrupt conditions is pending and enabled */
static void juart_update_irq(AlteraJUARTState *s)
{
    unsigned int irq;

    irq = ((s->jcontrol & CONTROL_WE) && (s->jcontrol & CONTROL_WI)) ||
          ((s->jcontrol & CONTROL_RE) && (s->jcontrol & CONTROL_RI));

    qemu_set_irq(s->irq, irq);
}

// read by remote (Linux)
static uint64_t juart_read(void *opaque, hwaddr addr,
                          unsigned int size)
{
    AlteraJUARTState *s = opaque;
    uint32_t r;

    addr >>= 2;

    switch (addr) {
    case R_DATA:
        r = s->rx_fifo[(s->rx_fifo_pos - s->rx_fifo_len) & (FIFO_LENGTH-1)];
        if (s->rx_fifo_len) {
            s->rx_fifo_len--;
            if (s->chr) {
                qemu_chr_accept_input(s->chr);
            }
            s->jdata = r | DATA_RVALID | (s->rx_fifo_len) << 16;
            s->jcontrol |= CONTROL_RI;
        } else {
            s->jdata = 0;
            s->jcontrol &= ~CONTROL_RI;
        }

        juart_update_irq(s);
        return s->jdata;

    case R_CONTROL:
        return s->jcontrol;
    }

    return 0;
}

// wrote by remote (Linux)
static void juart_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    AlteraJUARTState *s = opaque;
    uint32_t value = val64;
    unsigned char c;

    addr >>= 2;

    switch (addr) {
    case R_DATA:
        if ((s->chr) /*&& (s->control & UART_TRANSMIT_ENABLE)*/) {
            c = value & 0xFF;
            /* We do not decrement the write fifo,
             * we "tranmsmit" instanteniously, CONTROL_WI always asserted */
            s->jcontrol |= CONTROL_WI;
            s->jdata = c;
            qemu_chr_fe_write(s->chr, &c, 1);
            juart_update_irq(s);
        }
        break;

    case R_CONTROL:
        /* Only RE and WE are writable */
        value &= CONTROL_WMASK;
        s->jcontrol &= ~(CONTROL_WMASK);
        s->jcontrol |= value;

        /* Writing 1 to AC clears it to 0 */
        if (value & CONTROL_AC) {
             s->jcontrol &= ~CONTROL_AC;
        }
        juart_update_irq(s);
        break;
    }
}

static void juart_receive(void *opaque, const uint8_t *buf, int size)
{
    int i;
    AlteraJUARTState *s = opaque;

    if (s->rx_fifo_len >= FIFO_LENGTH) {
        printf("WARNING: UART dropped char.\n");
        return;
    }

    for (i = 0; i < size; i++) {
        s->rx_fifo[s->rx_fifo_pos] = buf[i];
        s->rx_fifo_pos++;
        s->rx_fifo_pos &= (FIFO_LENGTH-1);
        s->rx_fifo_len++;
    }
    s->jcontrol |= CONTROL_RI;
    juart_update_irq(s);
}

static int juart_can_receive(void *opaque)
{
    AlteraJUARTState *s = opaque;
    return FIFO_LENGTH - s->rx_fifo_len;
}

static void juart_event(void *opaque, int event)
{
//    TODO: process events
}

static void juart_reset(DeviceState *d)
{
    AlteraJUARTState *s = ALTERA_JUART(d);

    s->jdata = 0;

    /* The number of spaces available in the write FIFO */
    s->jcontrol = FIFO_LENGTH<<16;
    s->rx_fifo_pos = 0;
    s->rx_fifo_len = 0;
}


static const MemoryRegionOps juart_ops = {
    .read = juart_read,
    .write = juart_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4
    }
};


static int altera_juart_init(SysBusDevice *dev)
{
    AlteraJUARTState *s = ALTERA_JUART(dev);

    sysbus_init_irq(dev, &s->irq);

    memory_region_init_io(&s->mmio,  OBJECT(s), &juart_ops, s,
                          TYPE_ALTERA_JUART, 2 * 4);
    sysbus_init_mmio(dev, &s->mmio);
    qemu_chr_add_handlers(s->chr, juart_can_receive, juart_receive, juart_event, s);
    return 0;
}

void altera_juart_create(int uart, const hwaddr addr, qemu_irq irq)
{
    DeviceState *dev;
    SysBusDevice *bus;
    CharDriverState *chr;
    const char chr_name[] = "juart";
    char label[ARRAY_SIZE(chr_name) + 1];

    dev = qdev_create(NULL, TYPE_ALTERA_JUART);

    if (uart >= MAX_SERIAL_PORTS) {
        hw_error("Cannot assign uart %d: QEMU supports only %d ports\n",
                 uart, MAX_SERIAL_PORTS);
    }

    chr = serial_hds[uart];
    if (!chr) {
        snprintf(label, ARRAY_SIZE(label), "%s%d", chr_name, uart);
        chr = qemu_chr_new(label, "null", NULL);
        if (!(chr)) {
            hw_error("Can't assign serial port to altera juart%d.\n", uart);
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

static Property altera_juart_properties[] = {
    DEFINE_PROP_CHR("chardev", AlteraJUARTState, chr),
    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription vmstate_altera_juart = {
    .name = "altera-juart" ,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(jdata, AlteraJUARTState),
        VMSTATE_UINT32(jcontrol, AlteraJUARTState),
        VMSTATE_END_OF_LIST()
    }
};

static void altera_juart_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = altera_juart_init;
    dc->props = altera_juart_properties;
    dc->vmsd = &vmstate_altera_juart;
    dc->reset = juart_reset;
    dc->desc = "Altera JTAG UART";
}

static const TypeInfo altera_juart_info = {
    .name          = TYPE_ALTERA_JUART,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(AlteraJUARTState),
    .class_init    = altera_juart_class_init,
};

static void altera_juart_register(void)
{
    type_register_static(&altera_juart_info);
}

type_init(altera_juart_register)
