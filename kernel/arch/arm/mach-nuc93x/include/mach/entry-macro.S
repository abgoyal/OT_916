

#include <mach/hardware.h>
#include <mach/regs-irq.h>

	.macro  get_irqnr_preamble, base, tmp
	.endm

	.macro  arch_ret_to_user, tmp1, tmp2
	.endm

	.macro	get_irqnr_and_base, irqnr, irqstat, base, tmp

		mov	\base, #AIC_BA

		ldr	\irqnr, [ \base, #AIC_IPER]
		ldr	\irqnr, [ \base, #AIC_ISNR]
		cmp	\irqnr, #0

	.endm

	/* currently don't need an disable_fiq macro */

	.macro	disable_fiq
	.endm
