#ifndef FUNC_H
#define FUNC_H

#include "globals.h"
#include "symtab.h"

#define MAX_ARGS 3
struct lf_descr
{
	char *name;
	int  nparams;
	enum rettype ret_type;
	int param_types[MAX_ARGS];
};


void cgen_func_block(struct symbol *fsym);
void cgen_func_call(struct token func_token);


#endif