#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/symtab.h"
#include "include/error.h"
#include "include/globals.h"
#include "include/utils.h"

struct symbol *gvar_list = NULL, *func_list = NULL, *curr_func = NULL;
struct struct_defn_symbol *struct_defn_list = NULL;

struct stmt_symbol *curr_stmt = NULL;
struct rodata_segstr *rodata_str_list = NULL;

struct symbol *symbol_lookup(struct symbol **list, char *name)
{
	struct symbol *sym = *list;

	if(sym == NULL)
		return NULL;	//false

	while(sym)
	{
		if(strcmp(sym->name, name) == 0)
			return sym;	//true

		sym = sym->next;		
	}
	return NULL;
}

struct symbol *symbol_stmt_lookup(char *var_name, int *scope)
{
	struct symbol *var;
	struct stmt_symbol *stmt_node = curr_stmt;
	if(stmt_node)
	{
		var = symbol_lookup(&stmt_node->locals, var_name);
		if(var)
		{
			*scope = STMT_SCOPE;
			return var;
		}	
	}
	return NULL;
}

struct symbol *symbol_scope_lookup(char *var_name, int *scope)
{
	// search through all statements within scope
	struct symbol *var;
	struct stmt_symbol *stmt_node = curr_stmt;
	while(stmt_node)
	{
		var = symbol_lookup(&stmt_node->locals, var_name);
		if(var)
		{
			*scope = LOCAL_SCOPE;
			return var;
		}	
		stmt_node = stmt_node->parent;
	}

	var = symbol_lookup(&curr_func->defn.function.params, var_name);
	if(var)
	{		
		*scope = LOCAL_SCOPE;
		return var;
	}

	var = symbol_lookup(&curr_func->defn.function.lvars, var_name);
	if(var)
	{
		*scope = LOCAL_SCOPE;
		return var;
	}


	if(var = symbol_lookup(&gvar_list, var_name))
	{
		*scope = GLOBAL_SCOPE;
		return var;
	}

	return NULL;  // not found;
}

struct symbol *alloc_sym()
{
	struct symbol *ptr = alloc_mem(sizeof(struct symbol));
	return ptr;
}

struct struct_defn_symbol *alloc_struct_sym()
{
	struct struct_defn_symbol *ptr = alloc_mem(sizeof(struct struct_defn_symbol));
	return ptr;
}


void create_stmt_scope()
{
	struct stmt_symbol *stmt = alloc_mem(sizeof(struct stmt_symbol));
	stmt->parent = curr_stmt;
	curr_stmt = stmt;

	if(curr_func->defn.function.lvars || (curr_stmt->parent && curr_stmt->parent->locals))
	{
		struct symbol *last, *placeholder = alloc_sym();
		placeholder->name = alloc_mem(1);
		strcpy(placeholder->name, "");
		enqueue(&curr_stmt->locals, placeholder);
		 
        if(curr_func->defn.function.lvars)
			last = curr_func->defn.function.lvars;
        else
			last = curr_stmt->parent->locals;

		while(last->next)
			last = last->next;

		// relevant information calculate first stack offset in statement
		placeholder->stack.offset = last->stack.offset;
		placeholder->type = last->type;
		placeholder->status.is_array = last->status.is_array;
		placeholder->status.is_ptr_count = last->status.is_ptr_count;
		placeholder->defn.array.size = last->defn.array.size;
		placeholder->defn.structure.defn_ptr = last->defn.structure.defn_ptr;

	}
	else
	{
		curr_stmt->locals = NULL;
	}

}

void delete_stmt_scope(struct stmt_symbol *stmt)
{
	struct stmt_symbol *temp = stmt;
	curr_stmt = curr_stmt->parent;
	if(curr_stmt)
		curr_stmt->locals = curr_stmt->locals;

	free(temp);
}

struct struct_defn_symbol *struct_defn_lookup(struct struct_defn_symbol **list, char *name)
{
	struct struct_defn_symbol *sym = *list;

	if(sym == NULL)
		return NULL;	//false

