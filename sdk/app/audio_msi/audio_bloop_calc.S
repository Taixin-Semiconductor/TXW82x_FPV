.global auadc_calc_energy_asm

.align 2

    .type auadc_calc_energy_asm, %function
auadc_calc_energy_asm:
    push              r4-r9,      r15
    mov               r4,         r0
    mov               r5,         r1
    mov               r6,         r2
	movi              r7,         0
calc:
	ld.hs             r8,         (r4,0)
    mulll.s16         r8,         r5,r8
	clipi.s32         r8,         r8,16
	st.h              r8,         (r4,0)
	mulall.s16.s      r7,         r8,r8
	addi              r4,         2
	bloop             r6,         calc
    mov               r0,         r7
    pop               r4-r9,      r15	