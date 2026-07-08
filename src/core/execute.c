#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/execute.h"
#include "builtins/builtins.h"

int nsh_execute_builtins(char **args)
{
    for (size_t i = 0; i < nsh_builtins_num(); i++)
    {
        if (strcmp(args[0], builtin_cmds[i]) == 0)
            return (*builtin_functions[i])(args);
    }
    return BUILTIN_NOT_FOUND;
}

int nsh_execute_binaries(char **args)
{
    pid_t pid;
    int status;

    pid = fork();

    if (pid == 0)
    {
        setpgid(0, 0);
        signal(SIGINT, SIG_DFL);

        if (execvp(args[0], args) == -1)
        {
            perror("nsh");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        setpgid(pid, pid);
        tcsetpgrp(STDIN_FILENO, pid);


        do
        {
            waitpid(pid, &status, WUNTRACED);

        } while (!WIFEXITED(status) && !WIFSIGNALED(status));

        tcsetpgrp(STDIN_FILENO, getpid());
        
        if (WIFSIGNALED(status))
            printf("Process killed: %d\n", pid);
    }
    else
    {
        perror("nsh");
    }
    fflush(stderr);
    return 1;
}

int nsh_execute(char **args)
{
    if (args == NULL)
        return 1;
    if (args[0] == NULL)
        return 1;
    int builtin_status = nsh_execute_builtins(args);
    if (builtin_status == EXIT_SHELL)
        return 0;
    else if (builtin_status == BUILTIN_OK)
        return 1;
    else
        return nsh_execute_binaries(args);
}
