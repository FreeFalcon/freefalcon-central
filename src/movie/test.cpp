#include <windows.h>
#include <stdio.h>
#include "movie.h"
#include "surface.h"
#include "test.h"
//#include "sndmgr.h"


HWND                    hwnd;

LPDIRECTDRAW            lpdd;
DDSURFACEDESC           ddsd;
LPDIRECTDRAWSURFACE     lpFrontBuffer;
HRESULT                 ddVal;
DDSCAPS                 ddscaps;

void callBack( int handle, LPVOID surface,
               int totalFrames, int callBackID, int dropFlag )
{
   return;
}

long FAR PASCAL WndProc ( HWND hwnd, UINT message,
								 UINT wParam, LONG lParam )
{
	switch ( message )
	{
   	case WM_KEYDOWN:
	   	if ( wParam == VK_ESCAPE )
		   	SendMessage ( hwnd, WM_DESTROY, 0, 0 );
   		return 0;

      case WM_LBUTTONDOWN:
		   SendMessage ( hwnd, WM_DESTROY, 0, 0 );
   		return 0;

	   case WM_DESTROY:
		   PostQuitMessage ( 0 );
   		return 0;
	}

	return DefWindowProc ( hwnd, message, wParam, lParam );
}


void InitializeWindow ( HANDLE hInstance, int nCmdShow )
{
   WNDCLASS wndclass;

   wndclass.style          = CS_HREDRAW | CS_VREDRAW;
   wndclass.lpfnWndProc    = WndProc;
   wndclass.cbClsExtra     = 0;
   wndclass.cbWndExtra     = 0;
   wndclass.hInstance      = hInstance;
   wndclass.hIcon          = LoadIcon ( hInstance, IDI_APPLICATION );
   wndclass.hCursor        = LoadCursor ( NULL, IDC_ARROW );
   wndclass.hbrBackground  = GetStockObject ( BLACK_BRUSH );
   wndclass.lpszMenuName   = NULL;
   wndclass.lpszClassName  = "Movie Player";
	
   RegisterClass ( &wndclass );

   hwnd = CreateWindowEx (
                     WS_EX_APPWINDOW,
                     "Movie Player",
                     "Movie Player",
                     WS_VISIBLE |
                        WS_SYSMENU |
                        WS_POPUP,
                     CW_USEDEFAULT,
                     CW_USEDEFAULT,
                     CW_USEDEFAULT,
                     CW_USEDEFAULT,
                     NULL,
                     NULL,
                     hInstance,
                     NULL );

   ShowWindow ( hwnd, nCmdShow );
   UpdateWindow ( hwnd );
   ShowCursor ( 0 );
}

HRESULT InitializeGraphics ( void )
{
   if ( ( ddVal = DirectDrawCreate ( NULL, &lpdd, NULL ) ) != DD_OK )
      return ddVal;

   if ( ( ddVal = IDirectDraw_SetCooperativeLevel ( lpdd, hwnd,
                                             DDSCL_EXCLUSIVE |
                                             DDSCL_FULLSCREEN ) ) != DD_OK )
      return ddVal;

   if ( ( ddVal = IDirectDraw_SetDisplayMode ( lpdd, 640, 480, 16 ) ) != DD_OK )
      return ddVal;

   memset ( &ddsd, 0, sizeof ( ddsd ) );
   ddsd.dwSize = sizeof ( ddsd );
   ddsd.dwFlags = DDSD_CAPS;
   ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

   if ( ( ddVal = IDirectDraw_CreateSurface ( lpdd, &ddsd,
                                 &lpFrontBuffer, NULL ) ) != DD_OK )
      return ddVal;
}

void ExitGraphics ( void )
{
	if ( lpFrontBuffer )
	{
		IDirectDrawSurface_Release ( lpFrontBuffer );
		lpFrontBuffer = NULL;
	}

	if ( lpdd )
	{
		IDirectDraw_Release ( lpdd );
		lpdd = NULL;
	}
}

int PASCAL WinMain (
               HANDLE hInstance,
               HANDLE hPrevInstance,
               LPSTR lpszCmdParam,
               int nCmdShow )
{
   MSG               msg;
   int               hnd, count = 0;
   char              aviFile[128];
   char              wavFile[128];
   LPSTR             avi = NULL, wav = NULL, ptr;
   int               movieMode, audioFlag, xStart, yStart;
   int               i;

   movieMode = audioFlag = xStart = yStart = 0;
   ptr = lpszCmdParam;

   while ( ptr = strchr( ptr, '-' ) )
   {
      ptr++;
      switch ( *(ptr++) )
      {
         case 'f':
         case 'F':
            i = 0;
            while ( ( *ptr != ' ' ) && ( *ptr ) )
            {
               aviFile[i] = *ptr++;
               i++;
            }
            aviFile[i] = 0;
            avi = &( aviFile[0] );
            break;
            
         case 'm':
         case 'M':
            sscanf( ptr, "%d", &movieMode );
            break;

         case 'a':
         case 'A':
            i = 0;
            while ( ( *ptr != ' ' ) && ( *ptr ) )
            {
               wavFile[i] = *ptr++;
               i++;
            }
            wavFile[i] = 0;
            wav = &( wavFile[0] );
            break;

         case 's':
         case 'S':
            sscanf( ptr, "%d", &audioFlag );
            break;

         case 'x':
         case 'X':
            sscanf( ptr, "%d", &xStart );
            break;

         case 'y':
         case 'Y':
            sscanf( ptr, "%d", &yStart );
            break;

         default:
            break;
      }
   }

   if ( !avi )
      return -1;

   switch ( movieMode )
   {
      default:
      case 0:
         movieMode = MOVIE_MODE_NORMAL;
         break;

      case 1:
         movieMode = MOVIE_MODE_V_DOUBLE;
         break;

      case 2:
         movieMode = MOVIE_MODE_INTERLACE;
         break;
   }


   InitializeWindow( hInstance, nCmdShow );

   if ( InitializeGraphics( ) != DD_OK )
      return -1;

#if   AUDIO_ON

   if ( !SoundBegin( hwnd, 2, 2 ) )
   {
      ExitGraphics( );
      return -1;
   }

#endif

   movieInit ( 1, NULL );     // lpdd if DirectDraw Blit is used.
   hnd = -1;

 	while ( 1 )
	{
 		if ( PeekMessage ( &msg, NULL, 0 , 0, PM_NOREMOVE ) )
   	{
			if ( !GetMessage ( &msg, NULL, 0, 0 ) )
            break;

         TranslateMessage ( &msg );
         DispatchMessage ( &msg );
		}
      Sleep( 0 );
      if ( hnd == -1 )
      {
         hnd = movieOpen( avi, wav,
                        lpFrontBuffer, 0, 0,
                        xStart, yStart, movieMode, audioFlag );

         if ( hnd < 0 )
            break;

         if ( movieStart( hnd ) != MOVIE_OK )
         {
            movieClose( hnd );
            break;
         }
      }

      if ( !movieIsPlaying( hnd ) )
      {
         movieClose( hnd );
         break;
      }
   }

   movieUnInit( );

#if   AUDIO_ON

   SoundEnd( );

#endif

   ExitGraphics( );

   if ( hnd < 0 ) return -1;
   else return 0;
}
