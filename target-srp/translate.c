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

/* These instructions trap after executing, so defer them until after the
 conditional executions state has been updated.  */
#define DISAS_WFI 4
#define DISAS_SWI 5

static TCGv_ptr cpu_env;
static TCGv_i32 cpu_R[64];

static const char *regnames[] = { "R00", "R04", "R08", "R0c", "R10", "R14",
		"R18", "R1c", "R20", "R24", "R28", "R2c", "R30", "R34", "R38", "R3c",
		"R40", "R44", "R48", "R4c", "R50", "R54", "R58", "R5c", "R60", "R64",
		"R68", "R6c", "R70", "R74", "R78", "R7c", "R80", "R84", "R88", "R8c",
		"R90", "R94", "R98", "R9c", "Ra0", "Ra4", "Ra8", "Rac", "Rb0", "Rb4",
		"Rb8", "Rbc", "Rc0", "Rc4", "Rc8", "Rcc", "Rd0", "Rd4", "Rd8", "Rdc",
		"Re0", "Re4", "Re8", "Rec", "IRQ", "PSW", "SP", "PC" };

#include "gen-icount.h"

/* initialize TCG globals.  */
void srp_translate_init(void) {
	int i;

	cpu_env = tcg_global_reg_new_ptr(TCG_AREG0, "env");

	for (i = 0; i < SRP_REGS; i++) {
 cpu_R[i] = tcg_global_mem_new_i32(TCG_AREG0,
			offsetof(CPUState, regs[i]),
			regnames[i]);
}
 cpu_R[60]= tcg_global_mem_new_i32(TCG_AREG0,
		    offsetof(CPUState, irq),
		    regnames[60]);

 cpu_R[61]= tcg_global_mem_new_i32(TCG_AREG0,
		    offsetof(CPUState, psw),
		    regnames[61]);

 cpu_R[62]= tcg_global_mem_new_i32(TCG_AREG0,
		    offsetof(CPUState, sp),
		    regnames[62]);

 cpu_R[63]= tcg_global_mem_new_i32(TCG_AREG0,
		    offsetof(CPUState, pc),
		    regnames[63]);

}

static int num_temps;

/* Allocate a temporary variable.  */
static TCGv_i32 new_tmp(void) {
	num_temps++;
	return tcg_temp_new_i32();
}

/* Release a temporary variable.  */
static void dead_tmp(TCGv tmp) {
	tcg_temp_free(tmp);
	num_temps--;
}

static void gen_exception(int excp) {
	TCGv tmp = new_tmp();
	tcg_gen_movi_i32(tmp, excp);
	gen_helper_exception(tmp);
	dead_tmp(tmp);
}

///* Set a variable to the value of a CPU register.  */
//static void load_reg_var(DisasContext *s, TCGv var, int reg) {
//	tcg_gen_mov_i32(var, cpu_R[reg >> 2]);
//}

/* Create a new temporary and set it to the value of a CPU register.  */
static inline TCGv load_reg(DisasContext *s, int reg) {
	TCGv tmp = new_tmp();
	tcg_gen_mov_i32(tmp, cpu_R[reg >> 2]);
	return tmp;
}

/* Set a CPU register.  The source must be a temporary and will be
 marked as dead.  */
static void store_reg(DisasContext *s, int reg, TCGv var) {
	tcg_gen_mov_i32(cpu_R[reg >> 2], var);
	dead_tmp(var);
}

/*
 static inline void
 gen_set_condexec (DisasContext *s)
 {
 if (s->condexec_mask) {
 uint32_t val = (s->condexec_cond << 4) | (s->condexec_mask >> 1);
 TCGv tmp = new_tmp();
 tcg_gen_movi_i32(tmp, val);
 store_cpu_field(tmp, condexec_bits);
 }
 }*/

#define gen_set_CF(var) tcg_gen_st_i32(var, cpu_env, offsetof(CPUState, CF))

///* Set CF to the top bit of var.  */
//static void gen_set_CF_bit31(TCGv var) {
//	TCGv tmp = new_tmp();
//	tcg_gen_shri_i32(tmp, var, 31);
//	gen_set_CF(tmp);
//	dead_tmp(tmp);
//}

// Starts --- DXW
static inline TCGv load_cpu_offset(int offset)
{
    TCGv tmp = new_tmp();
    tcg_gen_ld_i32(tmp, cpu_env, offset);
    return tmp;
}

#define load_cpu_field(name) load_cpu_offset(offsetof(CPUState, name))

/* Set S and Z flags from var.  */
static inline void gen_logic_CC(TCGv var) {
tcg_gen_st_i32(var, cpu_env, offsetof(CPUState, SF));
tcg_gen_st_i32(var, cpu_env, offsetof(CPUState, ZF));
}

static void gen_test_cc(int cc, int label)
{
    TCGv tmp;
    TCGv tmp2;


    switch (cc) {
    case 0xC0:
        tmp = load_cpu_field(CF);
        tcg_gen_brcondi_i32(TCG_COND_EQ, tmp, 0, label);
        break;
    case 0xC1:
        tmp = load_cpu_field(ZF);
        tcg_gen_brcondi_i32(TCG_COND_EQ, tmp, 0, label);
        break;
    case 0xC2:
        tmp = load_cpu_field(SF);
        tcg_gen_brcondi_i32(TCG_COND_EQ, tmp, 0, label);
        break;
    case 0xC3:
        tmp = load_cpu_field(OF);
        tcg_gen_brcondi_i32(TCG_COND_EQ, tmp, 0, label);
        break;
    case 0xC4:
        tmp = load_cpu_field(CF);
        tmp2 = load_cpu_field(ZF);
        tcg_gen_or_i32(tmp, tmp, tmp2);
        dead_tmp(tmp2);
        tcg_gen_brcondi_i32(TCG_COND_EQ, tmp, 0, label);
        break;
    case 0xC5:
        tmp = load_cpu_field(SF);
        tmp2 = load_cpu_field(OF);
        tcg_gen_xor_i32(tmp, tmp, tmp2);
        dead_tmp(tmp2);
        tcg_gen_brcondi_i32(TCG_COND_EQ, tmp, 0, label);
        break;
    case 0xC6:
        tmp = load_cpu_field(SF);
        tmp2 = load_cpu_field(OF);
        tcg_gen_xor_i32(tmp, tmp, tmp2);
        dead_tmp(tmp2);
        tmp2 = load_cpu_field(ZF);
        tcg_gen_or_i32(tmp, tmp, tmp2);
        dead_tmp(tmp2);
        tcg_gen_brcondi_i32(TCG_COND_EQ, tmp, 0, label);
        break;
    case 0xC8:
        tmp = load_cpu_field(CF);
        tcg_gen_brcondi_i32(TCG_COND_NE, tmp, 0, label);
        break;
    case 0xC9:
        tmp = load_cpu_field(ZF);
        tcg_gen_brcondi_i32(TCG_COND_NE, tmp, 0, label);
        break;
    case 0xCA:
        tmp = load_cpu_field(SF);
        tcg_gen_brcondi_i32(TCG_COND_NE, tmp, 0, label);
        break;
    case 0xCB:
        tmp = load_cpu_field(OF);
        tcg_gen_brcondi_i32(TCG_COND_NE, tmp, 0, label);
        break;
    case 0xCC:
        tmp = load_cpu_field(CF);
        tmp2 = load_cpu_field(ZF);
        tcg_gen_or_i32(tmp, tmp, tmp2);
        dead_tmp(tmp2);
        tcg_gen_brcondi_i32(TCG_COND_NE, tmp, 0, label);
        break;
    case 0xCD:
        tmp = load_cpu_field(SF);
        tmp2 = load_cpu_field(OF);
        tcg_gen_xor_i32(tmp, tmp, tmp2);
        dead_tmp(tmp2);
        tcg_gen_brcondi_i32(TCG_COND_NE, tmp, 0, label);
        break;
    case 0xCE:
        tmp = load_cpu_field(SF);
        tmp2 = load_cpu_field(OF);
        tcg_gen_xor_i32(tmp, tmp, tmp2);
        dead_tmp(tmp2);
        tmp2 = load_cpu_field(ZF);
        tcg_gen_or_i32(tmp, tmp, tmp2);
        dead_tmp(tmp2);
        tcg_gen_brcondi_i32(TCG_COND_NE, tmp, 0, label);
    	break;
    default:
        fprintf(stderr, "Bad condition code 0x%x\n", cc);
        abort();
    }
    dead_tmp(tmp);
}
// Ends --- DXW
/* To store into memory*/
static inline void gen_st8(TCGv val, TCGv addr, int index) {
	tcg_gen_qemu_st8(val, addr, index);
	dead_tmp(val);
}

