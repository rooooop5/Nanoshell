#ifndef BUILTINS_H
#define BUILTINS_H
#include <stdio.h>
#include <stdint.h>

void free_old_pwd();

#define OPTION_NOT_FOUND ((void *)-1)

#define OPTION_TRUE ((void*)(intptr_t)1)

#define OPTION_FALSE ((void*)(intptr_t)0)

#define OPTION_AS_BOOL(v) ((int)(intptr_t)(v))
typedef enum
{
    BOOLEAN,
    STRING
} OptionValueType;

typedef struct
{
    char *name;
    void *value;
    OptionValueType type;

} Option;

#define INTIAL_OPTIONS_LENGTH 10

void *get_option(Option **options, char *option_name);

OptionValueType get_option_type(void* option);

void* get_option_value(void* option);

int nsh_cd(char **args);
int nsh_exit(char **args);
int nsh_help(char **args);
int nsh_pwd(char **args);
int nsh_trash(char **args);
int nsh_nix(char **args);
typedef enum BuiltinExitStatus
{
    EXIT_SHELL,
    BUILTIN_OK,
    BUILTIN_NOT_FOUND,
} BuiltinExitStatus;
extern char *builtin_cmds[];
extern int (*builtin_functions[])(char **);
size_t nsh_builtins_num();
#endif
