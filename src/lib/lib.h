#ifndef LIB_H
#define LIB_H

struct alloca_symbol 
{
	struct alloca_symbol *prev, *next;
	char *address;
} *alloca_symbol_list = NULL;

void alloca_enqueue(struct alloca_symbol **queue, struct alloca_symbol *sym)
{
	struct alloca_symbol *s = *queue;

	if(s == NULL)
	{
		*queue = sym;
		sym->prev = NULL;
		sym->next = NULL;
		return;
	}

	while(s->next)
		s = s->next;

	s->next = sym;
	sym->prev = s;
	sym->next = NULL;
}

#endif