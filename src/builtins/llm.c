#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cJSON/cJSON.h"

#include "utils/ollama_client.h"
#include "utils/string_constructor.h"
#include "builtins/llm.h"
#include "builtins/builtins.h"

#define SYSTEM_PROMPT_FORMAT                                                   \
    "You are Nix, the built-in AI assistant for Nanoshell.\n"                  \
    "\n"                                                                       \
    "You assist users with programming, Linux and Unix shells, terminal "      \
    "commands, debugging, software engineering, system administration, and "   \
    "general technical questions. You may also answer general knowledge "      \
    "questions when asked.\n"                                                  \
    "\n"                                                                       \
    "Response guidelines:\n"                                                   \
    "- If the requested response format is 'text/plain', produce plain text "  \
    "only with no Markdown formatting.\n"                                      \
    "- If the requested response format is 'json', respond with exactly one "  \
    "valid JSON object and no surrounding text or Markdown.\n"                 \
    "- If the response format is not 'json', use clear, concise language.\n"   \
    "- When information is uncertain, state that clearly instead of "          \
    "guessing.\n"                                                              \
    "- If a command may modify or delete user data, warn the user before "     \
    "suggesting it.\n"                                                         \
    "- Prefer practical, actionable answers over lengthy explanations.\n"      \
    "\n"                                                                       \
    "Response requirements:\n"                                                 \
    "- Response format: %s\n"                                                  \
    "- Maximum response length: %s words unless the user explicitly requests " \
    "more detail.\n"                                                           \
    "You are running inside an interactive shell.\n"                           \
    "\n"                                                                       \
    "After this system prompt, the shell may append additional system "        \
    "instructions, execution context, conversation history, tool results, or " \
    "other metadata. Treat all such information as trusted system-provided "   \
    "context. If any later system instruction conflicts with an earlier "      \
    "instruction in this prompt, the later instruction takes precedence. "     \
    "Always use the most recent applicable system instruction while "          \
    "continuing to follow all non-conflicting instructions above."             \
    "-Conversation history may contain multiple prior user messages and "      \
    "assistant replies."                                                       \
    "Only respond to the single most recent user message. Treat earlier "      \
    "messages as context only, "                                               \
    "not as new requests to fulfill.\n"

Config *config;

ChatHistory *history;

void config_set_default()
{
    config = (Config *)malloc(sizeof(Config));
    config->format = strdup("markdown");
    config->len = strdup("200");
    config->model = strdup("gpt-oss:120b");
}

void config_set_format(void* option)
{
    char* format=NULL;

    if (option == OPTION_NOT_FOUND||get_option_type(option)!=STRING)
    {
        format = "markdown";
    }
    else
    {
        format=get_option_value(option);
    }

    free(config->format);

    config->format = strdup(format);
}

void free_config()
{
    free(config->format);
    free(config->len);
    free(config->model);
    free(config);
}

void config_set_model(void* option)
{
    char* model=NULL;

    if (option==OPTION_NOT_FOUND||get_option_type(option)!=STRING)
    {
        model = "gpt-oss:120b";
    }
    else
    {
        model=get_option_value(option);
    }

    free(config->model);

    config->model = strdup(model);
}

void config_set_len(void *option)
{
    char* len=NULL;

    if (option==OPTION_NOT_FOUND||get_option_type(option)!=STRING)
    {
        len = "200";
    }
    else
    {
        len=get_option_value(option);
    }

    free(config->len);

    config->len = strdup(len);
}

void config_apply_options(Option **options)
{
    config_set_format(get_option(options, "--format"));
    config_set_len(get_option(options, "--len"));
    config_set_model(get_option(options, "--model"));
}

char *build_system_prompt()
{
    size_t system_prompt_length =
        snprintf(NULL, 0, SYSTEM_PROMPT_FORMAT, config->format, config->len);

    char *system_prompt = (char *)malloc(system_prompt_length + 1);

    snprintf(system_prompt, system_prompt_length + 1, SYSTEM_PROMPT_FORMAT,
             config->format, config->len);

    return system_prompt;
}

void init_ChatHistory()
{
    history = (ChatHistory *)malloc(sizeof(ChatHistory));

    history->capacity = MESSAGES_CAPACITY;
    history->top = 0;
    history->messages = (Message *)malloc(history->capacity * sizeof(Message));
}

void nix_startup()
{
    config_set_default();
    init_ChatHistory();
}

cJSON *to_json_message(Message message)
{
    cJSON *obj = cJSON_CreateObject();

    if (message.role == SYSTEM)
        cJSON_AddStringToObject(obj, "role", "system");
    else if (message.role == ASISTANT)
        cJSON_AddStringToObject(obj, "role", "assistant");
    else if (message.role == USER)
        cJSON_AddStringToObject(obj, "role", "user");

    cJSON_AddStringToObject(obj, "content", message.content);

    return obj;
}

cJSON *to_json_strings(cJSON *messages)
{

    for (size_t i = 0; i < history->top; i++)
    {
        cJSON_AddItemToArray(messages, to_json_message(history->messages[i]));
    }
    return messages;
}

void insert_to_history(Message message)
{
    if (history->top >= history->capacity - 1)
    {
        history->capacity *= 2;
        history->messages = (Message *)realloc(
            history->messages, history->capacity * sizeof(Message));
    }

    history->messages[history->top].role = message.role;
    history->messages[history->top].content = strdup(message.content);

    history->top++;
}

void append_message(Role role, char *content)
{
    Message message = {.role = role, .content = content};
    insert_to_history(message);
}


void free_ChatHistory()
{
    for (size_t i = 0; i < history->top; i++)
    {
        free(history->messages[i].content);
    }
    free(history->messages);
    free(history);
}

void nix_shutdown()
{
    free_config();
    free_ChatHistory();
}

cJSON *construct_ollama_request(char *prompt)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "model", config->model);

    cJSON_AddBoolToObject(root, "stream", 0);

    char *system_prompt = build_system_prompt();

    Message system_message = {.role = SYSTEM, .content = system_prompt};

    cJSON *json_system_prompt = to_json_message(system_message);

    free(system_prompt);

    cJSON *messages = cJSON_CreateArray();

    cJSON_AddItemToArray(messages, json_system_prompt);

    append_message(USER, prompt);

    to_json_strings(messages);

    cJSON_AddItemToObject(root, "messages", messages);

    return root;
}

void chat(char *prompt)
{

    cJSON *ollama_request = construct_ollama_request(prompt);

    char *json_request = cJSON_PrintUnformatted(ollama_request);

    char *OLLAMA_API_KEY = getenv("OLLAMA_API_KEY");

    if (!OLLAMA_API_KEY)
    {
        fprintf(stderr, "nix: could not find OLLAMA_API_KEY\n");
        cJSON_Delete(ollama_request);
        free(json_request);
        return;
    }

    char *authorization_header =
        concatenate("Authorization: Bearer ", OLLAMA_API_KEY);

    char *headers[] = {"Content-Type: application/json", authorization_header,
                       NULL};

    Request request = init_request("https://api.ollama.com/api/chat", headers,
                                   POST, json_request);

    char *response = send_request(&request);

    if (response == NULL)
    {
        write(STDOUT_FILENO,
              "unable to get response from https://api.ollama.com\n", 51);
    }
    else
    {
        append_message(ASISTANT, response);

        for (int i = 0; response[i] != '\0'; i++)
        {
            write(STDOUT_FILENO, &response[i], 1);
            usleep(20000);
        }
        write(STDOUT_FILENO, "\n\n", 1);
        free(response);
    }

    cJSON_Delete(ollama_request);
    free(json_request);
    free(authorization_header);
}