	while(sym)
	{
		if(strcmp(sym->name, name) == 0)
			return sym;	//true

		sym = sym->next;		
	}
	return NULL;
}

void create_struct_defn()
{
	struct struct_defn_symbol *struct_defn = alloc_struct_sym();
	get_token();

	if(curr_token.id == LT_IDENTIFIER || curr_token.id == LBRACE)
	{
		struct_defn->name = alloc_mem(MAX_IDEN_LEN+1);
		strcpy(struct_defn->name, "");
		if(curr_token.id == LT_IDENTIFIER)
		{
			if(symbol_lookup(&gvar_list, curr_token.lexeme) || symbol_lookup(&func_list, curr_token.lexeme) || struct_defn_lookup(&struct_defn_list, curr_token.lexeme))
				error(PREVIOUSLY_DECLARED);

			strcpy(struct_defn->name, curr_token.lexeme);
			get_token();
		}
	
		struct_defn->size = 0;
		struct_defn->members = NULL;
		struct_enqueue(&struct_defn_list, struct_defn);

		if(curr_token.id == LBRACE)
		{
			int type;
			struct symbol *member;
			struct struct_defn_symbol *type_struct_defn = NULL;
			get_token();
			while(curr_token.id != RBRACE && curr_token.id != END_OF_FILE)
			{
				type_struct_defn = struct_defn_lookup(&struct_defn_list, curr_token.lexeme);
				if(! is_datatype(curr_token.id) && !type_struct_defn)
					error(UNKNOWN_TYPENAME);

				type = curr_token.id;
				do
				{
					member = alloc_sym();
					member->type = type;
					if(type_struct_defn)
					{
						member->type = STRUCT;
						member->defn.structure.defn_ptr = type_struct_defn;
					}
					member->name = alloc_mem(MAX_IDEN_LEN+1);
						
					get_token();
					while(curr_token.id == STAR)
					{
						member->status.is_ptr_count++;
						get_token();
					}

					if(curr_token.id != LT_IDENTIFIER)
						error(EXPECTED_IDENTIFIER);

					if(symbol_lookup(&struct_defn->members, curr_token.lexeme))
						error(STRUCT_MEMBER_EXISTS);
			
                    strcpy(member->name, curr_token.lexeme);
					enqueue(&struct_defn->members, member);

					get_token();

					if(curr_token.id == LBRACKET)
					{
						float size = eval_expr();
						if(size <= 0)
							error(WRONG_ARRAY_SIZE);

						member->status.is_array = 1;
						member->defn.array.size =  (unsigned int)size;

						if(curr_token.id != RBRACKET)
							error(UNCLOSED_ARRAY);

						get_token();
					}

                    member->stack.offset = struct_defn->size;	// In this case, offset refers to the offset from the start of the structure in memory
					// for computing the size of the structure
					if(member->status.is_array)
					{
						if((member->type == CHAR || member->type == BOOL) && !member->status.is_ptr_count)
							struct_defn->size += member->defn.array.size;
						else if(member->type == STRUCT && !member->status.is_ptr_count)
							struct_defn->size += (member->defn.structure.defn_ptr->size * member->defn.array.size);
						else
							struct_defn->size += (4 * member->defn.array.size);
					}
					else if(member->type == STRUCT)
					{
						if(member->status.is_ptr_count)
							struct_defn->size += 4;
						else
							struct_defn->size += member->defn.structure.defn_ptr->size;
					}
					else 
					{
						if((member->type == CHAR || member->type == BOOL) && !member->status.is_ptr_count)
							struct_defn->size++;
						else
							struct_defn->size += 4;
					}

				}while(curr_token.id == COMMA);

	
				if(curr_token.id != SEMICOLON)
					error(EXPECTED_SEMICOLON);

				get_token();
			}
			
			// round memory size of struct definition
			int rem = struct_defn->size % 4;
			if(rem) struct_defn->size  += (4 - rem);

			if(curr_token.id != RBRACE)
				error(EXPECTED_STRUCT_RBRACE);

            // For variables being declared at the end of structure definition
			struct symbol *gvar;
			get_token();
			if(curr_token.id != SEMICOLON && (curr_token.id == STAR || curr_token.id == LT_IDENTIFIER))
			{
				put_token();
				do
				{
					get_token();
					gvar = alloc_sym();
					while(curr_token.id == STAR)
					{
						gvar->status.is_ptr_count++;
						get_token();
					}

					if(curr_token.id != LT_IDENTIFIER)
						error(EXPECTED_SEMICOLON);
                       
					if(symbol_lookup(&gvar_list, curr_token.lexeme) || symbol_lookup(&func_list, curr_token.lexeme) || struct_defn_lookup(&struct_defn_list, curr_token.lexeme))
						error(PREVIOUSLY_DECLARED);
						
					gvar->type = STRUCT;
					gvar->defn.structure.defn_ptr = struct_defn;
					gvar->name = alloc_mem(MAX_IDEN_LEN + 1);
					strcpy(gvar->name, curr_token.lexeme);
					enqueue(&gvar_list, gvar);
						
					get_token();
					if(curr_token.id == LBRACKET)
					{
						float size = eval_expr();
						if(size <= 0)
							error(WRONG_ARRAY_SIZE);

						gvar->status.is_array = 1;
						gvar->defn.array.size = (unsigned int)size;

						if(curr_token.id != RBRACKET)
							error(UNCLOSED_ARRAY);

						get_token();
					}
				}while(curr_token.id == COMMA);
			}
            if(curr_token.id != SEMICOLON)
                error(EXPECTED_STRUCT_SEMICOLON);
		}
		else
		{
			error(EXPECTED_STRUCT_LBRACE);
		}
	}
	else
	{
		error(EXPECTED_STRUCT_LBRACE);
	}
		
}

