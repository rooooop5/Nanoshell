#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils/string_constructor.h"
#include "utils/allocaters.h"


typedef struct Concatenate_Buffer
{
    char *buffer;
    int pointer;
    int capacity;
} Concatenate_Buffer;

Concatenate_Buffer create_buffer()
{
    Concatenate_Buffer buffer;
    buffer.capacity = CONCATENATE_BUFFER_LEN;
    buffer.pointer = 0;
    buffer.buffer = (char *)xmalloc(buffer.capacity * sizeof(char));
    return buffer;
}

char *get_home_dir()
{
    struct passwd *pwd = getpwuid(getuid());
    return pwd->pw_dir;
}

void _concatenate(Concatenate_Buffer *buffer, char *str)
{
    int i = 0;
    while (str[i] != '\0')
    {
        if (buffer->pointer == buffer->capacity - 1)
        {
            buffer->capacity *= 2;
            buffer->buffer = (char *)xrealloc(buffer->buffer,
                                             buffer->capacity * sizeof(char));
        }
        buffer->buffer[buffer->pointer++] = str[i++];
    }
    buffer->buffer[buffer->pointer] = '\0';
}

char *str_constructor(char *args[])
{
    Concatenate_Buffer buffer = create_buffer();
    int i = 0;
    while (args[i])
    {
        _concatenate(&buffer, args[i++]);
    }
    return buffer.buffer;
}

char* concatenate(char* s1,char* s2)
{
    char* args[]={s1,s2,NULL};
    char* result=str_constructor(args);
    return result;
}
