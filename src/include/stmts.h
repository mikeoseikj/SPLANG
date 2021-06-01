#ifndef STMTS_H
#define STMTS_H

#define  NO_PRINT_TYPE  0
#define  PRINT_STRING   1
#define  PRINT_CHAR     2
#define  PRINT_INT      3
#define  PRINT_FLOAT    4

void cgen_if_stmt();
void cgen_while_stmt();
void cgen_do_while_stmt();
void cgen_for_stmt();
void cgen_return_stmt();
void cgen_break_stmt(char *label);
void cgen_continue_stmt(char *label);
void cgen_echo_stmt();
void cgen_input_stmt();

#endif
