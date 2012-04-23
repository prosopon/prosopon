#ifndef prosopon_interpreter_pro_interpreter_state_h
#define prosopon_interpreter_pro_interpreter_state_h

#include "prosopon_interpreter.h"

struct pro_interpreter_state
{
    pro_alloc* alloc;
    pro_state_ref state;
    struct 
    {
        int debug;
        int yacc_debug;
        int flex_debug;
    } flags;
};

#endif
