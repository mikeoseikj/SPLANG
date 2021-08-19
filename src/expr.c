#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "include/globals.h"
#include "include/error.h"
#include "include/codegen.h"
#include "include/expr.h"
#include "include/queue.h"
#include "include/utils.h"
#include "include/symtab.h"
#include "include/warning.h"
#include "include/func.h"
#include "include/symtab.h"
#include "include/stmts.h"


void parse_declare()
{
    struct struct_defn_symbol *type_struct_defn = struct_defn_lookup(&struct_defn_list, curr_token.lexeme);

    if(is_datatype(curr_token.id) || type_struct_defn)
    {
        IN_VAR_DECL++; 
        int scope;
        struct symbol *local;
        char type_id = curr_token.id;

        do
        {
            get_token();
            local = alloc_sym();
            while(curr_token.id == STAR)
            {
                local->status.is_ptr_count++;
                get_token();
            }

            if(curr_token.id != LT_IDENTIFIER)
                error(EXPECTED_IDENTIFIER);

            struct token temp_token, iden_token = curr_token;
            if(curr_stmt)
                symbol_stmt_lookup(curr_token.lexeme, &scope);
            else
                symbol_scope_lookup(curr_token.lexeme, &scope);
        
            if(scope == LOCAL_SCOPE || scope == STMT_SCOPE) 
                error(LOCAL_PREVIOUSLY_DECLARED);

            local->type = type_id;
            if(type_struct_defn)
            {
                local->type = STRUCT;
                local->defn.structure.defn_ptr = type_struct_defn;
            }

            local->name = alloc_mem(strlen(curr_token.lexeme) + 1);
            strcpy(local->name, curr_token.lexeme);          
            if(curr_stmt)
                enqueue(&curr_stmt->locals, local);
            else
                enqueue(&curr_func->defn.function.lvars, local);

            // Calculate variable offset in assembly (stack)
            if(local->prev == NULL)
            {
                local->stack.offset = -4;  
            }
            else if(local->prev->type == STRUCT)
            {
                if(local->prev->status.is_array)
                {   
                    // Note: 4bytes + block of memory
                    if(local->prev->status.is_ptr_count)
                        local->stack.offset = local->prev->stack.offset - (local->prev->defn.array.size * 4) - 4;
                    else
                        local->stack.offset = local->prev->stack.offset - (local->prev->defn.structure.defn_ptr->size * local->prev->defn.array.size) - 4;
                }
                else if(local->prev->status.is_ptr_count)
                {
                    local->stack.offset = local->prev->stack.offset - 4;
                }
                else
                {
                    local->stack.offset = local->prev->stack.offset - local->prev->defn.structure.defn_ptr->size - 4;
                }
            }
            else if(local->prev->status.is_array)
            {
                int pad = (local->prev->defn.array.size % 4);
                if(pad) pad = 4 - pad;

                // Note: 4bytes + block of memory
                if((local->prev->type == CHAR || local->prev->type == BOOL) && !local->prev->status.is_ptr_count)    // char buf[]
                    local->stack.offset = local->prev->stack.offset - (local->prev->defn.array.size + pad) - 4;
                else
                    local->stack.offset = local->prev->stack.offset - (local->prev->defn.array.size  * 4) - 4;
                
            }
            else
            {
                local->stack.offset = local->prev->stack.offset - 4;
            }
            
            calc_max_func_stack(local);
            get_token();
            if(curr_token.id == ASSIGN)
            {
                temp_token = curr_token;
                if(local->type == CHAR && local->status.is_ptr_count)
                {
                    get_token();
                    if(curr_token.id == LT_STRING)
                    {
                        emit_assign_rodata_string(local, rodata_curr_seg_off, LOCAL_SCOPE);
                        create_rodata_segment_string(curr_token.lexeme);
                        get_token();
                        continue;
                    }
                    put_token();
                }
 
                curr_token = temp_token;
                put_token();
                curr_token = iden_token;  
                parse_assign();
            }
            else if(curr_token.id == LBRACKET)
            {
                local->status.is_array = 1; //true
                get_token();

                if(curr_token.id == RBRACKET)
                {
                    get_token();
                    goto ASSIGN_TO_ARRAY;
                }
                put_token();

                int size = eval_expr(); // array size
                local->defn.array.size = size;
                emit_assign_block_memory(local, LOCAL_SCOPE);
            
                if(curr_token.id != RBRACKET)
                    error(UNCLOSED_ARRAY);
                get_token();

                ASSIGN_TO_ARRAY:
                    calc_max_func_stack(local);
                    if(curr_token.id == ASSIGN)
                    {
                        get_token();  
                        // string, eg: char buf[] = "testing"
                        if(local->type == CHAR && !local->status.is_ptr_count && curr_token.id == LT_STRING)
                        {
                            int len = strlen(curr_token.lexeme);
                            if(local->defn.array.size == 0)                               
                                local->defn.array.size = (len + (4 - (len % 4))); 

                            if(local->defn.array.size <= len)
                                warning(STRING_TOO_LARGE);

                            calc_max_func_stack(local);
                            emit_assign_block_memory(local, LOCAL_SCOPE);
                            emit_buffer_string(local, curr_token.lexeme);
                            get_token();
                            continue;
                        }
                        if(curr_token.id != LBRACE)
                            error(MISSING_ARRAY_LBRACE);

                        // Predict size of array with no size explicitly provided
                        if(local->defn.array.size == 0)
                        {
                            int size = 1;
                            temp_token = curr_token;
                            long pos = get_file_pos();
                            int t_lnum = line_num, t_prev_cnum = prev_col_num, t_cnum = col_num;
                            do
                            {
                                get_token();
                                if(curr_token.id == COMMA)
                                    size++;
                            }while(curr_token.id != RBRACE && curr_token.id != SEMICOLON && curr_token.id != END_OF_FILE);

                            local->defn.array.size = size;
                            curr_token = temp_token;
                            set_file_pos(pos);
                            line_num = t_lnum, prev_col_num = t_prev_cnum, col_num = t_cnum;

                            emit_assign_block_memory(local, LOCAL_SCOPE);
                            calc_max_func_stack(local);
                        }
                        
                        int index = 0;
                        do
                        {
                            // if it is a string / char *
                            if(local->type == CHAR && local->status.is_ptr_count)
                            {
                                get_token();
                                if(curr_token.id == LT_STRING)
                                {
                                    emit_assign_rodata_string_init_array(local, index, rodata_curr_seg_off, LOCAL_SCOPE);
                                    create_rodata_segment_string(curr_token.lexeme);
                                    get_token();
                                    index++;
                                    continue;
                                }
                                put_token(); 
                            }

                            get_token();
                            parse_assign();
                            emit_array_init_element(local, index);
                            index++; 

                        }while(curr_token.id == COMMA);
          
                        if(index > local->defn.array.size)
                            warning(EXCESS_INIT_ELEMENTS);

                        if(curr_token.id != RBRACE)
                            error(MISSING_ARRAY_RBRACE);
                        get_token();
                    }

                    if(local->defn.array.size <= 0)
                        error(WRONG_ARRAY_SIZE);
            }

            if(local->type == STRUCT && !local->status.is_ptr_count && !local->status.is_array) // ensure that we don't do this twice; referring to 'array' part
                emit_assign_block_memory(local, LOCAL_SCOPE);
        
        }while(curr_token.id == COMMA);

        IN_VAR_DECL--; 
        return;
    }

    parse_assign();  
}



