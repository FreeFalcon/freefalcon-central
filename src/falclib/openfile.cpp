// File open functions to handle failed file openings
// These functions use a windows Dialog box (repetedly) to get the user to insert a CD
// Peter Ward

#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include "f4version.h"

extern "C"
{
#include "codelib/resources/reslib/src/resmgr.h"
}

char *InsertMedia[] =
{
    "Please mount the FreeFalcon media",// undefined
    "Please mount the FreeFalcon media",// English
    "Please mount the FreeFalcon media",// UK
    "Bitte mounten sie die FreeFalcon Medien",// German
    "S'il vous plait monter le media FreeFalcon",// French
    "Monte el soporte de FreeFalcon",// Spanish
    "Si prega di montare i media FreeFalcon",// Italian
    "Por favor, montar a m?dia FreeFalcon",// Portuguese
    "Please mount the FreeFalcon media",
    "Please mount the FreeFalcon media",
    "Please mount the FreeFalcon media",
    "Please mount the FreeFalcon media",
    "Please mount the FreeFalcon media",
};
char *UnableToOpen[] =
{
    "Unable to open file",// undefined
    "Unable to open file",// English
    "Unable to open file",// UK
    "Kann Datei nicht öffnen",// German
    "Impossible d'ouvrir le fichier",// French
    "Imposible abrir el archivo",// Spanish
    "Impossibile aprire il file",// Italian
    "Não é possível abrir arquivo",// Portuguese
    "Unable to open file",
    "Unable to open file",
    "Unable to open file",
    "Unable to open file",
    "Unable to open file",
};

static int Enabled = 1;

// This function opens a dialog box asking the user to insert the FreeFalcon media
// Possible return values:
// 0 = Retry
// 1 = Abort
// if Disabled... exit(-1);

int DoDialogBox()
{
    int retval;

    if (!Enabled)
        exit(0);

	retval = MessageBox(NULL, InsertMedia[gLangIDNum], UnableToOpen[gLangIDNum], MB_RETRYCANCEL);

    if (retval == IDRETRY)
        return(0);

    return(1);
}

FILE *FILE_Open(char *filename, char *Params)
{
    int done = 0;
    FILE *handle = NULL;

    while (!done)
    {
        handle = fopen(filename, Params);

        if (!handle)
        {
            done = DoDialogBox();
        }
        else
            done = 1;
    }

    return(handle);
}


int INT_Open(char *filename, int Params, int pmode)
{
    int done = 0;
    int handle = -1;

    while (!done)
    {
        handle = open(filename, Params, pmode);

        if (handle == -1)
        {
            done = DoDialogBox();
        }
        else
            done = 1;
    }

    return(handle);
}

HANDLE CreateFile_Open(char *filename, DWORD param1, DWORD param2, LPSECURITY_ATTRIBUTES param3, DWORD param4, DWORD param5, HANDLE param6)
{
    int done = 0;
    HANDLE handle = INVALID_HANDLE_VALUE;

    while (!done)
    {
        handle = CreateFile(filename, param1, param2, param3, param4, param5, param6);

        if (handle == INVALID_HANDLE_VALUE)
        {
            done = DoDialogBox();
        }
        else
            done = 1;
    }

    return(handle);
}


int ResAttach_Open(const char * attach_point, const char * filename, int replace_flag)
{
    int done = 0;
    int handle = -1;

    while (!done)
    {
        handle = ResAttach(attach_point, filename, replace_flag);

        if (handle < 0)
        {
            done = DoDialogBox();
        }
        else
            done = 1;
    }

    return(handle);
}
