#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "rsrc.h"
#include "hash.h"

char FileList[200],OutputFile[200];
char PathFile[200];

long TheTime;

struct ImageList
{
	ImageFmt Header;
	long ImageSize;
	long PaletteSize;
	void *Image;
	void *Palette;
};

C_Hash *ColorOrder=NULL;
C_Hash *IDOrder=NULL;
C_Hash *ImageTable=NULL;

long HeaderSize=0;
long DataSize=0;

long UsePathList=0;
long UseColorKey=0;
char InternalName[200];
char Label[32];
char *OriginalImage=NULL;
long ImageW,ImageH;
long UniqueID=1;
char Line[300];
long InputSize=0;
long OutputSize=0;

long COLORKEY=0x7c1f;

enum
{
	CLR_KEY=1,
	FILE_NAME,
};

int ParseCommandLine(LPSTR  lpCmdLine)
{
	char *Token;
	long expect=0;
	long expecttype=0;

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

	Token=strtok(NULL," \t\n\r,");
	while(Token)
	{
		if(!expect)
		{
			if(!strnicmp(Token,"/c",2))
			{
				expect=1;
				expecttype=CLR_KEY;
			}
			if(!strnicmp(Token,"/p",2))
			{
				expect=1;
				expecttype=FILE_NAME;
			}
		}
		else
		{
			switch(expecttype)
			{
				case CLR_KEY:
					expect=0;
					COLORKEY=atol(Token);
					expecttype=0;
					break;
				case FILE_NAME:
					UsePathList=1;
					strcpy(PathFile,Token);
					expect=0;
					expecttype=0;
					break;
				default:
					expect=0;
					expecttype=0;
					break;
			}
		}
		Token=strtok(NULL," \t\n\r,");
	}
	return(1);
}

long BuildColorTable(WORD *img,long x,long y,long w,long h,long width,long height)
{
	long i,j;
	long count;
	long color;

	if(!w || !h || !width || !height)
	{
		printf("Error invalid params to color table (%1ld,%1ld,%1ld,%1ld) imw=%1ld\n",x,y,w,h,width);
		return(0);
	}

	if(ColorOrder)
	{
		ColorOrder->Cleanup();
		delete ColorOrder;
	}
	if(IDOrder)
	{
		IDOrder->Cleanup();
		delete IDOrder;
	}
	ColorOrder=new C_Hash;
	ColorOrder->Setup(512);

	IDOrder=new C_Hash;
	IDOrder->Setup(512);

	if(UseColorKey)
	{
		color=COLORKEY;
		ColorOrder->Add(color,(void*)1);
		IDOrder->Add(0,(void*)COLORKEY);
		count=1;
	}
	else
		count=0;
	for(i=y;i<y+h;i++)
		for(j=x;j<x+w;j++)
		{
			color=img[(height-i)*width+j];
			if(!ColorOrder->Find(img[(height-i)*width+j]))
			{
				ColorOrder->Add(color,(void*)(count+1));
				IDOrder->Add(count,(void*)(img[(height-i)*width+j]));
				count++;
			}
		}
	return(count);
}

WORD *MakePalette(long entries)
{
	long i;
	WORD *palette;

	palette=new WORD[entries];

	for(i=0;i<entries;i++)
		palette[i]=(WORD)(IDOrder->Find(i));

	return(palette);
}

unsigned char *ConvertTo8Bit(WORD *img,long x,long y,long w,long h,long width,long height)
{
	unsigned char *newimage;
	long i,j,didx;
	short pal;

	if(!img || !width || !w || !h || !height)
		return(NULL);

	newimage=new unsigned char[w*h];
	if(newimage)
	{
		didx=0;
		for(i=y;i<y+h;i++)
			for(j=x;j<x+w;j++)
			{
				pal=(short)(ColorOrder->Find(img[(height-i)*width+j]));
				newimage[didx++]=(short)(ColorOrder->Find(img[(height-i)*width+j]))-1;
			}
	}
	return(newimage);
}

WORD *CopySubArea(WORD *Image,long x,long y,long w,long h,long width,long height)
{
	WORD *newimage;
	long i,j,didx;

	if(!Image || !w || !h || !width || !height)
		return(NULL);

	newimage=new WORD[w*h];
	if(newimage)
	{

		didx=0;
		for(i=y;i<y+h;i++)
			for(j=x;j<x+w;j++)
				newimage[didx++]=Image[(height-i)*width+j];
	}
	return(newimage);
}

