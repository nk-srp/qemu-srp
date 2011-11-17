
#include "hw/hw.h"
#include "hw/boards.h"
#include "cpu.h"

void cpu_save(QEMUFile * f, void * opaque)
{
	int i;
	CPUSRPState *env = (CPUSRPState *)opaque;

	for (i = 0; i < SRP_REGS; i++) {
		qemu_put_be32(f, env->regs[i]);
	}

	qemu_put_be32(f, env->irq);
	qemu_put_be32(f, env->psw);

	qemu_put_be32(f, env->sp);
	qemu_put_be32(f, env->pc);
	
}

int cpu_load(QEMUFile *f, void *opaque, int version_id)
{
	CPUSRPState *env = (CPUSRPState *)opaque;
	int i;
	//uint32_t val;

	if (version_id != CPU_SAVE_VERSION)
		return -EINVAL;	

	for (i = 0; i < SRP_REGS; i++) {
		qemu_get_be32(f);
	}

	qemu_get_be32(f);
	qemu_get_be32(f);

	qemu_get_be32(f);
	qemu_get_be32(f);

	return 0;
}