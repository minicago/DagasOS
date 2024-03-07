
user/_test:     file format elf64-littleriscv


Disassembly of section .text:

0000000000000000 <main>:
   0:	1141                	addi	sp,sp,-16
   2:	e422                	sd	s0,8(sp)
   4:	0800                	addi	s0,sp,16
   6:	4781                	li	a5,0
   8:	853e                	mv	a0,a5
   a:	6422                	ld	s0,8(sp)
   c:	0141                	addi	sp,sp,16
   e:	8082                	ret
