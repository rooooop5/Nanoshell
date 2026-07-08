#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/loop.h"
#include "core/parser.h"
#include "utils/allocaters.h"

char *trim_trailing_whitespace(char *str)
{
    char *trimmed;
    int str_len = strlen(str);
    int end = str_len - 1;
    while (isspace(str[end]) && end >= 0)
    {
        end--;
    }
    trimmed = strndup(str, end + 1);
    return trimmed;
}

char *nsh_read_input(void)
{
    ssize_t read_return;
    char *buffer;
    int buffer_capacity = BUFFER_SIZE, buffer_pointer = 0;
    buffer = (char *)xcalloc(buffer_capacity, sizeof(char)); // buffer is allocated
    while (1)
    {
        if (buffer_pointer == buffer_capacity - 1)
        {
            buffer_capacity *= 2;
            buffer = (char *)xrealloc(buffer, buffer_capacity * sizeof(char));
        }
        read_return = read(STDIN_FILENO, buffer + buffer_pointer, 1);
        if (!read_return)
        {
            // early return, need to free buffer
            (*input_state_ptr) = INPUT_TERMINATED;
            free(buffer); 
            return NULL;
        }
        else if (read_return == -1 && errno == EINTR)
        {
            // early return, need to free buffer
            (*input_state_ptr) = INPUT_INTERRRUPTED;
            free(buffer);
            return NULL;
        }
        else
        {
            if (buffer[buffer_pointer] == '\n')
            {
                if (buffer_pointer == 0)
                {
                    // need to free buffer
                    (*input_state_ptr) = INPUT_EMPTY;
                    free(buffer);
                    return NULL;
                }
                else
                {
                    (*input_state_ptr) = INPUT_OK;
                    buffer[buffer_pointer] = '\0';
                    char *trimmed = trim_trailing_whitespace(buffer); // this needs to freed by caller func
                    free(buffer); // final free, buffer is freed
                    return trimmed;
                }
            }
        }
        buffer_pointer++;
    }
}

char **nsh_parse_args(char *buffer, char separator)
{

    char **args;
    char *temp;

    int temp_capacity = WORD_SIZE, args_capacity = BUFFER_SIZE;

    ParsingState state = NORMAL;

    temp = (char *)xcalloc(temp_capacity, sizeof(char)); // owned by this function
    args = (char **)xcalloc(args_capacity, sizeof(char *)); // owned by the caller

    int temp_pointer = 0, buffer_pointer = 0, args_pointer = 0;

    while (1)
    {
        if (buffer[buffer_pointer] == '"')
        {
            if (state != IN_SINGLE_QUOTES)
            {
                if (state == IN_DOUBLE_QUOTES)
                    state = NORMAL;
                else
                    state = IN_DOUBLE_QUOTES;

                buffer_pointer++;
                continue;
            }
        }
        if (buffer[buffer_pointer] == '\'')
        {
            if (state != IN_DOUBLE_QUOTES)
            {
                if (state == IN_SINGLE_QUOTES)
                    state = NORMAL;
                else
                    state = IN_SINGLE_QUOTES;

                buffer_pointer++;
                continue;
            }
        }
        if (buffer[buffer_pointer] == separator && state == NORMAL)
        {
            if (temp_pointer == 0)
            {
                buffer_pointer++;
                continue;
            }
            temp[temp_pointer++] = '\0';
            args[args_pointer] = (char *)xcalloc(temp_pointer, sizeof(char));
            strcpy(args[args_pointer], temp);
            free(temp);
            temp_pointer = 0;
            temp_capacity = WORD_SIZE;
            args_pointer++;
            temp = (char *)xcalloc(temp_capacity, sizeof(char));
            buffer_pointer++;
            continue;
        }
        if (buffer[buffer_pointer] == '\0')
        {
            if (state != NORMAL)
            {
                fprintf(stderr, "error: quote unclosed\n");
                free(temp);
                for(int i=0;i<args_pointer;i++)
                {
                    free(args[i]);
                }
                free(args);
                return NULL;
            }
            temp[temp_pointer++] = '\0';
            args[args_pointer] = (char *)xcalloc(temp_pointer, sizeof(char));
            strcpy(args[args_pointer], temp);
            free(temp);
            args_pointer++;
            break;
        }
        if (temp_pointer == temp_capacity - 1)
        {
            temp_capacity *= 2;
            temp = (char *)xrealloc(temp, temp_capacity * sizeof(char));
        }
        temp[temp_pointer++] = buffer[buffer_pointer++];
    }
    args[args_pointer] = NULL;
    return args;
}
