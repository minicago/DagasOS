#ifndef __RISCV_SBI_H
#define __RISCV_SBI_H

#include "types.h"

#define LEGACY_SBI_CALL(eid, arg0) ({ \
	register uintptr_t a0 asm ("a0") = (uintptr_t)(arg0); \
	register uintptr_t a7 asm ("a7") = (uintptr_t)(eid); \
	asm volatile ("ecall" \
		      : "+r" (a0) \
		      : "r" (a7) \
		      : "memory"); \
	a0; \
})

// Legacy Extensions
// Only part of these extensions are used
#define SBI_SET_TIMER 			0
#define SBI_CONSOLE_PUTCHAR 	1
#define SBI_CONSOLE_GETCHAR 	2
#define SBI_SHUTDOWN 			8

static inline void sbi_console_putchar(int ch) {
	LEGACY_SBI_CALL(SBI_CONSOLE_PUTCHAR, ch);
}

static inline int sbi_console_getchar(void) {
	return LEGACY_SBI_CALL(SBI_CONSOLE_GETCHAR, 0);
}

static inline void sbi_shutdown(void) {
	LEGACY_SBI_CALL(SBI_SHUTDOWN, 0);
}

#endif