#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include "include/codegen.h"
#include "include/symtab.h"
#include "include/globals.h"
#include "include/error.h"
#include "include/utils.h"

int rodata_curr_seg_off = 0;

union
{
    float value;
    unsigned int converted;
} ieee754_float;

union
{
    unsigned int value;
    float converted;
} ieee754_int;


unsigned int IEEE_754_to_float_convert(float value)  
{
    ieee754_float.value = value;
    return ieee754_float.converted; 
}

float IEEE_754_to_int_convert(unsigned int value)  // used in optimization
{
    ieee754_int.value = value;
    return ieee754_int.converted; 
}

int is_datatype(int id)
{
	if(id == CHAR || id == BOOL || id == INT || id == FLOAT)
		return 1;
	
    return 0;
}

int valid_operator(int op)
{
    if(op == PLUS || op == MINUS || op == STAR || op == SLASH || op == MODULO)
        return 1;   
    if(op == LAND || op == LOR || op == XOR || op == AND || op == OR || op == LSHIFT || op == RSHIFT)
        return 1; 
    if(op == GREATER || op == LESS|| op == GREATER_EQUAL || op == LESS_EQUAL || op == EQUAL || op == NOT_EQUAL)
        return 1; 

    return 0;   //false
}

void *alloc_mem(int size)
{
	void *ptr = malloc(size);
	if(ptr  == NULL)
	{
		perror("error");
		exit(-1);
	}
    memset((char *)ptr, 0, size); // help set all structure fields to zero (used by alloc_sym and others)
	return ptr;
}
void append_array_element(struct symbol *sym, int element)
{
    if(sym->type == BOOL)
        element = (element == 0)? 0 : 1;
	unsigned int index = sym->defn.array.ecount;
	if(index >= MAX_ARRAY_INIT_ELEMENTS)
		error(TOO_MANY_INIT_ELEMENTS);

	sym->defn.array.elements[index].value = element;
	sym->defn.array.ecount++;

	if(sym->defn.array.size < sym->defn.array.ecount)
		sym->defn.array.size++;
}

void append_array_string_element(struct symbol *sym, char *string)
{
	unsigned int index = sym->defn.array.ecount;
	if(index >= MAX_ARRAY_INIT_ELEMENTS)
		error(TOO_MANY_INIT_ELEMENTS);
    
    char *ptr = sym->defn.array.elements[index].string = alloc_mem(strlen(string));
    strcpy(ptr, string);

	sym->defn.array.ecount++;

	if(sym->defn.array.size < sym->defn.array.ecount)
		sym->defn.array.size++;
}


void create_rodata_segment_string(char *string)
{
	int len = strlen(string);
    struct rodata_segstr *str = alloc_mem(sizeof(struct rodata_segstr));
    str->string = alloc_mem(len + 1);
    strcpy(str->string, string);
    str_enqueue(&rodata_str_list, str);

    rodata_curr_seg_off += len;
    rodata_curr_seg_off += 1;  // null will be appended to all strings
}


FILE *out_stream;
void create_output_file(char *file)
{
    out_stream = fopen(file, "w+");
    if(out_stream == NULL)
    {
        perror("error");
        exit(-1);
    }
}

FILE *in_stream;
void open_source_file(char *filename)
{
    in_stream = fopen(filename, "r+");
    if(in_stream == NULL)
    {
        perror("error - source file");
        exit(-1);
    }
    strncpy(curr_src_file, filename, MAX_FILENAME);
}


void emitcode(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    if(fputc('\t', out_stream ) == EOF)
        goto error;

    if(vfprintf(out_stream, fmt, ap) < 0)
    {
        error:
            fprintf(stderr, "error: emitting assembly code\n");
            exit(-1);
    }
    va_end(ap);
    if(fputc('\n', out_stream ) == EOF)
        goto error;
}

void emit(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    if(vfprintf(out_stream, fmt, ap) < 0)
    {
        error:
            fprintf(stderr, "error: emitting assembly code\n");
            exit(-1);
    }
    va_end(ap);   
}


void emit_label(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    if(vfprintf(out_stream, fmt, ap) < 0)
    {
        error:
            fprintf(stderr, "error: emitting assembly code (label)\n");
            exit(-1);
    }
    va_end(ap);
    if(fputc('\n', out_stream ) == EOF)
        goto error;
}

void emit_stmt_label(char *label)
{
    if(fprintf(out_stream, "  %s:\n", label) < 0)
    {
        fprintf(stderr, "error: emitting assembly code (stmt label)\n");
        exit(-1);
    }
}

void create_rodata_section()
{
    struct rodata_segstr *list = rodata_str_list;
    char *str;

    int has = 0;
    if(list)
    {
        has = 1; //true
        emit_label("section .rodata");
        emit("char: db ");
    }
    while(list)
    {
        str = list->string;
        int len = strlen(str);
        for(int i = 0; i < len; i++)
            emit("0x%x, ", str[i]);

        emit("0x0, "); // null terminate string
        list = list->next;
    }
    if(has)
        emit("0x0\n");
}

