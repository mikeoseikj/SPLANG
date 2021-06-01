#ifndef CODEGEN_H
#define CODEGEN_H

#include "symtab.h"
#include "utils.h"

#define FLOAT_TYPE  1
#define INT_TYPE    2

struct dregs
{
	int in_fpu_mode;
    struct
    {
        int busy;
        int vartype;  // store type of eax (check 'emit_cond_pop_eax()'')	
    }st0, eax;        // floating point and fixed point 'return' registers

 	struct
	{
		unsigned char *data;
		int size, top;
		void (*push)(int value);
		int  (*pop)();
	}cgen_stack;

};

void init_cgen_regs();

/*
	Notes:
		=> The 'deref's deals with pointer dereferences eg: *ptr = 3;
		=> The 'addof's deals with with & operation     eg: int 10 = &a + 1;
		=> The 'rodata's deals with const strings  that will be stored int the read only segment
*/

void emit_cond_pop_eax();
void emit_cond_push_eax();
void emit_push_const(unsigned int value);
void emit_add();
void emit_sub();
void emit_mul();
void emit_div();
void emit_mod();
void emit_neg();
void emit_not();
void emit_flip();
void emit_bit_xor();
void emit_bit_and();
void emit_bit_or();
void emit_bit_lshift();
void emit_bit_rshift();
void emit_logical_and();
void emit_logical_or();
void emit_equ();
void emit_not_equ();
void emit_gtr();
void emit_less();
void emit_gtr_equ();
void emit_less_equ();
void emit_assign_to_variable(struct symbol *sym, int scope);
void emit_assign_rodata_string(struct symbol *sym, unsigned int seg_offset, int scope);
void emit_assign_rodata_string_to_pointer(unsigned int seg_offset);
void emit_assign_to_pointer(int is_byte);
void emit_assign_member_rodata_string(unsigned int seg_offset);
void emit_assign_rodata_string_init_array(struct symbol *sym, int index, unsigned int seg_offset, int scope);
void emit_push_variable(struct symbol *sym, int scope);
void emit_deref_variable(struct symbol *sym, int dref_count, int is_byte);
void emit_deref_get_addr(struct symbol *sym, int dref_count);
void emit_push_addrof_variable(struct symbol *sym, int scope);
void emit_buffer_string(struct symbol *sym, char *string);
void emit_array_init_element(struct symbol *sym, int index);
void emit_array_element(struct symbol *sym, int offset);
void emit_push_array_init_element(struct symbol *sym, unsigned int index);
void emit_push_array_element(struct symbol *sym, int scope);
void emit_adjust_variable_array(struct symbol *sym, int scope);
void emit_push_addrof_array_element(struct symbol *sym, int scope);
void emit_assign_to_array_index(struct symbol *sym);
void emit_assign_rodata_string_to_index(unsigned int seg_offset);
void emit_assign_block_memory(struct symbol *sym, int scope);
void emit_struct_address(struct symbol *sym, int scope);
void emit_push_struct_address();
void emit_push_struct_value(struct symbol *sym, int is_byte);
void emit_reset_member_pointer();
void emit_load_member_address(struct symbol *sym);
void emit_adjust_struct_array(struct symbol *sym);
void emit_assign_to_member(struct symbol *sym);
void emit_zero_cmp_eax();
void emit_jeq(char *label);
void emit_jneq(char *label);
void emit_store_condition();
void emit_load_condition();
void emit_jmp(char *label);
void emit_call(char *label);
void emit_lib_call(char *label);
void emit_sub_esp(int size);
void emit_add_esp(int size);
void emit_func_prolog(int size);
void emit_func_epilog();
void emit_push_func_arg();
void emit_push_func_retval();
void emit_push_func_float_arg();
void emit_struct_copy(struct symbol *sym, int safe_mem);
void emit_rodata_func_string_arg(unsigned int seg_offset);
void emit_return(struct symbol *func);
void emit_return_rodata_string(unsigned int seg_offset);
void emit_push_all_regs();
void emit_pop_all_regs();
void emit_push_edi_reg();
void emit_pop_edi_reg();
void emit_lib_call_st0_to_eax();
void emit_param_cast(int type);

#endif
