#include <stdio.h>
#include <stdlib.h>
#include "include/globals.h"
#include "include/error.h"
#include "include/warning.h"
#include "include/codegen.h"
#include "include/symtab.h"
#include "include/stmts.h"
#include "include/func.h"
#include "include/utils.h"

struct lf_descr lib_funcs[] = {
		{"openfile",         2, RET_INT,   INT_TYPE, INT_TYPE},		// function name, number of args , return type, parameter types 
		{"readfile",         3, RET_INT,   INT_TYPE, INT_TYPE, INT_TYPE},
		{"writefile",        3, RET_INT,   INT_TYPE, INT_TYPE, INT_TYPE},
		{"closefile",        1, RET_INT,   INT_TYPE},
		{"strcmp",           2, RET_INT,   INT_TYPE, INT_TYPE},
		{"strncmp",          3, RET_INT,   INT_TYPE, INT_TYPE, INT_TYPE},
		{"strcat",           2, RET_PTR,   INT_TYPE, INT_TYPE},
		{"strlen",           1, RET_INT,   INT_TYPE},
		{"index",            2, RET_PTR,   INT_TYPE, INT_TYPE},
		{"rindex",           2, RET_PTR,   INT_TYPE, INT_TYPE},
		{"strcpy",           2, RET_PTR,   INT_TYPE, INT_TYPE},
		{"strncpy",          3, RET_PTR,   INT_TYPE, INT_TYPE, INT_TYPE},
		{"strstr",           2, RET_PTR,   INT_TYPE, INT_TYPE},
		{"strcasestr",       2, RET_PTR,   INT_TYPE, INT_TYPE},
		{"memcpy",           3, RET_PTR,   INT_TYPE, INT_TYPE, INT_TYPE},
		{"memcmp",           3, RET_INT,   INT_TYPE, INT_TYPE, INT_TYPE},
		{"memmove",          3, RET_PTR,   INT_TYPE, INT_TYPE, INT_TYPE},
		{"malloc",           1, RET_PTR,   INT_TYPE},
		{"free",             1, 0,         INT_TYPE},
		{"realloc",          2, RET_PTR,   INT_TYPE, INT_TYPE},
		{"sys_error",        1, 0,         INT_TYPE},
		{"exit",             1, 0,         INT_TYPE},
		{"readline",         1, RET_PTR,   INT_TYPE},
        {"alloca",           1, RET_PTR,   INT_TYPE},
        {"cleanup_alloca",   0, 0},
        {"pow",              2, RET_FLOAT, FLOAT_TYPE, FLOAT_TYPE},
        {"sqrt",             1, RET_FLOAT, FLOAT_TYPE},
        {"sin",              1, RET_FLOAT, FLOAT_TYPE},
        {"cos",              1, RET_FLOAT, FLOAT_TYPE},
        {"tan",              1, RET_FLOAT, FLOAT_TYPE},
        {"log",              1, RET_FLOAT, FLOAT_TYPE},
        {"log2",             1, RET_FLOAT, FLOAT_TYPE},
        {"log10",            1, RET_FLOAT, FLOAT_TYPE},
		{NULL, NULL, 0}
};


struct lf_descr *is_stdlib_func(char *func_name)
{
	for(int i = 0; lib_funcs[i].name != NULL; i++)
	{
		if(strcmp(lib_funcs[i].name, func_name) == 0)
        {
            if(strcmp(func_name, "alloca") == 0)
                alloca_count++;
			return (struct lf_descr *)&lib_funcs[i];
        }
	}
	return NULL;
}

