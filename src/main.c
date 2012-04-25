#include "prosopon.h"
#include "prosopon_interpreter.h"

#include "prosopon_interpreter_config.h"

#include "pro_alloc.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


typedef enum
{
    CL_STATE_FLAG_NONE = 0,
    LOAD_LIBRARY_CL_STATE_FLAG
} cl_state_flag;


typedef struct file_list file_list;
struct file_list
{
    file_list* next;
    const char* file;
};

static file_list* file_list_new(pro_interpreter_state* s,
    const char* file, file_list* next)
{
    pro_alloc* alloc = pro_interpreter_state_get_alloc(s);

    file_list* l = alloc(0, sizeof(*l));
    if (!l) return 0;
    
    l->file = file;
    l->next = next;
    return l;
}


typedef struct cl_state cl_state;
struct cl_state
{
    int load_standard_library;
    file_list* libraries;
    
    file_list* files;

    cl_state_flag flag;
};


/**
 *
 */
static int process_flag(cl_state* cl, pro_interpreter_state* istate, const char* flag, size_t len)
{
    if (len == 0)
        return -1;
    else if (len == 1)
    {
        switch (flag[0])
        {
        case 'l':
            cl->flag = LOAD_LIBRARY_CL_STATE_FLAG;
            return 0;
        default: return -1;
        }
    }
    else
    {
        if (strcmp(flag, "yydebug") == 0)
        {
            pro_interpreter_state_set_yacc_debug(istate, 1);
            return 0;
        }
        else if (strcmp(flag, "yy_flex_debug") == 0)
        {
            pro_interpreter_state_set_flex_debug(istate, 1);
            return 0;
        }
        else if (strcmp(flag, "no_std") == 0)
        {
            cl->load_standard_library = 0;
            return 0;
        }
        return -1;
    }
    
    return 0;
}

/**
 * Processes a single command line argument.
 * 
 * Arguments beginning with '-' are processed as flags.
 * Everything else is processed depending on the current state. 
 */
static int process_args(cl_state* cl, pro_interpreter_state* state, const char* arg)
{
    size_t len = strlen(arg);
    if (len == 0)
        return 0;
        
    switch (arg[0])
    {
    case '-':
        return process_flag(cl, state, arg + 1, len - 1);
    default:
        switch (cl->flag)
        {
        case CL_STATE_FLAG_NONE:
        {
            file_list* t = file_list_new(state, arg, 0);
            if (!cl->files)
                cl->files = t;
            else
            {
                file_list* last = cl->files;
                while (last->next)
                    last = last->next;
                last->next = t;
            }
            return 0;
        }   break;
        case LOAD_LIBRARY_CL_STATE_FLAG:
            cl->flag = CL_STATE_FLAG_NONE;
            return pro_process_library(state, arg);
        default: return -1;
        }
    }
}


/**
 * Main function to invoke the interpreter. The command takes the form
 * 
 * prosopon [-yydebug] [-yy_flex_debug] [-l PROSOPON_LIBRARY]* [FILE...]
 *
 * OPTIONS
 * -yydebug
 *     Enables yydebug for bison.
 * -yy_flex_debug
 *     Enables yy_flex_debug for flex.
 * -l PROSOPON_LIBRARY
 *     Load a Prosopon library.
 */
int main(int argc, char** argv)
{
    // create pro_state for execution
    pro_state_ref state;
    if (pro_state_create(simple_alloc, &state) != PRO_OK)
        return 0;
    
    // create interpreter state
    pro_interpreter_state* istate = pro_interpreter_state_create(state);
    
    cl_state cl = {
        .load_standard_library = 1
    };
 
    // set default options
    pro_interpreter_state_set_debug(istate, 0);
    pro_interpreter_state_set_yacc_debug(istate, 0);
    pro_interpreter_state_set_flex_debug(istate, 0);
    
    // process arguments
    for (unsigned int i = 1; i < argc; ++i)
    {
        int status = process_args(&cl, istate, argv[i]);
        if (status != 0)
            return -1;
    }
    
    // load standard library
    if (cl.load_standard_library)
    {
        if (pro_process_library(istate, PRO_STDLIB_PATH))
            return 0;
        if (load_stdlib_library(istate, PRO_STDLIB_PRO_PATH))
            return 0;
    }
    
    // load libraries
    file_list* libraries = cl.libraries;
    for (const char* library = 0; library && (library = libraries->file); )
    {
        file_list* next = libraries->next;
        int status = pro_process_library(istate, library);
        simple_alloc(libraries, 0);
        if (status != 0)
            return -1;
        
        libraries = next;
    }
    
    // process files
    file_list* files = cl.files;
    for (const char* file = 0; files && (file = files->file); )
    {
        file_list* next = files->next;
        int status = pro_eval(istate, file);
        simple_alloc(files, 0);
        if (status != 0)
            return -1;
            
        files = next;
    }
    
    pro_interpreter_state_release(istate);
    
    return 0;
}


