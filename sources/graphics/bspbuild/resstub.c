#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <windows.h>

void ResSetDirectory (char* a) {}
void ResAddPath (char* a, int b) {}
void ResCreatePath (char* a, int b) {}
int ResAttach (char* a, char* b, int c) {return 1;}
void ResInit (void) {}
void ResExit (void) {}
void ResDetach (int a) {}
int ResExistFile (char* fileName)
{
int retval = 0;
FILE* tmpFile;

   tmpFile = fopen (fileName, "r");
   if (tmpFile)
   {
      fclose (tmpFile);
      retval = 1;
   }
   return retval;
}
int ResWhereIs (char* fileName, char* path)
{
int retval = -1;

   if (ResExistFile (fileName))
      retval = 1;

   return retval;
}
