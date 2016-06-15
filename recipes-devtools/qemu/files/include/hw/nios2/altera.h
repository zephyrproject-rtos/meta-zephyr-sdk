/*
 * Altera emulation
 *
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

#ifndef ALTERA_H
#define ALTERA_H

void altera_juart_create(int uart, const hwaddr addr, qemu_irq irq);
void altera_uart_create(int uart, const hwaddr addr, qemu_irq irq);
void altera_timer_create(const hwaddr addr, qemu_irq irq, uint32_t frequency);

#endif /* ALTERA_H */
