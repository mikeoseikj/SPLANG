#include <stdio.h>
#include <stdlib.h>
#include "include/globals.h"
#include "include/error.h"
#include "include/eval.h"
#include "include/utils.h"

static float parse_logical()
{
    float result = parse_bitwise();
    int operator = curr_token.id;

    while(operator == LAND || operator == LOR)
    {
        if(! valid_operator(operator))
            error(INVALID_EXPRESSION);
    
        get_token();
        float temp = parse_bitwise();
        switch(operator)
        {
            case LAND:
                result = (result && temp);
                break;
            case  OR:
                result = (result || temp);
        }
        if(curr_token.id == SEMICOLON || curr_token.id == END_OF_FILE)
            return result; 

        operator = curr_token.id;
    }
    return result;
}


static float parse_bitwise()
{ 
    float result = parse_equality(); 
    int operator = curr_token.id;

    while(operator == XOR || operator == AND || operator == OR)
    {
        if(! valid_operator(operator))
            error(INVALID_EXPRESSION);
  

        get_token();
        float temp = parse_equality();
        switch(operator)
        {
            case XOR:
                result = (int)result ^ (int)temp;
                break;
            case AND:
                result = (int)result & (int)temp;
                break;
            case OR:
                result = (int)result | (int)temp;
        }
       
        operator = curr_token.id;
        
    }
    return result;
}



static float parse_equality()
{
    float result = parse_compare();
    int operator = curr_token.id;

    while(operator == EQUAL || operator == NOT_EQUAL)
    {
        if(! valid_operator(operator))
            error(INVALID_EXPRESSION);
    
        get_token();
        float temp = parse_compare();
        switch(operator)
        {
            case EQUAL:
                result = (result == temp);
                break;
            case  NOT_EQUAL:
                result = (result != temp);
        }
        operator = curr_token.id;
    }
    return result;
}


static float parse_compare()
{
    float result = parse_term();
    int operator = curr_token.id;

    while(operator == GREATER || operator == LESS || operator == GREATER_EQUAL || operator == LESS_EQUAL)
    {
        if(! valid_operator(operator))
            error(INVALID_EXPRESSION);
   
        get_token();
        float temp = parse_term();
        switch(operator)
        {
            case GREATER:
                result = (result > temp);
                break;
            case  LESS:
                result = (result < temp);
                break;
            case GREATER_EQUAL:
                result = (result >= temp);
                break;
            case LESS_EQUAL:
                result = (result <= temp);
        }   
        operator = curr_token.id;
    }
    return result;
}


static float parse_term()
{ 
    float result = parse_factor(); 
    int operator = curr_token.id;

    while(operator == PLUS || operator == MINUS)
    {
        if(! valid_operator(operator))
            error(INVALID_EXPRESSION);
  
        get_token();
        float temp = parse_factor();
        switch(operator)
        {
            case PLUS:
                result = result + temp;
                break;
            case MINUS:
                result = result - temp;
        }
        operator = curr_token.id;        
    }
    return result;
    
}


static float parse_factor()
{
    float result = parse_unary();
    int operator = curr_token.id;

    while(operator == STAR || operator == SLASH || operator == MODULO)
    {
        if(! valid_operator(operator))
            error(INVALID_EXPRESSION);
  
        get_token();
        float temp = parse_unary();
        switch(operator)
        {
            case STAR:
                result = result * temp;
                break;
            case  SLASH:
                result = result / temp;
                break;
            case MODULO:
                result = (int)result % (int)temp;
        }
        operator = curr_token.id;
    }
    return result;
}



static float parse_unary()
{
    float result = 0;
    int operator = curr_token.id;
    if(operator == PLUS || operator == MINUS ||  operator == NOT || operator == FLIP)
    {
        get_token();
        result = parse_atom();

        switch(operator)
        {
            case MINUS:
                result = -result;
                break;
            case NOT:
                result = !result;
                break;
            case FLIP:
                result = ~(int)result;
        }

    }
    else
    {
        result = parse_atom();
    }
    return result;
}