static void parse_assign()
{
    put_token();
    do
    {
        get_token(); 

        if((curr_token.id == STAR || curr_token.id == LT_IDENTIFIER) && check_equal_sign())
        {
            if(curr_token.id == STAR)
            {

                struct symbol *sym1 = pdref_symbol, *sym2 = paren_pdref_symbol;
                int int_1 = max_dref_count, int_2 = access_index, int_3 = gdref_count, int_4 = reset_dref_paren;
                pdref_symbol = paren_pdref_symbol = NULL;
                max_dref_count = access_index = gdref_count = 0;

                int is_byte;
                struct symbol *var = parse_left_ptr_expr(&is_byte);

                pdref_symbol = sym1, paren_pdref_symbol = sym2;
                max_dref_count = int_1, access_index = int_2, gdref_count = int_3, reset_dref_paren = int_4;
                if(!var)
                    return;

                if(curr_token.id != ASSIGN)
                    error(EXPECTED_EQUAL_SIGN);

                get_token();
                if(curr_token.id == LT_STRING)
                {
                    if(var->type == CHAR && var->status.is_ptr_count)   // char *,  char ** etc
                    {
                        emit_assign_rodata_string_to_pointer(rodata_curr_seg_off);
                        create_rodata_segment_string(curr_token.lexeme);
                        get_token();
                        continue;
                    }
                }

                emit_push_edi_reg();    // needed, eg: a = b = c = d
                parse_assign();
                emit_pop_edi_reg();
                emit_assign_to_pointer(is_byte);
                continue;
            }

            struct token iden_token = curr_token;
            int scope, iden_is_struct = 0; // false
            struct symbol *var = symbol_scope_lookup(iden_token.lexeme, &scope);
            if(! var)
            {
                error(UNKNOWN_VARIABLE);
                return;
            }
            
            int access_index = 0;
            if(var->type == STRUCT)
            {
                iden_is_struct = 1; // true
                iden_token = curr_token;
                struct symbol *member = NULL;
                
                emit_struct_address(var, scope);
                get_token();
            
                int firsttime = 1;
                for(;;)
                {
                    if(curr_token.id == LBRACKET)
                    { 
                        if(!var->status.is_array && !(var->type != STRUCT && var->status.is_ptr_count))
                            error(NOT_AN_ARRAY);

                        if(var->status.is_ptr_count && !firsttime)
                            emit_reset_member_pointer();

                        struct dregs temp_regs = regs;
                        get_token();
                        cgen_expr();
                        regs = temp_regs;

                        if(curr_token.id != RBRACKET)
                            error(UNCLOSED_ARRAY);

                        emit_adjust_struct_array(var);
                        get_token();
                        access_index = 1; // true
                    }
                    if(curr_token.id == PERIOD)
                    {
                        if(var->type != STRUCT)
                        {
                            curr_token = prev_token;
                            error(MEMBER_NOT_STRUCT);
                            return;
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
                            return;
                        }
                        emit_load_member_address(member);  

                    }
                    else if(curr_token.id == ARROW)
                    {
                        if(var->type != STRUCT)
                        {
                            curr_token = prev_token;
                            error(MEMBER_NOT_STRUCT);
                            return;
                        }

                        if(!var->status.is_ptr_count)
                        {
                            curr_token = prev_token;
                            error(STRUCT_NOT_POINTER);
                        }

                        emit_reset_member_pointer();
                        get_token();
                        member = find_struct_member(var->defn.structure.defn_ptr->members, curr_token.lexeme);
                        if(! member)
                        {
                            prev_token = iden_token;
                            error(NOT_A_MEMBER);
                            return;
                        }
                        emit_load_member_address(member);                                     
                    }
                    else
                    {
                        break;
                    }
    
                    iden_token = curr_token;
                    var = member;
                    get_token();
                    access_index = firsttime = 0;
                }
            
                put_token();
                if(var->type == STRUCT && !var->status.is_ptr_count)
                    error(ASSIGNING_TO_STRUCT);
            }


            get_token();
            if(curr_token.id == ASSIGN)
            {
                get_token();
                if(curr_token.id == LT_STRING)
                {
                    if(var->type == CHAR && var->status.is_ptr_count)   // char *,  char ** etc
                    {
                        if(iden_is_struct)
                            emit_assign_member_rodata_string(rodata_curr_seg_off);
                        else
                            emit_assign_rodata_string(var, rodata_curr_seg_off, scope);
                    
                        create_rodata_segment_string(curr_token.lexeme);
                        get_token();
                        continue;
                    }
                }
                
                if(iden_is_struct)
                    emit_push_edi_reg();    // needed, eg: a = b = c = d
                parse_assign();
                if(iden_is_struct)
                    emit_pop_edi_reg();
            
                if(var->status.is_array && !access_index)
                    error(ASSIGNING_TO_ARRAY);

                if(iden_is_struct)
                    emit_assign_to_member(var); // also works when assigning to a structure 
                else
                    emit_assign_to_variable(var, scope);

                continue;
            }
            if(!iden_is_struct && (var->status.is_ptr_count || var->status.is_array) && curr_token.id == LBRACKET)  // eg buf[x] = abc
            {
                struct dregs temp_regs = regs;
                get_token();
                cgen_expr();
                regs = temp_regs;
            
                if(curr_token.id != RBRACKET) 
                    error(EXPECTED_CLOSE_BRACKET);

                emit_adjust_variable_array(var, scope);
                get_token();
                if(curr_token.id == ASSIGN)
                {
                    get_token();
                    if(curr_token.id == LT_STRING && var->type == CHAR && var->status.is_ptr_count)
                    {
                        emit_assign_rodata_string_to_index(rodata_curr_seg_off);
                        create_rodata_segment_string(curr_token.lexeme);
                        get_token();
                        continue;
                    }
                    emit_push_edi_reg();
                    parse_assign();
                    emit_pop_edi_reg();
                    emit_assign_to_array_index(var);
                    continue;
                }
            }
            error(EXPECTED_EQUAL_SIGN);
            return;
        }
        else 
        {
            goto end;
        }

    }while(curr_token.id == COMMA && !IN_VAR_DECL);  
    return;

    end:
        parse_logical();
        emit_cond_pop_eax();
        // neccessay because of pushing constants in the parse_atom()
        if(d_regs->in_fpu_mode)
            print_type = PRINT_FLOAT;
        else if(print_type == NO_PRINT_TYPE)
            print_type = PRINT_INT;
}


