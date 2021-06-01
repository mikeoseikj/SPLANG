#include <string.h>
#include <stdlib.h>
#include "include/utils.h"
#include "include/globals.h"
#include "include/warning.h"
#include "include/error.h"
#include "include/codegen.h"
#include "include/func.h"

int main(int argc, char *argv[])
{
	int do_assemble = 1, do_optimize = 1;	//true
	char *binary_file = NULL, *source_file = NULL;

	for(int i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "-no-opt") == 0)
		{
			do_optimize = 0; //false
		}
		else if(strcmp(argv[i], "-no-bin") == 0)
		{
			do_assemble = 0; //false
		}
		else if(strcmp(argv[i], "-o") == 0)
		{
			if(argv[++i])
				binary_file = argv[i];
			else
				goto err_help;
		}
		else if(strcmp(argv[i], "-h") == 0)
		{
			printf("usage: %s -o  <output file>  <input file>\n", argv[0]);
			printf("       %s  <input file> : create an ELF executable with default name \'./a.out\'\n", argv[0]);
			printf("\nOther compiler options.\n\n");
			printf("-no-opt 	disable optimization of the generated assembly code\n");
			printf("-no-bin 	generate assembly code in \'code.s\' file instead of an ELF executable\n");
			return 0;
		}
		else
		{
			if(!source_file)
				source_file = argv[i];
		}

	}
	if(!binary_file)
		binary_file = "a.out";

	if(!source_file)
	{
		err_help:
			fprintf(stderr, "usage: %s  <-h> for more information\n", argv[0]);
			exit(-1);
	}

	open_source_file(source_file);
	scan_all_decls();
	create_output_file("code.s");	// always the name of the assembly file (also when -s is passed on commandline)
	create_bss_section();
	create_data_section();

	emit_label("section .text");
	emit_label("global _start");
	emit_label("  _start:");
	prepare_global_arrays();
	write_main_entry();

	struct symbol *main_sym, *fsym = func_list;
	f_seek(in_stream, 0, SEEK_SET);	// input file

	init_cgen_regs();
	int has_main = 0;
	while(fsym)
	{
		if(strcmp(fsym->name, "main") == 0)
		{
			main_sym = fsym;
			has_main = 1;
		}

		cgen_func_block(fsym);
		fsym = fsym->next;
	}
    
	create_rodata_section();
	extern_all_lib_funcs();

	if(!has_main)
		fprintf(stderr, "error: No main() function found in your code\n");
	if(has_main && main_sym->defn.function.num_params > 3)
	{
		line_num = main_sym->defn.function.line_num; 
		col_num = main_sym->defn.function.file_loc;		// pointing to the start of the function
		warning(TOO_MANY_MAIN_ARGS);
	}

	fclose(in_stream);
	fclose(out_stream);

	if(error_count)
	{
		system("tools/rm code.s");
		return 0;
	}
	if(do_optimize)
		optimize_code("code.s");
	if(!do_assemble)
		return 0;

	char *link = alloc_mem(strlen(binary_file)+300);
	sprintf(link, "tools/ld -m elf_i386 code.o -o %s -L ./mylibs -lm -lc  -lreadline -lmystdlib --dynamic-linker=/usr/lib32/ld-linux.so.2", binary_file);
	system("tools/nasm -felf32 code.s -o code.o");
	system(link);
	system("tools/rm code.o code.s");
	return 0;
}
