#include "pro_case_expr.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>


static pro_ref case_expr_eval(pro_state_ref s, pro_expr* t)
{
    assert(0);
    return PRO_EMPTY_REF;
}


static void case_expr_print(pro_state_ref s, const pro_expr* t, const char* end)
{
    assert(pro_expr_get_type(t) == PRO_CASE_EXPR_TYPE);
    
    pro_expr* pattern = t->value.binary.left;
    pro_expr* body = t->value.binary.right;
    
    printf("<case pattern:");
    if (pattern)
        pro_print_expr(s, pattern, " body:");
    if (body)
        pro_print_expr(s, body, "");
    printf(">%s", end);
}


static void case_expr_release(pro_expr* t)
{
    pro_expr* pattern = t->value.binary.left;
    pro_expr* body = t->value.binary.right;
    if (pattern)
        pro_release_expr(pattern);
    if (body)
        pro_release_expr(body);
    free(t);
}


#pragma mark -
#pragma mark Internal

const pro_expr_type_info pro_case_expr_type_info = {
    .eval = case_expr_eval,
    .print = case_expr_print,
    .release = case_expr_release
};


PRO_INTERNAL pro_expr* pro_case_expr_create(pro_expr* pattern, pro_expr* body)
{
    pro_expr* t = pro_expr_create(PRO_CASE_EXPR_TYPE);
    if (!t) return 0;
    t->value.binary.left = pattern;
    t->value.binary.right = body;
    return t;
}


PRO_INTERNAL int pro_case_expr_match(pro_state_ref s,
    pro_expr* t, pro_ref msg)
{
    pro_env_ref current_env, env;
    pro_get_env(s, &current_env);
    pro_env_create(s, current_env, &env);
    pro_env_release(s, current_env);
    pro_push_env(s, env);
    pro_env_release(s, env);

    pro_expr* body = t->value.binary.right;
    pro_expr* pattern = t->value.binary.left;

    unsigned int msg_length;
    pro_message_length(s, msg, &msg_length);
    
    pro_expr_list* match_list = pattern->value.list;
    for (unsigned int index = 0; index < msg_length; )
    {
        pro_ref arg;
        pro_message_get(s, msg, index, &arg);
        pro_expr* match = match_list->value;
        
        switch (pro_expr_get_type(match))
        {
        case PRO_CAPTURE_IDENTIFIER_EXPR_TYPE:
            pro_bind(s, arg, match->value.identifier);
            break;
        default:
        {
            pro_matching do_match;
            pro_ref match_ref = pro_eval_expr(s, match);
            pro_match(s, match_ref, arg, &do_match);
            pro_release(s, match_ref);
            switch (do_match)
            {
            case PRO_MATCH_FAIL:
                pro_release(s, arg);
                pro_pop_env(s);
                return 0;
            case PRO_MATCH_CONTINUE:
                continue;
            }
        }   break;
        }
        
        ++index;
        pro_release(s, arg);
        
        if (!(match_list = match_list->next) && index != msg_length )
        {
            pro_pop_env(s);
            return 0;
        }
    }
    
    if (body)
    {
        pro_ref out = pro_eval_expr(s, body);
        pro_release(s, out);
    }
    
    pro_pop_env(s);
    
    return 1;
}


