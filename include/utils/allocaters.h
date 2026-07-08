#ifndef ALLOCATERS_H
#define ALLOCATERS_H

#include <stdio.h>

void *xcalloc(size_t n, size_t size);

void *xrealloc(void *ptr, size_t size);

void* xmalloc(size_t size);

#endif