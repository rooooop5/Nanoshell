#ifndef LLM_H
#define LLM_H

#include "builtins/builtins.h"
#include "cJSON/cJSON.h"
#include <stdio.h>

#define MESSAGES_CAPACITY 100

typedef enum
{
    SYSTEM,
    USER,
    ASISTANT
} Role;

typedef struct
{
    Role role;
    char *content;
} Message;

typedef struct
{
    Message *messages;
    size_t capacity;
    size_t top;
} ChatHistory;

typedef struct
{
    char *model;
    char *len;
    char *format;
} Config;

void config_apply_options(Option **options);

void chat(char *prompt);

void nix_startup();

void nix_shutdown();

void chat(char *prompt);

void append_message(Role role, char *content);

cJSON *construct_ollama_request(char *prompt);

#endif