BOOL LoadTarga16File( char *filename, char **image, BITMAPINFO *bmi )
{
	HANDLE hFile;
	char buf[16];
	char *data;
	short width,height;
	DWORD dwBytesRead;


	hFile = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,	   
		(LPSECURITY_ATTRIBUTES) NULL, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,(HANDLE) NULL);	    

	if (hFile == INVALID_HANDLE_VALUE) 
		return FALSE;

	InputSize+=GetFileSize(hFile,NULL);

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
	data = new char[ width * height * 2 ];
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

void SaveResource(char *filename)
{
	FILE *header,*data;
	long curpos;
	char outname[200];
	ImageList *rec;

	strcpy(outname,filename);
	strcat(outname,".idx");

	header=fopen(outname,"wb");

	if(!header)
	{
		printf("Can't create output file (%s)... exiting\n",OutputFile);
		return;
	}

	strcpy(outname,filename);
	strcat(outname,".rsc");

	data=fopen(outname,"wb");
	if(!data)
	{
		fclose(header);
		printf("Can't create output file (%s)... exiting\n",OutputFile);
		return;
	}

	fwrite(&HeaderSize, sizeof(long), 1, header);
	fwrite(&TheTime,sizeof(long),1, header);

	fwrite(&DataSize, sizeof(long), 1, data);
	fwrite(&TheTime,sizeof(long),1, data);

	curpos=0;

	rec=(ImageList*)ImageTable->GetFirst();
	while(rec)
	{
		rec->Header.imageoffset=curpos;
		fwrite(rec->Image,rec->ImageSize,1,data);
		curpos+=rec->ImageSize;
		if(rec->Header.flags & _RSC_8_BIT_)
		{
			rec->Header.paletteoffset=curpos;
			fwrite(rec->Palette,rec->PaletteSize,1,data);
			curpos+=rec->PaletteSize;
		}

		fwrite(&rec->Header,sizeof(ImageFmt),1,header);
		rec=(ImageList*)ImageTable->GetNext();
	}

	fclose(header);
	fclose(data);
	OutputSize=HeaderSize+DataSize;
}

char *TODOList[]=
{
	NULL,
	"[LOADIMAGE]",
	"[LOADTRANSIMAGE]",
	"[LOADDISCARD]",
	"[LOADTRANSDISCARD]",
	"[ADDIMAGE]",
	"[LOADPATH]",
	"[LOADTRANSPATH]",
	NULL,
};

enum
{
	LOAD_IMAGE=1,
	LOAD_TRANSPARENT,
	LOAD_DISCARD,
	LOAD_DISCARD_TRANSPARENT,
	ADD_IMAGE,
	LOAD_PATH,
	LOAD_PATH_TRANSPARENT,
};

long FindTODO(char *token)
{
	int i;

	i=1;
	while(TODOList[i])
	{
		if(!stricmp(token,TODOList[i]))
			return(i);
		i++;
	}
	return(0);
}