static float parse_atom()
{
    float result;
    if(curr_token.id == LPAREN)
    {
        get_token();
        result = parse_equality();
        if(curr_token.id != RPAREN)
            error(EXPECTED_RIGHT_PAREN);
      
        get_token();
        return result;

    }
    if(curr_token.id == LT_CHARACTER)
    {
        result = (unsigned char)curr_token.lexeme[0];
        get_token();
        return result;
    }
    if(curr_token.id == LT_FLOAT)
    {
        result = atof(curr_token.lexeme);
        get_token();
        return result;
    }
    if(curr_token.id == LT_INTEGER)
    {
         result = atoi(curr_token.lexeme);
         get_token();   
         return result;
    }
    if(curr_token.id == TRUE)
    {
        get_token();
        return 1;
    }
    if(curr_token.id == FALSE || curr_token.id == NUL)
    {
        get_token();
        return 0;
    }
    if(curr_token.id == SIZEOF)
    {
        int has_paren = 0, size;
        get_token();
        if(curr_token.id == LPAREN)
        {
            has_paren = 1;
            get_token();
        }

        struct struct_defn_symbol *type_struct_defn = struct_defn_lookup(&struct_defn_list, curr_token.lexeme);
        if(type_struct_defn || curr_token.id == INT || curr_token.id == FLOAT || curr_token.id == CHAR || curr_token.id == BOOL)
        {
            int has_star = 0;
            get_token();
            if(curr_token.id == STAR)
            {
                has_star = 1;
                while(curr_token.id == STAR)
                    get_token();
            }
            put_token();

            if(!has_star && type_struct_defn)
                size = type_struct_defn->size;
            else if(!has_star && (curr_token.id == CHAR || curr_token.id == BOOL))
                size = 1;
            else
                size = 4;

            goto end;
        }
        
        struct token iden_token = curr_token;
        struct symbol *var = symbol_lookup(&gvar_list, curr_token.lexeme);
        if(! var)
        {
            error(UNKNOWN_VARIABLE);
            return 0;
        }

        int access_index = 0;
        if(var->type == STRUCT)
        {
            struct symbol *member = NULL;
            
            get_token();
            for(;;)
            {
                if(curr_token.id == LBRACKET)
                {
                    if(!var->status.is_array && !(var->type != STRUCT && var->status.is_ptr_count))
                        error(NOT_AN_ARRAY);

                    access_index = 1; //true
                    get_token();
                    eval_expr();

                    if(curr_token.id != RBRACKET)
                        error(UNCLOSED_ARRAY);
                    get_token();

                }
                if(curr_token.id == PERIOD)
                {
                    if(var->type != STRUCT)
                    {
                        curr_token = prev_token;
                        error(MEMBER_NOT_STRUCT);
                        return 0;
                    }

                    if(var->status.is_ptr_count)
                    {
                        curr_token = prev_token;
                        error(STRUCT_IS_POINTER);
                    }
                    get_token();

                    member = find_struct_member(var->defn.structure.defn_ptr->members, curr_token.lexeme);
                    if(! member)
                    {
                        prev_token = iden_token;
                        error(NOT_A_MEMBER);
                        return 0;
                    }

                }
                else if(curr_token.id == ARROW)
                {
                    if(var->type != STRUCT)
                    {
                        curr_token = prev_token;
                        error(MEMBER_NOT_STRUCT);
                        return 0;
                    }

                    if(!var->status.is_ptr_count)
                    {
                        curr_token = prev_token;
                        error(STRUCT_NOT_POINTER);
                    }
                    get_token();
                    member = find_struct_member(var->defn.structure.defn_ptr->members, curr_token.lexeme);
                    if(! member)
                    {
                        prev_token = iden_token;
                        error(NOT_A_MEMBER);
                        return 0;
                    }
                }
                else
                {
                    put_token();
                    if(access_index)
                    {
                        if(var->type == STRUCT && !var->status.is_ptr_count)
                            size = var->defn.structure.defn_ptr->size;
                        else if((var->type == CHAR || var->type == BOOL) && var->status.is_ptr_count == 1 && !var->status.is_array)
                            size = 1;
                        else if((var->type == CHAR || var->type == BOOL) && !var->status.is_ptr_count)
                            size = 1;
                        else
                            size = 4;
                    }
                    else if(var->status.is_array)
                    {
                        if(var->type == STRUCT && !var->status.is_ptr_count)
                            size = var->defn.array.size * var->defn.structure.defn_ptr->size;
                        else if((var->type == CHAR || var->type == BOOL) && !var->status.is_ptr_count)
                            size = var->defn.array.size;
                        else 
                            size = var->defn.array.size * 4;

                    }
                    else if(var->type == STRUCT && !var->status.is_ptr_count)
                    {
                        size = var->defn.structure.defn_ptr->size;
                    }
                    else
                    {
                        if((var->type == BOOL || var->type == CHAR)  && !var->status.is_ptr_count)
                            size = 1;
                        else 
                            size = 4;
                    }

                    break;
                }

                var = member;
                iden_token = curr_token;
                access_index = 0;
                get_token();
            }
        }
        else
        {
            get_token();
            if(curr_token.id == LBRACKET)
            {
                if(!var->status.is_array && !var->status.is_ptr_count)
                    error(NOT_AN_ARRAY);

                access_index = 1; //true
                get_token();
                eval_expr();
               
                if(curr_token.id != RBRACKET)
                    error(UNCLOSED_ARRAY);
            }
            else
            {
                put_token();
            }

            if(access_index)
            {
                if((var->type == CHAR || var->type == BOOL) && !var->status.is_ptr_count)
                    size = 1;
                else if((var->type == CHAR || var->type == BOOL) && var->status.is_ptr_count == 1 && !var->status.is_array)
                    size = 1;
                else
                    size = 4;
            }
            else if(var->status.is_array)
            {
                if((var->type == CHAR || var->type == BOOL) && !var->status.is_ptr_count)
                    size = var->defn.array.size;
                else 
                    size = var->defn.array.size * 4;
            }
            else
            {
                if((var->type == BOOL || var->type == CHAR)  && !var->status.is_ptr_count)
                    size = 1;
                else
                    size = 4;
            }
        }

        end:
            get_token();  
            if(has_paren && curr_token.id != RPAREN)
                error(EXPECTED_RIGHT_PAREN);
            if(has_paren)
                get_token();

            return (float)size;
    }

    error(INVALID_EXPRESSION);

    // skip other entities in the expression to produce 'clean' errors
    while(!(curr_token.id <= INPUT) && curr_token.id != SEMICOLON && curr_token.id != END_OF_FILE)
        get_token();
    
}

float eval_expr()
{
    get_token();
    float val = parse_logical();
    return val;
}

