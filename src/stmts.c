#include <stdio.h>
#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>
#include "include/globals.h"
#include "include/symtab.h"
#include "include/codegen.h"
#include "include/error.h"
#include "include/utils.h"
#include "include/stmts.h"

char *continue_label = NULL, *break_label = NULL;	// used to determine where to jump to when either break or continue is call within an 'if' or 'else' statement 
int print_type = 0; 	// used by the echo keyword

void cgen_if_stmt()
{
	create_stmt_scope();

	char *beg_label = new_if_beg_label();
	char *end_label = new_if_end_label();
	emit_stmt_label(beg_label);

	cgen_expr();
	emit_zero_cmp_eax();
	emit_store_condition();
	emit_jeq(end_label);
		
	if(curr_token.id == LBRACE)
	{
		get_token();
		while(curr_token.id != RBRACE && curr_token.id != END_OF_FILE)
		{
			if(curr_token.id == RETURN)
			{
				get_token();
				cgen_return_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == BREAK && break_label)
			{
				get_token();
				cgen_break_stmt(break_label);
				get_token();
				continue;
			}
			if(curr_token.id == CONTINUE && continue_label)
			{
				get_token();
				cgen_continue_stmt(continue_label);
				get_token();
				continue;
			}
			if(curr_token.id == IF)
			{
				get_token();
				cgen_if_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == WHILE)
			{
				get_token();
				cgen_while_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == DO)
			{
				get_token();
				cgen_do_while_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == FOR)
			{
				get_token();
				cgen_for_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == ECHO)
			{
				get_token();
				cgen_echo_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == INPUT)
			{
				get_token();
				cgen_input_stmt();
				get_token();
				continue;
			}

			cgen_expr();	
			if(curr_token.id != SEMICOLON)
				error(EXPECTED_SEMICOLON);
			get_token();

		}

		if(curr_token.id != RBRACE)
			error(UNCLOSED_IF_STMT);
	}
	else
	{
		if(curr_token.id == RETURN)
		{
			get_token();
			cgen_return_stmt();
		}
		else if(curr_token.id == BREAK && break_label)
		{
			get_token();
			cgen_break_stmt(break_label);
		}
		else if(curr_token.id == CONTINUE && continue_label)
		{
			get_token();
			cgen_continue_stmt(continue_label);
		}
		else if(curr_token.id == IF)
		{
			get_token();
			cgen_if_stmt();
		}
		else if(curr_token.id == WHILE)
		{
			get_token();
			cgen_while_stmt();
		}
		else if(curr_token.id == DO)
		{
			get_token();
			cgen_do_while_stmt();
		}
		else if(curr_token.id == FOR)
		{
			get_token();
			cgen_for_stmt();
		}
		else if(curr_token.id == ECHO)
		{
			get_token();
			cgen_echo_stmt();
		}
		else if(curr_token.id == INPUT)
		{
			get_token();
			cgen_input_stmt();
		}
		else
		{
			cgen_expr();	
			if(curr_token.id != SEMICOLON)
				error(EXPECTED_SEMICOLON);
		}
	}

	emit_stmt_label(end_label);
	emit_load_condition();			// has to be placed here to remove condition from stack in case the 'if' branch is taken
	delete_stmt_scope(curr_stmt);
	free(beg_label);
	free(end_label);
    
    // Code for else statements
	get_token();
	if(curr_token.id == ELSE)
	{
		create_stmt_scope();
		beg_label = new_else_beg_label();
		end_label = new_else_end_label();
		emit_stmt_label(beg_label);

		// Note: conditon was stored in eax at the end 'if' branch
		emit_zero_cmp_eax();
		emit_jneq(end_label);

		get_token();
		if(curr_token.id == LBRACE)
		{
			get_token();
			while(curr_token.id != RBRACE && curr_token.id != END_OF_FILE)
			{
				if(curr_token.id == RETURN)
				{
					get_token();
					cgen_return_stmt();
					get_token();
					continue;
				}
				if(curr_token.id == BREAK && break_label)
				{
					get_token();
					cgen_break_stmt(break_label);
					get_token();
					continue;
				}
				if(curr_token.id == CONTINUE && continue_label)
				{
					get_token();
					cgen_continue_stmt(continue_label);
					get_token();
					continue;
				}
				if(curr_token.id == IF)
				{
					get_token();
					cgen_if_stmt();
					get_token();
					continue;
				}
				if(curr_token.id == WHILE)
				{
					get_token();
					cgen_while_stmt();
					get_token();
					continue;
				}
				if(curr_token.id == DO)
				{
					get_token();
					cgen_do_while_stmt();
					get_token();
					continue;
				}
				if(curr_token.id == FOR)
				{
					get_token();
					cgen_for_stmt();
					get_token();
					continue;
				}
				if(curr_token.id == ECHO)
				{
					get_token();
					cgen_echo_stmt();
					get_token();
					continue;
				}
				if(curr_token.id == INPUT)
				{
					get_token();
					cgen_input_stmt();
					get_token();
					continue;
				}

				cgen_expr();	
				if(curr_token.id != SEMICOLON)
					error(EXPECTED_SEMICOLON);
				get_token();

			}

			if(curr_token.id != RBRACE)
				error(UNCLOSED_ELSE_STMT);
		}
		else
		{
			if(curr_token.id == RETURN)
			{
				get_token();
				cgen_return_stmt();
			}
			else if(curr_token.id == BREAK && break_label)
			{
				get_token();
				cgen_break_stmt(break_label);
			}
			else if(curr_token.id == CONTINUE && continue_label)
			{
				get_token();
				cgen_continue_stmt(continue_label);
			}
			else if(curr_token.id == IF)
			{
				get_token();
				cgen_if_stmt();
			}
			else if(curr_token.id == WHILE)
			{
				get_token();
				cgen_while_stmt();
			}
			else if(curr_token.id == DO)
			{
				get_token();
				cgen_do_while_stmt();
			}
			else if(curr_token.id == FOR)
			{
				get_token();
				cgen_for_stmt();
			}
			else if(curr_token.id == ECHO)
			{
				get_token();
				cgen_echo_stmt();
			}
			else if(curr_token.id == INPUT)
			{
				get_token();
				cgen_input_stmt();
			}
			else
			{
				cgen_expr();	
				if(curr_token.id != SEMICOLON)
					error(EXPECTED_SEMICOLON);
			}
		}
		emit_stmt_label(end_label);
		delete_stmt_scope(curr_stmt);

		free(beg_label);
		free(end_label);

		return;
	}

	put_token();
   
}


