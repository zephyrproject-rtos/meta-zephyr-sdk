#ifndef QEMU_HW_ALTERA_IIC_H
#define QEMU_HW_ALTERA_IIC_H

#include "qemu-common.h"

void altera_iic_update_cr_status(DeviceState *d);
void altera_iic_update_cr_ienable(DeviceState *d);

#endif /* QEMU_HW_ALTERA_IIC_H */

