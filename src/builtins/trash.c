
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "builtins/trash.h"
#include "utils/allocaters.h"
#include "utils/string_constructor.h"
#include "utils/time.h"

TrashLog *trash_log;

char *trash_dir;

LogIndex idx_log[LOG_LENGTH];

int log_top = -1;

int check_last_idx(char *key)
{
    for (int i = 0; i <= log_top; i++)
    {
        if (strcmp(idx_log[i].key, key) == 0)
        {
            return idx_log[i].idx;
        }
    }
    return -1;
}

void parse_info(char *line)
{
    char *p1 = strchr(line, '|');
    char *p2 = strchr(p1 + 1, '|');
    char *p3 = strchr(p2 + 1, '|');
    char *p4 = strchr(p3 + 1, '|');
    char *is_active_char = p4 + 1;

    char *trash_path = strndup(p2 + 1, p3 - p2 - 1);

    int last_idx = check_last_idx(trash_path);

    if (last_idx != -1)
    {
        trash_log->log[last_idx].is_active = *is_active_char - '0';
        free(trash_path);
        return;
    }

    TrashObj obj;

    obj.org_name = strndup(p1 + 1, p2 - p1 - 1);
    obj.org_path = strndup(p2 + 1, p3 - p2 - 1);
    obj.trash_path = trash_path;
    obj.timestamp = strndup(p3 + 1, p4 - p3 - 1);
    obj.is_active = *is_active_char - '0';

    trash_log->log[++trash_log->top] = obj;

    LogIndex log_idx;

    log_idx.key = obj.trash_path;
    log_idx.idx = trash_log->top;
    idx_log[++log_top] = log_idx;
}

void reconstruct_log()
{
    log_top = -1;

    trash_log = (TrashLog *)malloc(sizeof(TrashLog));
    trash_log->capacity = LOG_LENGTH;
    trash_log->top = -1;
    trash_log->log = malloc(trash_log->capacity * sizeof(TrashObj));

    char *home = get_home_dir();
    char *log_path_args[] = {home, "/.deleted/.log.txt", NULL};
    char *log_path = str_constructor(log_path_args);

    FILE *fp = fopen(log_path, "r");

    free(log_path);

    if (!fp)
        return;

    char *line = NULL;
    size_t capacity = 0;

    while (getline(&line, &capacity, fp) != -1)
    {
        parse_info(line);
    }

    free(line);

    fclose(fp);
}

TrashObj create_trash_obj(char *filename)
{
    TrashObj element;

    char *time_str = time_string();
    char *pwd = getcwd(NULL, 0);    

    element.org_name = strdup(filename);
    element.timestamp = timestamp();

    char *trash_path_args[] = {trash_dir, time_str, "_", element.org_name,
                               NULL};
    element.trash_path = str_constructor(trash_path_args);

    char *org_path[] = {pwd, "/", element.org_name, NULL};
    element.org_path = str_constructor(org_path);

    element.is_active = 1;

    free(time_str);
    free(pwd);

    return element;
}

void build_trash_dir_path(void)
{
    char *home = get_home_dir();
    char *trash_dir_args[] = {home, TRASH_DIR, "/", NULL};
    trash_dir = str_constructor(trash_dir_args);
}

void create_trash_dir()
{
    if(mkdir(trash_dir,0755)==-1)
    {
        if(errno!=EEXIST)
        {
            perror("trash");
        }
    }
}

void trash_startup()
{
    build_trash_dir_path();
    create_trash_dir();
    reconstruct_log();
    
}

void free_trash_log()
{
    for (int i = 0; i <= trash_log->top; i++)
    {
        free(trash_log->log[i].trash_path);
        free(trash_log->log[i].org_path);
        free(trash_log->log[i].org_name);
        free(trash_log->log[i].timestamp);
    }

    free(trash_log->log);

    free(trash_log);
}

void free_trash_dir() { free(trash_dir); }

void trash_cleanup()
{
    free_trash_log();
    free_trash_dir();
}

void delete(char *filename)
{
    if (trash_log->top >= trash_log->capacity - 1)
    {
        fprintf(stderr,"trash: trash is full, purge trash before deleting more\n");
        fprintf(stderr,"run trash --purge to empty trash\n");
        return;
    }

    char *home = get_home_dir();

    char *log_path_args[] = {home, "/.deleted/.log.txt", NULL};
    char *log_path = str_constructor(log_path_args);

    FILE *fp = fopen(log_path, "a");

    free(log_path);

    struct stat st;

    if (lstat(filename, &st) != 0)
    {
        perror("trash");
        fclose(fp);
        return;
    }

    if (S_ISDIR(st.st_mode))
    {
        int count = 0;

        DIR *directory = opendir(filename);

        if (directory == NULL)
        {
            perror("trash");
        }

        struct dirent *entry;

        while (1)
        {
            entry = readdir(directory);

            if (entry == NULL)
                break;

            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
                continue;

            count++;
        }

        if (count > 0)
        {
            fprintf(stderr,"trash: directory not empty\n");

            closedir(directory);

            fclose(fp);

            return;
        }
        
        closedir(directory);
    }

    TrashObj element = create_trash_obj(filename);

    int value = rename(element.org_path, element.trash_path);

    if (!value)
    {

        fprintf(fp, "%s|%s|%s|%s|%d\n", element.org_name, element.org_path,
                element.trash_path, element.timestamp, element.is_active);

        trash_log->log[++trash_log->top] = element;
    }
    else
    {
        free(element.org_name);
        free(element.org_path);
        free(element.timestamp);
        free(element.trash_path);

        perror("trash");
    }

    fclose(fp);
}

