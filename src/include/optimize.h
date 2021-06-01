#ifndef	OPTIMIZE_H
#define OPTIMIZE_H

#include <string.h>
#include <regex.h>
#include "utils.h"

int regex_match(char *subject, char *pattern, regmatch_t pmatch[])
{
	regex_t preg;
	if(regcomp(&preg, pattern, REG_EXTENDED) != 0)
		return 0;


	if(regexec(&preg, subject, 1, pmatch, 0) != 0)
			return 0;

	return 1;
}

char *regex_get(char *subject, char *pattern)
{
	regex_t preg;
	regmatch_t pmatch[1];
	if(regcomp(&preg, pattern, REG_EXTENDED) != 0)
		return NULL;

	if(regexec(&preg, subject, 1, pmatch, 0) != 0)
			return NULL;

    int len = pmatch[0].rm_eo-pmatch[0].rm_so;
	char *ptr = (char *)alloc_mem(len+1);
	strncpy(ptr, subject+pmatch[0].rm_so, len);
	return ptr;

}


int regex_replace(char *subject, char *pattern, char *replacement)
{
	// it is assumed that the size of the replacement is less than or equal to the size matched pattern
	// because is written specifically for its usage in optimizing the generated code

	regex_t preg;
	regmatch_t pmatch[1];
	if(! regex_match(subject, pattern, pmatch))
		return 0;

	int start = pmatch[0].rm_so, end = pmatch[0].rm_eo, diff = pmatch[0].rm_eo - pmatch[0].rm_so;
	int s_len = strlen(subject), r_len = strlen(replacement);

	memmove(subject +start+r_len, subject+end, s_len-end);
	memcpy(subject+start, replacement, r_len);
	subject[(s_len-diff)+r_len] = 0;
	return 1;
}


#endif
