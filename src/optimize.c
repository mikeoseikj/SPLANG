#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "include/optimize.h"


/*
	These are just a few patterns (peephole optimization) in the generated assembly code. 
	There are more patterns and even some of the optimized code can further be reoptimized
*/

int fixed_expr_const_fold1(char *subject)
{
	char match_pattern[] = "\tpush 0x[[:xdigit:]]+\n"
					       "\tpush 0x[[:xdigit:]]+\n"
				 	       "\tpop (ebx|ecx|edx)\n"
					       "\tpop eax\n"
					       "(\tcdq\n|)"
					       "\t((add|sub|imul|xor|or|and|shl|shr) eax, (edx|cl)|idiv ebx)\n"
					       "(\tmov eax, edx\n|)";

	regmatch_t pmatch[1];
	char *substr = regex_get(subject, match_pattern);
	if(! substr)
		return 0;

    char *ptr = substr;
    char num_str1[16], num_str2[16];
	regex_match(ptr, "0x[[:xdigit:]]+\n", pmatch);
	strncpy(num_str1, ptr+pmatch[0].rm_so, pmatch[0].rm_eo-pmatch[0].rm_so);
	ptr += pmatch[0].rm_eo;
	regex_match(ptr, "0x[[:xdigit:]]+\n", pmatch);
	strncpy(num_str2, ptr+pmatch[0].rm_so, pmatch[0].rm_eo-pmatch[0].rm_so);

	unsigned int num1 = (unsigned int)atof(num_str1), num2 = (unsigned int)atof(num_str2);
	unsigned int value;

    regmatch_t m[1];
    if(regex_match(ptr, "shl", m))
    	value = num1 << num2;
    else if(regex_match(ptr, "shr", m))
    	value = num1 >> num2;
	else if(regex_match(ptr, "add", m))
		value = num1 + num2;
	else if(regex_match(ptr, "sub", m))
		value = num1 - num2;
	else if(regex_match(ptr, "imul", m))
		value = num1 * num2;
	else if(regex_match(ptr, "xor", m))
		value = num1 ^ num2;
	else if(regex_match(ptr, "or", m))
		value = num1 | num2;
	else if(regex_match(ptr, "and", m))
		value = num1 & num2;
	else if(regex_match(ptr, "mov eax, edx", m))	// modulo 
		value = num1 % num2;
	else
		value = num1 / num2;
	free(substr);

	char rep_str[30];
	sprintf(rep_str, "\tmov eax, 0x%x\n", value);
	regex_replace(subject, match_pattern, rep_str);

	return 1;
}

int fixed_expr_const_fold2(char *subject)
{
	char match_pattern[] = "\tpush eax\n"
					   	   "\tpush 0x[[:xdigit:]]+\n"
				 	       "\tpop (ebx|ecx|edx)\n"
					       "\tpop eax\n"
					       "(\tcdq\n|)"
					       "\t((add|sub|imul|xor|or|and|shl|shr) eax, (edx|cl)|idiv ebx)\n"
					       "(\tmov eax, edx\n|)";


	regmatch_t pmatch[1];
	char *substr = regex_get(subject, match_pattern);
	if(substr == NULL)
		return 0;

    char num_str[16];
	regex_match(substr, "0x[[:xdigit:]]+\n", pmatch);
	strncpy(num_str, substr+pmatch[0].rm_so, pmatch[0].rm_eo-pmatch[0].rm_so);

	unsigned int num = (unsigned int)atof(num_str);
	char *instr, rep_str[50];
    
    regmatch_t m[1];

    if(regex_match(substr, "shl", m))
    	instr = "shl";
    else if(regex_match(substr, "shr", m))
    	instr = "shr";
	else if(regex_match(substr, "add", m))
		instr = "add";
	else if(regex_match(substr, "sub", m))
		instr = "sub";
	else if(regex_match(substr, "imul", m))
		instr = "imul";
	else if(regex_match(substr, "xor", m))
		instr = "xor";
	else if(regex_match(substr, "or", m))
		instr = "or";
	else if(regex_match(substr, "and", m))
		instr = "and";
	else if(regex_match(substr, "mov eax, edx", m))	// modulo (has to be checked with idiv)
		instr = "mod";
	else
		instr = "div";

	free(substr);
	char *div_code = "\tmov ebx, 0x%x\n"
				     "\tcdq\n"
				     "\tidiv ebx\n";

    char *mod_code = "\tmov ebx, 0x%x\n"
				     "\tcdq\n"
				     "\tidiv ebx\n"
				     "\tmov eax, edx\n";

	if(strcmp(instr, "div") == 0)
		sprintf(rep_str, div_code, num);
	else if(strcmp(instr, "mod") == 0)
		sprintf(rep_str, mod_code, num);
	else if(strcmp(instr, "shl") == 0 || strcmp(instr, "shr") == 0)
		sprintf(rep_str, "\t%s eax, 0x%x\n", instr, (unsigned char)num);
	else
		sprintf(rep_str, "\t%s eax, 0x%x\n", instr, num);
	regex_replace(subject, match_pattern, rep_str);
	return 1;
}


