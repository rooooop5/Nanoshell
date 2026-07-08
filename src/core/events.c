#include "builtins/builtins.h"
#include "builtins/llm.h"
#include "builtins/trash.h"

void startup()
{
    nix_startup();
    trash_startup();
}

void shutdown()
{
    nix_shutdown();
    trash_cleanup();
    free_old_pwd();
}