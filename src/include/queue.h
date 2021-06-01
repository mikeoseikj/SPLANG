#ifndef QUEUE_H
#define QUEUE_H

#include "globals.h"
#include "symtab.h"

void enqueue(struct symbol **queue, struct symbol *sym)
{
	struct symbol *s = *queue;

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

void struct_enqueue(struct struct_defn_symbol **queue, struct struct_defn_symbol *sym)
{
	struct struct_defn_symbol *s = *queue;

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


void str_enqueue(struct rodata_segstr **queue, struct rodata_segstr *sym)
{
	struct rodata_segstr *s = *queue;

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