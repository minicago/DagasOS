#include "dagaslib.h"

void *memcpy(void *dest, const void *src, unsigned int n) {
  for (unsigned int i = 0; i < n; i++)
  {
      ((char*)dest)[i] = ((char*)src)[i];
  }
  return dest;
}

void* memset(void* dest, int c, unsigned long n) {
  char* s = (char*)dest;
  for (unsigned int i = 0; i < n; ++i) {
    s[i] = c;
  }
  return dest;
}