int alloca_count = 0;
void cgen_func_block(struct symbol *fsym)
{
	emit_label("_%s:", fsym->name);
	int startpos = f_tell(out_stream);

	curr_func = fsym;
	set_file_pos(fsym->defn.function.file_loc);
	line_num = fsym->defn.function.line_num;
	col_num = fsym->defn.function.col_num;

	get_token();
	if(curr_token.id != LBRACE)
		error(EXPECTED_FUNC_LBRACE);

    int empty = 0;
    get_token();
    if(curr_token.id == RBRACE)
    	empty = 1;
    	
   	for(;;)
   	{
   		if(empty)	
   			break;

        if(curr_token.id == IF)
        {
        	get_token();
            cgen_if_stmt();
           	goto brk;
        }

        if(curr_token.id == WHILE)
        {
            get_token();
            cgen_while_stmt();
        	goto brk;   	
        }
        if(curr_token.id == DO)
        {
        	get_token();
        	cgen_do_while_stmt();
        	goto brk;
        }        
        if(curr_token.id == FOR)
        {
        	get_token();
            cgen_for_stmt();
            goto brk;
        }
        if(curr_token.id == ECHO)
        {
        	get_token();
        	cgen_echo_stmt();
        	goto brk;
        }
        if(curr_token.id == RETURN)
        {
        	get_token();
        	cgen_return_stmt();
        	goto brk;
        }
        if(curr_token.id == INPUT)
        {
        	get_token();
        	cgen_input_stmt();
        	goto brk;
        }
         
   		cgen_expr();
   		if(curr_token.id != SEMICOLON)
            error(EXPECTED_SEMICOLON);
            
        brk:
        	get_token();
   			if(curr_token.id == RBRACE || curr_token.id == END_OF_FILE)
   				break;
   	}
   	
   	int size = f_tell(out_stream) - startpos;
   	char *code_buf = alloc_mem(size+1);
   	f_seek(out_stream, startpos, SEEK_SET);
   	f_read(code_buf, size, out_stream);
   	f_seek(out_stream, startpos, SEEK_SET);
  
    emit_func_prolog(fsym->defn.function.stacksize);
   	emit(code_buf);
    if(alloca_count)
    {
        emit_lib_call("cleanup_alloca"); 
        alloca_count = 0; 
    }
   	emit_func_epilog();
    free(code_buf);

    reset_stmt_label_counters();
    if(curr_token.id != RBRACE)
        error(EXPECTED_FUNC_RBRACE);

}

