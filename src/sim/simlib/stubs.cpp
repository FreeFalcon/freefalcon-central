#include "stdhdr.h"
#include "Renderer\RenderOW.h"
#include "3Dlib\3d.h"
#include "f4vu.h"

extern "C"
{
int _CrtDbgReport( int reportType, const char *filename, int linenumber, const char *moduleName, const char *format, ... );
};

int _CrtDbgReport( int reportType, const char *filename, int linenumber, const char *moduleName, const char *format, ... )
{
char	buffer[80];															  		
int		choice = IDRETRY;														

   sprintf( buffer, "Assertion at %0d  %s", linenumber, filename);
   while (choice != IDIGNORE)
   {
      choice = MessageBox(NULL, buffer, "Failed:  ",		 	
         MB_ICONERROR | MB_ABORTRETRYIGNORE | MB_TASKMODAL);
      if (choice == IDABORT)
      {
         exit(-1);															
      }
      else if (choice == IDRETRY)
      {
         __asm int 3															
      }																		
   }																		
   return (0);

}