void cgen_while_stmt()
{
	create_stmt_scope();

	char *beg_label = continue_label = new_while_beg_label();
	char *end_label = break_label = new_while_end_label();
	emit_stmt_label(beg_label);

	cgen_expr();
	emit_zero_cmp_eax();
	emit_jeq(end_label);

	if(curr_token.id == LBRACE)
	{
		get_token();
		while(curr_token.id != RBRACE && curr_token.id != END_OF_FILE)
		{
			if(curr_token.id == RETURN)
			{
				get_token();
				cgen_return_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == BREAK)
			{
				get_token();
				cgen_break_stmt(end_label);
				get_token();
				continue;
			}
			if(curr_token.id == CONTINUE)
			{
				get_token();
				cgen_continue_stmt(beg_label);
				get_token();
				continue;
			}
			if(curr_token.id == IF)
			{
				get_token();
				cgen_if_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == WHILE)
			{
				get_token();
				cgen_while_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == DO)
			{
				get_token();
				cgen_do_while_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == FOR)
			{
				get_token();
				cgen_for_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == ECHO)
			{
				get_token();
				cgen_echo_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == INPUT)
			{
				get_token();
				cgen_input_stmt();
				get_token();
				continue;
			}

			cgen_expr();
			if(curr_token.id != SEMICOLON)
				error(EXPECTED_SEMICOLON);

			get_token();
		}
		if(curr_token.id != RBRACE)
			error(UNCLOSED_WHILE_STMT);
	}
	else
	{
		if(curr_token.id == RETURN)
		{
			get_token();
			cgen_return_stmt();
		}
		else if(curr_token.id == BREAK)
		{
			get_token();
			cgen_break_stmt(end_label);
		}
		else if(curr_token.id == CONTINUE)
		{
			get_token();
			cgen_continue_stmt(beg_label);
		}
		else if(curr_token.id == IF)
		{
			get_token();
			cgen_if_stmt();
		}
		else if(curr_token.id == WHILE)
		{
			get_token();
			cgen_while_stmt();
		}
		else if(curr_token.id == DO)
		{
			get_token();
			cgen_do_while_stmt();
		}
		else if(curr_token.id == FOR)
		{
			get_token();
			cgen_for_stmt();
		}
		else if(curr_token.id == ECHO)
		{
			get_token();
			cgen_echo_stmt();
		}
		else if(curr_token.id == INPUT)
		{
			get_token();
			cgen_input_stmt();
		}
		else
		{
			cgen_expr();	
			if(curr_token.id != SEMICOLON)
				error(EXPECTED_SEMICOLON);
		}
	}

	emit_jmp(beg_label);
	emit_stmt_label(end_label);
	delete_stmt_scope(curr_stmt);
	free(beg_label);
	free(end_label);
	continue_label = NULL;
	break_label = NULL;
}


