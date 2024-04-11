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

char* strcpy(char* dest, const char* src) {
    char* ptr = dest;
    while ((*dest++ = *src++) != '\0');
    return ptr;
}

//TIP: "/a" is 2, "a" is 1, "a/" is 1
int get_file_depth(const char* path) {
    int depth = 1;
    for (int i = 0; path[i] != '\0'; i++) {
        if (path[i] == '/' && path[i + 1] != '\0') {
            depth++;
        }
    }
    return depth;
}

int remove_last_file(char* path) {
    int i = strlen(path) - 1;
    while (path[i] != '/' && i >= 0) {
        i--;
    }
    path[i] = '\0';
    return i;
}

int get_last_file(const char* path, char* filename) {
    int i = strlen(path) - 1;
    while (path[i] != '/' && i >= 0) {
        i--;
    }
    strcpy(filename, path + i + 1);
    return i;
}