#ifndef __STRAP__H__
#define __STRAP__H__

void stvec();

void strap_handler();

void strap_init();

void intr_on();

void intr_off();

void intr_push_on();

void intr_push_off();

#endif 