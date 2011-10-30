/*
 * Tiny Code Generator for QEMU
 *
 * 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#define TCG_TARGET_SRP 1

#define TCG_TARGET_REG_BITS 32
#undef TCG_TARGET_WORDS_BIGENDIAN
#undef TCG_TARGET_STACK_GROWSUP

enum {
    TCG_REG_R0 = 0,
    TCG_REG_R1,
    TCG_REG_R2,
    TCG_REG_R3,
    TCG_REG_R4,
    TCG_REG_R5,
    TCG_REG_R6,
    TCG_REG_R7,
    TCG_REG_R8,
    TCG_REG_R9,
    TCG_REG_R10,
    TCG_REG_R11,
    TCG_REG_R12,
    TCG_REG_R13,
    TCG_REG_R14,
    TCG_REG_R15,
    TCG_REG_R16,
    TCG_REG_R17,
    TCG_REG_R18,
    TCG_REG_R19,
    TCG_REG_R20
    TCG_REG_R21,
    TCG_REG_R22,
    TCG_REG_R23,
    TCG_REG_R24,
    TCG_REG_R25,
    TCG_REG_R26,
    TCG_REG_R27,
    TCG_REG_R28,
    TCG_REG_R29,
    TCG_REG_R30,
    TCG_REG_R31,
    TCG_REG_R32,
    TCG_REG_R33,
    TCG_REG_R34,
    TCG_REG_R35,
    TCG_REG_R36,
    TCG_REG_R37,
    TCG_REG_R38,
    TCG_REG_R39,
    TCG_REG_R40,
    TCG_REG_R41,
    TCG_REG_R42,
    TCG_REG_R43,
    TCG_REG_R44,
    TCG_REG_R45,
    TCG_REG_R46,
    TCG_REG_R47,
    TCG_REG_R48,
    TCG_REG_R49,
    TCG_REG_R50,
    TCG_REG_R51,
    TCG_REG_R52,
    TCG_REG_R53,
    TCG_REG_R54,
    TCG_REG_R55,
    TCG_REG_R56,
    TCG_REG_R57,
    TCG_REG_R58,
    TCG_REG_R59,
    TCG_REG_R60,
    TCG_REG_IRQ,
    TCG_REG_PSW,
    TCG_REG_SP,
    TCG_REG_PC,
};

#define TCG_TARGET_NB_REGS 16

#define TCG_CT_CONST_SRP 0x100

/* used for function call generation */
#define TCG_REG_CALL_STACK		TCG_REG_R13
#define TCG_TARGET_STACK_ALIGN		8
#define TCG_TARGET_CALL_ALIGN_ARGS	1
#define TCG_TARGET_CALL_STACK_OFFSET	0

/* optional instructions */
// #define TCG_TARGET_HAS_orc_i32
// #define TCG_TARGET_HAS_eqv_i32
// #define TCG_TARGET_HAS_nand_i32
// #define TCG_TARGET_HAS_nor_i32

#define TCG_TARGET_HAS_GUEST_BASE

enum {
    /* Note: must be synced with dyngen-exec.h */
    TCG_AREG0 = TCG_REG_R7,
};

static inline void flush_icache_range(unsigned long start, unsigned long stop)
{

}