static inline void gen_st32(TCGv val, TCGv addr, int index) {
	tcg_gen_qemu_st32(val, addr, index);
	dead_tmp(val);
}

/*To load from memory*/
static inline TCGv gen_ld8s(TCGv addr, int index) {
	TCGv tmp = new_tmp();
	tcg_gen_qemu_ld8s(tmp, addr, index);
	return tmp;
}

static inline TCGv gen_ld32(TCGv addr, int index) {
	TCGv tmp = new_tmp();
	tcg_gen_qemu_ld32u(tmp, addr, index);
	return tmp;
}

/* --------------------------2011-11-22 by wanglizhi----------------------------- */
//begin
static inline void gen_set_pc_im(uint32_t val) {
	tcg_gen_movi_i32(cpu_R[63], val);

}

static inline void gen_goto_tb(DisasContext *s, int n, uint32_t dest) {
	TranslationBlock *tb;
	tb = s->tb;
	if ((tb->pc & TARGET_PAGE_MASK) == (dest & TARGET_PAGE_MASK)) {
		tcg_gen_goto_tb(n);
		gen_set_pc_im(dest);
		tcg_gen_exit_tb((long) tb + n);
	} else {
		gen_set_pc_im(dest);
		tcg_gen_exit_tb(0);
	}
}

/* Set PC state from an immediate address.  */
static inline void gen_bx_im(DisasContext *s, uint32_t addr) {
	//     TCGv tmp;
	s->is_jmp = DISAS_UPDATE;
	tcg_gen_movi_i32(cpu_R[63], addr);
}

/* functions of jump */
static inline void gen_jmp(DisasContext *s, uint32_t dest) {
	if (unlikely(s->singlestep_enabled)) {
		/* An indirect jump so that we still trigger the debug exception.  */
		gen_bx_im(s, dest);
	} else {
		gen_goto_tb(s, 0, dest);
		s->is_jmp = DISAS_TB_JUMP;
	}
}

/* PUSH and POP sp */
static inline void gen_push(DisasContext *s, int reg) {
	TCGv tmp, addr;
	tmp = new_tmp();
	addr = new_tmp();
	tmp = load_reg(s, reg);
	addr = load_reg(s, 62);
	gen_st32(tmp, addr, IS_USER(s));
	tcg_gen_subi_i32(addr, addr, 2);
	store_reg(s, 62, addr);
	dead_tmp(tmp);
	dead_tmp(addr);
}

static inline void gen_pop(DisasContext *s, int reg) {
	TCGv tmp, addr;
	tmp = new_tmp();
	addr = new_tmp();
	addr = load_reg(s, 62);
	tcg_gen_addi_i32(addr, addr, 2);
	tmp = gen_ld32(addr, IS_USER(s));
	store_reg(s, 62, addr);
	store_reg(s, reg, tmp);
	dead_tmp(tmp);
	dead_tmp(addr);
}
//end


static inline TCGv load_reg_8(DisasContext *s, int reg) {
	TCGv tmp;
	tmp = load_reg(s, reg);
	tcg_gen_shri_i32(tmp, tmp, (3 - reg % 4) * 8);
	tcg_gen_andi_i32(tmp, tmp, 0xff);
	return tmp;
}

static inline void store_reg_8(DisasContext *s, int reg, TCGv var) {
	TCGv tmp;
	tmp = load_reg(s, reg);
	tcg_gen_andi_i32(tmp, tmp, ~(0xff << ((3 - reg % 4) * 8)));
	tcg_gen_shli_i32(var, var, (3 - reg % 4) * 8);
	tcg_gen_or_i32(tmp, tmp, var);
	store_reg(s, reg, tmp);
	dead_tmp(var);
}

static inline void check_ZF_SF_8(DisasContext *s, TCGv var) {
	//check ZF
	TCGv tmp1 = new_tmp();
	tmp1 = load_reg(s, 61);
	if (var.i32 == 0x00) {
		tcg_gen_ori_i32(tmp1, tmp1, 0x4000);
		store_reg(s, 61, tmp1);
	} else {
		tcg_gen_andi_i32(tmp1, tmp1, 0xffffbfff);
		store_reg(s, 61, tmp1);
	}
	//check SF
	TCGv tmp2 = new_tmp();
	tmp2 = load_reg(s, 61);
	if ((var.i32 & 0x80) == 0x80) {
		tcg_gen_ori_i32(tmp2, tmp2, 0x00002000);
		store_reg(s, 61, tmp2);
	} else {
		tcg_gen_andi_i32(tmp2, tmp2, 0xffffDfff);
		store_reg(s, 61, tmp2);
	}

}


static unsigned int get_insn_length(unsigned int insn) {
	unsigned int length;

	switch (insn >> 28) {
	case 0x00:
		length = 1;
		break;
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x0C:
		length = 3;
		break;
	case 0x04:
	case 0x05:
		length = 6;
		break;
	case 0x06:
	case 0x0E:
	case 0x0F:
		length = 2;
		break;
	case 0x0B:
		length = 4;
		break;
	case 0x0D:
		length = 5;
		break;
	default:
		length = 0;
		break;
	}

	return length;
}