void cgen_do_while_stmt()
{
	create_stmt_scope();
	char *do_label = new_do_label();
	char *beg_label = continue_label = new_dwhile_beg_label();
	char *end_label = break_label = new_dwhile_end_label();

	if(curr_token.id != LBRACE)
		error(EXPECTED_DO_STMT_LBRACE);

    emit_stmt_label(do_label);
	get_token();
	while(curr_token.id != RBRACE && curr_token.id != END_OF_FILE)
	{
		if(curr_token.id == RETURN)
		{
			get_token();
			cgen_return_stmt();
			get_token();
			continue;
		}
		if(curr_token.id == BREAK)
		{
			get_token();
			cgen_break_stmt(end_label);
			get_token();
			continue;
		}
		if(curr_token.id == CONTINUE)
		{
			get_token();
			cgen_continue_stmt(beg_label);
			get_token();
			continue;
		}
		if(curr_token.id == IF)
		{
			get_token();
			cgen_if_stmt();
			get_token();
			continue;
		}
		if(curr_token.id == WHILE)
		{
			get_token();
			cgen_while_stmt();
			get_token();
			continue;
		}
		if(curr_token.id == FOR)
		{
			get_token();
			cgen_for_stmt();
			get_token();
			continue;
		}
		if(curr_token.id == DO)
		{
			get_token();
			cgen_do_while_stmt();
			get_token();
			continue;
		}
		if(curr_token.id == ECHO)
		{
			get_token();
			cgen_echo_stmt();
			get_token();
			continue;
		}
		if(curr_token.id == INPUT)
		{
			get_token();
			cgen_input_stmt();
			get_token();
			continue;
		}

		cgen_expr();
		if(curr_token.id != SEMICOLON)
			error(EXPECTED_SEMICOLON);

		get_token();
	}

	if(curr_token.id != RBRACE)
		error(UNCLOSED_DO_STMT);

	get_token();
	if(curr_token.id != WHILE)
		error(EXPECTED_A_WHILE_STMT);

    emit_stmt_label(beg_label);
    get_token();
	cgen_expr();
	emit_zero_cmp_eax();
	emit_jeq(end_label);
	emit_jmp(do_label);
	emit_stmt_label(end_label);

	delete_stmt_scope(curr_stmt);
	free(do_label);
	free(beg_label);
	free(end_label);
	continue_label = NULL;
	break_label = NULL;

	if(curr_token.id != SEMICOLON)
		error(EXPECTED_SEMICOLON);
}