static void parse_logical()
{
    parse_bitwise();
    int operator = curr_token.id;

    while(operator == LAND || operator == LOR)
    {
        if(! valid_operator(operator))
            error(INVALID_EXPRESSION);

        get_token();
        parse_bitwise();
        switch(operator)
        {
            case LAND:
                emit_logical_and();
                break;
            case LOR:
                emit_logical_or();
        }
        if(curr_token.id == SEMICOLON || curr_token.id == END_OF_FILE)
            return;  

        operator = curr_token.id;
    }
}

static void parse_bitwise()
{ 
    parse_equality(); 
    int operator = curr_token.id;

    while(operator == XOR || operator == AND || operator == OR || operator == LSHIFT || operator == RSHIFT)
    {
        if(! valid_operator(operator))
            error(INVALID_EXPRESSION);

        get_token();
        parse_equality();
        switch(operator)
        {
            case XOR:
                emit_bit_xor();
                break;
            case AND:
                emit_bit_and();
                break;
            case OR:
                emit_bit_or();
                break;
            case LSHIFT:
                emit_bit_lshift();
                break;
            case RSHIFT:
                emit_bit_rshift();
        }       
        operator = curr_token.id;
    }
}


static void parse_equality()
{
    parse_compare();
    int operator = curr_token.id;

    while(operator == EQUAL || operator == NOT_EQUAL)
    {
        if(! valid_operator(operator))
            error(INVALID_EXPRESSION);

        get_token();
        parse_compare();
        switch(operator)
        {
            case EQUAL:
                emit_equ();
                break;
            case  NOT_EQUAL:
                emit_not_equ();
        }
        operator = curr_token.id;
    }
}