static void disas_srp_insn(CPUState * env, DisasContext *s) {
	unsigned int insn, rs, rd, iLength, val;
	int32_t imm32, offset;
	TCGv tmp, tmp1, tmp2, addr;


	insn = ldl_code(s->pc);
	iLength = get_insn_length(insn);

	switch (iLength) {
	case 1:
		switch (insn >> 24) {
		case 0x00:
		case 0x01:
			gen_exception(EXCP_SWI);
			s->is_jmp = DISAS_SWI;
			break;
		case 0x02:
			break;
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		default:
			goto illegal_op;
		}
		break;
	case 2:
		rd = (insn >> 16) & 0xff;
		switch (insn >> 24) {
		/* --------------------------2011-11-22 by wanglizhi----------------------------- */
		//begin
		/*
		case 0x60:
			tmp1 = load_reg(s, rd);
			tcg_gen_rotli_i32(tmp1, tmp1, 1);
			store_reg(s, rd, tmp1);
			break;
		case 0x61:
			tmp1 = load_reg(s, rd);
			tcg_gen_rotli_i32(tmp1, tmp1, 1);
			tcg_gen_ori_i32(tmp1, tmp1, 0x00000001);
			tmp2 = load_reg(s, 61);
			tcg_gen_shri_i32(tmp2, tmp2, 15);
			tcg_gen_ori_i32(tmp2, tmp2, 0xfffffffe);
			tcg_gen_andi_i32(tmp1, tmp1, tmp2.i32);
			store_reg(s, rd, tmp1);
			break;
		case 0x62:
			tmp1 = load_reg(s, rd);
			tcg_gen_rotri_i32(tmp1, tmp1, 1);
			store_reg(s, rd, tmp1);
			break;
		case 0x63:
			tmp1 = load_reg(s, rd);
			tcg_gen_rotri_i32(tmp1, tmp1, 1);
			tcg_gen_ori_i32(tmp1, tmp1, 0x80000000);
			tmp2 = load_reg(s, 61);
			tcg_gen_shli_i32(tmp2, tmp2, 16);
			tcg_gen_ori_i32(tmp2, tmp2, 0x7fffffff);
			tcg_gen_andi_i32(tmp1, tmp1, tmp2.i32);
			store_reg(s, rd, tmp1);
			break;
		*/
        //Modification Starts---DXW
		case 0x60:
			rd = (insn >> 16) & 0xff;
			tmp1 = load_reg(s, rd);
			gen_helper_rol_cc(tmp1, tmp1);
			store_reg(s, rd, tmp1);
			break;
	    case 0x61:
			rd = (insn >> 16) & 0xff;
			tmp1 = load_reg(s, rd);
			gen_helper_rcl_cc(tmp1, tmp1);
			store_reg(s, rd, tmp1);
			break;

		case 0x62:
		    rd = (insn >> 16) & 0xff;
			tmp1 = load_reg(s, rd);
			gen_helper_ror_cc(tmp1,tmp1);
			store_reg(s, rd, tmp1);
			break;
		case 0x63:
			rd = (insn >> 16) & 0xff;
			tmp1 = load_reg(s, rd);
			gen_helper_rcr_cc(tmp1, tmp1);
			store_reg(s, rd, tmp1);
			break;
        //Modification Ends---DXW
		case 0x64:
			gen_push(s, rd);
			break;
		case 0x65:
			gen_pop(s, rd);
			break;
		case 0x66:
			tmp1 = load_reg(s, rd);
			tmp2 = load_reg(s, rd);
			//bit[3:0] mov to bit[32:29]
			tcg_gen_shli_i32(tmp1, tmp1, 28);
			//bit[32:29] mov to bit[3:0]
			tcg_gen_shri_i32(tmp2, tmp2, 28);
			tcg_gen_ori_i32(tmp1, tmp1, tmp2.i32);
			tmp2 = load_reg(s, rd);
			tcg_gen_andi_i32(tmp2, tmp2, 0x0ffffff0);
			tcg_gen_ori_i32(tmp1, tmp1, tmp2.i32);

			//check ZF
			tmp2 = load_reg(s, 61);
			if (tmp1.i32 == 0x00) {
				tcg_gen_ori_i32(tmp2, tmp2, 0x00004000);
				store_reg(s, 61, tmp2);
			} else {
				tcg_gen_andi_i32(tmp2, tmp2, 0xffffBfff);
				store_reg(s, 61, tmp2);
			}

			//check SF
			TCGv tmp3 = new_tmp();
			tmp3 = load_reg(s, 61);
			if ((tmp1.i32 & 0x80000000) == 0x10000000) {
				tcg_gen_ori_i32(tmp3, tmp3, 0x00002000);
				store_reg(s, 61, tmp3);
			} else {
				tcg_gen_andi_i32(tmp3, tmp3, 0xffffDfff);
				store_reg(s, 61, tmp3);
			}
			store_reg(s, rd, tmp1);
			break;
		case 0x67:
			addr = load_reg(s, rd);
			tmp = gen_ld32(addr, IS_USER(s));
			gen_push(s, 63);
			gen_bx_im(s, tmp.i32);
			break;
		case 0x68:
			tmp1 = load_reg(s, rd);
			tcg_gen_addi_i32(tmp1, tmp1, 1);
			if (tmp1.i32 == 0x00) {
				tmp2 = load_reg(s, 61);
				tcg_gen_ori_i32(tmp1, tmp1, 0x4000);
				store_reg(s, 61, tmp2);
			} else {
				tmp2 = load_reg(s, 61);
				tcg_gen_andi_i32(tmp1, tmp1, 0xffffBfff);
				store_reg(s, 61, tmp2);
			}
			store_reg(s, rd, tmp1);
			break;
		case 0x69:
			tmp1 = load_reg(s, rd);
			tcg_gen_addi_i32(tmp1, tmp1, 2);
			if (tmp1.i32 == 0x00) {
				tmp2 = load_reg(s, 61);
				tcg_gen_ori_i32(tmp1, tmp1, 0x4000);
				store_reg(s, 61, tmp2);
			} else {
				tmp2 = load_reg(s, 61);
				tcg_gen_andi_i32(tmp1, tmp1, 0xffffBfff);
				store_reg(s, 61, tmp2);
			}
			store_reg(s, rd, tmp1);
			break;
		case 0x6A:
			tmp1 = load_reg(s, rd);
			tcg_gen_addi_i32(tmp1, tmp1, 3);
			if (tmp1.i32 == 0x00) {
				tmp2 = load_reg(s, 61);
				tcg_gen_ori_i32(tmp1, tmp1, 0x4000);
				store_reg(s, 61, tmp2);
			} else {
				tmp2 = load_reg(s, 61);
				tcg_gen_andi_i32(tmp1, tmp1, 0xffffBfff);
				store_reg(s, 61, tmp2);
			}
			store_reg(s, rd, tmp1);
			break;
		case 0x6B:
			tmp1 = load_reg(s, rd);
			tcg_gen_addi_i32(tmp1, tmp1, 4);
			if (tmp1.i32 == 0x00) {
				tmp2 = load_reg(s, 61);
				tcg_gen_ori_i32(tmp1, tmp1, 0x4000);
				store_reg(s, 61, tmp2);
			} else {
				tmp2 = load_reg(s, 61);
				tcg_gen_andi_i32(tmp1, tmp1, 0xffffBfff);
				store_reg(s, 61, tmp2);
			}
			store_reg(s, rd, tmp1);
			break;
		case 0x6C:
			tmp1 = load_reg(s, rd);
			tcg_gen_subi_i32(tmp1, tmp1, 1);
			if (tmp1.i32 == 0x00) {
				tmp2 = load_reg(s, 61);
				tcg_gen_ori_i32(tmp1, tmp1, 0x4000);
				store_reg(s, 61, tmp2);
			} else {
				tmp2 = load_reg(s, 61);
				tcg_gen_andi_i32(tmp1, tmp1, 0xffffBfff);
				store_reg(s, 61, tmp2);
			}
			store_reg(s, rd, tmp1);
			break;
		case 0x6D:
			tmp1 = load_reg(s, rd);
			tcg_gen_subi_i32(tmp1, tmp1, 2);
			if (tmp1.i32 == 0x00) {
				tmp2 = load_reg(s, 61);
				tcg_gen_ori_i32(tmp1, tmp1, 0x4000);
				store_reg(s, 61, tmp2);
			} else {
				tmp2 = load_reg(s, 61);
				tcg_gen_andi_i32(tmp1, tmp1, 0xffffBfff);
				store_reg(s, 61, tmp2);
			}
			store_reg(s, rd, tmp1);
			break;
		case 0x6E:
			tmp1 = load_reg(s, rd);
			tcg_gen_subi_i32(tmp1, tmp1, 3);
			if (tmp1.i32 == 0x00) {
				tmp2 = load_reg(s, 61);
				tcg_gen_ori_i32(tmp1, tmp1, 0x4000);
				store_reg(s, 61, tmp2);
			} else {
				tmp2 = load_reg(s, 61);
				tcg_gen_andi_i32(tmp1, tmp1, 0xffffBfff);
				store_reg(s, 61, tmp2);
			}
			store_reg(s, rd, tmp1);
			break;
		case 0x6F:
			tmp1 = load_reg(s, rd);
			tcg_gen_subi_i32(tmp1, tmp1, 4);
			if (tmp1.i32 == 0x00) {
				tmp2 = load_reg(s, 61);
				tcg_gen_ori_i32(tmp1, tmp1, 0x4000);
				store_reg(s, 61, tmp2);
			} else {
				tmp2 = load_reg(s, 61);
				tcg_gen_andi_i32(tmp1, tmp1, 0xffffBfff);
				store_reg(s, 61, tmp2);
			}
			store_reg(s, rd, tmp1);
			break;
		case 0xE0:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_andi_i32(tmp1, tmp1, 0x7f);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xE1:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_andi_i32(tmp1, tmp1, 0xBf);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xE2:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_andi_i32(tmp1, tmp1, 0xDf);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xE3:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_andi_i32(tmp1, tmp1, 0xEf);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xE4:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_andi_i32(tmp1, tmp1, 0xF7);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xE5:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_andi_i32(tmp1, tmp1, 0xFB);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xE6:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_andi_i32(tmp1, tmp1, 0xFD);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xE7:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_andi_i32(tmp1, tmp1, 0xFE);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xE8:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_andi_i32(tmp1, tmp1, 0x80);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xE9:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_andi_i32(tmp1, tmp1, 0x40);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xEA:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_andi_i32(tmp1, tmp1, 0x20);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xEB:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_andi_i32(tmp1, tmp1, 0x10);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xEC:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_andi_i32(tmp1, tmp1, 0x08);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xED:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_andi_i32(tmp1, tmp1, 0x04);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xEE:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_andi_i32(tmp1, tmp1, 0x02);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xEF:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_andi_i32(tmp1, tmp1, 0x01);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;

		case 0xF0:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_ori_i32(tmp1, tmp1, 0x80);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xF1:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_ori_i32(tmp1, tmp1, 0x40);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xF2:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_ori_i32(tmp1, tmp1, 0x20);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xF3:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_ori_i32(tmp1, tmp1, 0x10);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xF4:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_ori_i32(tmp1, tmp1, 0x08);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xF5:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_ori_i32(tmp1, tmp1, 0x04);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xF6:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_ori_i32(tmp1, tmp1, 0x02);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xF7:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_ori_i32(tmp1, tmp1, 0x01);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xF8:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_xori_i32(tmp1, tmp1, 0x80);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xF9:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_xori_i32(tmp1, tmp1, 0x40);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xFA:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_xori_i32(tmp1, tmp1, 0x20);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xFB:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_xori_i32(tmp1, tmp1, 0x10);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xFC:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_xori_i32(tmp1, tmp1, 0x08);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xFD:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_xori_i32(tmp1, tmp1, 0x04);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xFE:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_xori_i32(tmp1, tmp1, 0x02);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
		case 0xFF:
			tmp1 = load_reg_8(s, rd);
			tcg_gen_xori_i32(tmp1, tmp1, 0x01);
			check_ZF_SF_8(s, tmp1);
			store_reg_8(s, rd, tmp1);
			break;
			//end
		default:
			goto illegal_op;
		}
		break;
	case 3:
		switch (insn >> 24) {
		///DXW//
		case 0x10:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			tmp1 = load_reg_8(s, rd);
			tcg_gen_shli_i32(tmp1, tmp1, 24);
			tmp2 = load_reg_8(s, rs);
			tcg_gen_shli_i32(tmp2, tmp2, 24);
			gen_helper_add_cc(tmp1, tmp1, tmp2);
			tcg_gen_shri_i32(tmp1, tmp1, 24);
			store_reg_8(s, rd, tmp1);
			dead_tmp(tmp2);
			break;
		case 0x11:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			tmp1 = load_reg_8(s, rd);
			tcg_gen_shli_i32(tmp1, tmp1, 24);
			tmp2 = load_reg_8(s, rs);
			tcg_gen_shli_i32(tmp2, tmp2, 24);
			gen_helper_sub_cc(tmp1, tmp1, tmp2);
			tcg_gen_shri_i32(tmp1, tmp1, 24);
			store_reg_8(s, rd, tmp1);
			dead_tmp(tmp2);
			break;
		case 0x12:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			tmp1 = load_reg_8(s, rd);
			tcg_gen_shli_i32(tmp1, tmp1, 24);
			tmp2 = load_reg_8(s, rs);
			tcg_gen_shli_i32(tmp2, tmp2, 24);
			tcg_gen_or_i32(tmp1, tmp1, tmp2);
			gen_logic_CC(tmp1);
			tcg_gen_shri_i32(tmp1, tmp1, 24);
			store_reg_8(s, rd, tmp1);
			dead_tmp(tmp2);
			break;
		case 0x13:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			tmp1 = load_reg_8(s, rd);
			tcg_gen_shli_i32(tmp1, tmp1, 24);
			tmp2 = load_reg_8(s, rs);
			tcg_gen_shli_i32(tmp2, tmp2, 24);
			tcg_gen_xor_i32(tmp1, tmp1, tmp2);
			gen_logic_CC(tmp1);
			tcg_gen_shri_i32(tmp1, tmp1, 24);
			store_reg_8(s, rd, tmp1);
			dead_tmp(tmp2);
			break;
		case 0x14:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			tmp1 = load_reg_8(s, rd);
			tcg_gen_shli_i32(tmp1, tmp1, 24);
			tmp2 = load_reg_8(s, rs);
		    tcg_gen_shli_i32(tmp2, tmp2, 24);
			tcg_gen_and_i32(tmp1, tmp1, tmp2);
			gen_logic_CC(tmp1);
			tcg_gen_shri_i32(tmp1, tmp1, 24);
			store_reg_8(s, rd, tmp1);
			dead_tmp(tmp2);
			break;
		case 0x15:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			tmp1 = load_reg_8(s, rd);
			tcg_gen_shli_i32(tmp1, tmp1, 24);
			tmp2 = load_reg_8(s, rs);
			tcg_gen_shli_i32(tmp2, tmp2, 24);
			tcg_gen_or_i32(tmp1, tmp1, tmp2);
			gen_logic_CC(tmp1);
			dead_tmp(tmp1);
			dead_tmp(tmp2);
			break;
		case 0x16:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			tmp1 = load_reg_8(s, rd);
			tcg_gen_shli_i32(tmp1, tmp1, 24);
			tmp2 = load_reg_8(s, rs);
			tcg_gen_shli_i32(tmp2, tmp2, 24);
			tcg_gen_sub_i32(tmp1, tmp1, tmp2);
			gen_logic_CC(tmp1);
			dead_tmp(tmp1);
			dead_tmp(tmp2);
			break;
		case 0x30:
			rd = (insn >> 16) & 0xff;
			imm32 = (insn >> 8) & 0xff;
			tmp1 = load_reg_8(s, rd);
			tmp2=new_tmp();
			tcg_gen_shli_i32(tmp1, tmp1, 24);
			tcg_gen_movi_i32(tmp2, imm32 << 24);
			gen_helper_add_cc(tmp1, tmp1, tmp2);
			tcg_gen_shri_i32(tmp1, tmp1, 24);
			store_reg_8(s, rd, tmp1);
			dead_tmp(tmp2);
			break;
		case 0x31:
			rd = (insn >> 16) & 0xff;
			imm32 = (insn >> 8) & 0xff;
			tmp1 = load_reg_8(s, rd);
			tmp2=new_tmp();
		    tcg_gen_shli_i32(tmp1, tmp1, 24);
			tcg_gen_movi_i32(tmp2, imm32 << 24);
			gen_helper_sub_cc(tmp1, tmp1, tmp2);
			tcg_gen_shri_i32(tmp1, tmp1, 24);
			store_reg_8(s, rd, tmp1);
			dead_tmp(tmp2);
			break;
		case 0x32:
			rd = (insn >> 16) & 0xff;
			imm32 = (insn >> 8) & 0xff;
			tmp1 = load_reg_8(s, rd);
			tmp2=new_tmp();
			tcg_gen_shli_i32(tmp1, tmp1, 24);
			tcg_gen_movi_i32(tmp2, imm32 << 24);
			tcg_gen_or_i32(tmp1, tmp1, tmp2);
			gen_logic_CC(tmp1);
			tcg_gen_shri_i32(tmp1, tmp1, 24);
			store_reg_8(s, rd, tmp1);
			dead_tmp(tmp2);
			break;
		case 0x33:
			rd = (insn >> 16) & 0xff;
			imm32 = (insn >> 8) & 0xff;
			tmp1 = load_reg_8(s, rd);
			tmp2=new_tmp();
			tcg_gen_shli_i32(tmp1, tmp1, 24);
			tcg_gen_movi_i32(tmp2, imm32 << 24);
			tcg_gen_xor_i32(tmp1, tmp1, tmp2);
			gen_logic_CC(tmp1);
			tcg_gen_shri_i32(tmp1, tmp1, 24);
			store_reg_8(s, rd, tmp1);
			dead_tmp(tmp2);
			break;
		case 0x34:
			rd = (insn >> 16) & 0xff;
			imm32 = (insn >> 8) & 0xff;
			tmp1 = load_reg_8(s, rd);
			tmp2=new_tmp();
			tcg_gen_shli_i32(tmp1, tmp1, 24);
			tcg_gen_movi_i32(tmp2, imm32 << 24);
			tcg_gen_and_i32(tmp1, tmp1, tmp2);
			gen_logic_CC(tmp1);
			tcg_gen_shri_i32(tmp1, tmp1, 24);
			store_reg_8(s, rd, tmp1);
			dead_tmp(tmp2);
			break;
		case 0x35:
			rd = (insn >> 16) & 0xff;
			imm32 = (insn >> 8) & 0xff;
			tmp1 = load_reg_8(s, rd);
			tmp2=new_tmp();
			tcg_gen_shli_i32(tmp1, tmp1, 24);
			tcg_gen_movi_i32(tmp2, imm32 << 24);
			tcg_gen_or_i32(tmp1, tmp1, tmp2);
			gen_logic_CC(tmp1);
			dead_tmp(tmp1);
			dead_tmp(tmp2);
			break;
		case 0x36:
			rd = (insn >> 16) & 0xff;
			imm32 = (insn >> 8) & 0xff;
			tmp1 = load_reg_8(s, rd);
			tmp2=new_tmp();
		    tcg_gen_shli_i32(tmp1, tmp1, 24);
			tcg_gen_movi_i32(tmp2, imm32 << 24);
			tcg_gen_sub_i32(tmp1, tmp1, tmp2);
			gen_logic_CC(tmp1);
			dead_tmp(tmp1);
			dead_tmp(tmp2);
			break;

		case 0x17:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			tmp = load_reg_8(s, rs);
			store_reg_8(s, rd, tmp);
			break;
		case 0x1C:
		case 0x1E:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			addr = load_reg(s, rs);
			tmp = gen_ld8s(addr, IS_USER(s));
			store_reg_8(s, rd, tmp);
			dead_tmp(addr);
			break;
		case 0x1D:
		case 0x1F:
			rs = (insn >> 16) & 0xff;
			rd = (insn >> 8) & 0xff;
			addr = load_reg(s, rd);
			tmp = load_reg_8(s, rs);
			gen_st8(tmp, addr, IS_USER(s));
			dead_tmp(addr);
			break;
		case 0x27:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			tcg_gen_mov_tl(cpu_R[rd >> 2], cpu_R[rs >> 2]);
			break;
		case 0x2C:
		case 0x2E:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			addr = load_reg(s, rs);
			tmp1 = gen_ld32(addr, IS_USER(s));
			store_reg(s, rd, tmp1);
			dead_tmp(addr);
			break;
		case 0x2D:
		case 0x2F:
			rs = (insn >> 16) & 0xff;
			rd = (insn >> 8) & 0xff;
			tmp1 = load_reg(s, rs);
			addr = load_reg(s, rd);
			gen_st32(tmp1, addr, IS_USER(s));
			dead_tmp(addr);
			break;

		case 0x37:
			rd = (insn >> 16) & 0xff;
			imm32 = (insn >> 8) & 0xff;
			tmp1 = new_tmp();
			tcg_gen_movi_i32(tmp1, imm32);
			store_reg_8(s, rd, tmp1);
			break;
		case 0x38:
		case 0x3A:
			rd = (insn >> 16) & 0xff;
			imm32 = (insn >> 8) & 0xff;
			tmp1 = new_tmp();
			addr = new_tmp();
			tcg_gen_movi_i32(tmp1, imm32);
			tcg_gen_movi_i32(addr, rd);
			gen_st8(tmp1, addr, IS_USER(s));
			dead_tmp(addr);
			break;
		case 0x39:
		case 0x3B:
			rd = (insn >> 16) & 0xff;
			imm32 = (insn >> 8) & 0xff;
			tmp1 = new_tmp();
			tcg_gen_movi_i32(tmp1, imm32);
			addr = load_reg(s, rd);
			gen_st8(tmp1, addr, IS_USER(s));
			dead_tmp(addr);
			break;
		case 0x3C:
		case 0x3E:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			addr = new_tmp();
			tcg_gen_movi_i32(addr, rs);
			tmp1 = gen_ld32(addr, IS_USER(s));
			store_reg(s, rd, tmp1);
			dead_tmp(addr);
			break;
		case 0x3D:
		case 0x3F:
			rs = (insn >> 16) & 0xff;
			rd = (insn >> 8) & 0xff;
			addr = new_tmp();
			tcg_gen_movi_i32(addr, rd);
			tmp1 = load_reg(s, rs);
			gen_st32(tmp1, addr, IS_USER(s));
			dead_tmp(addr);
			break;
			/*********************************Arithmetic Operations******************************/
		case 0x20:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			tmp1 = load_reg(s, rd);
			tmp2 = load_reg(s, rs);
			gen_helper_add_cc(tmp1, tmp1, tmp2);
			store_reg(s, rd, tmp1);
			dead_tmp(tmp2);
			break;
		case 0x21:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			tmp1 = load_reg(s, rd);
			tmp2 = load_reg(s, rs);
			gen_helper_sub_cc(tmp1, tmp1, tmp2);
			store_reg(s, rd, tmp1);
			dead_tmp(tmp2);
			break;
		case 0x22:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			tmp1 = load_reg(s, rd);
			tmp2 = load_reg(s, rs);
			tcg_gen_or_i32(tmp1, tmp1, tmp2);
			gen_logic_CC(tmp1);
			store_reg(s, rd, tmp1);
			dead_tmp(tmp2);
			break;
		case 0x23:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			tmp1 = load_reg(s, rd);
			tmp2 = load_reg(s, rs);
			tcg_gen_xor_i32(tmp1, tmp1, tmp2);
			gen_logic_CC(tmp1);
			store_reg(s, rd, tmp1);
			dead_tmp(tmp2);
			break;
		case 0x24:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			tmp1 = load_reg(s, rd);
			tmp2 = load_reg(s, rs);
			tcg_gen_and_i32(tmp1, tmp1, tmp2);
			gen_logic_CC(tmp1);
			store_reg(s, rd, tmp1);
			dead_tmp(tmp2);
			break;
		case 0x25:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			tmp1 = load_reg(s, rd);
			tmp2 = load_reg(s, rs);
			tcg_gen_or_i32(tmp1, tmp1, tmp2);
			gen_logic_CC(tmp1);
			dead_tmp(tmp1);
			dead_tmp(tmp2);
			break;
		case 0x26:
			rd = (insn >> 16) & 0xff;
			rs = (insn >> 8) & 0xff;
			tmp1 = load_reg(s, rd);
			tmp2 = load_reg(s, rs);
			tcg_gen_sub_i32(tmp1, tmp1, tmp2);
			gen_logic_CC(tmp1);
			dead_tmp(tmp1);
			dead_tmp(tmp2);
			break;


			/* --------------------------2011-11-22 by wanglizhi----------------------------- */
			//begin
            //Modification Starts---DXW
		case 0xC0:
		case 0xC1:
		case 0xC2:
		case 0xC3:
		case 0xC4:
		case 0xC5:
		case 0xC6:
		case 0xC8:
		case 0xC9:
		case 0xCA:
		case 0xCB:
		case 0xCC:
		case 0xCD:
		case 0xCE:
			offset = (insn >> 8) & 0xffff;
            s->condlabel = gen_new_label();
            gen_test_cc(insn >> 24, s->condlabel);
            s->condjmp = 1;
			val = (int32_t) s->pc;
			val += (offset) + 3;
			gen_jmp(s, val);
			break;

		case 0xC7:
			offset = (insn >> 8) & 0xffff;
			val = (int32_t) s->pc;
			val += (offset) + 3;
			gen_jmp(s, val);
			break;

		case 0xCF:
            break;
            //Modification Ends---DXW
			//end
		default:
			goto illegal_op;
		}
		break;
	case 4:
		/* --------------------------2011-11-22 by wanglizhi----------------------------- */
		//begin
		rs = insn & 0xff;
		offset = (insn >> 8) & 0xffff;
		tmp1 = load_reg_8(s, rs);
		switch (insn >> 24) {
		case 0xB0:
			tcg_gen_shri_i32(tmp1, tmp1, 1);
		case 0xB1:
			tcg_gen_shri_i32(tmp1, tmp1, 1);
		case 0xB2:
			tcg_gen_shri_i32(tmp1, tmp1, 1);
		case 0xB3:
			tcg_gen_shri_i32(tmp1, tmp1, 1);
		case 0xB4:
			tcg_gen_shri_i32(tmp1, tmp1, 1);
		case 0xB5:
			tcg_gen_shri_i32(tmp1, tmp1, 1);
		case 0xB6:
			tcg_gen_shri_i32(tmp1, tmp1, 1);
		case 0xB7:
			tcg_gen_andi_i32(tmp1, tmp1, 1);
        //Modification Starts---DXW
            s->condlabel = gen_new_label();
            tcg_gen_brcondi_i32(TCG_COND_NE, tmp1, 0, s->condlabel);
            s->condjmp = 1;
			val = (int32_t) s->pc;
			val += (offset) + 4;
			gen_jmp(s, val);
        //Modification Ends---DXW
			break;

		case 0xB8:
			tcg_gen_shri_i32(tmp1, tmp1, 1);
		case 0xB9:
			tcg_gen_shri_i32(tmp1, tmp1, 1);
		case 0xBA:
			tcg_gen_shri_i32(tmp1, tmp1, 1);
		case 0xBB:
			tcg_gen_shri_i32(tmp1, tmp1, 1);
		case 0xBC:
			tcg_gen_shri_i32(tmp1, tmp1, 1);
		case 0xBD:
			tcg_gen_shri_i32(tmp1, tmp1, 1);
		case 0xBE:
			tcg_gen_shri_i32(tmp1, tmp1, 1);
		case 0xBF:
			tcg_gen_andi_i32(tmp1, tmp1, 1);
        //Modification Starts---DXW
            s->condlabel = gen_new_label();
            tcg_gen_brcondi_i32(TCG_COND_EQ, tmp1, 0, s->condlabel);
            s->condjmp = 1;
			val = (int32_t) s->pc;
			val += (offset) + 4;
			gen_jmp(s, val);
        //Modification Ends---DXW
			break;
		default:
			goto illegal_op;
		}
		//end
		dead_tmp(tmp1);
		break;
	case 5:
		switch (insn >> 24) {
		/* --------------------------2011-11-29 by wanglizhi----------------------------- */
		 //Modification Starts---DXW
		 case 0xD4:
		    rd = (insn >> 16) & 0xff;
		    tmp1 = load_reg(s, rd);
		    tmp2 = new_tmp();
		    tcg_gen_subi_i32(tmp1, tmp1, 1);
		    tcg_gen_mov_i32(tmp2, tmp1);
		    store_reg(s, rd, tmp2);
		    offset = ldl_code(s->pc + 1);
		    offset = offset & 0xffffff;
            s->condlabel = gen_new_label();
            tcg_gen_brcondi_i32(TCG_COND_EQ, tmp1, 0, s->condlabel);
            dead_tmp(tmp1);
            s->condjmp = 1;
		    val = (int32_t)s->pc;
		    val += offset+5;
		    gen_jmp(s,val);
		    break;

		 case  0xD6:
			offset = ldl_code(s->pc + 1);
		    val = (int32_t)s->pc;
		    val += offset + 5;
		    gen_jmp(s,val);
		    break;
		 //Modification Ends---DXW
		 /*
		 case 0xD7:
		 gen_push(s,63);
		 val = gen_ld32( (int32_t)s->pc +1, IS_USER(s) );
		 gen_bx_im(s,val);
		 break;*/
		default:
			goto illegal_op;

		//end
		}

		break;
	case 6:
		switch (insn >> 24) {
		case 0x40:
			rd = (insn >> 16) & 0xff;
			imm32 = ldl_code(s->pc + 2);
			tmp1 = load_reg(s, rd);
			tmp2 = new_tmp();
			tcg_gen_movi_i32(tmp2, imm32);
			gen_helper_add_cc(tmp1, tmp1, tmp2);
			store_reg(s, rd, tmp1);
			dead_tmp(tmp2);
			break;
		case 0x41:
			rd = (insn >> 16) & 0xff;
			imm32 = ldl_code(s->pc + 2);
			tmp1 = load_reg(s, rd);
			tmp2 = new_tmp();
			tcg_gen_movi_i32(tmp2, imm32);
			gen_helper_sub_cc(tmp1, tmp1, tmp2);
			store_reg(s, rd, tmp1);
			dead_tmp(tmp2);
			break;
		case 0x42:
			rd = (insn >> 16) & 0xff;
			imm32 = ldl_code(s->pc + 2);
			tmp1 = load_reg(s, rd);
			tcg_gen_ori_i32(tmp1, tmp1, imm32);
			gen_logic_CC(tmp1);
			store_reg(s, rd, tmp1);
			break;
		case 0x43:
			rd = (insn >> 16) & 0xff;
			imm32 = ldl_code(s->pc + 2);
			tmp1 = load_reg(s, rd);
			tcg_gen_xori_i32(tmp1, tmp1, imm32);
			gen_logic_CC(tmp1);
			store_reg(s, rd, tmp1);
			break;
		case 0x44:
			rd = (insn >> 16) & 0xff;
			imm32 = ldl_code(s->pc + 2);
			tmp1 = load_reg(s, rd);
			tcg_gen_andi_i32(tmp1, tmp1, imm32);
			gen_logic_CC(tmp1);
			store_reg(s, rd, tmp1);
			break;
		case 0x45:
			rd = (insn >> 16) & 0xff;
			imm32 = ldl_code(s->pc + 2);
			tmp1 = load_reg(s, rd);
			tcg_gen_ori_i32(tmp1, tmp1, imm32);
			gen_logic_CC(tmp1);
			dead_tmp(tmp1);
			break;
		case 0x46:
			rd = (insn >> 16) & 0xff;
			imm32 = ldl_code(s->pc + 2);
			tmp1 = load_reg(s, rd);
			tcg_gen_subi_i32(tmp1, tmp1, imm32);
			gen_logic_CC(tmp1);
			dead_tmp(tmp1);
			break;

		case 0x47:
			rd = (insn >> 16) & 0xff;
			imm32 = ldl_code(s->pc + 2);
			tcg_gen_movi_i32(cpu_R[rd >> 2], imm32);
			break;
		case 0x48:
		case 0x4A:
			rd = (insn >> 16) & 0xff;
			addr = new_tmp();
			tcg_gen_movi_i32(addr, rd);
			imm32 = ldl_code(s->pc + 2);
			tmp1 = new_tmp();
			tcg_gen_movi_i32(tmp1, imm32);
			gen_st32(tmp1, addr, IS_USER(s));
			dead_tmp(addr);
			break;
		case 0x49:
		case 0x4B:
			rd = (insn >> 16) & 0xff;
			imm32 = ldl_code(s->pc + 2);
			addr = load_reg(s, rd);
			tmp1 = new_tmp();
			tcg_gen_movi_i32(tmp1, imm32);
			gen_st32(tmp1, addr, IS_USER(s));
			dead_tmp(addr);
			break;
		case 0x4C:
		case 0x4E:
			rd = (insn >> 16) & 0xff;
			rs = ldl_code(s->pc + 2);
			addr = new_tmp();
			tcg_gen_movi_i32(addr, rs);
			tmp1 = gen_ld32(addr, IS_USER(s));
			store_reg(s, rd, tmp1);
			dead_tmp(addr);
			break;
		case 0x4D:
		case 0x4F:
			rs = (insn >> 16) & 0xff;
			rd = ldl_code(s->pc + 2);
			addr = new_tmp();
			tcg_gen_movi_i32(addr, rd);
			tmp1 = load_reg(s, rs);
			gen_st32(tmp1, addr, IS_USER(s));
			dead_tmp(addr);
			break;

		case 0x5C:
		case 0x5E:
			rd = (insn >> 16) & 0xff;
			rs = ldl_code(s->pc + 2);
			addr = new_tmp();
			tcg_gen_movi_i32(addr, rs);
			tmp1 = gen_ld8s(addr, IS_USER(s));
			store_reg_8(s, rd, tmp1);
			dead_tmp(addr);
			break;
		case 0x5D:
		case 0x5F:
			rd = (insn >> 16) & 0xff;
			rs = ldl_code(s->pc + 2);
			addr = new_tmp();
			tcg_gen_movi_i32(addr, rs);
			tmp1 = load_reg_8(s, rd);
			gen_st8(tmp1, addr, IS_USER(s));
			dead_tmp(addr);
			break;
		default:
			goto illegal_op;
		}
		break;
	default:
		illegal_op:
		//       gen_set_condexec(s);
		gen_set_pc_im(s->pc - iLength);
		gen_exception(EXCP_UDEF);
		s->is_jmp = DISAS_JUMP;
		break;
	}

	s->pc += iLength;

}