void cgen_for_stmt()
{
	create_stmt_scope();

	char *beg_label = new_for_beg_label();
	char *loop_label = floop_label();
	char *cnt_label = continue_label = fcnt_label();
	char *end_label = break_label = new_for_end_label();

	int has_paren = 0, is_indefinite_loop = 0;
	char *code_buf;

    // for indefinite loop eg: for(;;) or for ;;
	if(curr_token.id == LPAREN || curr_token.id == TWO_SEMICOLONS)
	{
		if(curr_token.id == LPAREN)
			get_token();
		if(curr_token.id == TWO_SEMICOLONS)
		{
        	if(prev_token.id == LPAREN)
        	{
        		get_token();
        		if(curr_token.id != RPAREN)
        			error(EXPECTED_FOR_RPAREN);
        	}
        	get_token();
			is_indefinite_loop = 1;
		}
		else
		{
			has_paren = 1;
		}
	}


    emit_stmt_label(beg_label);
    if(! is_indefinite_loop)
    {
    	// generate => (i = 0; i < 10; i++)
    	cgen_expr();					// ie: i = 0
    	if(curr_token.id != SEMICOLON)
			error(EXPECTED_FOR_SEMICOLON);
		get_token();

		emit_stmt_label(loop_label);	// ie: i < 10
		cgen_expr();
		emit_zero_cmp_eax();
		emit_jeq(end_label);
    	if(curr_token.id != SEMICOLON)
			error(EXPECTED_FOR_SEMICOLON);
		get_token();

		// Note that the "i++" code should be at the end of the code in the "for"
    	long endpos, begpos = f_tell(out_stream);		
		cgen_expr();
		endpos = f_tell(out_stream);		// calculate the size of the "i++" code
		int size = endpos - begpos;
		code_buf  = alloc_mem(size+1);
	        
        f_seek(out_stream, begpos, SEEK_SET);
		f_read(code_buf, size, out_stream);   // read the "i++" code and "remove it"
		f_seek(out_stream, begpos, SEEK_SET);
    }    

	if(has_paren)
	{
		if(curr_token.id != RPAREN)
			error(EXPECTED_FOR_RPAREN);
		get_token();
	}

	indefinite_loop:
	if(curr_token.id == LBRACE)
	{
		get_token();
		while(curr_token.id != RBRACE && curr_token.id != END_OF_FILE)
		{
			if(curr_token.id == RETURN)
			{
				get_token();
				cgen_return_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == BREAK)
			{
				get_token();
				cgen_break_stmt(end_label);
				get_token();
				continue;
			}
			if(curr_token.id == CONTINUE)
			{
				get_token();
				cgen_continue_stmt(cnt_label);
				get_token();
				continue;
			}
			if(curr_token.id == IF)
			{
				get_token();
				cgen_if_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == WHILE)
			{
				get_token();
				cgen_while_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == DO)
			{
				get_token();
				cgen_do_while_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == FOR)
			{
				get_token();
				cgen_for_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == ECHO)
			{
				get_token();
				cgen_echo_stmt();
				get_token();
				continue;
			}
			if(curr_token.id == INPUT)
			{
				get_token();
				cgen_input_stmt();
				get_token();
				continue;
			}

			cgen_expr();
			if(curr_token.id != SEMICOLON)
				error(EXPECTED_SEMICOLON);

			get_token();
		}
		if(curr_token.id != RBRACE)
			error(UNCLOSED_FOR_STMT);
	}
	else
	{
		if(curr_token.id == RETURN)
		{
			get_token();
			cgen_return_stmt();
		}
		else if(curr_token.id == BREAK)
		{
			get_token();
			cgen_break_stmt(end_label);
		}
		else if(curr_token.id == CONTINUE)
		{
			get_token();
			cgen_continue_stmt(cnt_label);
		}
		else if(curr_token.id == IF)
		{
			get_token();
			cgen_if_stmt();
		}
		else if(curr_token.id == WHILE)
		{
			get_token();
			cgen_while_stmt();
		}
		else if(curr_token.id == DO)
		{
			get_token();
			cgen_do_while_stmt();
		}
		else if(curr_token.id == FOR)
		{
			get_token();
			cgen_for_stmt();
		}
		else if(curr_token.id == ECHO)
		{
			get_token();
			cgen_echo_stmt();
		}
		else if(curr_token.id == INPUT)
		{
			get_token();
			cgen_input_stmt();
		}
		else
		{
			cgen_expr();	
			if(curr_token.id != SEMICOLON)
				error(EXPECTED_SEMICOLON);
		}
	}
	if(!is_indefinite_loop)
	{
		emit_stmt_label(cnt_label);
    	emit(code_buf);		// write back the "i++" code back
    	emit_jmp(loop_label);
	}
	else
	{
		emit_jmp(beg_label);
	}
	emit_stmt_label(end_label);
	delete_stmt_scope(curr_stmt);
	free(beg_label);
	free(cnt_label);
	free(loop_label);
	free(end_label);
	continue_label = NULL;
	break_label = NULL;
}

void cgen_return_stmt()
{
	if(curr_token.id != SEMICOLON)
	{
		if(curr_token.id == LT_STRING)
		{
			emit_return_rodata_string(rodata_curr_seg_off);
            create_rodata_segment_string(curr_token.lexeme);
            get_token();
             
		}
		else
		{
			cgen_expr();
		}
	}
	
	emit_return(curr_func);
	if(curr_token.id != SEMICOLON)
		error(EXPECTED_SEMICOLON);
}

void cgen_break_stmt(char *label)
{
	if(curr_token.id != SEMICOLON)
		error(EXPECTED_SEMICOLON);
	emit_jmp(label);
}

void cgen_continue_stmt(char *label)
{
	if(curr_token.id != SEMICOLON)
		error(EXPECTED_SEMICOLON);
	emit_jmp(label);
}