int floating_expr_const_fold1(char *subject)
{
	char match_pattern[] = "\tpush 0x[[:xdigit:]]+\n"
					       "\tpush 0x[[:xdigit:]]+\n"
				 	       "\tfld dword \\[esp\\]\n"
					       "\tfld dword \\[esp\\+4\\]\n"
					       "\tadd esp, 8\n"
					       "(\tfxch\n|)"
					       "\t(faddp|fsubp|fmulp|fdivp) st1\n";

	regmatch_t pmatch[1];
	char *substr = regex_get(subject, match_pattern);
	if(! substr)
		return 0;

    char *ptr = substr;
    char num_str1[16], num_str2[16];
	regex_match(ptr, "0x[[:xdigit:]]+\n", pmatch);
	strncpy(num_str1, ptr+pmatch[0].rm_so, pmatch[0].rm_eo-pmatch[0].rm_so);
	ptr += pmatch[0].rm_eo;
	regex_match(ptr, "0x[[:xdigit:]]+\n", pmatch);
	strncpy(num_str2, ptr+pmatch[0].rm_so, pmatch[0].rm_eo-pmatch[0].rm_so);

	unsigned int num1 = (unsigned int)atof(num_str1), num2 = (unsigned int)atof(num_str2);
	unsigned int value;

	regmatch_t m[1];
	if(regex_match(ptr, "faddp", m))
		value = IEEE_754_to_int_convert(num1) + IEEE_754_to_int_convert(num2);
	else if(regex_match(ptr, "fsubp", m))
		value = IEEE_754_to_int_convert(num1) - IEEE_754_to_int_convert(num2);
	else if(regex_match(ptr, "fmulp", m))
		value = IEEE_754_to_int_convert(num1) * IEEE_754_to_int_convert(num2);
	else 
		value = IEEE_754_to_int_convert(num1) / IEEE_754_to_int_convert(num2);

	free(substr);
	char rep_str[50];
	char *code = "\tpush 0x%x\n"
				 "\tfld dword [esp]\n"
				 "\tadd esp, 4\n";
	sprintf(rep_str, code, IEEE_754_to_float_convert(value));
	regex_replace(subject, match_pattern, rep_str);

	return 1;
}

int floating_expr_const_fold2(char *subject)
{
	char match_pattern[] = "\tsub esp, 4\n"
						   "\tfstp dword \\[esp\\]\n"
					   	   "\tpush 0x[[:xdigit:]]+\n"
				 	       "\tfld dword \\[esp\\]\n"
					       "\tfld dword \\[esp\\+4\\]\n"
					       "\tadd esp, 8\n"
					       "(\tfxch\n|)"
					       "\t(faddp|fsubp|fmulp|fdivp) st1\n";

	regmatch_t pmatch[1];
	char *substr = regex_get(subject, match_pattern);
	if(substr == NULL)
		return 0;

    char num_str[16];
	regex_match(substr, "0x[[:xdigit:]]+\n", pmatch);
	strncpy(num_str, substr+pmatch[0].rm_so, pmatch[0].rm_eo-pmatch[0].rm_so);

	unsigned int num = (unsigned int)atof(num_str);

	regmatch_t m[1];
	char *instr;
	if(regex_match(substr, "faddp", m))
		instr = "faddp";
	else if(regex_match(substr, "fsubp", m))
		instr = "fsubp";
	else if(regex_match(substr, "fmulp", m))
		instr = "fmulp";
	else 
		instr = "fdivp";
	
	free(substr);
	char rep_str[60];
	char *code = "\tpush 0x%x\n"
				 "\tfld dword [esp]\n"
				 "\tadd esp, 4\n"
				 "\t%s st1\n";

	sprintf(rep_str, code, num, instr);
	regex_replace(subject, match_pattern, rep_str);
	return 1;
}


