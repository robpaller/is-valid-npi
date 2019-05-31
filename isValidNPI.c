#define SQL_TEXT Latin_Text
#include "sqltypes_td.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define IS_NULL -1
#define IS_NOT_NULL 0
#define EOS '\0'
#define UDF_OK "00000"
#define UDF_ERR_1 "U0001"
#define UDF_MSG_1 "Invalid NPI - NPI Length must be 10 or 15 bytes"
#define UDF_ERR_2 "U0002"
#define UDF_MSG_2 "Invalid NPI - NPI may only contain digits"
/* These functions are donated to the public domain by Claredi. Use of these functions
 * is permitted without restrictions.  You may modify them as desired.  Even though 
 * this code has been tested, it is provided without guarantee. If you would like to
 * credit Claredi when you use these functions, you can use words like "NPI check-digit
 * validation functions contributed by Claredi (www.claredi.com)" or something similar.
 * January 23, 2004
 */

/*****************************************************************
 *
 *	Check if the NPI Luhn check-digit is correct.
 *	This code is adapted from the ISO 7812 annex.
 *	Return one if NPI luhn check is ok, else zero.
 *
 *****************************************************************/

void isValidNPI(VARCHAR_LATIN *inStr,
	            int *result,
	            int *inStr_null_ind,	            int *result_ind,
	            char sqlstate[6],
	            SQL_TEXT extname[129],
	            SQL_TEXT specific_name[129],
	            SQL_TEXT error_message[257])
{	int tmp, sum, i, j;

	/* Handle NULL input string */
	if (*inStr_null_ind == IS_NULL)  {
		*result_ind = IS_NULL;
		strcpy(sqlstate, UDF_OK);
		error_message[0] = EOS;
		return;
	}

	/* Initialize result position */
	*result_ind = IS_NOT_NULL;	strcpy(sqlstate, UDF_OK);
	error_message[0] = EOS;

	/* the NPI is a 10 digit number, but it could be
	 * preceded by the ISO prefix for the USA (80840)
	 * when stored as part of an ID card.  The prefix
	 * must be accounted for, so the NPI check-digit
	 * will be the same with or without prefix.
	 * The magic constant for 80840 is 24.
	 */
	i = strlen(inStr);
	if ((i == 15) && (strncmp(inStr,"80840",5) == 0))
		sum = 0;
	else if (i == 10)
		sum = 24;	/* to compensate for the prefix */
	else  /* length must be 10 or 15 bytes */  
	{
		*result_ind = IS_NULL;
		strcpy(sqlstate, UDF_ERR_1);
		strcpy(error_message, UDF_MSG_1);
		return;
	}		

	/* the algorithm calls for calculating the check-digit
	 * from right to left
	 */
	/* first, intialize the odd/even counter, taking into account
	 * that the rightmost digit is presumed to be the check-sum
	 * so in this case the rightmost digit is even instead of
	 * being odd
	 */
	j = 0;
	/* now scan the NPI from right to left */
	while (i--)
	{	/* only digits are valid for the NPI */
		if (!isdigit(inStr[i]))  {
			*result_ind = IS_NULL;
			strcpy(sqlstate, UDF_ERR_2);
			strcpy(error_message, UDF_MSG_2);
			return;
		}
		/* this conversion works for ASCII and EBCDIC */
		tmp = inStr[i] - '0';
		/* the odd positions are multiplied by 2 */
		if (j++ & 1)
		{	/* instead of multiplying by 2, in C
			 * we can just shift-left by one bit
			 * which is a faster way to multiply
			 * by two.  Same as (tmp * 2)
			 */
			if ((tmp <<= 1) > 9)
			{	/* when the multiplication by 2 
				 * results in a two digit number
				 * (i.e., greater than 9) then the
				 * two digits are added up.  But we
				 * know that the left digit must be
				 * '1' and the right digit must be
				 * x mod 10.  In that case we can
				 * just subtract 10 instead of 'mod'
				 */
				tmp -= 10;	/* 'tmp mod 10' */
				tmp++;		/* left digit is '1' */
			}
		}
		sum += tmp;
	}

	/* If the checksum mod 10 is zero then the NPI is valid */
	if ((sum % 10) == 0)
		*result = 1;
	else	*result = 0;

	return;
}
