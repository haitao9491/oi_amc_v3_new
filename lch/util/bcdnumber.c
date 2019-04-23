/*
 *
 * bcdnumber.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "bcdnumber.h"

bcdnumber str2bcdnumber_ex(char *str, int base)
{
	unsigned long long int ull;

	if (str && *str) {
		char *ptr;
#ifndef WIN32
		ull = strtoull(str, &ptr, base);
#else
		ull = _strtoui64(str, &ptr, base);
#endif

		if (*ptr == 0)
			return (bcdnumber)ull;
	}

	return 0;
}

bcdnumber str2bcdnumber(char *str)
{
	return str2bcdnumber_ex(str, 0);
}

char *bcdnumber2str(bcdnumber bcdnum, char *str)
{
	static char static_str[21];
	char *p;

	/* WARNING: The usage of static_str is not thread safe. */
	p = str ? str : static_str;
	sprintf(p, "%llu", (unsigned long long int)bcdnum);

	return p;
}