static void parse_compare()
{
    parse_term();
    int operator = curr_token.id;

    while(operator == GREATER || operator == LESS || operator == GREATER_EQUAL || operator == LESS_EQUAL)
    {
        if(! valid_operator(operator))
            error(INVALID_EXPRESSION);

        get_token();
        parse_term();
        switch(operator)
        {
            case GREATER:
                emit_gtr();
                break;
            case  LESS:
                emit_less();
                break;
            case GREATER_EQUAL:
                emit_gtr_equ();
                break;
            case LESS_EQUAL:
                emit_less_equ();
        }   
        operator = curr_token.id;
    }
}


static void parse_term()
{ 
    parse_factor(); 
    int operator = curr_token.id;

    while(operator == PLUS || operator == MINUS)
    {
        if(! valid_operator(operator))
            error(INVALID_EXPRESSION);

        get_token();
        parse_factor();
        switch(operator)
        {
            case PLUS:
                emit_add();
                break;
            case MINUS:
                emit_sub();
        }
        operator = curr_token.id;        
    }    
}


static void parse_factor()
{
    parse_unary();
    int operator = curr_token.id;

    while(operator == STAR || operator == SLASH || operator == MODULO)
    {
        if(! valid_operator(operator))
            error(INVALID_EXPRESSION);

        get_token();
        parse_unary();
        switch(operator)
        {
            case STAR:
                emit_mul();
                break;
            case  SLASH:
                emit_div();
                break;
            case MODULO:
                emit_mod();
        }
        operator = curr_token.id;
    }
}