void cgen_echo_stmt()
{
	int has_paren = 0;
	if(curr_token.id == LPAREN)
		has_paren = 1;
	else
		put_token();

	int explicit = 0, explicit_char = 0, explicit_int = 0, explicit_float = 0, explicit_hex = 0, explicit_string = 0;

	do
	{
		emit_push_all_regs();
		get_token();
		if(curr_token.id == LT_STRING)
    	{
    		emit_rodata_func_string_arg(rodata_curr_seg_off);
        	create_rodata_segment_string(curr_token.lexeme);
        	get_token();
        	emit_push_func_arg();
        	print_type = PRINT_STRING;
    	}
    	else
    	{
    		if(strcmp("int_t", curr_token.lexeme) == 0)
    			explicit_int = 1, explicit = 1;
    		else if(strcmp("char_t", curr_token.lexeme) == 0)
    			explicit_char = 1, explicit = 1;
    		else if(strcmp("float_t", curr_token.lexeme) == 0)
    			explicit_float = 1, explicit = 1;
    		else if(strcmp("hex_t", curr_token.lexeme) == 0)
    			explicit_hex = 1, explicit = 1;
    		else if(strcmp("str_t", curr_token.lexeme) == 0)
    			explicit_string = 1, explicit = 1;

    		if(explicit)
    		{
    			get_token();
    			if(curr_token.id != LPAREN)
    				error(EXPECTED_LPAREN);
    			get_token();
    		}
    		cgen_expr();
    		if((print_type == PRINT_FLOAT && !explicit) || explicit_float)
    			emit_push_func_float_arg();
    		else
    			emit_push_func_arg();
    	}

    	emit_rodata_func_string_arg(rodata_curr_seg_off);
    	emit_push_func_arg();	// format eg: %c

        int size = ((print_type == PRINT_FLOAT && !explicit) || (explicit_float))? 12 : 8;   	
    	if(explicit)
    	{
    		if(explicit_char)
    			create_rodata_segment_string("%c");
    		else if(explicit_int)
    			create_rodata_segment_string("%i");
    		else if(explicit_float)
    			create_rodata_segment_string("%f");
    		else if(explicit_hex)
    			create_rodata_segment_string("%x");
    		else 
    			create_rodata_segment_string("%s");

    		if(curr_token.id != RPAREN)
    			error(EXPECTED_RPAREN);
    		get_token();
    		explicit = 0;
    	}
    	else
    	{
    		if(print_type == PRINT_STRING)
    			create_rodata_segment_string("%s");
    		else if(print_type == PRINT_CHAR)
    			create_rodata_segment_string("%c");
   			else if(print_type == PRINT_FLOAT)
   				create_rodata_segment_string("%f");
    		else
    			create_rodata_segment_string("%i");
    	}
    
    	emit_lib_call("printf"); 
    	emit_add_esp(size);
    	emit_pop_all_regs();

    	explicit = explicit_char = explicit_int = explicit_float = explicit_hex = explicit_string = 0;
	}while(curr_token.id == COMMA);

	if(has_paren && curr_token.id != RPAREN)
		error(EXPECTED_CALL_RPAREN);

    if(has_paren)
		get_token();

	if(curr_token.id != SEMICOLON)
		error(EXPECTED_SEMICOLON);
}

void cgen_input_stmt()
{
	int has_paren = 0;
	if(curr_token.id == LPAREN)
		has_paren = 1;
	else
		put_token();

	char *fmt;
	get_token();
	if(strcmp("int_t", curr_token.lexeme) == 0)
		fmt = "%i";
	else if(strcmp("hex_t", curr_token.lexeme) == 0)
		fmt = "%x";
	else if(strcmp("char_t", curr_token.lexeme) == 0)
		fmt = "%c";
	else if(strcmp("float_t", curr_token.lexeme) == 0)
		fmt = "%f";
	else if(strcmp("str_t", curr_token.lexeme) == 0)
		fmt = "%s";
	else
		error(UNKNOWN_INPUT_TYPE);

	get_token();
	if(curr_token.id != COMMA)
		error(INVALID_CHARACTER);

    get_token();
	cgen_expr();

    emit_push_all_regs();
	emit_push_func_arg();
	emit_rodata_func_string_arg(rodata_curr_seg_off);
	create_rodata_segment_string(fmt);
    emit_push_func_arg();	// format eg: %s
   	emit_lib_call("scanf");
   	emit_add_esp(8);
   	emit_pop_all_regs();
	
   	if(has_paren)
   	{
   		if(curr_token.id != RPAREN)
   			error(EXPECTED_CALL_RPAREN);
   		get_token();
   	}
   	if(curr_token.id != SEMICOLON)
   		error(EXPECTED_SEMICOLON);
}