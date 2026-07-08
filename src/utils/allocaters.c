#include <stdlib.h>
#include <stdio.h>

#include "utils/allocaters.h"

void *xcalloc(size_t n, size_t size)
{
    void *ptr = calloc(n, size);
    if (ptr == NULL)
    {
        fprintf(stderr, "memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size)
{
    ptr = realloc(ptr, size);
    if (ptr == NULL)
    {
        fprintf(stderr, "memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void* xmalloc(size_t size)
{
    void* ptr=malloc(size);
    if(ptr==NULL)
    {
        fprintf(stderr,"memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}