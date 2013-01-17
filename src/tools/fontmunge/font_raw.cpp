#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>

#include "fontres.h"

typedef struct
{
	unsigned char r,g,b;
} RGB24BIT;

short ReplaceChar[][2]=
{
	{130,','},
	{131,'f'},
	{132,'"'},
	{133,'_'},
	{134,'+'},
	{135,'+'},
	{136,'^'},
	{137,'%'},
	{138,'S'},
	{139,'<'},
	{140,'E'},
	{145,'`'},
	{146,'\''},
	{147,'"'},
	{148,'"'},
	{149,'*'},
	{150,'-'},
	{151,'_'},
	{152,'~'},
	{153,'M'},
	{154,'S'},
	{155,'>'},
	{156,'e'},
	{159,'Y'},
	{160,' '},
	{161,';'},
	{162,'c'},
	{164,'o'},
	{166,'|'},
	{167,'S'},
	{168,'"'},
	{169,'C'},
	{170,'a'},
	{172,'-'},
	{173,'-'},
	{174,'O'},
	{175,'_'},
	{176,'o'},
	{177,'+'},
	{178,'2'},
	{179,'3'},
	{180,'\''},
	{181,'U'},
	{182,'P'},
	{183,'.'},
	{184,','},
	{185,'1'},
	{186,'o'},
	{191,'?'},
	{192,'A'},
	{193,'A'},
	{194,'A'},
	{195,'A'},
	{199,'C'},
	{200,'E'},
	{201,'E'},
	{202,'E'},
	{203,'E'},
	{204,'I'},
	{205,'I'},
	{206,'I'},
	{207,'I'},
	{208,'D'},
	{210,'O'},
	{211,'O'},
	{212,'O'},
	{213,'O'},
	{214,'O'},
	{215,'x'},
	{217,'U'},
	{218,'U'},
	{219,'U'},
	{221,'Y'},
	{222,'P'},
	{223,'B'},
	{224,'a'},
	{225,'a'},
	{226,'a'},
	{227,'a'},
	{228,'a'},
	{229,'a'},
	{230,'a'},
	{231,'c'},
	{232,'e'},
	{233,'e'},
	{234,'e'},
	{235,'e'},
	{236,'i'},
	{237,'i'},
	{238,'i'},
	{239,'i'},
	{240,'o'},
	{241,'n'},
	{242,'o'},
	{243,'o'},
	{244,'o'},
	{245,'o'},
	{246,'o'},
	{247,'-'},
	{248,'o'},
	{249,'u'},
	{250,'u'},
	{251,'u'},
	{252,'u'},
	{253,'y'},
	{254,'p'},
	{255,'y'},
	{0,0},
};

C_Fontmgr *thefont;

#define OUTPUT_X    256
#define OUTPUT_Y    256

long UseReplace=0;

void ExportToRaw(char *filename)
{
	RGB24BIT *mem;
	long i,j,s,x,y,k,l,count;
	short rep;
	long owidth;
	_TCHAR buffer[200];
	FILE *fp,*rfp;
	CharStr *chr;
	unsigned char *data,seg;

	mem=new RGB24BIT[OUTPUT_X*OUTPUT_Y];
	memset(mem,0,OUTPUT_X*OUTPUT_Y*sizeof(RGB24BIT));

	count=0;
	y=2;
	x=2;

	strcpy(buffer,filename);
	strcat(buffer,".rct");
	rfp=fopen(buffer,"w");
	if(!rfp)
	{
		printf("Can't open output rectangle file (%s)\n",buffer);
		return;
	}
	fprintf(rfp,"\"%s\" %1ld %1ld %1ld\n",thefont->GetName(),(long)thefont->First(),(long)thefont->Last(),thefont->ByteWidth());

	data=(unsigned char*)thefont->GetData();

	for(i=thefont->First();i <= thefont->Last();i++)
	{
		owidth = thefont->Width((char) i);

		chr=thefont->GetChar(i);
		rep=i;
		if(UseReplace)
		{
			for(s=0;ReplaceChar[s][0];s++)
				if(i == ReplaceChar[s][0])
					rep=ReplaceChar[s][1];
			if(rep != i)
				printf("char [%1ld] replaced with [%1d]\n",i,rep);
		}
		chr=thefont->GetChar(rep);

		for(j=0;j<thefont->Height();j++)
		{
			for(k=0;k<thefont->ByteWidth();k++)
			{
				seg=data[(rep-thefont->First())*thefont->ByteWidth()*thefont->Height() + k+j*thefont->ByteWidth()];
				for(l=0;l<8;l++)
				{
					if(seg & 1)
					{
						if(rep == i)
						{
							mem[(y+j)*OUTPUT_X+x+k*8+l].r=0xff;
							mem[(y+j)*OUTPUT_X+x+k*8+l].g=0xff;
							mem[(y+j)*OUTPUT_X+x+k*8+l].b=0xff;
						}
						else
						{
							mem[(y+j)*OUTPUT_X+x+k*8+l].g=0xff;
						}
					}
					seg >>= 1;
				}
			}
		}

		fprintf(rfp,"%1ld %1ld %1ld %1ld %1ld %1d %1d\n",i,x,y,chr->w,thefont->Height(),chr->lead,chr->trail);

		x+=owidth;
		if(x > (256-owidth))
		{
			x=2;
			y+=thefont->Height();
		}
	}

	fclose(rfp);

	strcpy(buffer,filename);
	strcat(buffer,".raw");
	fp=fopen(buffer,"wb");
	if(fp)
	{
		fwrite(mem,OUTPUT_X*OUTPUT_Y*sizeof(RGB24BIT),1,fp);
		fclose(fp);
	}
	else
		printf("Can't open output bitmap (%s)\n",buffer);
	delete mem;
}

void LoadFont(char *fontname)
{
	thefont=new C_Fontmgr;
	thefont->Setup(1,fontname);
}

void main(int argc,char **argv)
{
	printf("FONT_RAW - Version 1.0 by Peter Ward - Export my Font Bitmap to 800x600 16bit RAW file\n");
	if(argc < 3)
	{
		printf("Usage: <fontfile.bft> <output (no .ext)> [/r]\n");
		printf("       /r = replace foreign language characters with\n");
		printf("            a similar (if they are missing)\n");
		exit(0);
	}
	if(argc > 3)
		if(!stricmp(argv[3],"/r"))
		{
			UseReplace=1;
			printf("Substituting missing characters\n");
		}
	LoadFont(argv[1]);
	ExportToRaw(argv[2]);
	if(thefont)
		delete thefont;
}
