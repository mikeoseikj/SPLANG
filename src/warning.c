#include <stdio.h>
#include "include/warning.h"
#include "include/globals.h"

const char *warn_msg[] = {
	[EXCESS_INIT_ELEMENTS]   "excess elements in array initilizer",
	[STRING_TOO_LARGE]       "string literal is longer than the buffer",
	[TOO_MANY_MAIN_ARGS]     "main function expects a maximum of 3 args, main(int argc, char *argv[], char *envp[])"
};

void warning(int warn_code)
{
	fprintf(stderr, "%s:%d:%d: warning: %s\n", curr_src_file, line_num, col_num, warn_msg[warn_code]);
}