#ifndef __DAGASLIB__H__
#define __DAGASLIB__H__

void* memcpy(void *dest, const void *src, unsigned int n);
void* memset(void* dest, int c, unsigned long n);

typedef struct {
    struct linked_list_t* next;
    void* content;
} linked_list_t;

#endif