void create_data_section()
{
    struct symbol *gvar = gvar_list;
    if(gvar)
        emit_label("section .data");

    while(gvar)
    {
        if(gvar->status.is_initialized)
        {
            if(gvar->status.is_array)
            {
                if((gvar->type == CHAR || gvar->type == BOOL) && !gvar->status.is_ptr_count)  // char buf[] but not char *buf[]
                {
                    emit("%s: db 0, 0, 0, 0, ", gvar->name);  // '0s' => pointer to address

                    if(gvar->defn.variable.is_string_assigned)  // ie, char buf[] = "my string"
                    {
                        int rem = gvar->defn.array.size - strlen(gvar->defn.variable.value.pointer);
                        for(int i = 0; i < strlen(gvar->defn.variable.value.pointer); i++)
                            emit("0x%x, ", gvar->defn.variable.value.pointer[i]);
                        for(int i = 0; i < rem; i++)
                            emit("0x0, ");
                        emit("0x0\n");
                    }
                    else
                    {
                        int i, size = gvar->defn.array.size;
                        for(i = 0; i < (size - 1); i++)     // size -1 => because of ',' values
                        {
                            if(!gvar->defn.array.elements[i].string)
                                emit("%d, ", gvar->defn.array.elements[i].value);
                            else
                                emit("0, ");
                        }
                        if(!gvar->defn.array.elements[i].string)
                            emit("%d\n", gvar->defn.array.elements[i].value);
                        else
                            emit("0\n");
                    }
                }
                else
                {
                    emit("%s: dd 0, ", gvar->name);  // '1st 0' => pointer to address
                    int size = gvar->defn.array.size;

                    int i;
                    for(i = 0; i < (size - 1); i++)     // size -1 => because of ',' values
                    {
                        if(!gvar->defn.array.elements[i].string)
                            emit("%d, ", gvar->defn.array.elements[i].value);
                        else
                            emit("0, ");
                    }
                    if(!gvar->defn.array.elements[i].string)
                        emit("%d\n", gvar->defn.array.elements[i].value);
                    else 
                        emit("0\n");
                }
            
            }
            else
            {
                if(gvar->defn.variable.is_string_assigned)  // char *
                    emitcode("%s: dd 0x0", gvar->name);
                else if(gvar->status.is_ptr_count)
                    emitcode("%s: dd 0x%x", gvar->name, gvar->defn.variable.value.pointer);
                else if(gvar->type == CHAR)
                    emitcode("%s: db \'%c\'", gvar->name, gvar->defn.variable.value.character);
                else if(gvar->type == FLOAT)
                    emitcode("%s: dq %f", gvar->name, gvar->defn.variable.value.real);
                else if(gvar->type == BOOL)
                    emitcode("%s: db %d", gvar->name, gvar->defn.variable.value.boolean);
                else 
                    emitcode("%s: dd %d", gvar->name, gvar->defn.variable.value.integer);
                

            }
        }
        gvar = gvar->next;
    }

}


void create_bss_section()
{
    struct symbol *gvar = gvar_list;
    if(gvar)
        emit_label("section .bss");

    while(gvar)
    {
        if(!gvar->status.is_initialized)
        {
            if(gvar->status.is_array)
            {
                if((gvar->type == CHAR || gvar->type == BOOL || gvar->type == STRUCT) && !gvar->status.is_ptr_count)
                {
                    int size = gvar->defn.array.size;
                    if(gvar->type == STRUCT)
                        size = size * gvar->defn.structure.defn_ptr->size;
                    emitcode("%s: resb %d", gvar->name, size+4); // +4byte
                }
                else
                {
                    int size = gvar->defn.array.size;
                    emitcode("%s: resd %d", gvar->name, size+1);
                }
            
            }
            else
            {
                if(gvar->type == STRUCT)
                    emitcode("%s: resb %d", gvar->name, gvar->defn.structure.defn_ptr->size+4); // +4byte
                else if(gvar->type == FLOAT)
                    emitcode("%s: resq 1", gvar->name);
                else if(gvar->status.is_ptr_count)
                    emitcode("%s: resd 1", gvar->name);
                else if(gvar->type == CHAR || gvar->type == BOOL)
                    emitcode("%s: resb 1", gvar->name);
                else
                    emitcode("%s: resd 1", gvar->name);
            }
        }
        gvar = gvar->next;
    }

}

void prepare_global_arrays()
{
    struct symbol *gvar = gvar_list;
    if(! gvar)
        return;

    while(gvar)
    {
        if(gvar->status.is_array || (gvar->type == STRUCT && !gvar->status.is_ptr_count))
            emit_assign_block_memory(gvar, GLOBAL_SCOPE);

        if(gvar->status.is_initialized)
        {
            if(gvar->status.is_array && gvar->type == CHAR && gvar->status.is_ptr_count)  // ie: char *array[]
            {
                for(int index = 0; index < gvar->defn.array.size; index++)
                {
                    if(gvar->defn.array.elements[index].string) // strings char *arr[] = {"abc", "def"}
                    {
                        emit_assign_rodata_string_init_array(gvar, index, rodata_curr_seg_off, GLOBAL_SCOPE);
                        create_rodata_segment_string(gvar->defn.array.elements[index].string);
                    }
                }
            }
            else if(gvar->type == CHAR && gvar->status.is_ptr_count)   // ie: char *
            {
                if(gvar->defn.variable.is_string_assigned)
                {
                    emit_assign_rodata_string(gvar, rodata_curr_seg_off, GLOBAL_SCOPE); 
                    create_rodata_segment_string(gvar->defn.variable.value.pointer);
                }
            }
        }

        gvar = gvar->next;
    }
}


