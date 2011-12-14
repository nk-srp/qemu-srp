/*
 *  SRP emulation helpers for qemu.
 *
 *  Copyright (c) 2011 Kevin Hjc
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
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>

#include "cpu.h"
#include "exec-all.h"

void cpu_reset(CPUSRPState *env)
{
	if (qemu_loglevel_mask(CPU_LOG_RESET)) {
        qemu_log("CPU Reset (CPU %d)\n", env->cpu_index);
        log_cpu_state(env, 0);
    }

	memset(env, 0, offsetof(CPUSRPState, breakpoints));

	tlb_flush(env, 1);
}


CPUSRPState *cpu_srp_init(const char *cpu_model)
{
	CPUSRPState *env;
	static int inited;

	env=qemu_mallocz(sizeof(CPUSRPState));

	cpu_exec_init(env);
	if(!inited) {
		inited = 1;
		srp_translate_init();
	}

	env->cpu_model_str = cpu_model;
	
	cpu_reset(env);
	qemu_init_vcpu(env);
    return env;
}

//dxw begin
uint32_t psw_read(CPUSRPState *env)
{
    int ZF;
    ZF = (env->ZF == 0);
    return (env->CF << 15) | (ZF << 14)
        | ((env->SF & 0x80000000) >> 18) | ((env->OF & 0x80000000) >> 19)
        | ((env->WI) << 7) | ((env->WD) << 6) | ((env->TS) << 5) ;   // WI, WD, TS to be added

}//dxw end

#if defined(CONFIG_USER_ONLY)
void do_interrupt(CPUSRPState *env)
{
	env->exception_index =  -1;
}

int cpu_srp_handle_mmu_fault(CPUSRPState *env, target_ulong addr,
                             int rw, int mmu_idx, int is_softmmu)
{
	
	return 0;
}

#else

void do_interrupt(CPUSRPState *env)
{
	env->exception_index =  -1;
}

int cpu_srp_handle_mmu_fault(CPUSRPState *env, target_ulong addr,
                             int rw1, int mmu_idx, int is_softmmu)
{

	return 0;
}

#endif
