#ifndef __DAGASLIB__H__
#define __DAGASLIB__H__

void* memcpy(void *dest, const void *src, unsigned int n);
void* memset(void* dest, int c, unsigned long n);
int strcmp(const char *s1, const char *s2);
int strlen(const char *str);
int ceil_div(int dividend, int divisor);
char* strcpy(char* dest, const char* src);
int get_file_depth(const char* path);
int remove_last_file(char* path);
int get_last_file(const char* path, char* filename);

typedef struct linked_struct_t{
    struct linked_list_t* next;
    void* content;
} linked_list_t;

#endif