struct symbol *extern_list = NULL;
void extern_all_lib_funcs()
{
    int mcpy = 0, print = 0, aloca = 0;
    emit_label("extern printf");
    emit_label("extern scanf");
    emit_label("extern cleanup_alloca");

    struct symbol *e_sym = extern_list;
    while(e_sym)
    {
        if(strcmp(e_sym->name, "exit") == 0)
            print = 1;
        else if(strcmp(e_sym->name, "memcpy") == 0)
            mcpy = 1;
        else if(strcmp(e_sym->name, "alloca") == 0)
            aloca = 1;

        emit_label("extern %s", e_sym->name);
        e_sym = e_sym->next;
    }
    if(mcpy == 0)
        emit_label("extern memcpy");
    if(print == 0)
        emit_label("extern exit");   
    if(aloca == 0)
        emit_label("extern alloca");
}

void write_main_entry()
{
    // this code is neccessary because the compiler treats every variable as a pointer (arrays, structures etc are pointer to a 'block' of memory) so 'argc', 'argv[]' and 'envp[]' has to be readjusted
    emitcode("sub esp, 12");
    emitcode("mov eax, [esp+12]");
    emitcode("mov dword [esp], eax");
    emitcode("lea edx, [esp+16]");
    emitcode("mov dword [esp+4], edx");
    emitcode("imul eax, 4");    // argc * 4
    emitcode("add eax, edx"); 
    emitcode("add eax, 4");
    emitcode("mov dword [esp+8], eax");

    emitcode("call _main");
    emitcode("push 0");
    emitcode("call exit");
}

unsigned int if_count = 0, else_count = 0, while_count = 0, for_count = 0, do_count = 0;
char *new_if_beg_label()
{
    char *buf = alloc_mem(20);
    sprintf(buf, ".if_%u", if_count);
    return buf;
}

char *new_if_end_label()
{
    char *buf = alloc_mem(20);
    sprintf(buf, ".end_if_%u", if_count);
    if_count++;
    return buf;
}

char *new_else_beg_label()
{
    char *buf = alloc_mem(20);
    sprintf(buf, ".else_%u", else_count);
    return buf;
}

char *new_else_end_label()
{
    char *buf = alloc_mem(20);
    sprintf(buf, ".end_else_%u", else_count);
    else_count++;
    return buf;
}

char *new_while_beg_label()
{
    char *buf = alloc_mem(20);
    sprintf(buf, ".while_%u", while_count);
    return buf;
}

char *new_while_end_label()
{
    char *buf = alloc_mem(20);
    sprintf(buf, ".end_while_%u", while_count);
    while_count++;
    return buf;
}

char *new_for_beg_label()
{
    char *buf = alloc_mem(20);
    sprintf(buf, ".for_%u", for_count);
    return buf;
}

char *floop_label()
{
    char *buf = alloc_mem(20);
    sprintf(buf, ".floop_%u", for_count);
    return buf;
}

char *fcnt_label()
{
    char *buf = alloc_mem(20);
    sprintf(buf, ".fcnt_%u", for_count);
    return buf;
}

char *new_for_end_label()
{
    char *buf = alloc_mem(20);
    sprintf(buf, ".end_for_%u", for_count);
    for_count++;
    return buf;
}

char *new_do_label()
{
    char *buf = alloc_mem(20);
    sprintf(buf, ".do_%u", do_count);
    return buf;
}

char *new_dwhile_beg_label()
{
    char *buf = alloc_mem(20);
    sprintf(buf, ".dwhile_%u", do_count);
    return buf;
}

char *new_dwhile_end_label()
{
    char *buf = alloc_mem(20);
    sprintf(buf, ".end_dwhile_%u", do_count);
    do_count++;
    return buf;
}

void reset_stmt_label_counters()
{
    if_count = 0, else_count = 0;
    while_count = 0, for_count = 0, do_count = 0;
}

long f_tell(FILE *stream)
{
    long pos = ftell(stream);
    if(pos < 0)
    {
        perror("error");
        exit(-1);
    }
    return pos;
}

void f_seek(FILE *stream, long offset, int whence)
{
    if(fseek(stream, offset, whence) < 0)
    {
        perror("error");
        exit(-1);
    }
}

void f_read(char *ptr, int size, FILE *stream)
{
    int count = fread(ptr, 1, size, stream);
    if(ferror(stream))
    {
        perror("error");
        exit(-1);
    }
    //ptr[count] = 0;   // not needed. Check alloc_mem()
}  
