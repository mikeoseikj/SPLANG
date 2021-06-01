#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "include/error.h"
#include "include/scan.h"
#include "include/symtab.h"
#include "include/utils.h"
#include <ctype.h>

void get_token()
{
	prev_token = curr_token;

	char c = get_byte();
	int count = 0;

	while(isspace(c))
		c = get_byte();

	// code for dealing with comments
	while(c == '/')
	{
		c = get_byte();
		if(c == '/' || c == '*')
		{
			if(c == '*')	// deals with multiline comments
			{
				c = get_byte();
				for(;;)
				{
					char d = get_byte();
					if((c == '*' && d == '/') || c == EOF)
						break;
					c = d;
				}
			}
			else	// deals with single line comments
			{
				while(get_byte() != '\n');
			}
			
			c = get_byte();
			while(isspace(c))
				c = get_byte();
		}
		else
		{
			put_byte(c);
			c = '/';
			break;
		}
	}

	curr_token.filepos = get_file_pos()-1;
	if(isdigit(c))
	{
		int (*is_number)(int c) = &isdigit;
		if(c == '0')
		{
			c = get_byte();
			if(c == 'X' || c == 'x')
			{
				is_number = &isxdigit;
				curr_token.lexeme[count++] = '0';
				curr_token.lexeme[count++] = c;

				char my_x = c;
				c = get_byte();
				if(!is_number(c))
				{
					put_byte(c);
					c = my_x;
				}
			}
			else
			{
				put_byte(c);
				c = '0';
			}
		}

		while(is_number(c))
		{
			curr_token.lexeme[count++] = c;
			c = get_byte();
		}
		if(c == '.')
		{
			curr_token.lexeme[count++] = c;
			c = get_byte();
			while(is_number(c))
			{
				curr_token.lexeme[count++] = c;
				c = get_byte();
			}
			curr_token.id = LT_FLOAT;
			put_byte(c);
			curr_token.lexeme[count] = 0;
			return;

		}
		curr_token.id = LT_INTEGER;
		put_byte(c);
		curr_token.lexeme[count] = 0;
	}
	else if(isalpha(c) || c == '_')
	{
		while(isalnum(c) || c  == '_')
		{
			curr_token.lexeme[count++] = c;
			c = get_byte();
			if(count > MAX_IDEN_LEN)
			{
				error(TOKEN_TOO_LONG);
				exit(-1);
			}
		}
		int is_func = 0;
		put_byte(c);
		if(c == '(')
			is_func = 1;
		
		curr_token.lexeme[count] = 0;
        int id;
		if((id = is_keyword(curr_token.lexeme)) >= 0)
			curr_token.id = id;
		else if(is_func)
			curr_token.id = FUNCALL;
		else
			curr_token.id = LT_IDENTIFIER;

	}
	else if(c == '\'')
	{
		beg_lnum = line_num;
		beg_cnum = col_num;
		c = get_byte();
		if(c == '\\')
		{
			c = get_byte();
			switch(c)
			{
				case '0':
					curr_token.lexeme[count++] = '\0';
					break;
				case 'f':
					curr_token.lexeme[count++] = '\f';
					break;
				case 'r':
					curr_token.lexeme[count++] = '\r';
					break;
				case 'n':
					curr_token.lexeme[count++] = '\n';
					break;
				case 't':
					curr_token.lexeme[count++] = '\t';
					break;
				case 'v':
					curr_token.lexeme[count++] = '\v';
					break;
				case 'b':
					curr_token.lexeme[count++] = '\b';
					break;
				case 'a':
					curr_token.lexeme[count++] = '\a';
					break;
				case '\\':
					curr_token.lexeme[count++] = '\\';
					break;
				case '"':
					curr_token.lexeme[count++] = '\"';
					break;
				case '\'':
					curr_token.lexeme[count++] = '\'';
					break;
				default:
					error(MISSING_CHAR_TERMINATOR);
			}
		}
		else
		{
			curr_token.lexeme[count++] = c;
		}

		if(get_byte() != '\'')
		{
			error(MISSING_CHAR_TERMINATOR);
		}
		else
		{
			curr_token.lexeme[count] = 0;
			curr_token.id = LT_CHARACTER;
		}
	}
	else if(c == '\"')
	{
		beg_lnum = line_num;
		beg_cnum = col_num;

        int extra = 0;		// this is neccesary in the code because all non-printable (isspace()) characters will be converted back if there is an error, check error(int error_code) in 'error.c'
		while(isprint(c))
		{
			c  = get_byte();
			if(c == '\\')
			{
				c = get_byte();
				switch(c)
				{

					case '0':
						curr_token.lexeme[count++] = '\0';
						extra++;
						break;
					case 'f':
						curr_token.lexeme[count++] = '\f';
						extra++;
						break;
					case 'r':
						curr_token.lexeme[count++] = '\r';
						extra++;
						break;
					case 'n':
						curr_token.lexeme[count++] = '\n';
						extra++;
						break;
					case 't':
						curr_token.lexeme[count++] = '\t';
						extra++;
						break;
					case 'v':
						curr_token.lexeme[count++] = '\v';
						extra++;
						break;
					case 'b':
						curr_token.lexeme[count++] = '\b';
						extra++;
						break;
					case 'a':
						curr_token.lexeme[count++] = '\a';
						extra++;
						break;
					case '\\':
						curr_token.lexeme[count++] = '\\';
						extra++;
						break;
					case '"':
						curr_token.lexeme[count++] = '\"';
						extra++;
						break;
					case '\'':
						curr_token.lexeme[count++] = '\'';
						extra++;
						break;
					default:
						error(MISSING_STRING_TERMINATOR);
				}
				continue;
			}

			if(c == '\"')
			{
				curr_token.lexeme[count] = 0;
				curr_token.id = LT_STRING;
				return;
			}
			curr_token.lexeme[count++] = c;
			if((count-extra) > MAX_STRING_LEN)
			{
				error(TOKEN_TOO_LONG);
				exit(-1);
			}
		}

		error(MISSING_STRING_TERMINATOR);
	}


	// Start of single char tokens
	else if(c == '.')
	{
		strcpy(curr_token.lexeme, ".");
		curr_token.id = PERIOD;
	}
	else if(c == '+')
	{
		strcpy(curr_token.lexeme, "+");
		curr_token.id = PLUS;
	}
	else if(c == '*')
	{
		strcpy(curr_token.lexeme, "*");
		curr_token.id = STAR;
	}
	else if(c == '/')
	{
		strcpy(curr_token.lexeme, "/");
		curr_token.id = SLASH;
	}
	else if(c == '%')
	{
		strcpy(curr_token.lexeme, "%");
		curr_token.id = MODULO;
	}
	else if(c == '(')
	{
		strcpy(curr_token.lexeme, "(");
		curr_token.id = LPAREN;
	}
	else if(c == ')')
	{
		strcpy(curr_token.lexeme, ")");
		curr_token.id = RPAREN;
	
	}
	else if(c == '{')
	{
		strcpy(curr_token.lexeme, "{");
		curr_token.id = LBRACE;
	}
	else if(c == '}')
	{
		strcpy(curr_token.lexeme, "}");
		curr_token.id = RBRACE;
	
	}
	else if(c == '[')
	{
		strcpy(curr_token.lexeme, "[");
		curr_token.id = LBRACKET;
	}
	else if(c == ']')
	{
		strcpy(curr_token.lexeme, "]");
		curr_token.id = RBRACKET;
	
	}
	else if(c == ',')
	{
		strcpy(curr_token.lexeme, ",");
		curr_token.id = COMMA;
	}
	else if(c == '^')
	{
		strcpy(curr_token.lexeme, "^");
		curr_token.id = XOR;
	
	}
	else if(c == '~')
	{
		strcpy(curr_token.lexeme, "~");
		curr_token.id = FLIP;
	
	}
	else if(c == ';')
	{
		c = get_byte();
		switch(c)
		{
			case ';':
				strcpy(curr_token.lexeme, ";;");
				curr_token.id = TWO_SEMICOLONS;
				break;
			default:
				strcpy(curr_token.lexeme, ";");
				put_byte(c);
				curr_token.id = SEMICOLON;
		}
	
	}
	else if(c == '-')
	{
		c = get_byte();
		switch(c)
		{
			case '>':
				strcpy(curr_token.lexeme, "->");
				curr_token.id = ARROW;
				break;
			default:
				strcpy(curr_token.lexeme, "-");
				put_byte(c);
				curr_token.id = MINUS;
		}
	}
	else if(c == '=')
	{
		c = get_byte();
		switch(c)
		{
			case '=':
				strcpy(curr_token.lexeme, "==");
				curr_token.id = EQUAL;
				break;
			default:
				strcpy(curr_token.lexeme, "=");
				put_byte(c);
				curr_token.id = ASSIGN;
		}
	}
	else if(c == '&')
	{
		c = get_byte();
		switch(c)
		{
			case '&':
				strcpy(curr_token.lexeme, "&&");
				curr_token.id = LAND;
				break;
			default:
				strcpy(curr_token.lexeme, "&");
				put_byte(c);
				curr_token.id = AND;
		}
	}
	else if(c == '|')
	{
		c = get_byte();
		switch(c)
		{
			case '|':
				strcpy(curr_token.lexeme, "||");
				curr_token.id = LOR;
				break;
			default:
				strcpy(curr_token.lexeme, "|");
				put_byte(c);
				curr_token.id = OR;
		}
	}
	else if(c == '!')
	{
		c = get_byte();
		switch(c)
		{
			case '=':
				strcpy(curr_token.lexeme, "!=");
				curr_token.id = NOT_EQUAL;
				break;
			default:
				strcpy(curr_token.lexeme, "!");
				put_byte(c);
				curr_token.id = NOT;
		}
	}
	else if(c == '<')
	{
		c = get_byte();
		switch(c)
		{
			case '<':
				strcpy(curr_token.lexeme, "<<");
				curr_token.id = LSHIFT;
				break;
			case '=':
				strcpy(curr_token.lexeme, "<=");
				curr_token.id = LESS_EQUAL;
				break;
			default:
				strcpy(curr_token.lexeme, "<");
				put_byte(c);
				curr_token.id = LESS;
		}
	}
	else if(c == '>')
	{
		c = get_byte();
		switch(c)
		{
			case '>':
				strcpy(curr_token.lexeme, ">>");
				curr_token.id = RSHIFT;
				break;
			case '=':
				strcpy(curr_token.lexeme, ">=");
				curr_token.id = GREATER_EQUAL;
				break;
			default:
				strcpy(curr_token.lexeme, ">");
				put_byte(c);
				curr_token.id = GREATER;
		}
	}
	else if(c == EOF)
	{
		strcpy(curr_token.lexeme, "END-OF-FILE");
		curr_token.id = END_OF_FILE;
	}
	else
	{
		curr_token.lexeme[count++] = c;
		curr_token.lexeme[count] = 0;
		error(INVALID_CHARACTER);	
	}

}

void put_token()
{
	// Note: we have to reduce 'line_num' and 'col_num' to the right positions
	int count = get_file_pos() - curr_token.filepos;
	set_file_pos(curr_token.filepos);

	for(int i = 0; i < count; i++)
	{
		char c = fgetc(in_stream);
		if(c  == '\n')
		{
			line_num--;
			col_num = prev_col_num;
			continue;
		}
    
    	if(c == '\t')
    		col_num -= TAB_SIZE;
    	else if(c != EOF)
			col_num--;
	}

	set_file_pos(curr_token.filepos);
	curr_token = prev_token;
}

