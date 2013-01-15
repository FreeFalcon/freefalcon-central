/*
 * $Id: mddriver.c,v 1.1.1.1 2003/09/26 20:20:46 Red Exp $
 *
 * Derived from:
 */

/*
 * MDDRIVER.C - test driver for MD2, MD4 and MD5
 */

/*
 *  Copyright (C) 1990-2, RSA Data Security, Inc. Created 1990. All
 *  rights reserved.
 *
 *  RSA Data Security, Inc. makes no representations concerning either
 *  the merchantability of this software or the suitability of this
 *  software for any particular purpose. It is provided "as is"
 *  without express or implied warranty of any kind.
 *
 *  These notices must be retained in any copies of any part of this
 *  documentation and/or software.
 */

/*
.\" $Id: mddriver.c,v 1.1.1.1 2003/09/26 20:20:46 Red Exp $
Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.
.Pp
License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.
.Pp
License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.
.Pp
RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.
.Pp
These notices must be retained in any copies of any part of this
documentation and/or software.
*/

#ifndef lint
static const char rcsid[] =
	"$Id: mddriver.c,v 1.1.1.1 2003/09/26 20:20:46 Red Exp $";
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include "md5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Length of test block, number of test blocks.
 */

int EncodeFiles (char* fileList);
int TestFiles (char* fileList);
void PrintHelp (void);

/* Main driver.*/
int main(int argc, char* argv[])
{
int retval = 0;

	if (argc != 3)
	{
		PrintHelp();
	}
	else
	{
		if (!strcmp (argv[1], "-e"))
		{
			retval = EncodeFiles (argv[2]);
		}
		else if (!strcmp (argv[1], "-t"))
		{
			retval = TestFiles (argv[2]);
		}
		else
		{
			PrintHelp();
		}
	}

	exit(0);
}

int EncodeFiles (char* fileList)
{
FILE* filePtr;
int retval = 0;
char fileName[1024];
char buf[33];
char* p;
int count = 0;

	printf ("char* fileList[] = {\n");
	filePtr = fopen (fileList, "r");
	while (fgets (fileName, 1024, filePtr))
	{
		fileName[strlen(fileName)-1] = 0;
		p = MD5File(fileName, buf);
		if (p)
			printf ("\"%s = %s\",\n", fileName, p);
	}
	printf ("\"LASTFILE = 0\",\n");
	printf ("};\n");
	retval = -1;

	return retval;
}

int TestFiles (char* fileList)
{
FILE* filePtr;
int retval = 0;
char fileName[1024];
char buf[33];
char checksum[33];
char* p;
struct _stat statBuf;
int result;

	filePtr = fopen (fileList, "r");

	while (fgets (fileName, 1024, filePtr))
	{
		strcpy (checksum, strchr(fileName, '=') + 2);
		checksum[strlen(checksum)-1] = 0;
		*(strchr (fileName, '=') - 1) = 0;

		printf ("Checking %s - ", fileName);

		result = _stat( fileName, &statBuf );
			
		if (result == 0)
		{
			p = MD5File(fileName, buf);

			if (strcmp (p, checksum))
			{
				printf ("FAILED\n");
				retval++;
			}
			else
			{
				printf ("OK\n");
			}
		}
		else
		{
			printf ("NOT FOUND\n");
			retval++;
		}
	}

	if (retval > 0)
		printf ("%d Errors found !!!!!\n", retval);
	else if (retval == 0)
		printf ("No Errors found :)\n");
	return (retval);
}

void PrintHelp (void)
{
	printf ("Usage: checksum [-e filename] [-t filename]\n");
	printf ("    -e filename       compute checksum on all files in filname\n");
	printf ("    -t filename       test checksum on all files in filname\n");
}