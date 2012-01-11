/*
 * SRP virtual CPU header
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
#ifndef CPU_SRP_H
#define CPU_SRP_H

#define TARGET_LONG_BITS 32

#define ELF_MACHINE EM_SRP  //elf.h #define EM_SRP 0x4e4b

#define CPUState struct CPUSRPState

#include "cpu-defs.h"
//include "softfloat.h"

#define TARGET_HAS_ICE 1  //something abou Debug breakpoint--used in file exec.c

/* all the instructions of interrupt, in case the TCG doesn't contain any of them */
#define EXCP_UDEF	-1
#define EXCP_SWI	0x01
#define EXCP_RET	0x02
#define EXCP_IRET	0x03
#define EXCP_CLI	0x04
#define EXCP_STI	0x05
#define EXCP_CWI	0x06
#define EXCP_EWI	0x07
#define EXCP_CWD	0x08
#define EXCP_EWD	0x09
#define EXCP_CTS	0x0a
#define EXCP_ETS	0x0b


enum srp_cpu_mode {
  SRP_CPU_MODE_USR = 0x10,
  SRP_CPU_MODE_SYS = 0x1f
};

#define NB_MMU_MODES 2

#define SRP_REGS	60

typedef struct CPUSRPState {
	/*Regs */
	uint32_t regs[SRP_REGS];
	
	uint32_t irq;
	uint32_t psw;

	uint32_t sp;
	uint32_t pc;

	/* PSW */
	uint32_t CF; /* 0 or 1 */
	uint32_t OF; /* V is the bit 31. All other bits are undefined */
	uint32_t SF; /* S is bit 31. All other bits are undefined.  */
	uint32_t ZF; /* Z set if zero.  */

	uint32_t WI;
	uint32_t WD;
	uint32_t TS;
	
/* Callback for vectored interrupt controller.  */
    int (*get_irq_vector)(struct CPUSRPState *);
    void *irq_opaque;

	CPU_COMMON

}  CPUSRPState;


#if defined(CONFIG_USER_ONLY)
#define TARGET_PAGE_BITS 12
#else
#define TARGET_PAGE_BITS 10
#endif

#define TARGET_PHYS_ADDR_SPACE_BITS 32
#define TARGET_VIRT_ADDR_SPACE_BITS 32

CPUSRPState *cpu_srp_init(const char *cpu_model);
void srp_translate_init(void);
int cpu_srp_exec(CPUSRPState *s);
void do_interrupt(CPUSRPState *s);
int cpu_srp_signal_handler(int host_signum, void *pinfo,void *puc);
int cpu_srp_handle_mmu_fault (CPUSRPState *env, target_ulong address, int rw,
                              int mmu_idx, int is_softmuu);

#define cpu_init			cpu_srp_init
#define cpu_exec			cpu_srp_exec
#define cpu_gen_code		cpu_srp_gen_code

#define cpu_signal_handler	cpu_srp_signal_handler
#define cpu_handle_mmu_fault cpu_srp_handle_mmu_fault

#define CPU_SAVE_VERSION	5

#define MMU_MODE0_SUFFIX _kernel
#define MMU_MODE1_SUFFIX _user
#define MMU_USER_IDX 1
static inline int cpu_mmu_index (CPUSRPState *env)
{
  	return SRP_CPU_MODE_USR ? 1 : 0;
}

/* Return the current PSW value.  */
uint32_t psw_read(CPUSRPState *env);

#if defined(CONFIG_USER_ONLY)
static inline void cpu_clone_regs(CPUState *env, target_ulong newsp)
{
    if (newsp)
        env->sp = newsp;
    env->regs[0] = 0;
}
#endif

#include "cpu-all.h"

static inline void cpu_get_tb_cpu_state(CPUState *env, target_ulong *pc,
                                        target_ulong *cs_base, int *flags)
{
	*pc = env->pc;
	*cs_base = 0;
	*flags = env->psw;	//need modify later 11-08-22 
}

#endif





