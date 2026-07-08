#ifndef LOOP_H
#define LOOP_H
#include "core/parser.h"
#define MAGENTA "\033[35m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"
#define RESET "\033[0m"
extern InputState input_state;
extern InputState *input_state_ptr;
void nsh_loop();
#endif