/* generate intermediate code in gen_opc_buf and gen_opparam_buf for
 basic block 'tb'. If search_pc is TRUE, also generate PC
 information for each intermediate instruction. */
static inline void gen_intermediate_code_internal(CPUState *env,
		TranslationBlock *tb, int search_pc) {
	DisasContext dc1, *dc = &dc1;
	uint16_t *gen_opc_end;
	int j, lj;
	target_ulong pc_start;
	uint32_t next_page_start;
	int num_insns;
	int max_insns;

	/* generate intermediate code */
	num_temps = 0;

	pc_start = tb->pc;

	gen_opc_end = gen_opc_buf + OPC_MAX_SIZE;

	dc->tb = tb;
	dc->is_jmp = DISAS_NEXT;
	dc->condjmp = 0;
	dc->pc = pc_start;
	dc->singlestep_enabled = env->singlestep_enabled;
#if !defined(CONFIG_USER_ONLY)
	dc->user = 0;
#endif

	next_page_start = (pc_start & TARGET_PAGE_MASK) + TARGET_PAGE_SIZE;
	lj = -1;
	num_insns = 0;
	max_insns = tb->cflags & CF_COUNT_MASK;
	if (max_insns == 0)
		max_insns = CF_COUNT_MASK;

	gen_icount_start();

	do {
		/* Intercept jump to the magic kernel page.  */
		if (dc->pc >= 0xffff0000) {
			/* We always get here via a jump, so know we are not in a
			 conditional execution block.  */
			//             gen_exception(EXCP_KERNEL_TRAP);
			dc->is_jmp = DISAS_UPDATE;
			break;
		}

		//         if (unlikely(!QTAILQ_EMPTY(&env->breakpoints))) {
		//             QTAILQ_FOREACH(bp, &env->breakpoints, entry) {
		//                 if (bp->pc == dc->pc) {
		//                     gen_set_condexec(dc);
		//                     gen_set_pc_im(dc->pc);
		//                     gen_exception(EXCP_DEBUG);
		//                     dc->is_jmp = DISAS_JUMP;
		//                     /* Advance PC so that clearing the breakpoint will
		//                        invalidate this TB.  */
		//                     dc->pc += 2;
		//                     goto done_generating;
		//                     break;
		//                 }
		//             }
		//         }

		if (search_pc) {
			j = gen_opc_ptr - gen_opc_buf;
			if (lj < j) {
				lj++;
				while (lj < j)
					gen_opc_instr_start[lj++] = 0;
			}
			gen_opc_pc[lj] = dc->pc;
			gen_opc_instr_start[lj] = 1;
			gen_opc_icount[lj] = num_insns;
		}

		if (num_insns + 1 == max_insns && (tb->cflags & CF_LAST_IO))
			gen_io_start();

		disas_srp_insn(env, dc);

		if (num_temps) {
			fprintf(stderr, "Internal resource leak before %08x\n", dc->pc);
			num_temps = 0;
		}

		if (dc->condjmp && !dc->is_jmp) {
			gen_set_label(dc->condlabel);
			dc->condjmp = 0;
		}
		/* Translation stops when a conditional branch is encountered.
		 * Otherwise the subsequent code could get translated several times.
		 * Also stop translation when a page boundary is reached.  This
		 * ensures prefetch aborts occur at the right place.  */
		num_insns++;
	} while (!dc->is_jmp && gen_opc_ptr < gen_opc_end
			&& !env->singlestep_enabled && !singlestep && dc->pc
			< next_page_start && num_insns < max_insns);

	if (tb->cflags & CF_LAST_IO) {
		if (dc->condjmp) {
			/* FIXME:  This can theoretically happen with self-modifying
			 code.  */
			cpu_abort(env, "IO on conditional branch instruction");
		}
		gen_io_end();
	}

	/* At this stage dc->condjmp will only be set when the skipped
	 instruction was a conditional branch or trap, and the PC has
	 already been written.  */
	if (unlikely(env->singlestep_enabled)) {
		/* Make sure the pc is updated, and raise a debug exception.  */
		if (dc->condjmp) {
			//gen_set_condexec(dc);
			if (dc->is_jmp == DISAS_SWI) {
				//gen_exception(EXCP_SWI);
			} else {
				//gen_exception(EXCP_DEBUG);
			}
			gen_set_label(dc->condlabel);
		}
		if (dc->condjmp || !dc->is_jmp) {
			gen_set_pc_im(dc->pc);
			dc->condjmp = 0;
		}
		//gen_set_condexec(dc);
		if (dc->is_jmp == DISAS_SWI && !dc->condjmp) {
			//gen_exception(EXCP_SWI);
		} else {
			/* FIXME: Single stepping a WFI insn will not halt
			 the CPU.  */
			//gen_exception(EXCP_DEBUG);
		}
	} else {
		/* While branches must always occur at the end of an IT block,
		 there are a few other things that can cause us to terminate
		 the TB in the middel of an IT block:
		 - Exception generating instructions (bkpt, swi, undefined).
		 - Page boundaries.
		 - Hardware watchpoints.
		 Hardware breakpoints have already been handled and skip this code.
		 */
		//gen_set_condexec(dc);
		switch (dc->is_jmp) {
		case DISAS_NEXT:
			gen_goto_tb(dc, 1, dc->pc);
			break;
		default:
		case DISAS_JUMP:
		case DISAS_UPDATE:
			/* indicate that the hash table must be used to find the next TB */
			tcg_gen_exit_tb(0);
			break;
		case DISAS_TB_JUMP:
			/* nothing more to generate */
			break;
		case DISAS_WFI:
			//gen_helper_wfi();
			break;
		case DISAS_SWI:
			gen_exception(EXCP_SWI);
			break;
		}
		if (dc->condjmp) {
			gen_set_label(dc->condlabel);
			//    gen_set_condexec(dc);
			gen_goto_tb(dc, 1, dc->pc);
			dc->condjmp = 0;
		}
	}

	// done_generating:
	gen_icount_end(tb, num_insns);
	*gen_opc_ptr = INDEX_op_end;

#ifdef DEBUG_DISAS
	if (qemu_loglevel_mask(CPU_LOG_TB_IN_ASM)) {
		qemu_log("----------------\n");
		qemu_log("IN: %s\n", lookup_symbol(pc_start));
		log_target_disas(pc_start, dc->pc - pc_start, 0);
		qemu_log("\n");
	}
#endif
	if (search_pc) {
		j = gen_opc_ptr - gen_opc_buf;
		lj++;
		while (lj <= j)
			gen_opc_instr_start[lj++] = 0;
	} else {
		tb->size = dc->pc - pc_start;
		tb->icount = num_insns;
	}
}