static void parse_unary()
{ 
    int operator = curr_token.id;
    if(operator == PLUS || operator == MINUS || operator == NOT || operator == FLIP || operator == STAR)
    {
        int dref_count = 0;
        int parenthesized = 0, is_byte = 0, first = 0, c_num;
        if(curr_token.id == STAR)
        {
            for(;;)
            {
                dref_count++;
                get_token();
                if(curr_token.id != STAR)
                    goto label;
            }
        }

        get_token();
        label:

            if(dref_count)  // eg: *ptr
            {
                gdref_count = dref_count;
                if(curr_token.id == LPAREN)
                {
                    if(max_dref_count == 0)
                    {
                        reset_dref_paren = 1;
                        first = 1; 
                    }
                    parenthesized = 1; //true
                    max_dref_count -= dref_count;
                    c_num = col_num, gdref_count = 0;
                }
            }
            
        parse_atom();
        switch(operator)
        {
            case MINUS:
                emit_neg();
                break;
            case NOT:
                emit_not();
                break;
            case FLIP:
                emit_flip();
                break;
            case STAR:

                // For something like this => *(ptr),
                if(parenthesized)
                {  
                    pdref_symbol = paren_pdref_symbol;
                    if(pdref_symbol == NULL || !pdref_symbol->status.is_ptr_count)
                    {
                        error(NOT_A_POINTER);
                        return;
                    }
                    if(max_dref_count < 0)
                    {
                        col_num = c_num;
                        sprintf(curr_token.lexeme, "%d asterisk(s)", pdref_symbol->status.is_ptr_count);
                        error(EXCESS_POINTER_DEREFS);
                        return;
                    }
                    
                    if(first && pdref_symbol->type == CHAR && max_dref_count == 0 && !pdref_symbol->status.is_array)
                    {
                        is_byte = 1;
                        print_type = PRINT_CHAR;
                    }

                    emit_deref_variable(pdref_symbol, dref_count, is_byte);
                    if(first)
                    {
                        pdref_symbol = paren_pdref_symbol = NULL;
                        max_dref_count = access_index = gdref_count = reset_dref_paren = 0;
                    }
                    return;
                }

                if(pdref_symbol == NULL)
                {
                    error(NOT_A_POINTER);
                    return;
                }

                // For something like this => *ptr
                int count = pdref_symbol->status.is_ptr_count;
                int max_count = dref_count;
                if(access_index)
                {
                    count--, max_count++;
                    access_index = 0;
                }

                if(!pdref_symbol->status.is_ptr_count) // pdef_sym will be null if a char * cannot be derefenced ie: (char *a; *a[0])
                {
                    error(NOT_A_POINTER);
                    return;
                }
                if(dref_count > count)
                {
                    sprintf(curr_token.lexeme, "%d asterisk(s)", pdref_symbol->status.is_ptr_count);
                    error(EXCESS_POINTER_DEREFS);
                    return;
                }
                if(pdref_symbol->type == CHAR && (max_count == pdref_symbol->status.is_ptr_count) && !pdref_symbol->status.is_array)
                {
                    is_byte = 1;
                    print_type = PRINT_CHAR;
                }
                emit_deref_variable(pdref_symbol, dref_count, is_byte);
                pdref_symbol = NULL;
                if(! reset_dref_paren)
                    paren_pdref_symbol = NULL;

        }
    }
    else
    {
        parse_atom();
        pdref_symbol = NULL;
        access_index = 0;
    }
}


