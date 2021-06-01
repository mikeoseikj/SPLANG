#ifndef SYMTAB_H
#define SYMTAB_H

#include "globals.h"
#include <string.h>
   
#define  GLOBAL_SCOPE  1
#define  LOCAL_SCOPE   2
#define  STMT_SCOPE    3


struct struct_defn_symbol
{
	struct struct_defn_symbol *prev, *next;
	char *name;
	unsigned int size;
	struct symbol *members;
};

struct stmt_symbol
{
	struct stmt_symbol *parent;
	struct symbol  *locals;
};

struct symbol
{
	struct symbol *prev, *next;
	unsigned int type;
	char *name;

	struct
	{
		int offset;
	}stack;

	struct
    {
    	int is_array;
    	int is_initialized;
    	int is_ptr_count;
    		
    }status;


    struct
    {
    	struct
    	{ 
    		struct struct_defn_symbol *defn_ptr;
    	}structure;
		
		struct
		{
			unsigned int size;
			struct
			{
				char *string; 
				int  value;
			}elements[MAX_ARRAY_INIT_ELEMENTS];
			unsigned int ecount;
		}array;

   	    struct
        {
        	int is_string_assigned;
			union	// many of the variables in the union is 'unnecessary' but they are used to improve code readability
			{
				char *pointer;
				char character;
				unsigned int integer;	// range is 0x0 => 0xFFFFFFFF
				int  boolean;
				float real;
			}value;
		}variable;

		struct
		{
			enum rettype ret_type;
			int stacksize;
			struct symbol *params;
			struct symbol *lvars;
			struct stmt_symbol *stmts;
			long   file_loc;
			int num_params;
			unsigned int line_num, col_num;
		}function;
	}defn;
	
};

struct rodata_segstr
{
	struct rodata_segstr *prev, *next; 
	char *string;

};


struct symbol *symbol_lookup(struct symbol **list, char *name);
struct symbol *symbol_stmt_lookup(char *var_name, int *scope);
struct symbol *symbol_scope_lookup(char *var_name, int *scope);
struct symbol *alloc_sym();
void   create_stmt_scope();
void   delete_stmt_scope(struct stmt_symbol *stmt);
struct struct_defn_symbol *alloc_struct_sym();
struct struct_defn_symbol *struct_defn_lookup(struct struct_defn_symbol **list, char *name);
struct symbol *find_struct_member(struct symbol *members, char *name);

#endif
