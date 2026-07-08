#ifndef PARSER_H
#define PARSER_H

typedef enum InputState
{
    INPUT_OK,
    INPUT_INTERRRUPTED,
    INPUT_TERMINATED,
    INPUT_EMPTY
} InputState;
#define BUFFER_SIZE 1024
#define WORD_SIZE 50

typedef enum ParsingState
{
    IN_DOUBLE_QUOTES,
    IN_SINGLE_QUOTES,
    NORMAL
} ParsingState;

char *nsh_read_input();

char **nsh_parse_args(char *buffer,char separator);

int nsh_execute(char **args);

#endif
