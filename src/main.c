#include "core/events.h"
#include "core/loop.h"
int main()
{
    startup();

    nsh_loop();
    shutdown();
    return 0;
}
