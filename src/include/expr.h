#ifndef EXPR_H
#define EXPR_H

/*

@@@-----------------------------------------------------------------------------------@@@
    
    calc_max_func_stack() is used to know the size of the stack in the function
    prologue. ie:

        push ebp
        mov ebp, esp
        sub esp, N  => call_max_func_stack() is used to determine 'N'

    It is called in multiple places in the parse_declare() which might 
    be confusing.

    calc_max_func_stack() - first call  => Note that at that point we don't know 
    if it is an array or not so its size will not be correct if we later determine 
    that it is an array. Other calls to the function are used to recalculate the size 
    if we later identify something that influences the size of the variable

@@@-----------------------------------------------------------------------------------@@@

    emit_assign_rodata_string_to_index(), emit_assign_to_array_index() works with
    with edi regsiter just like the 'struct functions'. This is neccessary because
    in the parse_assign(),  we check whether the index of an array is being accessed. 
    The emit_adjust_variable_array() adjust the accessed array be 'eax' and store
    stores the results in edi register. This 'forces' the functions to work with the
    edi

@@@--------------------------variables memory structures------------------------------@@@

    size of variables           => 4bytes
    size of pointers            => 4bytes
    size of arrays/structures   => Needs some explanation


    The size of structures and arrays in memory => ROUNDUP(size) to closest 4bytes + extra 4bytes

                   @-----Global arrays and structures-----@
    eg: int arr[24];

    [x, 0, 1, ..., 23] => memory layout, they are adjacent in memory

    x => is is the variable 'arr' which contains a pointer to next memory ie; '0'
    which is the storage memory for the array. The same applies to global structures


                   @----Local arrays and structures-----@
    eg: int myarr[24]
    [x, 23, ..., 2, 0] => memory layout, stack grows from higher address to lower addresses
                  
    x => is the variable 'myarr' which contains a pointer to (current memory + sizeof(myarr))
    in the case address of '0'; This applies to local structures too.
    
    The calculation of the 'pointers to memory' for the arrays and structures is done by 
    the emit_assign_block_memory().

    Note: This design style makes my implementation 'simple' but it has some obvious and critical
    downsides. Writing past(overflowing) an array or structure might overwrite the 'memory pointers'
    of other arrays/structures adjacent to it(locals overwrite their own pointers). This might lead to 
    segmentation fault when you try to used the overwritten array/structures later on because their pointers 
    have been replaced with 4bytes of the overflowed data. In languages like C/C++ overflows usually leads to 
    segmentation faults when you overwrites function return addresses or function and variable pointers. 

*/

static void parse_assign();
static void parse_logical();
static void parse_bitwise();
static void parse_equality();
static void parse_compare();
static void parse_term();
static void parse_factor();
static void parse_unary();
static void parse_atom();


int IN_FUNC_CALL = 0, IN_VAR_DECL = 0;    // used by 'parse_assign()' function 

struct symbol *pdref_symbol = NULL, *paren_pdref_symbol = NULL;
int max_dref_count = 0, access_index = 0, gdref_count = 0, reset_dref_paren = 0;

static void calc_max_func_stack(struct symbol *local)   // Note: might be called multiple times
{
    int stacksize, size = -(local->stack.offset + 4); // Note: first stack 'variable' starts at [ebp-4]
    if(local->defn.array.size)
    {
        if(local->type == STRUCT && !local->status.is_ptr_count)
            size = size + (local->defn.structure.defn_ptr->size * local->defn.array.size) + 4;
        else if((local->type == CHAR || local->type == BOOL) && !local->status.is_ptr_count)   // char buf[]
            size = size + local->defn.array.size + 4;
        else
            size = size + (local->defn.array.size * 4) + 4;
    }
    else if(local->type == STRUCT && !local->status.is_ptr_count)
    {
        size = (size + local->defn.structure.defn_ptr->size) + 4;
    }
    else
    {
            size = (size + 4);
    }
    stacksize = curr_func->defn.function.stacksize;
    stacksize = stacksize < size? size : stacksize;
    curr_func->defn.function.stacksize = stacksize; 
}


static int check_equal_sign()
{
    struct token temp_token = curr_token;
    long pos = get_file_pos();
    int t_lnum = line_num, t_prev_cnum = prev_col_num, t_cnum = col_num;

    int bracks = 0, has_equal = 0;
    while(curr_token.id != SEMICOLON && curr_token.id != END_OF_FILE)
    {
        get_token();
        if(curr_token.id == LBRACKET || curr_token.id == LPAREN)
            bracks++;
        else if(curr_token.id == RBRACKET || curr_token.id == RPAREN)
            bracks--;


        if(curr_token.id == ASSIGN && bracks == 0) 
        {
            has_equal = 1; // true
            break;
        }
        if(valid_operator(curr_token.id) && bracks == 0)
            break;
        if(curr_token.id == COMMA && bracks == 0)
        	break;
    }
    set_file_pos(pos);
    line_num = t_lnum, prev_col_num = t_prev_cnum, col_num = t_cnum;
    curr_token = temp_token;

    return has_equal;
}


// used in something like this => *(*str) = '323';
struct symbol *parse_left_ptr_expr(int *is_byte)
{  
    int dref_count = 0;
    int parenthesized = 0, c_num;
        
    for(;;)
    {
        dref_count++;
        get_token();
        if(curr_token.id != STAR)
            goto label;
    }

    get_token();
    label:

        gdref_count = dref_count;
        if(curr_token.id == LPAREN)
        {
            parenthesized = 1; //true
            max_dref_count -= dref_count;
            c_num = col_num, gdref_count = 0;
        }
            
        parse_atom();

        // For something like this => *(ptr),
        if(parenthesized)
        {  
            pdref_symbol = paren_pdref_symbol;
            if(pdref_symbol == NULL || !pdref_symbol->status.is_ptr_count)
            {
                error(NOT_A_POINTER);
                return NULL;
            }

            if(max_dref_count < 0)
            {
                col_num = c_num;
                sprintf(curr_token.lexeme, "%d asterisk(s)", pdref_symbol->status.is_ptr_count);
                error(EXCESS_POINTER_DEREFS);
                return NULL;
            }        
            if(pdref_symbol->type == CHAR && max_dref_count == 0 && !pdref_symbol->status.is_array)
                *is_byte = 1;
            else if(pdref_symbol->type == BOOL)
                *is_byte = 1;
            else
                *is_byte = 0;

            emit_deref_get_addr(pdref_symbol, dref_count);
            return pdref_symbol;
        }

        if(pdref_symbol == NULL)
        {
            error(NOT_A_POINTER);
            return NULL;
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
            return NULL;
        }
        if(dref_count > count)
        {
            sprintf(curr_token.lexeme, "%d asterisk(s)", pdref_symbol->status.is_ptr_count);
            error(EXCESS_POINTER_DEREFS);
            return NULL;
        }
        if(pdref_symbol->type == CHAR && (max_count == pdref_symbol->status.is_ptr_count) && !pdref_symbol->status.is_array)
            *is_byte = 1;
        else if(pdref_symbol->type == BOOL)
            *is_byte = 1;
        else
            *is_byte = 0;
                    
        emit_deref_get_addr(pdref_symbol, dref_count);
        return pdref_symbol;
    
}


#endif