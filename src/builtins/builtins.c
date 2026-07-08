#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtins/builtins.h"
#include "builtins/llm.h"
#include "builtins/trash.h"
#include "utils/allocaters.h"
#include "utils/string_constructor.h"

int check_operand(char **args)
{
    if (args == NULL)
    {
        return 0;
    }

    size_t args_length = 0;

    for (int i = 0; args[i] != NULL; i++)
    {
        args_length++;
    }

    if (args_length <= 1)
        return 0;

    return 1;
}

int is_valid_option(char **valid_options, char *option)
{
    for (int i = 0; valid_options[i] != NULL; i++)
    {
        if (strcmp(option, valid_options[i])==0)
        {
            return 1;
        }
    }
    return 0;
}

int is_option(char *option)
{
    if (!strncmp("--", option, 2))
    {
        return 1;
    }
    return 0;
}

int has_option(Option **options, char *option_name)
{
    for (int i = 0; options[i] != NULL; i++)
    {
        if (!strcmp(option_name, options[i]->name))
        {
            return 1;
        }
    }

    return 0;
}

void free_options(Option **options)
{
    for (int i = 0; options[i] != NULL; i++)
    {
        if (options[i]->type == BOOLEAN)
        {
            free(options[i]->name);
        }
        else if (options[i]->type == STRING)
        {
            free(options[i]->name);
            free(options[i]->value);
        }
        free(options[i]);
    }
    free(options);
}

Option **parse_options(char **args, char **valid_options,char** illegal_option)
{
    int parsed_options_top = 0,parsed_options_capacity=INTIAL_OPTIONS_LENGTH;
    Option **parsed_options = (Option **)malloc(parsed_options_capacity * sizeof(Option *));
    parsed_options[parsed_options_top] = NULL;
    for (int i = 1; args[i] != NULL; i++)
    {
        if (!is_option(args[i]))
        {
            continue;
        }

        char *eq_ptr = strchr(args[i], '=');

        char *option_name =
            eq_ptr ? strndup(args[i], eq_ptr - args[i]) : strdup(args[i]);

        if (!is_valid_option(valid_options, option_name))
        {
            *illegal_option=option_name;
            free_options(parsed_options);
            return NULL;
        }

        if (has_option(parsed_options, option_name))
        {
            free(option_name);
            continue;
        }

        Option *option_entry = (Option *)malloc(sizeof(Option));
        option_entry->name = option_name;

        if (eq_ptr)
        {
            option_entry->value = strdup(eq_ptr + 1);
            option_entry->type = STRING;
        }
        else
        {
            option_entry->value = OPTION_TRUE;
            option_entry->type = BOOLEAN;
        }

        if(parsed_options_top>=parsed_options_capacity-1)
        {
            parsed_options_capacity*=2;
            parsed_options=(Option**)xrealloc(parsed_options,parsed_options_capacity*sizeof(Option*));
        }
        parsed_options[parsed_options_top++] = option_entry;
        parsed_options[parsed_options_top] = NULL;
    }

    return parsed_options;
}

void* get_option(Option **options, char *option_name)
{
    for (int i = 0; options[i] != NULL; i++)
    {
        if (strcmp(options[i]->name, option_name) == 0)
        {
            return options[i];
        }
    }
    return OPTION_NOT_FOUND;
}

OptionValueType get_option_type(void* option)
{
    return ((Option*)option)->type;
}

void* get_option_value(void* option)
{
    return ((Option*)option)->value;
}

char *get_positional_argument(char **args)
{
    if (args == NULL)
        return NULL;

    for (int i = 1; args[i] != NULL; i++)
    {
        if (strncmp(args[i], "--", 2) != 0)
            return args[i];
    }

    return NULL;
}

char *builtin_cmds[] = {"cd", "exit", "help", "pwd", "trash", "nix"};

int (*builtin_functions[])(char **) = {&nsh_cd,  &nsh_exit,  &nsh_help,
                                       &nsh_pwd, &nsh_trash, &nsh_nix};

size_t nsh_builtins_num() { return sizeof(builtin_cmds) / sizeof(char *); }

char *old_pwd = NULL;

