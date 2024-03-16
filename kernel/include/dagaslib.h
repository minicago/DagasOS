#ifndef __DAGASLIB__H__
#define __DAGASLIB__H__

void* memcpy(void *dest, const void *src, unsigned int n);
void* memset(void* dest, int c, unsigned long n);
int strcmp(const char *s1, const char *s2);
typedef struct linked_struct_t{
    struct linked_list_t* next;
    void* content;
} linked_list_t;

#endif

