
/* this struct defines the way the registers are stored on the
   stack during a system call. */

struct target_pt_regs {
    abi_ulong uregs[60];
	
	abi_ulong irp;
	abi_ulong psw;

	abi_ulong sp;
	abi_ulong pc;
};


#define SRP_r60		uregs[60]
#define SRP_r59		uregs[59]
#define SRP_r58		uregs[58]
#define SRP_r57		uregs[57]
#define SRP_r56		uregs[56]
#define SRP_r55		uregs[55]
#define SRP_r54		uregs[54]
#define SRP_r53		uregs[53]
#define SRP_r52		uregs[52]
#define SRP_r51		uregs[51]
#define SRP_r50		uregs[50]
#define SRP_r49		uregs[49]
#define SRP_r48		uregs[48]
#define SRP_r47		uregs[47]
#define SRP_r46		uregs[46]
#define SRP_r45		uregs[45]
#define SRP_r44		uregs[44]
#define SRP_r43		uregs[43]
#define SRP_r42		uregs[42]
#define SRP_r41		uregs[41]
#define SRP_r40		uregs[40]
#define SRP_r39		uregs[39]
#define SRP_r38		uregs[38]
#define SRP_r37		uregs[37]
#define SRP_r36		uregs[36]
#define SRP_r35		uregs[35]
#define SRP_r34		uregs[34]
#define SRP_r33		uregs[33]
#define SRP_r32		uregs[32]
#define SRP_r31		uregs[31]
#define SRP_r30		uregs[30]
#define SRP_r29		uregs[29]
#define SRP_r28		uregs[28]
#define SRP_r27		uregs[27]
#define SRP_r26		uregs[26]
#define SRP_r25		uregs[25]
#define SRP_r24		uregs[24]
#define SRP_r23		uregs[23]
#define SRP_r22		uregs[22]
#define SRP_r21		uregs[21]
#define SRP_r20		uregs[20]
#define SRP_r19		uregs[19]
#define SRP_r18		uregs[18]
#define SRP_r17		uregs[17]
#define SRP_r16		uregs[16]
#define SRP_r15		uregs[15]
#define SRP_r14		uregs[14]
#define SRP_r13		uregs[13]
#define SRP_r12		uregs[12]
#define SRP_r11		uregs[11]
#define SRP_r10		uregs[10]
#define SRP_r9		uregs[9]
#define SRP_r8		uregs[8]
#define SRP_r7		uregs[7]
#define SRP_r6		uregs[6]
#define SRP_r5		uregs[5]
#define SRP_r4		uregs[4]
#define SRP_r3		uregs[3]
#define SRP_r2		uregs[2]
#define SRP_r1		uregs[1]
#define SRP_r0		uregs[0]

#define SRP_SYSCALL_BASE	0x900000

#define SRP_NR_BASE	  0xf0000
#define SRP_NR_cacheflush (SRP_NR_BASE + 2)
#define SRP_NR_set_tls	  (SRP_NR_BASE + 5)

#define SRP_NR_semihosting	  0x123456