void cgen_func_call(struct token func_token)
{

    struct dregs temp_regs = regs;
	int nparams, ncommas, size, memz;
    int is_lfunc = 0, p_index = 0;
	get_token();

    struct symbol *fcall, *sym, *e_sym, *param;
	struct lf_descr *descr = is_stdlib_func(func_token.lexeme);
    fcall = symbol_lookup(&func_list, func_token.lexeme);

    if(!fcall && !descr)
    {
        curr_token = func_token;
        error(UNKNOWN_FUNC_CALL);
        return;
    }

    // if there is a user defined version of a 'library' function, the user's own is given priority
    if(fcall)
    {
        param = fcall->defn.function.params;
        nparams = fcall->defn.function.num_params;
        param = fcall->defn.function.params;
    }
	else
	{
		// so that we extern only library functions that we only use
		sym = extern_list;
		while(sym)
		{
			if(strcmp(sym->name, func_token.lexeme) == 0)
				goto done;
			sym = sym->next;
		}
		e_sym = alloc_sym();
		e_sym->name = alloc_mem(strlen(func_token.lexeme) + 1);
		strcpy(e_sym->name, func_token.lexeme);
		enqueue(&extern_list, e_sym);

		done:
			nparams = descr->nparams;
			is_lfunc = 1; // true
	}
	
    ncommas = (nparams > 0)? nparams - 1 : 0;
    size = ncommas * 4;
    memz = nparams * 4;		// used to adjust esp after function calls

    // Note: Functions accept args in the reverse order but I pushed them in the forwared order and 
    // reversed them on the stack on subsequent pushes(as I push). This method I used is unnecessary,
    // complex and inefficient because I could've used a recursive function to push the args in the
    // reverse order which will produce short, simple and efficient but i just wanted to try this method and experiment/play with it
    // The reversing is done with the 'emit_add_esp()' and 'emit_sub_esp()' functions
   
    get_token();
    if(nparams == 0)
    	goto end;

    emit_push_all_regs();
    if(ncommas)
    	emit_sub_esp(size);
    
    if(curr_token.id == LT_STRING)
    {
    	emit_rodata_func_string_arg(rodata_curr_seg_off);
        create_rodata_segment_string(curr_token.lexeme);
        emit_push_func_arg();
        get_token();

        if(is_lfunc)
            p_index++;
        else
            param = param->next;
    }
    else
    {
    	cgen_expr();
     
        if(is_lfunc)
        {
            if(descr->param_types[p_index] == FLOAT_TYPE && !d_regs->in_fpu_mode)
                emit_param_cast(FLOAT_TYPE);
            else if(descr->param_types[p_index] == INT_TYPE && d_regs->in_fpu_mode)
                emit_param_cast(INT_TYPE);
            p_index++;
        }
        else
        {
            if(param->type == FLOAT && !d_regs->in_fpu_mode)
                emit_param_cast(FLOAT_TYPE);
            else if(param->type != FLOAT && d_regs->in_fpu_mode)
                emit_param_cast(INT_TYPE);

            if(param->type == STRUCT && !param->status.is_ptr_count)
                emit_struct_copy(param, memz);
            param = param->next; 
        }

    	emit_push_func_arg();
    }

    if(curr_token.id == COMMA)
    	emit_add_esp(8);
    
    nparams--;
    while(curr_token.id == COMMA && nparams)
    {
    	get_token();
    	if(curr_token.id == LT_STRING)
    	{
    		emit_rodata_func_string_arg(rodata_curr_seg_off);
        	create_rodata_segment_string(curr_token.lexeme);
        	get_token();

            if(is_lfunc)
                p_index++;
            else
                param = param->next;
    	}
    	else
    	{    		
    		cgen_expr();
            if(is_lfunc)
            {
                if(descr->param_types[p_index] == FLOAT_TYPE && !d_regs->in_fpu_mode)
                    emit_param_cast(FLOAT_TYPE);
                else if(descr->param_types[p_index] == INT_TYPE && d_regs->in_fpu_mode)
                    emit_param_cast(INT_TYPE);
                p_index++;
             }
             else
             {
                if(param->type == FLOAT && !d_regs->in_fpu_mode)
                    emit_param_cast(FLOAT_TYPE);
                else if(param->type != FLOAT && d_regs->in_fpu_mode)
                    emit_param_cast(INT_TYPE);

                if(param->type == STRUCT && !param->status.is_ptr_count)
                    emit_struct_copy(param, memz);
                param = param->next;
        
             }  

    	}
           
    	emit_push_func_arg();
    	if(curr_token.id == COMMA)
    		emit_add_esp(8);
    	nparams--;
    }
    if(ncommas)
    	emit_sub_esp(size);

    if(nparams)
    	error(EXPECTED_A_COMMA);
   
    end:
        if(is_lfunc)
    	    emit_lib_call(func_token.lexeme);
        else
            emit_call(func_token.lexeme);
        
    	if(memz)
        {
    		emit_add_esp(memz);
            emit_pop_all_regs();
        }
         
    	if(curr_token.id != RPAREN)
        	error(EXPECTED_CALL_RPAREN);

        enum rettype type;
        if(is_lfunc)
        	type = descr->ret_type;
        else
        	type = fcall->defn.function.ret_type;

       
        if(is_lfunc && type == RET_FLOAT)
            emit_lib_call_st0_to_eax();

        regs = temp_regs;     // restore regs structure before function call code
        if(type == RET_FLOAT)
            d_regs->in_fpu_mode = 1; //true
        
        if(type == RET_CHAR)
        	print_type = PRINT_CHAR;
        else if(type == RET_PTR)
        	print_type = PRINT_STRING;
        else if(type == RET_FLOAT)
        	print_type = PRINT_FLOAT;
        else
        	print_type = PRINT_INT;
}