struct symbol *find_struct_member(struct symbol *members, char *name)
{
	struct symbol *member = members;
	while(member)
	{
		if(strcmp(member->name, name) == 0)
			return member;
		member = member->next;
	}
	return NULL;
}

void symbol_decl_global()
{
	struct symbol *gvar;

	struct struct_defn_symbol *type_struct_defn = struct_defn_lookup(&struct_defn_list, curr_token.lexeme);
	if(! is_datatype(curr_token.id) && curr_token.id != STRUCT && !type_struct_defn)
			error(UNKNOWN_TYPENAME);
	int type_id = curr_token.id;

	if(curr_token.id == STRUCT)
	{
		in_global_decls = 0; //false
		create_struct_defn();
		return;
	}
	do
	{
		get_token();
		gvar = alloc_sym();

        // check if it is pointer
		while(curr_token.id == STAR)
		{
			gvar->status.is_ptr_count++;
			get_token();
		}

		if(curr_token.id != LT_IDENTIFIER)
			error(EXPECTED_IDENTIFIER);
		                                               	
		if(symbol_lookup(&gvar_list, curr_token.lexeme) || symbol_lookup(&func_list, curr_token.lexeme) || struct_defn_lookup(&struct_defn_list, curr_token.lexeme))
			error(PREVIOUSLY_DECLARED);

		gvar->name = alloc_mem(MAX_IDEN_LEN + 1);
		gvar->type = type_id;
		if(type_struct_defn)
		{
			gvar->type = STRUCT;
			gvar->defn.structure.defn_ptr = type_struct_defn;
		}
		strncpy(gvar->name, curr_token.lexeme, MAX_IDEN_LEN);
		enqueue(&gvar_list, gvar);

		get_token();
		if(curr_token.id == ASSIGN)
		{
            // if it is a string / char *
			if(gvar->type == CHAR && gvar->status.is_ptr_count)
			{
				get_token();
				if(curr_token.id == LT_STRING)
				{
					gvar->defn.variable.is_string_assigned = 1;
					gvar->status.is_initialized = 1;
					int len = strlen(curr_token.lexeme);
					gvar->defn.variable.value.pointer = alloc_mem(len+1);
					strcpy(gvar->defn.variable.value.pointer, curr_token.lexeme);
					get_token();
					continue;
				}
				put_token();
			}

			float ans = eval_expr();
			if(gvar->status.is_ptr_count)
			{
				if(gvar->type == BOOL)
					gvar->defn.variable.value.pointer = ((int)ans == 0)? 0 : 1;
				else 
					gvar->defn.variable.value.pointer = (unsigned int)ans;
				gvar->status.is_initialized = 1;
			}
			else if(type_id == CHAR)
			{
				gvar->defn.variable.value.character = (char)ans;
				gvar->status.is_initialized = 1;
			}
			else if(type_id == FLOAT)
			{	
				gvar->defn.variable.value.real = ans;
				gvar->status.is_initialized = 1; 
					
			}
			else if(type_id == BOOL)
			{
				gvar->defn.variable.value.boolean =  ((int)ans == 0)? 0 : 1;
				gvar->status.is_initialized = 1;
			}
			else
			{
				gvar->defn.variable.value.integer = (int)ans;
				gvar->status.is_initialized = 1; 			
			}
		}
		else if(curr_token.id == LBRACKET)
		{
			gvar->status.is_array = 1;	
			get_token();

			if(curr_token.id == RBRACKET)	// check if no parameter was passed to array
			{
				get_token();
				goto ASSIGN_TO_ARRAY;
			}
			put_token();

			unsigned int size = eval_expr();
			gvar->defn.array.size = size;
			
			if(curr_token.id != RBRACKET)
				error(UNCLOSED_ARRAY);
			get_token();

          	ASSIGN_TO_ARRAY:
				if(curr_token.id == ASSIGN)
				{
					get_token();

                	// string, eg: char buf[] = "testing"
					if(gvar->type == CHAR && gvar->status.is_ptr_count == 0 && curr_token.id == LT_STRING)
					{
						gvar->status.is_initialized = 1;
						gvar->defn.variable.is_string_assigned = 1;
						int len = (gvar->defn.array.size > strlen(curr_token.lexeme))? gvar->defn.array.size : strlen(curr_token.lexeme);
						gvar->defn.array.size = len;
						gvar->defn.variable.value.pointer = alloc_mem(len+1);
						strncpy(gvar->defn.variable.value.pointer, curr_token.lexeme, len);
						get_token();
						continue;
					}
					if(curr_token.id != LBRACE)
						error(MISSING_ARRAY_LBRACE);

					gvar->status.is_initialized = 1; 
					do
					{
						// if it is a string / char *
						if(type_id == CHAR && gvar->status.is_ptr_count)
						{
							get_token();
							if(curr_token.id == LT_STRING)
							{
								append_array_string_element(gvar, curr_token.lexeme);
								get_token();
								continue;
							}
							put_token();
						}
						float val = eval_expr();
						append_array_element(gvar, val);
					}while(curr_token.id == COMMA);

					if(curr_token.id != RBRACE)
						error(MISSING_ARRAY_RBRACE);
					get_token();
			    }

				if(gvar->defn.array.size <= 0)
				error(WRONG_ARRAY_SIZE);	
		}

	}while(curr_token.id == COMMA);

	if(curr_token.id != SEMICOLON)
		error(EXPECTED_SEMICOLON);
}


