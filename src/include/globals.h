#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdio.h>
#include <setjmp.h>

#define TAB_SIZE    4

#define MAX_FILENAME             1024
#define MAX_IDEN_LEN              255
#define MAX_STRING_LEN          65535
#define MAX_ARRAY_INIT_ELEMENTS   256


enum token_id { 
	// LKeywords
	CHAR = 0, BOOL, INT, FLOAT, STRUCT, DEF, IF, ELSE, FOR, DO,
	WHILE, BREAK, CONTINUE, RETURN, ECHO, INPUT, SIZEOF, TRUE, FALSE, NUL,


	PERIOD, PLUS, STAR, SLASH, MODULO, LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET, 
	COMMA, SEMICOLON, TWO_SEMICOLONS, XOR, FLIP,

	MINUS, ARROW, ASSIGN, AND, LAND, OR, LOR, LSHIFT, RSHIFT, NOT, EQUAL, NOT_EQUAL, LESS, GREATER, LESS_EQUAL, 
	GREATER_EQUAL,

	FUNCALL, LT_IDENTIFIER, LT_STRING, LT_CHARACTER, LT_INTEGER, LT_FLOAT, END_OF_FILE };

enum rettype{
	RET_INT = 1, RET_FLOAT, RET_CHAR, RET_PTR, RET_BOOL
};


struct token
{
	char lexeme[MAX_STRING_LEN + 1];
	enum token_id id;
	long filepos;
}prev_token, curr_token;

extern char curr_src_file[MAX_FILENAME + 1];
extern unsigned int line_num, prev_col_num, col_num;
extern unsigned int beg_lnum, beg_cnum;  
extern int error_count;
extern FILE *out_stream;
extern FILE *in_stream;

extern struct symbol *extern_list;
extern struct symbol *gvar_list, *func_list, *curr_func;
extern struct struct_defn_symbol *struct_defn_list;

extern struct stmt_symbol *curr_stmt;
extern struct rodata_segstr *rodata_str_list;

extern struct dregs regs, *d_regs;
extern int rodata_curr_seg_off;
extern int print_type;	// used by the echo keyword
extern int alloca_count;

extern int error_count;
extern jmp_buf gdecl_jmp_buf;		// used 'error.h'
extern int in_global_decls;

char  get_byte();
void  get_token();
void  put_token();

long  get_file_pos();
void  set_file_pos(long pos);

float eval_expr();
void  cgen_expr();
void  scan_all_decls();
void  optimize_code(char *filename);

#endif
