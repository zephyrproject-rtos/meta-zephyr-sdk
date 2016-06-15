/*
 * Altera Nios II MMU emulation for qemu.
 *
 * Copyright (C) 2012 Chris Wulff <crwulff@gmail.com>
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

#include "hw/hw.h"
#include "hw/boards.h"

void cpu_save(QEMUFile *f, void *opaque)
{
    /* TODO */
//    printf("CPU %s\n",__FUNCTION__);
}

int cpu_load(QEMUFile *f, void *opaque, int version_id)
{
    /* TODO */
//    printf("CPU %s\n",__FUNCTION__);
    return 0;
}
