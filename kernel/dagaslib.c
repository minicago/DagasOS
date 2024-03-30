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

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strlen(const char *str) {
    const char *s;
    for (s = str; *s; ++s);
    return (s - str);
}

int ceil_div(int dividend, int divisor) {
    return (dividend + divisor - 1) / divisor;
}