#include <curl/curl.h>
#include <string.h>

#include "cJSON/cJSON.h"

#include "utils/ollama_client.h"
#include "utils/allocaters.h"

typedef struct
{
    char* buffer;
    size_t size;
}Buffer;

char* ollama_response_parser(char* data)
{
    cJSON* response=cJSON_Parse(data);

    if(!response)
        return NULL;

    cJSON* message=cJSON_GetObjectItem(response,"message");

    if(!message)
    {
        cJSON_Delete(response);
        free(data);
        return NULL;
    }

    cJSON* content=cJSON_GetObjectItem(message,"content");
    if(!content)
    {
        cJSON_Delete(response);
        free(data);
        return NULL;
    }

    char* model_response=strdup(content->valuestring);

    cJSON_Delete(response);

    free(data);

    return model_response;
}

struct curl_slist* construct_header(char* headers[])
{
    struct curl_slist* curl_headers=NULL;
    int i=0;
    while(headers[i]!=NULL)
    {
        curl_headers=curl_slist_append(curl_headers,headers[i]);
        i++;
    }
    return curl_headers;
}


size_t write_callback(void* ptr,size_t size,size_t nmemb,void* data)
{
    size_t new_size=size*nmemb;

    Buffer* buffer=(Buffer*)data;

    buffer->buffer=xrealloc(buffer->buffer,buffer->size+new_size+1);

    memcpy(buffer->buffer+buffer->size,ptr,new_size);

    buffer->size+=new_size;
    buffer->buffer[buffer->size]='\0';

    return new_size;

}
char* get(Request* request)
{
    CURL* curl;
    curl=curl_easy_init();
    char* model_response=NULL;
    if(curl)
    {
        Buffer buff={0};
        struct curl_slist* headers=construct_header(request->headers);

        curl_easy_setopt(curl,CURLOPT_HTTPHEADER,headers);
        curl_easy_setopt(curl,CURLOPT_URL,request->url);
        curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,write_callback);
        curl_easy_setopt(curl,CURLOPT_WRITEDATA,&buff);

        CURLcode res=curl_easy_perform(curl);

        if(res!=CURLE_OK)
        {
            fprintf(stderr,"curl failed %s\n",curl_easy_strerror(res));
        }
        else
            model_response=ollama_response_parser(buff.buffer);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return model_response;
    }
    return NULL;

}

char* post(Request* request)
{
    CURL* curl;
    curl=curl_easy_init();
    char* model_response=NULL;
    if(curl)
    {
        Buffer buff={0};
        struct curl_slist* headers=construct_header(request->headers);
        curl_easy_setopt(curl,CURLOPT_URL,request->url);
        curl_easy_setopt(curl,CURLOPT_POST,1L);
        curl_easy_setopt(curl,CURLOPT_POSTFIELDS,request->body);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER,headers);
        curl_easy_setopt(curl,CURLOPT_WRITEDATA,&buff);
        curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,write_callback);
        CURLcode res=curl_easy_perform(curl);
        if(res!=CURLE_OK)
        {
            fprintf(stderr,"curl failed %s\n",curl_easy_strerror(res));
        }
        else
            model_response=ollama_response_parser(buff.buffer);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return model_response;
    }
    return NULL;
}

char* send_request(Request* request)
{
    char* response=NULL;
    if(request->method==GET)
    {
        response=get(request);
    }
    else if (request->method==POST)
    {
        response=post(request);
    }
    return response;
}

Request init_request(char* url,char* headers[],Method method,char* body)
{
    Request request;
    request.url=url;
    request.headers=headers;
    request.method=method;
    request.body=body;
    return request;
}
