/*
 *  SRP translation
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

#include "cpu.h"
#include "exec-all.h"
#include "disas.h"
#include "tcg-op.h"
#include "qemu-log.h"

#include "helper.h"
#define GEN_HELPER 1
#include "helper.h"


/* internal defines */
typedef struct DisasContext {
    target_ulong pc;
    int is_jmp;
    /* Nonzero if this instruction has been conditionally skipped.  */
    int condjmp;
    /* The label that will be jumped to when the instruction is skipped.  */
    int condlabel;
    struct TranslationBlock *tb;
    int singlestep_enabled;

#if !defined(CONFIG_USER_ONLY)
    int user;
#endif
} DisasContext;

#if defined(CONFIG_USER_ONLY)
#define IS_USER(s) 1
#else
#define IS_USER(s) (s->user)
#endif

static TCGv_i32 cpu_R[60];

static const char *regnames[] =
    { "R00", "R04", "R08", "R0c", "R10", "R14", "R18", "R1c","R20", "R24", "R28", "R2c",
       "R30", "R34", "R38", "R3c", "R30", "R44", "R48", "R4c","R40", "R54", "R58", "R5c",
       "R60", "R64", "R68", "R6c", "R70", "R74", "R78", "R7c","R80", "R84", "R88", "R8c",
       "R90", "R94", "R98", "R9c", "Ra0", "Ra4", "Ra8", "Rac","Rb0", "Rb4", "Rb8", "Rbc",
       "Rc0", "Rc4", "Rc8", "Rcc", "Rd0", "Rd4", "Rd8", "Rdc","Re0", "Re4", "Re8", "Rec",
       "IRQ", "PSW", "SP", "PC"};


static int num_temps;

/* Allocate a temporary variable.  */
static TCGv_i32 new_tmp(void)
{
    num_temps++;
    return tcg_temp_new_i32();
}

/* Release a temporary variable.  */
static void dead_tmp(TCGv tmp)
{
    tcg_temp_free(tmp);
    num_temps--;
}


/* Set a variable to the value of a CPU register.  */
static void load_reg_var(DisasContext *s, TCGv var, int reg)
{
     tcg_gen_mov_i32(var, cpu_R[reg]);
}

/* Create a new temporary and set it to the value of a CPU register.  */
static inline TCGv load_reg(DisasContext *s, int reg)
{
    TCGv tmp = new_tmp();
    load_reg_var(s, tmp, reg);
    return tmp;
}

/* Set a CPU register.  The source must be a temporary and will be
   marked as dead.  */
static void store_reg(DisasContext *s, int reg, TCGv var)
{
    tcg_gen_mov_i32(cpu_R[reg], var);
    dead_tmp(var);
}


static unsigned int GetInsnLength(unsigned int  insn)
{
	unsigned int opcode, length;
	opcode = insn >> 28;
	switch(opcode) {
	case 0x4:
		length = 3;
	default:
		length = 0;
	}

	return length;
}

static void disas_srp_insn(CPUState * env, DisasContext *s)
{
	unsigned int insn, rs, rd, iLength;
	int32_t imm32;

	insn = ldl_code(s->pc);
	iLength = GetInsnLength(insn);
    s->pc += iLength;
	
	if(insn >> 24 == 0x47) {
		rd = s->pc + 1;
		imm32 = (int32_t)s->pc + 2;
		tcg_gen_movi_i32(cpu_R[rd], imm32);
	}

		
		
		
}

/* generate intermediate code in gen_opc_buf and gen_opparam_buf for
   basic block 'tb'. If search_pc is TRUE, also generate PC
   information for each intermediate instruction. */
static inline void gen_intermediate_code_internal(CPUState *env,
                                                  TranslationBlock *tb,
                                                  int search_pc)
{
	DisasContext dc1, *dc = &dc1;
	target_ulong pc_start;

	pc_start = tb->pc;

	dc->tb = tb;
	dc->is_jmp = DISAS_NEXT;
	dc->condjmp = 0;
	dc->pc = pc_start;
	dc->singlestep_enabled = env->singlestep_enabled;
#if !defined(CONFIG_USER_ONLY)
    dc->user = 0;
#endif
	disas_srp_insn(env, dc);
}

void gen_intermediate_code(CPUState *env, TranslationBlock *tb)
{
    gen_intermediate_code_internal(env, tb, 0);
}

void gen_intermediate_code_pc(CPUState *env, TranslationBlock *tb)
{
    gen_intermediate_code_internal(env, tb, 1);
}

void cpu_dump_state(CPUState *env, FILE *f,
                    int (*cpu_fprintf)(FILE *f, const char *fmt, ...),
                    int flags)
{
    int i;

    for(i=0;i<60;i++) {
        cpu_fprintf(f, "%s=%08x", regnames[i], env->regs[i]);
        ((i % 4) == 0) ? cpu_fprintf(f, "\n") : cpu_fprintf(f, " ");
    }
   
    cpu_fprintf(f, "PSR=%08x,  SP=%08x,  PC=%08x\n",
                env->psw,
                env->sp,
                env->pc);

}


void gen_pc_load(CPUState *env, TranslationBlock *tb,
                unsigned long searched_pc, int pc_pos, void *puc)
{
    env->pc = gen_opc_pc[pc_pos];
}