void symbol_decl_func()
{
	struct symbol *func, *param;
	int brace_count = 0;
	
	get_token();
	if(curr_token.id != FUNCALL)
			error(EXPECTED_IDENTIFIER);
		                                               	
	if(symbol_lookup(&gvar_list, curr_token.lexeme) || symbol_lookup(&func_list, curr_token.lexeme) || struct_defn_lookup(&struct_defn_list, curr_token.lexeme))
		error(PREVIOUSLY_DECLARED);

    func = alloc_sym();
	func->type = DEF;
	func->name = alloc_mem(MAX_IDEN_LEN + 1);
	func->defn.function.num_params = 0;
	strncpy(func->name, curr_token.lexeme, MAX_IDEN_LEN);
    enqueue(&func_list, func);

	get_token();
	if(curr_token.id != LPAREN)
		error(EXPECTED_FUNC_LPAREN);

	get_token();
	if(curr_token.id == RPAREN)
		goto done;
	else
		put_token();

	struct struct_defn_symbol *type_struct_defn = NULL;	
    do 
    {
    	get_token();
    	type_struct_defn = struct_defn_lookup(&struct_defn_list, curr_token.lexeme);
 		if(! is_datatype(curr_token.id) && !type_struct_defn)
			error(UNKNOWN_TYPENAME);
		
		int type_id = curr_token.id;
		param = alloc_sym();
		get_token();
		// check if it is pointer
		while(curr_token.id == STAR)
		{
			param->status.is_ptr_count++;
			get_token();
		}

		if(curr_token.id != LT_IDENTIFIER)
			error(EXPECTED_IDENTIFIER);

		if(symbol_lookup(&func->defn.function.params, curr_token.lexeme))
			error(PREVIOUSLY_DECLARED);


		param->type = type_id;
        if(type_struct_defn)
        {
            param->type = STRUCT;
            param->defn.structure.defn_ptr = type_struct_defn;
        }

		param->name = alloc_mem(MAX_IDEN_LEN+1);
		strncpy(param->name, curr_token.lexeme, MAX_IDEN_LEN);
		enqueue(&func->defn.function.params, param);
		func->defn.function.num_params++;

		if(func->defn.function.num_params == 1)
			param->stack.offset = 8;
		else
			param->stack.offset = param->prev->stack.offset + 4;

		get_token();
		if(curr_token.id == LBRACKET)
		{
			param->status.is_array = 1;	//true
			get_token();
			if(curr_token.id == RBRACKET)
			{
				get_token();
				param->defn.array.size = 0;
				continue;
			}
			put_token();

			unsigned int size = eval_expr();
			if(size <= 0)
				error(WRONG_ARRAY_SIZE);
			param->defn.array.size = size;

			if(curr_token.id != RBRACKET)
				error(UNCLOSED_ARRAY);
			get_token();

		}

	}while(curr_token.id == COMMA);

    	done:
    		func->defn.function.ret_type = 0;
    		func->defn.function.file_loc = get_file_pos();
    		func->defn.function.line_num = line_num;
    		func->defn.function.col_num = col_num;
    		get_token();

    		// explicit function return type
    		if(curr_token.id == ARROW)
    		{
    			get_token();
    			if(strcmp("int_t", curr_token.lexeme) == 0)
    				func->defn.function.ret_type = RET_INT;
    			else if(strcmp("float_t", curr_token.lexeme) == 0)
    				func->defn.function.ret_type = RET_FLOAT;
    			else if(strcmp("char_t", curr_token.lexeme) == 0)
    				func->defn.function.ret_type = RET_CHAR;
    			else if(strcmp("ptr_t", curr_token.lexeme) == 0)
    				func->defn.function.ret_type = RET_PTR;
    			else if(strcmp("bool_t", curr_token.lexeme) == 0)
    				func->defn.function.ret_type = RET_BOOL;
    			else
    				error(UNKNOWN_RETTYPE);

    			func->defn.function.file_loc = get_file_pos();
    			func->defn.function.line_num = line_num;
    			func->defn.function.col_num = col_num;
    			get_token();
    		}
    		if(curr_token.id == LBRACE)
    			brace_count++;

    		while(brace_count && curr_token.id != END_OF_FILE)
    		{
    			get_token();
    			if(curr_token.id == LBRACE)
    				brace_count++;
    			if(curr_token.id == RBRACE)
    				brace_count--;
    		}

}


int in_global_decls = 0;
jmp_buf gdecl_jmp_buf;
void scan_all_decls()
{
	get_token();
	do
	{
		setjmp(gdecl_jmp_buf);
		in_global_decls = 0;
		if(curr_token.id == DEF)
		{
			symbol_decl_func();
		}
		else
		{
			in_global_decls = 1;
			symbol_decl_global();
			in_global_decls = 0;
		}
		get_token();
	}while(curr_token.id != END_OF_FILE);
}
