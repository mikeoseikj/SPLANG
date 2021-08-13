#include <stdio.h>
#include <setjmp.h>
#include "include/error.h"
#include "include/globals.h"

const char *error_msgs[] = {
	[MISSING_STRING_TERMINATOR]	    "missing terminating \" character",
	[MISSING_CHAR_TERMINATOR]	    "missing terminating \' character",
	[INVALID_CHARACTER]             "invalid character",
	[UNKNOWN_TYPENAME]              "unknown type name",
	[EXPECTED_IDENTIFIER]           "expected an identifier",
	[EXPECTED_SEMICOLON]            "expected \';\' to terminate line",
	[EXPECTED_CLOSE_BRACKET]        "expected a ] for the array",
	[PREVIOUSLY_DECLARED]           "redeclaration of identifier",
	[EXPECTED_RIGHT_PAREN]          "expected \')\' in expression",
	[INVALID_EXPRESSION]            "unexpected entity the expression",
	[UNCLOSED_ARRAY]                "expected \']\' to terminate the array",
	[MISSING_ARRAY_LBRACE]          "expected \'{\' to begin array initialization",
	[MISSING_ARRAY_RBRACE]          "expected \'}\' to end array initialization",
	[TOO_MANY_INIT_ELEMENTS]        "too many array initialization elements",
	[EXPECTED_STRING_LITERAL]       "trying to assign non-string literal to \'string\' type",
	[EXPECTED_FUNC_LPAREN]          "expected \'(\' for function declaration",
	[EXPECTED_FUNC_RPAREN]          "expected \')\' for function declaration",
	[EXPECTED_FUNC_LBRACE]          "expected \'{\' to begin function definition",
	[EXPECTED_FUNC_RBRACE]          "expected \'}\' to end function definition",
	[LOCAL_PREVIOUSLY_DECLARED]     "redeclaration of local variable",
	[UNKNOWN_VARIABLE]              "trying to use an undeclared variable",
	[NOT_AN_ARRAY]                  "trying to access a non-array as an array",
	[EXPECTED_STRUCT_NAME]          "expected the name of the structure",
	[STRUCT_MEMBER_EXISTS]          "identifier name previously used for a member",
	[EXPECTED_STRUCT_RBRACE]        "expected \'}\' to terminate structure",
	[EXPECTED_STRUCT_SEMICOLON]     "expected \';\' to terminate structure",
	[EXPECTED_STRUCT_LBRACE]        "expected \'{\' to begin structure definition",
	[STRUCT_IS_POINTER]             "is a pointer; should be \'->\' not \'.\'",
	[NOT_A_MEMBER]                  "not a member of the structure",
	[STRUCT_NOT_POINTER]            "is not a pointer; should be \'.\' not \'->\'",
	[MEMBER_NOT_STRUCT]             "is not a structure variable",
	[EXPECTED_EQUAL_SIGN]           "expected an \'=\' or perhaps \'[\' after variable",
	[WRONG_ARRAY_SIZE]              "array size should be greater than zero",
	[ASSIGNING_TO_STRUCT]           "incompatible types; assignment to structure not allowed, assigning to",
	[UNCLOSED_IF_STMT]              "expected \'}\' to end previous if statement",
	[UNCLOSED_ELSE_STMT]            "expected \'}\' to end previous else statement",
	[UNCLOSED_WHILE_STMT]           "expected \'}\' to end previous while statement",
	[UNCLOSED_FOR_STMT]             "expected \'}\' to end previous for statement",
	[EXPECTED_FOR_RPAREN]           "expected \')\' at the the end of the previous for statement",
	[EXPECTED_FOR_SEMICOLON]        "expected \';\' to end expression in for statement",
	[EXPECTED_DO_STMT_LBRACE]       "expected \'{\' to begin previous do stmt",
	[UNCLOSED_DO_STMT]	            "expected \'}\' at the end of previous do stmt",
	[EXPECTED_A_WHILE_STMT]         "expected a while statement at the end of the previous do statement",
	[FLOATING_MODULO_ARITH]         "modulo arithmetic in expressions with floating point numbers not allowed",
	[FLOATING_BITWISE_ARITH]        "bitwise operation in expression with floating point numbers not allowed", 
	[EXPECTED_CALL_LPAREN]          "expected \'(\' for function call",
	[EXPECTED_CALL_RPAREN]          "expected \')\' to end function call",
    [UNKNOWN_INPUT_TYPE]            "input expects \'char\' , \'int\', \'float\' and \'string\' as first argument",
    [UNKNOWN_FUNC_CALL]				"trying to call an undefined function,",
    [EXPECTED_A_COMMA]              "expected a \',\' after argument",
    [NOT_IN_LOOP]                   "must be used in a \'while\' or \'for\' loop",
    [NOT_A_POINTER]                 "trying to dereference a non-pointer",
    [EXCESS_POINTER_DEREFS]         "pointer deference limit exceeded; should have at most,",
    [EXPECTED_LPAREN]               "expected \'(\' at this position ",
    [EXPECTED_RPAREN]               "expected \')\' at this position ",
    [ASSIGNING_TO_ARRAY]            "trying to assign to variable which is an array",
    [UNKNOWN_RETTYPE]			    "unknown function return type specified",
    [TOKEN_TOO_LONG]				"token is too long"
 };


