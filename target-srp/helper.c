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

CPUSRPState *cpu_srp_init(const char *cpu_model)
{
	CPUSRPState *env;
	static int inited;

	env=qemu_mallocz(sizeof(CPUSRPState));

	cpu_exec_init(env);
	if(!inited) {
		inited = 1;
	}

	env->cpu_model_str = cpu_model;
	
	qemu_init_vcpu(env);
    return env;
}

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