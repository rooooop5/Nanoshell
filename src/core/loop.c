#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/loop.h"
#include "core/parser.h"

InputState input_state = INPUT_OK;
InputState *input_state_ptr = &input_state;

void ignore_signal()
{
    write(STDIN_FILENO, "\n", 1);
    return;
}

char *nsh_prompt_pwd()
{
    struct passwd *pw = getpwuid(getuid());
    char *pwd = getcwd(NULL, 0);
    char *base;
    if (strcmp(pwd, pw->pw_dir) == 0)
    {
        free(pwd);
        base = strdup("/home");
        return base;
    }
    char *internal_pointer_in_pwd = strrchr(pwd, '/');
    base = strdup(internal_pointer_in_pwd);
    free(pwd);
    return base;
}

void nsh_loop()
{
    char **args;
    int status = 1;
    struct sigaction sa;
    sa.sa_handler = ignore_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    pid_t shell_pgid = getpid();
    setpgid(shell_pgid, shell_pgid);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    do
    {
        tcsetpgrp(STDIN_FILENO, shell_pgid);
        char *base = nsh_prompt_pwd();
        printf(BLUE "[" MAGENTA "nsh" BLUE "]" CYAN " %s " RESET, base + 1);
        fflush(stdout);
        char *buffer = nsh_read_input();
        if (input_state == INPUT_TERMINATED)
        {
            free(base);
            free(buffer);
            break;
        }
        else if (input_state == INPUT_EMPTY)
        {
            free(base); 
            free(buffer); 
            continue;
        }
        else if (input_state == INPUT_INTERRRUPTED)
        {

            free(base);
            free(buffer);
            continue;
        }
        else
        {
            args = nsh_parse_args(buffer, ' '); 
            if(args==NULL)
            {
                free(base);
                free(buffer);
                //count++;
                continue;
            }
            status = nsh_execute(args); 
            for (int i = 0; args[i] != NULL; i++)
            {
                free(args[i]);
            }
            free(buffer); 
            free(args); 
            free(base); 
        }
    } while (status);
}
