#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "symtab.h"

unsigned int IEEE_754_to_float_convert(float value);  
float IEEE_754_to_int_convert(unsigned int value);
int  is_datatype(int id);
int  valid_operator(int op);
void *alloc_mem(int size);
void append_array_element(struct symbol *sym, int element);
void append_array_string_element(struct symbol *sym, char *string);

void create_rodata_segment_string(char *string);
void open_source_file(char *filename);
void create_output_file(char *file);
void emitcode(char *fmt, ...);
void emit(char *fmt, ...);
void emit_label(char *fmt, ...);
void emit_stmt_label(char *label);

void create_rodata_section();
void create_data_section();
void create_bss_section();
void prepare_global_arrays();
void extern_all_lib_funcs();
void write_main_entry();

char *new_if_beg_label();
char *new_if_end_label();
char *new_else_beg_label();
char *new_else_end_label();
char *new_while_beg_label();
char *new_while_end_label();
char *new_for_beg_label();
char *floop_label();
char *fcnt_label();
char *new_for_end_label();
char *new_do_label();
char *new_dwhile_beg_label();
char *new_dwhile_end_label();
void reset_stmt_label_counters();

long f_tell(FILE *stream);
void f_seek(FILE *stream, long offset, int whence);
void f_read(char *ptr, int size, FILE *stream);

#endif 
