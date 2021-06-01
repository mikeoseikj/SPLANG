#ifndef SCAN_H
#define SCAN_H

#include <string.h>
#include "globals.h"

#define TAB_SIZE    4

// arranged so that index for keyword corresponds to its token id
const char *KEYWORDS[] = {
	"char", "bool", "int", "float", "struct", "def", "if", "else", "for", "do","while",
	"break", "continue", "return", "echo", "input", "sizeof","true", "false", "NULL",  NULL };


char curr_src_file[MAX_FILENAME + 1];
unsigned int line_num = 1, prev_col_num, col_num = 0;
unsigned int beg_lnum, beg_cnum;  // for string and char literal tokens

static int is_keyword(char *word)
{
	for(int i = 0; KEYWORDS[i] != NULL; i++)
	{
		if(strcmp(word, KEYWORDS[i]) == 0)
			return i; // token id 
	}
	return -1; // not keyword
}


char get_byte()
{
	char c = fgetc(in_stream);
	if(c == '\n')
	{
		line_num++;
		prev_col_num = col_num;
		col_num = 0;
		return c;
	}
	if(c == '\t')
		col_num += TAB_SIZE;
	else if(c != EOF)
		col_num++;
	return c;
}

void put_byte(char c)
{
	if(c == EOF)	
		return;

	set_file_pos(get_file_pos()-1);
	if(c  == '\n')
	{
		line_num--;
		col_num = prev_col_num;
		return;
	}
    
    if(c == '\t')
    	col_num -= TAB_SIZE;
    else if(c != EOF)
		col_num--;
}

long get_file_pos()
{
	int pos = ftell(in_stream);
	if(pos < 0)
	{
		perror("error");
		exit(-1);
	}
	return pos;
}

void set_file_pos(long pos)
{
	if(fseek(in_stream, pos, SEEK_SET) < 0)
	{
		perror("error");
		exit(-1);
	}
}

#endif