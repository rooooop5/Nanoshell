#ifndef OLLAMA_CLIENT_H
#define OLLAMA_CLIENT_H
#include <curl/curl.h>
#include <stdlib.h>
typedef enum
{
    GET,
    POST,
    PUT,
    DELETE
} Method;

typedef struct
{
    char *url;
    Method method;
    char **headers;
    char *body;
    char **params;
    char **behaviour;

} Request;

struct curl_slist *construct_header(char *headers[]);
Request init_request(char *url, char *headers[], Method method, char *body);
char *get(Request *request);
char *post(Request *request);
char *send_request(Request *request);
#endif