void restore_by_filename(char *filename, FILE *fp)
{
    int capacity = INITIAL_MATCHES_LENGTH, matches_top = 0;

    int *matches = (int *)xmalloc(capacity * sizeof(int));

    for (int i = trash_log->top; i >= 0; i--)
    {
        TrashObj log_obj = trash_log->log[i];

        if (strcmp(filename, log_obj.org_name) == 0 && log_obj.is_active)
        {
            if (matches_top >= capacity - 1)
            {
                capacity *= 2;
                matches = (int *)realloc(matches, capacity*sizeof(int));
            }

            matches[matches_top++] = i;
        }
    }

    if (matches_top == 0)
    {
        fprintf(stderr,"trash: no matches with the file name have been found\n");
        free(matches);
        return;
    }

    if (matches_top == 1)
    {
        TrashObj log_entry = trash_log->log[matches[0]];
        int value = rename(log_entry.trash_path, log_entry.org_path);
        if (!value)
        {
            trash_log->log[matches[0]].is_active = 0;

            fprintf(fp, "%s|%s|%s|%s|%d\n", log_entry.org_name,
                    log_entry.org_path, log_entry.trash_path,
                    log_entry.timestamp, 0);
        }
        else
        {
            perror("trash");
        }
    }
    else
    {
        for (int i = 0; i < matches_top; i++)
            printf("[%d] %s (deleted at %s from %s)\n", i,
                   trash_log->log[matches[i]].org_name,
                   trash_log->log[matches[i]].timestamp,
                   trash_log->log[matches[i]].org_path);

        int match_id;

        printf("Enter the id to delete - ");
        scanf("%d", &match_id);

        TrashObj log_entry = trash_log->log[matches[match_id]];

        int value = rename(log_entry.trash_path, log_entry.org_path);

        if (!value)
        {
            trash_log->log[matches[match_id]].is_active = 0;

            fprintf(fp, "%s|%s|%s|%s|%d\n", log_entry.org_name,
                    log_entry.org_path, log_entry.trash_path,
                    log_entry.timestamp, 0);
        }
        else
        {
            perror("trash");
        }
    }

    free(matches);
}

void restore(char *filename)
{
    if (trash_log->top == -1)
    {
        fprintf(stderr,"trash: trash is empty\n");
        return;
    }

    char *home = get_home_dir();

    char *log_path_args[] = {home, "/.deleted/.log.txt", NULL};
    char *log_path = str_constructor(log_path_args);

    FILE *fp = fopen(log_path, "a");

    free(log_path);

    if (filename == NULL)
    {
        int idx = -1;

        for (int i = trash_log->top; i >= 0; i--)
        {
            if (trash_log->log[i].is_active)
            {
                idx = i;
                break;
            }
        }

        if (idx == -1)
        {
            fprintf(stderr,"trash: trash is empty\n");
            fclose(fp);
            return;
        }

        TrashObj log_entry = trash_log->log[idx];

        int value = rename(log_entry.trash_path, log_entry.org_path);

        if (!value)
        {
            trash_log->log[idx].is_active = 0;

            fprintf(fp, "%s|%s|%s|%s|%d\n", log_entry.org_name,
                    log_entry.org_path, log_entry.trash_path,
                    log_entry.timestamp, 0);
        }
        else
        {
            perror("trash");
        }

        fclose(fp);

        return;
    }

    restore_by_filename(filename, fp);

    fclose(fp);
}

void purge()
{

    DIR *deleted = opendir(trash_dir);

    if (deleted == NULL)
    {
        perror("trash");
        return;
    }

    struct dirent *entry;

    while (1)
    {
        entry = readdir(deleted);

        if (!entry)
        {
            break;
        }

        if (strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0)
            continue;

        char *path_args[] = {trash_dir, entry->d_name, NULL};
        char *path = str_constructor(path_args);

        struct stat st;

        if (lstat(path, &st) != 0)
        {
            perror("trash");
            free(path);
            continue;
        }

        if (S_ISLNK(st.st_mode))
        {
            if (unlink(path) != 0)
            {
                perror("trash");
            }

            free(path);
            continue;
        }
        if (S_ISDIR(st.st_mode))
        {
            if (rmdir(path) != 0)
            {
                perror("trash");
            }

            free(path);
            continue;
        }
        else if (S_ISREG(st.st_mode))
        {
            if (remove(path) != 0)
            {
                perror("trash");
            }

            free(path);
            continue;
        }

        free(path);
    }

    char *log_path_args[] = {trash_dir, ".log.txt", NULL};
    char *log_path = str_constructor(log_path_args);

    free_trash_log();

    reconstruct_log();

    free(log_path);

    closedir(deleted);
}

void list()
{
    for (int i = 0; i <= trash_log->top; i++)
    {
        if (trash_log->log[i].is_active)
        {
            printf(" FILENAME: %s | DELETED AT: %s | ORIGINAL PATH: %s\n",
                   trash_log->log[i].org_name, trash_log->log[i].timestamp,
                   trash_log->log[i].org_path);
        }
    }
}