static void parse_atom()
{
    if(curr_token.id == LPAREN)
    {
        // Storing 'd_regs->in_fpu_mode' prevents excessive dependence on the FPU
        int mode = d_regs->in_fpu_mode;  
        d_regs->in_fpu_mode = 0;
        emit_cond_push_eax();
        get_token();
        parse_declare();
        if(curr_token.id != RPAREN)
            error(EXPECTED_RIGHT_PAREN);
        get_token();
        emit_cond_push_eax();
        if(mode)
            d_regs->in_fpu_mode = mode;
    }
    else if(curr_token.id == FUNCALL)
    {
        IN_FUNC_CALL++;
        emit_cond_push_eax();
        cgen_func_call(curr_token);
        emit_push_func_retval();
        get_token();
        IN_FUNC_CALL--;
    }
    else if(curr_token.id == AND || curr_token.id == LT_IDENTIFIER) // ampersand/and/& (address of) 
    {
        int NEEDS_ADDROF = 0;
        if(curr_token.id == AND) 
        {
            NEEDS_ADDROF = 1;
            get_token();
        }
  
        emit_cond_push_eax();
        struct token iden_token = curr_token;
        int scope;  
        struct symbol *var = symbol_scope_lookup(curr_token.lexeme, &scope);
        if(! var)
        {
            error(UNKNOWN_VARIABLE);
            return;
        }

        if(var->type == STRUCT)
        {
            struct symbol *member = NULL;
            emit_struct_address(var, scope);
            get_token();
            
            int is_byte = 0, needs_addr = 0, firsttime = 1;
            for(;;)
            {
                if(curr_token.id == LBRACKET)
                {
                    needs_addr = 0, access_index = 1;
                    if(!var->status.is_array && !(var->type != STRUCT && var->status.is_ptr_count))
                        error(NOT_AN_ARRAY);

                    if(var->status.is_ptr_count && !firsttime)
                        emit_reset_member_pointer();
                    
                    struct dregs temp_regs = regs;
                    get_token();
                    cgen_expr();
                    regs = temp_regs;

                    // no need to check for bool => look at code before calling 'push_struct_value' 
                    if((var->type == CHAR && var->status.is_array && !var->status.is_ptr_count) || (var->type == CHAR && var->status.is_ptr_count == 1 && !var->status.is_array))
                        is_byte = 1;  

                    if(curr_token.id != RBRACKET)
                        error(UNCLOSED_ARRAY);

                    emit_adjust_struct_array(var);
                    get_token();
                }
                if(curr_token.id == PERIOD)
                {
                    if(var->type != STRUCT)
                    {
                        curr_token = prev_token;
                        error(MEMBER_NOT_STRUCT);
                        return;
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
                        return;
                    }
                   
                    emit_load_member_address(member);
                }
                else if(curr_token.id == ARROW)
                {
                    if(var->type != STRUCT)
                    {
                        curr_token = prev_token;
                        error(MEMBER_NOT_STRUCT);
                        return;
                    }

                    if(!var->status.is_ptr_count)
                    {
                        curr_token = prev_token;
                        error(STRUCT_NOT_POINTER);
                    }

                    emit_reset_member_pointer();
                    get_token();
                    member = find_struct_member(var->defn.structure.defn_ptr->members, curr_token.lexeme);
                    if(! member)
                    {
                        prev_token = iden_token;
                        error(NOT_A_MEMBER);
                        return;
                    }
                    emit_load_member_address(member); 
                }
                else
                {
                    if(var->type == FLOAT)
                        print_type = PRINT_FLOAT;
                    else if(!is_byte && ((var->type == CHAR && var->status.is_ptr_count) || (var->type == CHAR && var->status.is_array)))
                        print_type = PRINT_STRING;
                    else if(var->type == CHAR)
                        print_type = PRINT_CHAR, is_byte = 1;
                    else if(var->type == BOOL)
                        print_type = PRINT_INT, is_byte = 1;
                    else
                        print_type = PRINT_INT;

                    pdref_symbol = var;
                    if(paren_pdref_symbol == NULL || !paren_pdref_symbol->status.is_ptr_count)
                    {
                        paren_pdref_symbol = var;     
                        max_dref_count += var->status.is_ptr_count;

                        if(access_index)
                            max_dref_count--; 
                        max_dref_count -= gdref_count;
                       
                    }
                    gdref_count = 0;

                    if(NEEDS_ADDROF || needs_addr || (IN_FUNC_CALL && var->type == STRUCT && !var->status.is_ptr_count))
                        emit_push_struct_address();
                    else
                        emit_push_struct_value(var, is_byte);

                    return;
                }
                
                firsttime = access_index = 0;
                iden_token = curr_token;
                var = member;
                if(is_datatype(var->type) && var->status.is_array)
                    needs_addr = 1; //true
                get_token();
            }
        }

        if(var->type == FLOAT)
            print_type = PRINT_FLOAT;
        else if(var->type == CHAR && var->status.is_ptr_count)
            print_type = PRINT_STRING;
        else if(var->type == CHAR && var->status.is_array)
            print_type = PRINT_STRING;
        else if(var->type == CHAR && !var->status.is_ptr_count && !var->status.is_array)
            print_type = PRINT_CHAR;
        else
            print_type = PRINT_INT;


        get_token();

        pdref_symbol = var;
        if(paren_pdref_symbol == NULL || !paren_pdref_symbol->status.is_ptr_count)
        {
            paren_pdref_symbol = var;
            max_dref_count += var->status.is_ptr_count;
            if(curr_token.id == LBRACKET)
                max_dref_count--;
            max_dref_count -= gdref_count;    
        }
        gdref_count = 0;   

        // If we are dealing with an array eg: myvar[2]
        if(curr_token.id == LBRACKET)
        {
            access_index = 1; //true       
            if(!var->status.is_array && !var->status.is_ptr_count)
                error(NOT_AN_ARRAY);

            int ptype = print_type;
            struct dregs temp_regs = regs;
            get_token();
            cgen_expr();
            regs = temp_regs;
            print_type = ptype;

            if(var->type == CHAR && var->status.is_array && !var->status.is_ptr_count)
                print_type = PRINT_CHAR;
           
            if(curr_token.id != RBRACKET)
                error(EXPECTED_CLOSE_BRACKET);
    
            if(NEEDS_ADDROF)
                emit_push_addrof_array_element(var, scope);
            else
                emit_push_array_element(var, scope);
            get_token();
            return;

        }
        // if an 'ordinary' variable
        put_token();
        if(NEEDS_ADDROF)
            emit_push_addrof_variable(var, scope);
        else
            emit_push_variable(var, scope);
        get_token();
    }
    else if(curr_token.id == LT_CHARACTER)
    {
        int value = curr_token.lexeme[0];
        emit_cond_push_eax();
        emit_push_const((char)value, INT_TYPE);
        get_token();
        print_type = PRINT_CHAR;
    }
    else if(curr_token.id == LT_FLOAT)
    {
        d_regs->in_fpu_mode = 1;
        float value = atof(curr_token.lexeme);
        emit_cond_push_eax();
        emit_push_const(IEEE_754_to_float_convert((float)value), FLOAT_TYPE);
        get_token();
    }
    else if(curr_token.id == LT_INTEGER)
    {
        unsigned int value = (unsigned int)atof(curr_token.lexeme);   // Note: atoi() does not work with hexadecimals but atof() does
        emit_cond_push_eax();
        emit_push_const(value, INT_TYPE);
        get_token();   
    }
    else if(curr_token.id == TRUE)
    {
        int value = 1;
        emit_cond_push_eax();
        emit_push_const(value, INT_TYPE);
        get_token();
    }
    else if(curr_token.id == FALSE || curr_token.id == NUL)
    {
        int value = 0;
        emit_cond_push_eax();
        emit_push_const(value, INT_TYPE);
        get_token();
    }
    else if(curr_token.id == SIZEOF)
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
        int scope;  
        struct token iden_token = curr_token;
        struct symbol *var = symbol_scope_lookup(curr_token.lexeme, &scope);
        if(! var)
        {
            error(UNKNOWN_VARIABLE);
            return;
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
                    struct dregs temp_regs = regs;
                    get_token();
                    cgen_expr();
                    regs = temp_regs;
                   
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
                        return;
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
                        return;
                    }

                }
                else if(curr_token.id == ARROW)
                {
                    if(var->type != STRUCT)
                    {
                        curr_token = prev_token;
                        error(MEMBER_NOT_STRUCT);
                        return;
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
                        return;
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
                struct dregs temp_regs = regs;
                get_token();
                cgen_expr();
                regs = temp_regs;

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
            emit_push_const(size, INT_TYPE);
            get_token();  
            if(has_paren && curr_token.id != RPAREN)
                error(EXPECTED_RIGHT_PAREN);
            if(has_paren)
                get_token();

    }
    else
    {
        if(curr_token.id == CONTINUE || curr_token.id == BREAK)
            error(NOT_IN_LOOP);
        else
            error(INVALID_EXPRESSION);

        while(!(curr_token.id <= INPUT) && curr_token.id != SEMICOLON && curr_token.id != END_OF_FILE)
            get_token();
    }
}


void cgen_expr()
{    
    // store some global variables  (putthing these directly in the code will make it clumsy)
    struct symbol *sym1 = pdref_symbol, *sym2 = paren_pdref_symbol;
    int int_1 = max_dref_count, int_2 = access_index, int_3 = gdref_count, int_4 = reset_dref_paren;
      
    pdref_symbol = paren_pdref_symbol = NULL;
    max_dref_count = access_index = gdref_count = reset_dref_paren = 0;

    d_regs->in_fpu_mode = 0;
    d_regs->eax.busy = 0, d_regs->st0.busy = 0;
    d_regs->eax.vartype = d_regs->st0.vartype = 0;
    d_regs->cgen_stack.top = 0;
    print_type = NO_PRINT_TYPE;
    pdref_symbol = paren_pdref_symbol = NULL;
    parse_declare();

    pdref_symbol = sym1, paren_pdref_symbol = sym2;
    max_dref_count = int_1, access_index = int_2, gdref_count = int_3, reset_dref_paren = int_4;
    

}