void gen_intermediate_code(CPUState *env, TranslationBlock *tb) {
	gen_intermediate_code_internal(env, tb, 0);
}

void gen_intermediate_code_pc(CPUState *env, TranslationBlock *tb) {
	gen_intermediate_code_internal(env, tb, 1);
}

void cpu_dump_state(CPUState *env, FILE *f, int(*cpu_fprintf)(FILE *f,
		const char *fmt, ...), int flags) {
	int i;

	for (i = 0; i < SRP_REGS; i++) {
		cpu_fprintf(f, "%s=%08x", regnames[i], env->regs[i]);
		((i % 4) == 3) ? cpu_fprintf(f, "\n") : cpu_fprintf(f, " ");
	}

	uint32_t psr;
	psr = psw_read(env);
	cpu_fprintf(f, "PSR=%08x %c%c%c%c\t  ", psr,
			psr & (1 << 15) ? 'C' : '-', psr & (1 << 14) ? 'Z' : '-',
	        psr & (1 << 13) ? 'S' : '-', psr & (1 << 12) ? 'O' : '-');

	cpu_fprintf(f, " SP=%08x, PC=%08x\n", env->sp, env->pc);

}

void gen_pc_load(CPUState *env, TranslationBlock *tb,
		unsigned long searched_pc, int pc_pos, void *puc) {
	env->pc = gen_opc_pc[pc_pos];
}