void ProcessImageLine(char buffer[])
{
	char *token;
	long whattodo;
	int i;
	char ImageName[200];
	char ParentID[32];
	char *img;
	BITMAPINFO bmi;
	long colors;
	WORD *Palette;
	WORD *Image16;
	char *Image8;
	long x,y,w,h,cx,cy,size;
	ImageList *ImageRecord;

	x=0;
	y=0;
	w=0;
	h=0;
	cx=0;
	cy=0;
	size=0;
	Image8=NULL;
	Image16=NULL;
	Palette=NULL;

	token=strtok(buffer," ,\t\n");
	if(!token)
		return;

	whattodo=FindTODO(token);
	if(!whattodo)
		return;

	if(whattodo == LOAD_PATH || whattodo == LOAD_PATH_TRANSPARENT)
	{
		token=strtok(NULL," ,\t\n");
		if(!token)
			return;
		if(token[0] != '"')
		{
			printf("Error in output filename (%s)\n",token);
			return;
		}
		token++;
		i=0;
		while(token[i] && token[i] != '"')
			i++;
		token[i]=0;
		strcpy(OutputFile,token);
	}

	token=strtok(NULL," ,\t\n");
	if(!token)
		return;
	strcpy(Label,token);

	token=strtok(NULL," ,\t\n");
	if(!token)
		return;

	if(whattodo == LOAD_IMAGE || whattodo == LOAD_DISCARD || whattodo == LOAD_TRANSPARENT || whattodo == LOAD_DISCARD_TRANSPARENT || whattodo == LOAD_PATH || whattodo == LOAD_PATH_TRANSPARENT)
	{
		if(token[0] != '"')
		{
			printf("Error in filename (%s)\n",token);
			return;
		}
		token++;
		i=0;
		while(token[i] && token[i] != '"')
			i++;
		token[i]=0;
		strcpy(ImageName,token);
		token=strtok(NULL," ,\t\n");
	}

	if(whattodo == LOAD_IMAGE || whattodo == LOAD_TRANSPARENT || whattodo == LOAD_DISCARD || whattodo == LOAD_DISCARD_TRANSPARENT || whattodo == LOAD_PATH || whattodo == LOAD_PATH_TRANSPARENT)
	{
		if(OriginalImage)
			delete OriginalImage;
		if(!LoadTarga16File(ImageName, &OriginalImage, &bmi ))
		{
			printf("Error loading (%s)... ignoring this line\n",ImageName);
			OriginalImage=NULL;
			return;
		}
		ImageW=bmi.bmiHeader.biWidth;
		ImageH=bmi.bmiHeader.biHeight;
		x=0;
		y=0;
		w=ImageW;
		h=ImageH;
	}

	if(!OriginalImage)
	{
		printf("No image to process\n");
		return;
	}

	if(whattodo == LOAD_IMAGE || whattodo == LOAD_DISCARD)
		UseColorKey=0;
	if(whattodo == LOAD_TRANSPARENT || whattodo == LOAD_DISCARD_TRANSPARENT || whattodo == LOAD_PATH_TRANSPARENT)
		UseColorKey=_RSC_COLORKEY_;

	if(whattodo == LOAD_IMAGE || whattodo == LOAD_TRANSPARENT || whattodo == LOAD_PATH || whattodo == LOAD_PATH_TRANSPARENT)
	{
		if(token)
		{
			cx=atol(token);

			token=strtok(NULL," ,\t\n");
			if(token)
				cy=atol(token);
		}
	}
	else if(whattodo != LOAD_DISCARD && whattodo != LOAD_DISCARD_TRANSPARENT)
	{
		i=0;
		while(token)
		{
			switch(i)
			{
				case 0:
					x=atol(token);
					break;
				case 1:
					y=atol(token);
					break;
				case 2:
					w=atol(token);
					break;
				case 3:
					h=atol(token);
					break;
				case 4:
					cx=atol(token);
					break;
				case 5:
					cy=atol(token);
					break;
			}
			i++;
			token=strtok(NULL," ,\t\n");
		}
		if(i < 4)
		{
			printf("Not enough parameters...\n");
			return;
		}
	}

	if(whattodo != LOAD_DISCARD && whattodo != LOAD_DISCARD_TRANSPARENT && whattodo != LOAD_PATH && whattodo != LOAD_PATH_TRANSPARENT)
	{
		if(cx == -1)
			cx=w/2;
		if(cy == -1)
			cy=h/2;
		ImageRecord=new ImageList;
		memset(ImageRecord,0,sizeof(ImageList));

		colors=BuildColorTable((WORD*)OriginalImage,x,y,w,h,ImageW,ImageH-1);
		if(colors && colors <= 256)
		{
			size=w*h;
			Palette=MakePalette(colors);
			Image8=(char *)ConvertTo8Bit((WORD*)OriginalImage,x,y,w,h,ImageW,ImageH-1);

			ImageRecord->Header.Type=_RSC_IS_IMAGE_;
			strcpy(ImageRecord->Header.ID,Label);
			ImageRecord->Header.flags=_RSC_8_BIT_ | UseColorKey;
			ImageRecord->Header.centerx=cx;
			ImageRecord->Header.centery=cy;
			ImageRecord->Header.w=w;
			ImageRecord->Header.h=h;
			ImageRecord->Header.imageoffset=0;
			ImageRecord->Header.palettesize=colors;
			ImageRecord->Header.paletteoffset=0;

			ImageRecord->ImageSize=size;
			ImageRecord->PaletteSize=colors*sizeof(WORD);
			ImageRecord->Image=Image8;
			ImageRecord->Palette=Palette;

			HeaderSize+=sizeof(ImageFmt);
			DataSize+=(ImageRecord->ImageSize+ImageRecord->PaletteSize);
		}
		else
		{
			size=w*h*sizeof(WORD);

			Image16=CopySubArea((WORD*)OriginalImage,x,y,w,h,ImageW,ImageH-1);

			ImageRecord->Header.Type=_RSC_IS_IMAGE_;
			strcpy(ImageRecord->Header.ID,Label);
			ImageRecord->Header.flags=_RSC_16_BIT_ | UseColorKey;
			ImageRecord->Header.centerx=cx;
			ImageRecord->Header.centery=cy;
			ImageRecord->Header.w=w;
			ImageRecord->Header.h=h;
			ImageRecord->Header.imageoffset=0;
			ImageRecord->Header.paletteoffset=0;

			ImageRecord->ImageSize=size;
			ImageRecord->PaletteSize=0;
			ImageRecord->Image=Image16;

			HeaderSize+=sizeof(ImageFmt);
			DataSize+=(ImageRecord->ImageSize);
		}
		ImageTable->Add(UniqueID++,ImageRecord);
	}
}