int fixed_expr_const_refold(char *subject)
{
	char match_pattern[] = "(\tadd eax, 0x[[:xdigit:]]+\n"
					       "\tadd eax, 0x[[:xdigit:]]+\n|"

					       "\timul eax, 0x[[:xdigit:]]+\n"
					       "\timul eax, 0x[[:xdigit:]]+\n|"

					       "\tsub eax, 0x[[:xdigit:]]+\n"
					       "\tsub eax, 0x[[:xdigit:]]+\n)";

	regmatch_t pmatch[1];
	char *substr = regex_get(subject, match_pattern);
	if(! substr)
		return 0;

    char *ptr = substr;
    char num_str1[16], num_str2[16];
	regex_match(ptr, "0x[[:xdigit:]]+\n", pmatch);
	strncpy(num_str1, ptr+pmatch[0].rm_so, pmatch[0].rm_eo-pmatch[0].rm_so);
	ptr += pmatch[0].rm_eo;
	regex_match(ptr, "0x[[:xdigit:]]+\n", pmatch);
	strncpy(num_str2, ptr+pmatch[0].rm_so, pmatch[0].rm_eo-pmatch[0].rm_so);

	unsigned int num1 = (unsigned int)atof(num_str1), num2 = (unsigned int)atof(num_str2);
	free(substr);

	char rep_str[30];
	unsigned int value;
	if(regex_match(ptr, "add", pmatch))
	{
		value = num1 + num2;
		sprintf(rep_str, "\tadd eax, 0x%x\n", value);
	}
	else if(regex_match(ptr, "imul", pmatch))
	{
		value = num1 * num2;
		sprintf(rep_str, "\timul eax, 0x%x\n", value);
	}
	else
	{
		value = num1 + num2;
		sprintf(rep_str, "\tsub eax, 0x%x\n", value);
	}
	regex_replace(subject, match_pattern, rep_str);
	return 1;
}


int remove_redundant_pushes(char *subject)
{
	char match_pattern[] = "(\tpop eax\n"
						   "\tpush eax\n|"

						   "\tpush eax\n"
						   "\tpop eax\n|"

						   "\tpopad\n"
						   "\tpushad\n)";

	regmatch_t m[1];
	if(! regex_match(subject, match_pattern, m))
		return 0;

	regex_replace(subject, match_pattern, "");
	return 1;
}


void optimize_code(char *filename)
{
	int fd = open(filename, O_RDWR);
    if(fd < 0)
    {
        perror("optimize - error");
        return;
    }
	struct stat statbuf;
	if(fstat(fd, &statbuf) < 0)
	{
		perror("optmize stat - error");
		return;
	}
	char *buf = alloc_mem(statbuf.st_size+1);
	if(read(fd, buf, statbuf.st_size) < 0)
	{
		perror("optimize read - error");
		return;
	}
	buf[statbuf.st_size] = 0;
	char *ptr = buf;

    while(remove_redundant_pushes(buf));	// has to be placed here because of some 'push' and 'pop' eaxs
	while(fixed_expr_const_fold1(buf));
	while(fixed_expr_const_fold2(buf));
	while(floating_expr_const_fold1(buf));
	while(floating_expr_const_fold2(buf));
	while(fixed_expr_const_refold(buf));
	
	if(ftruncate(fd, 0) < 0)
	{
		perror("optimize trunc - error");
		return;
	}
	if(lseek(fd, 0, SEEK_SET) < 0)
	{
		perror("optimize seek - error");
		return;
	}
	if(write(fd, buf, strlen(buf)) < 0)
	{
		perror("optimize write - error");
	}
	free(ptr);
}
