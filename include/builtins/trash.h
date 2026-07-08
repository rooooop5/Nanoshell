#ifndef TRASH_H
#define TRASH_H

#include <stdio.h>

#define LOG_LENGTH 100

#define INITIAL_MATCHES_LENGTH 2

#define MAX_LENGTH 1000

#define TRASH_DIR "/.deleted"

typedef struct TrashObj
{
    char *org_name;
    char *timestamp;
    char *trash_path;
    char *org_path;
    int is_active;
} TrashObj;

typedef struct TrashLog
{
    TrashObj *log;
    int top;
    int capacity;
} TrashLog;

typedef struct LogIndex
{
    char *key;
    int idx;
} LogIndex;

void trash_startup();

void trash_cleanup();

void delete(char *filename);

void restore(char *filename);

void purge();

void list();

#endif
