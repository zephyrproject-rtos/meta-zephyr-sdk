/*
 * Nios2 gdb server stub
 *
  * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#include "config.h"
#include "qemu-common.h"
#include "exec/gdbstub.h"

/*
The Nios II architecture supports a flat register file, consisting of
thirty-two 32-bit general-purpose integer registers, and up to thirty-two
32-bit control registers.
*/

int nios2_cpu_gdb_read_register(CPUState *cs, uint8_t *mem_buf, int n)
{
	fprintf(stderr,"here %s\n",__FUNCTION__);
    Nios2CPU *cpu = NIOS2_CPU(cs);
    CPUNios2State *env = &cpu->env;

    if (n < NIOS2_NUM_CORE_REGS) {
        return gdb_get_reg32(mem_buf, env->regs[n]);
    }

    return 0;
}

int nios2_cpu_gdb_write_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    Nios2CPU *cpu = NIOS2_CPU(cs);
    CPUNios2State *env = &cpu->env;
    uint32_t tmp;

    if (n > NIOS2_NUM_CORE_REGS) {
        return 0;
    }

    tmp = ldl_p(mem_buf);
    env->regs[n] = tmp;

    return 4;
}
