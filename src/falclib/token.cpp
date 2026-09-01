#include <ciso646>
#include <string.h>
#include <stdlib.h>
// stricmp (:105) is an MSVC CRT extension. <string.h> above supplies it on Windows; glibc's does not have the name
// at all, and the shim keeps it with the other CRT extensions behind <windows.h>. Included unconditionally and not
// behind an #ifndef: stricmp is a function rather than a macro on both platforms, so no preprocessor test can detect
// it -- a guard here would look meaningful while always taking the same branch. On Windows this is a redundant
// include of a header the rest of the codebase already pulls everywhere, which costs nothing and changes nothing.
#include <windows.h>
#include "token.h"

// MLR 12/13/2003 - Simple token parsing

char *tokenStr = 0;

float TokenF(float def)
{
    return (TokenF(tokenStr, def));
}


float TokenF(char *str, float def)
{
    char *bs;

    tokenStr = 0;

    if (bs = strtok(str, " ,\t\n"))
    {
        return ((float)atof(bs));
    }

    return (def);
}


int TokenI(int def)
{
    return (TokenI(tokenStr, def));
}


int TokenI(char *str, int def)
{
    char *bs;

    tokenStr = 0;

    if (bs = strtok(str, " ,\t\n"))
    {
        return (atoi(bs));
    }

    return (def);
}


int TokenFlags(int def, char *flagstr)
{
    return (TokenFlags(tokenStr, def, flagstr));
}


int TokenFlags(char *str, int def, char *flagstr)
{
    char *arg;
    int flags = 0;

    tokenStr = 0;

    if (arg = strtok(str, " ,\t\n"))
    {
        while (*arg)
        {
            int l;

            for (l = 0; l < 32 and flagstr[l]; l++)
            {
                if (*arg == flagstr[l])
                {
                    flags or_eq 1 << l;
                }
            }

            arg++;
        }

        return (flags);
    }

    return (def);
}

int TokenEnum(char **enumnames, int def)
{
    return (TokenEnum(tokenStr, enumnames, def));
}


int TokenEnum(char *str, char **enumnames, int def)
{
    char *arg;
    int i = 0;

    tokenStr = 0;

    if (arg = strtok(str, " ,\t\n"))
    {
        while (*enumnames)
        {
            if (stricmp(arg, *enumnames) == 0)
            {
                return i;
            }

            enumnames++;
            i++;
        }
    }

    return def;
}

void SetTokenString(char *str)
{
    tokenStr = str;
}

char *TokenStr(char *def)
{
    return (TokenStr(tokenStr, def));
}


char *TokenStr(char *str, char *def)
{
    char *bs;

    tokenStr = 0;

    if (bs = strtok(str, " :,\t\n"))
    {
        return (bs);
    }

    return (def);
}
