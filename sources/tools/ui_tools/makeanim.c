#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "..\delta\anim.h"

char FileList[200],OutputFile[200];

ANIMATION MyHeader;

char Line[300];
int ParseCommandLine(LPSTR  lpCmdLine)
{
	char *Token;

	memcpy(&Line[0],lpCmdLine,strlen(lpCmdLine)+1);

	Token=strtok(&Line[0]," \t\n\r,");
	if(Token == NULL)
	{
		printf("No List file specified\n");
		return(0);
	}
	sprintf(FileList,"%s",Token);
	Token=strtok(NULL," \t\n\r,");
	if(Token == NULL)
	{
		printf("No output file specified\n");
		return(0);
	}
	sprintf(OutputFile,"%s",Token);
	return(1);
}


BOOL LoadTarga16File( char *filename, char **image, BITMAPINFO *bmi )
{
	HANDLE hFile;
	char buf[16];
	char *data;
	short width,height;
	DWORD dwBytesRead;


 	hFile = CreateFile(filename,   
    	GENERIC_READ,              
        FILE_SHARE_READ,           
        (LPSECURITY_ATTRIBUTES) NULL, 
        OPEN_EXISTING,             
        FILE_ATTRIBUTE_NORMAL,     
        (HANDLE) NULL);            

	if (hFile == INVALID_HANDLE_VALUE) 
		return FALSE;

	// For 15-bit Targa file, skip first 12 bytes.
  	if ( !ReadFile(hFile, buf, 12, &dwBytesRead, NULL) )
		return NULL;

	// Read width
	if ( !ReadFile(hFile, &width, 2, &dwBytesRead, NULL) )
		return NULL;

	// Read height
  	if ( !ReadFile(hFile, &height, 2, &dwBytesRead, NULL) )
		return NULL;

	// For 15-bit Targa file, skip last 2 bytes.
  	if ( !ReadFile(hFile, buf, 2, &dwBytesRead, NULL) )
		return NULL;

	// Read in image data
	data = (char *) malloc( width * height * 2 );
	if ( !ReadFile(hFile, data, width * height * 2, &dwBytesRead, NULL) )
		return NULL;	

	CloseHandle(hFile);

	*image = data;

	bmi->bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	bmi->bmiHeader.biWidth = width;
	bmi->bmiHeader.biHeight = height;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biBitCount = 16;
	bmi->bmiHeader.biCompression = BI_RGB;
	bmi->bmiHeader.biSizeImage = 0;
	bmi->bmiHeader.biXPelsPerMeter = 72;
	bmi->bmiHeader.biYPelsPerMeter = 72;
	bmi->bmiHeader.biClrUsed = 0;
	bmi->bmiHeader.biClrImportant = 0;

	return TRUE;
}

int PASCAL WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR  lpCmdLine, int nCmdShow)
{
	HANDLE ifp,ofp;
	char *List,*Token;
	char *Image;
	long Size,i;
	BITMAPINFO bmi;
	long ResX=-1,ResY=-1;
	DWORD bytesread;

	strcpy(MyHeader.Header,"ANIM");
	MyHeader.Version=1;

	if(!ParseCommandLine (lpCmdLine))
	{
		printf("Usage: MAKEANIM <list> <output>\n");
		printf("    Sorry... only handles 15bit targa at the moment\n");
		return(0);
	}
	ifp=CreateFile(FileList,GENERIC_READ,FILE_SHARE_READ,NULL,
				   OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
				   NULL);

	if(ifp == INVALID_HANDLE_VALUE)
		return(0);

	Size=GetFileSize(ifp,NULL);
	List=(char *)malloc(Size+1);

	ReadFile(ifp,List,Size,&bytesread,NULL);
	List[Size]=0;
	CloseHandle(ifp);

	ofp=CreateFile(OutputFile,GENERIC_WRITE,FILE_SHARE_WRITE,NULL,
				   CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,
				   NULL);

	if(ofp == INVALID_HANDLE_VALUE)
	{
		free(List);
		printf("Can't create output file (%s)... exiting\n",OutputFile);
		return(0);
	}

	Token=strtok(List," \t\n\r,");
	if(Token == NULL)
	{
		printf("No Frame # Specified\n");
		return(0);
	}
	MyHeader.Frames=atol(Token);

	Token=strtok(NULL," \t\n\r,");

	while(Token != NULL)
	{
		if(LoadTarga16File(Token, &Image, &bmi ))
		{
			if(ResX == -1 && ResY == -1)
			{
				MyHeader.Width=bmi.bmiHeader.biWidth;
				MyHeader.Height=bmi.bmiHeader.biHeight;
				MyHeader.BytesPerPixel=2;
				MyHeader.Compression=COMP_NONE;
				MyHeader.Background=-1;
				ResX=MyHeader.Width;
				ResY=MyHeader.Height;
				Size=ResX * ResY * MyHeader.BytesPerPixel;
				WriteFile(ofp,&MyHeader,sizeof(ANIMATION),&bytesread,NULL);
				WriteFile(ofp,&Size,sizeof(long),&bytesread,NULL);

				for(i=0;i<MyHeader.Height;i++)
				{
//					printf("%1X\n",(long)&Image[(MyHeader.Height - i - 1) * MyHeader.Width * MyHeader.BytesPerPixel]);
					WriteFile(ofp,&Image[(MyHeader.Height - i - 1) * MyHeader.Width * MyHeader.BytesPerPixel],MyHeader.Width * MyHeader.BytesPerPixel,&bytesread,NULL);
				}
			}
			else
			{
				if(ResX != bmi.bmiHeader.biWidth || ResY != bmi.bmiHeader.biHeight)
				{
					printf("(%s) has a different resolution than the prev targa\n",Token);
					printf("Skipping file\n");
					free(Image);
				}
				else
				{
					Size=ResX * ResY * MyHeader.BytesPerPixel;
					WriteFile(ofp,&Size,sizeof(long),&bytesread,NULL);
					for(i=0;i<MyHeader.Height;i++)
					{
//						printf("%1X\n",(long)&Image[(MyHeader.Height - i - 1) * MyHeader.Width * MyHeader.BytesPerPixel]);
						WriteFile(ofp,&Image[(MyHeader.Height - i - 1) * MyHeader.Width * MyHeader.BytesPerPixel],MyHeader.Width * MyHeader.BytesPerPixel,&bytesread,NULL);
					}
					free(Image);
				}
			}
		}
		Token=strtok(NULL," \t\n\r,");
	}
	CloseHandle(ofp);
	return(0);
}