int nsh_pwd(char **args)
{
    char *pwd = getcwd(NULL, 0);
    printf("%s\n", pwd);
    free(pwd);
    return BUILTIN_OK;
}

void set_old_pwd()
{
    char *new_pwd = getcwd(NULL, 0);

    if (new_pwd != NULL)
    {
        free(old_pwd);
        old_pwd = new_pwd;
    }
}

int nsh_cd(char **args)
{
    char *argument = get_positional_argument(args);
    if (argument == NULL)
    {
        char *home = get_home_dir();
        set_old_pwd();
        if (chdir(home) != 0)
            perror("nsh");
        return BUILTIN_OK;
    }
    else if (strcmp(argument, "-") == 0)
    {
        if (old_pwd == NULL)
            fprintf(stderr, "cd: No previous directory.\n");
        else
        {
            printf("%s\n", old_pwd);
            if (chdir(old_pwd) != 0)
                perror("nsh");
        }
        return BUILTIN_OK;
    }
    if (argument[0] == '~')
    {
        char *home = get_home_dir();

        char *resolved = concatenate(home, args[1] + 1);

        set_old_pwd();

        if (chdir(resolved) != 0)
            perror("cd");

        free(resolved);

        return BUILTIN_OK;
    }
    set_old_pwd();

    if (chdir(argument) != 0)
        perror("cd");

    return BUILTIN_OK;
}

void free_old_pwd() { free(old_pwd); }

int nsh_exit(char **args) { return EXIT_SHELL; }

int nsh_trash(char **args)
{
    if (check_operand(args) == 0)
    {
        fprintf(stderr, "trash: no operand provided\n");
        return BUILTIN_OK;
    }

    char *valid_options[] = {"--list", "--restore", "--purge", NULL};

    char* illegal_option=NULL;

    Option **options = parse_options(args, valid_options,&illegal_option);

    if(options==NULL)
    {
        fprintf(stderr,"trash: illegal option %s\n",illegal_option);
        free(illegal_option);
        return BUILTIN_OK;
    }

    void* purge_option=get_option(options,"--purge");
    void* list_option=get_option(options,"--list");
    void* restore_option=get_option(options,"--restore");


    if (purge_option != OPTION_NOT_FOUND)
    {
        if(get_option_type(purge_option)!=BOOLEAN)
        {
            fprintf(stderr,"trash: option --purge cannot have a value\n");
        }
        else
            purge();
    }
    else if (list_option != OPTION_NOT_FOUND)
    {
        if(get_option_type(list_option)!=BOOLEAN)
        {
            fprintf(stderr,"trash: option --list cannot have a value\n");
        }   
        else
            list();
    }
    else if (restore_option != OPTION_NOT_FOUND)
    {
        if(get_option_type(restore_option)!=BOOLEAN)
        {
            fprintf(stderr,"trash: option --restore cannot have a value\n");
        }   
        else
        {
            char *argument = get_positional_argument(args);
            restore(argument);
        }
    }
    else
    {
        char *argument = get_positional_argument(args);
        delete (argument);
    }

    free_options(options);
    return BUILTIN_OK;
}

int nsh_nix(char **args)
{
    if (check_operand(args) == 0)
    {
        fprintf(stderr, "nix: no operand provided\n");
        return BUILTIN_OK;
    }

    char *valid_options[] = {"--model", "--format", "--len", NULL};

    char* illegal_option=NULL;

    Option **options = parse_options(args, valid_options,&illegal_option);

    if(options==NULL)
    {
        fprintf(stderr, "nix: illegal option %s\n",illegal_option);
        free(illegal_option);
        return BUILTIN_OK;
    }

    config_apply_options(options);

    char *argument = get_positional_argument(args);
    if (!argument)
    {
        fprintf(stderr, "nix: no prompt provided\n");
    }
    else
    {
        chat(argument);
    }
    free_options(options);
    return BUILTIN_OK;
}

int nsh_help(char **args)
{

    printf("This is a nano shell, developed for learning purposes.\n");
    printf("It supports the following builtins:\n");
    for (size_t i = 0; i < nsh_builtins_num(); i++)
    {
        printf("%s\n", builtin_cmds[i]);
    }

    return BUILTIN_OK;
}