void ProcessPathList(char buffer[])
{
	FILE *pfp;
	char pathbuf[202];

	ImageTable=new C_Hash;
	ImageTable->Setup(512);
	ImageTable->SetFlags(HSH_REMOVE);

	ProcessImageLine(buffer);

	pfp=fopen(PathFile,"r");
	if(!pfp)
	{
		printf("Can't open path file (%s)\n",PathFile);
		return;
	}

	while(fgets(pathbuf,200,pfp) > 0)
		ProcessImageLine(pathbuf);

	fclose(pfp);

	SaveResource(OutputFile);
	if(ImageTable)
	{
		ImageTable->Cleanup();
		delete ImageTable;
		ImageTable=NULL;
	}
	if(OriginalImage)
	{
		delete OriginalImage;
		OriginalImage=NULL;
	}
	if(InputSize)
		printf("Input (%1ld)... Output (%1ld)  %%%1ld\n",InputSize,OutputSize,OutputSize*100/InputSize);
	InputSize=0;
	OutputSize=0;
	HeaderSize=0;
	DataSize=0;
}

int PASCAL WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR  lpCmdLine, int nCmdShow)
{
	FILE *ifp;
	char buffer[220];

	if(!ParseCommandLine (lpCmdLine))
	{
		printf("Make Resource - Version 1.0 - by Peter Ward\n\n");
		printf("Usage: MAKERSC [path]<imagerc.irc> [path]<output>\n");
		printf("    Sorry... ALL input MUST be in 16bit targa format\n");
		return(0);
	}

	ifp=fopen(FileList,"r");
	if(!ifp)
	{
		printf("Can't open imagerc.irc file (%s)\n",FileList);
		return(0);
	}

	TheTime=GetCurrentTime();

	if(!UsePathList)
	{
		ImageTable=new C_Hash;
		ImageTable->Setup(512);
		ImageTable->SetFlags(HSH_REMOVE);

		while(fgets(buffer,200,ifp) > 0)
			ProcessImageLine(buffer);
		fclose(ifp);

		SaveResource(OutputFile);
		if(ImageTable)
		{
			ImageTable->Cleanup();
			delete ImageTable;
		}
		if(OriginalImage)
			delete OriginalImage;

		if(InputSize)
			printf("Input (%1ld)... Output (%1ld)  %%%1ld\n",InputSize,OutputSize,OutputSize*100/InputSize);
	}
	else
	{
		while(fgets(buffer,200,ifp) > 0)
			ProcessPathList(buffer);
		fclose(ifp);
	}

	if(ColorOrder)
	{
		ColorOrder->Cleanup();
		delete ColorOrder;
	}
	if(IDOrder)
	{
		IDOrder->Cleanup();
		delete IDOrder;
	}
	return(0);
}
