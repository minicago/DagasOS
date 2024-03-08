#ifndef __CLINT__H__
#define __CLINT__H__

#define CLINT 0x2000000L
#define CLINT_MTIMECMP (CLINT + 0x4000)
#define CLINT_MTIME (CLINT + 0xBFF8) // cycles since boot.

#endif