int error_count = 0;
int prev_lnum = 0, prev_cnum = 0;
void error(int error_code)
{
	error_count++;
	if(curr_token.id == LT_STRING)
	{
		int line = line_num, col = col_num;
		int pos = get_file_pos();
		set_file_pos(curr_token.filepos+1);
		char c;
		int count = 0;
		while((c = get_byte()) != '"')
		{
			curr_token.lexeme[count] = c;
			count++;
		}
		curr_token.lexeme[count] = 0;
		set_file_pos(pos);
		line_num = line, col_num = col;
	}

	
	// To avoid repeating errors in the same place (line and column) multiple time
	if((prev_lnum == line_num) && (prev_cnum == col_num))
		return;
	prev_lnum = line_num, prev_cnum = col_num;

    if(error_code == NOT_A_MEMBER)
    	fprintf(stderr, "%s:%d:%d: error: \'%s\' %s \'%s\'\n", curr_src_file, line_num, col_num, curr_token.lexeme, error_msgs[error_code], prev_token.lexeme);
	else if(error_code == STRUCT_IS_POINTER || error_code == STRUCT_NOT_POINTER || error_code == MEMBER_NOT_STRUCT || error_code == NOT_IN_LOOP)
		fprintf(stderr, "%s:%d:%d: error: \'%s\' %s\n", curr_src_file, line_num, col_num, curr_token.lexeme, error_msgs[error_code]);
	else if(error_code == MISSING_STRING_TERMINATOR || error_code == MISSING_CHAR_TERMINATOR)
		fprintf(stderr, "%s:%d:%d: error: %s\n", curr_src_file, beg_lnum, beg_cnum, error_msgs[error_code]);
	else if(error_code == INVALID_CHARACTER || error_code == EXCESS_POINTER_DEREFS || error_code == ASSIGNING_TO_STRUCT || error_code == UNKNOWN_VARIABLE || error_code == STRUCT_MEMBER_EXISTS )
		fprintf(stderr, "%s:%d:%d: error: %s \'%s\'\n", curr_src_file, line_num, col_num, error_msgs[error_code], curr_token.lexeme);	
	else if(error_code == UNKNOWN_TYPENAME || error_code == PREVIOUSLY_DECLARED || error_code == INVALID_EXPRESSION || error_code == LOCAL_PREVIOUSLY_DECLARED || error_code == UNKNOWN_RETTYPE || error_code == UNKNOWN_FUNC_CALL )	// this condition and the above one are the same. They have been splitted for readability
		fprintf(stderr, "%s:%d:%d: error: %s \'%s\'\n", curr_src_file, line_num, col_num, error_msgs[error_code], curr_token.lexeme);
	else if(error_code == TOO_MANY_INIT_ELEMENTS)
		fprintf(stderr, "%s:%d:%d: error: %s, MAX => %d\n", curr_src_file, line_num, col_num, error_msgs[error_code], MAX_ARRAY_INIT_ELEMENTS);
	else		
		fprintf(stderr, "%s:%d:%d: error: %s\n", curr_src_file, line_num, col_num, error_msgs[error_code]);

	if(in_global_decls)
	{
		if(curr_token.id == LBRACE)	// To help produce better and cleaner error messages
		{	
			int braces_count = 0;
			while(curr_token.id != END_OF_FILE)
			{
			
				if(curr_token.id == LBRACE)
					braces_count++;
				else if(curr_token.id == RBRACE)
					braces_count--;
				if(braces_count == 0)
					break;
				get_token();
			}
			get_token();
		}
	
		in_global_decls = 0;
		longjmp(gdecl_jmp_buf, 1);
	}
	if(error_code == EXPECTED_SEMICOLON)
	{
        while(!(curr_token.id <= INPUT) && curr_token.id != SEMICOLON && curr_token.id != END_OF_FILE)
   	        get_token();
	}
}
