#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include "fontres.h"

struct RGBType
{
	unsigned char r,g,b;
};

void ImportFromRaw8(char *name)
{
	char buffer[200];
	short first, last;
	long x,y,height;
	char *token;
	long bytesperline;
	unsigned char *mem;
	char fontname[32];
	CharStr *chr;
	long i;
	RECT *rct;
	FILE *fp;
	unsigned char *fontdata;

	strcpy(buffer,name);
	strcat(buffer,".rct");

	fp=fopen(buffer,"r");
	if(!fp)
		return;

	if(!(fgets(buffer,200,fp) > 0))
	{
		fclose(fp);
		return;
	}
	token=strtok(buffer,"\"");
	if(!token)
	{
		fclose(fp);
		return;
	}
	memset(fontname,0,32);
	strcpy(fontname,token);

	token=strtok(NULL," ,\n\t");
	if(token)
		first=atol(token);
	token=strtok(NULL," ,\n\t");
	if(token)
		last=atol(token);
	token=strtok(NULL," ,\n\t");
	if(token)
		bytesperline=atol(token);

	chr=new CharStr[last-first+1];
	memset(chr,0,sizeof(CharStr)*(last-first+1));
	rct=new RECT[last-first+1];
	memset(rct,0,sizeof(RECT)*(last-first+1));

	for(i=0;i<last-first+1;i++)
	{
		if(fgets(buffer,200,fp) > 0)
		{
			token=strtok(buffer," \t\n,");
			if(!token)
			{
				fclose(fp);
				return;
			}
			token=strtok(NULL," \t\n,");
			if(token)
				rct[i].left=atol(token);
			token=strtok(NULL," \t\n,");
			if(token)
				rct[i].top=atol(token);
			token=strtok(NULL," \t\n,");
			if(token)
				rct[i].right=atol(token);
			token=strtok(NULL," \t\n,");
			if(token)
				rct[i].bottom=atol(token);
			token=strtok(NULL," \t\n,");
			if(token)
				chr[i].lead=atol(token);
			token=strtok(NULL," \t\n,");
			if(token)
				chr[i].trail=atol(token);

			chr[i].w=rct[i].right;
		}
	}
	fclose(fp);

	mem=new unsigned char[800*600];

	strcpy(buffer,name);
	strcat(buffer,".raw");

	fp=fopen(buffer,"rb");
	if(fp)
	{
		fread(mem,800*600,1,fp);
		fclose(fp);
	}
	else
		return;

	height=rct[0].bottom;
	fontdata=new unsigned char[(last-first+1)*bytesperline*height];
	memset(fontdata,0,(last-first+1)*bytesperline*height);

	for(i=0;i<(last-first+1);i++)
	{
		for(y=0;y<rct[i].bottom;y++)
			for(x=0;x<rct[i].right;x++)
				if(mem[(y + rct[i].top)*800+(x+rct[i].left)])
					fontdata[i*bytesperline*height + y*bytesperline + x/8] |= 1 << (x % 8);
	}

	i=last-first+1;
	strcpy(buffer,name);
	strcat(buffer,".bft");
	fp=fopen(buffer,"wb");
	if(fp)
	{
		fwrite(fontname,32,1,fp);
		fwrite(&height,sizeof(long),1,fp);
		fwrite(&first,sizeof(short),1,fp);
		fwrite(&last, sizeof(short),1,fp);
		fwrite(&bytesperline,sizeof(long),1,fp);
		fwrite(&i,sizeof(long),1,fp);
		i=0;
		fwrite(&i,sizeof(long),1,fp);
		i=(last-first+1)*bytesperline*height;
		fwrite(&i,sizeof(long),1,fp);
		i=last-first+1;
		if(i)
			fwrite(chr,sizeof(CharStr),i,fp);
		i=(last-first+1)*bytesperline*height;
		if(i)
			fwrite(fontdata,i,1,fp);
		fclose(fp);
	}
}

void ImportFromRaw16(char *name)
{
	char buffer[200];
	short first, last;
	long x,y,height;
	char *token;
	long bytesperline;
	WORD *mem;
	char fontname[32];
	CharStr *chr;
	long i;
	RECT *rct;
	FILE *fp;
	unsigned char *fontdata;

	strcpy(buffer,name);
	strcat(buffer,".rct");

	fp=fopen(buffer,"r");
	if(!fp)
		return;

	if(!(fgets(buffer,200,fp) > 0))
	{
		fclose(fp);
		return;
	}
	token=strtok(buffer,"\"");
	if(!token)
	{
		fclose(fp);
		return;
	}
	memset(fontname,0,32);
	strcpy(fontname,token);

	token=strtok(NULL," ,\n\t");
	if(token)
		first=atol(token);
	token=strtok(NULL," ,\n\t");
	if(token)
		last=atol(token);
	token=strtok(NULL," ,\n\t");
	if(token)
		bytesperline=atol(token);

	chr=new CharStr[last-first+1];
	memset(chr,0,sizeof(CharStr)*(last-first+1));
	rct=new RECT[last-first+1];
	memset(rct,0,sizeof(RECT)*(last-first+1));

	for(i=0;i<last-first+1;i++)
	{
		if(fgets(buffer,200,fp) > 0)
		{
			token=strtok(buffer," \t\n,");
			if(!token)
			{
				fclose(fp);
				return;
			}
			token=strtok(NULL," \t\n,");
			if(token)
				rct[i].left=atol(token);
			token=strtok(NULL," \t\n,");
			if(token)
				rct[i].top=atol(token);
			token=strtok(NULL," \t\n,");
			if(token)
				rct[i].right=atol(token);
			token=strtok(NULL," \t\n,");
			if(token)
				rct[i].bottom=atol(token);
			token=strtok(NULL," \t\n,");
			if(token)
				chr[i].lead=atol(token);
			token=strtok(NULL," \t\n,");
			if(token)
				chr[i].trail=atol(token);

			chr[i].w=rct[i].right;
		}
	}
	fclose(fp);

	mem=new WORD[800*600];

	strcpy(buffer,name);
	strcat(buffer,".raw");

	fp=fopen(buffer,"rb");
	if(fp)
	{
		fread(mem,800*600*2,1,fp);
		fclose(fp);
	}
	else
		return;

	height=rct[0].bottom;
	fontdata=new unsigned char[(last-first+1)*bytesperline*height];
	memset(fontdata,0,(last-first+1)*bytesperline*height);

	for(i=0;i<(last-first+1);i++)
	{
		for(y=0;y<rct[i].bottom;y++)
			for(x=0;x<rct[i].right;x++)
				if(mem[(y + rct[i].top)*800+(x+rct[i].left)])
					fontdata[i*bytesperline*height + y*bytesperline + x/8] |= 1 << (x % 8);
	}

	i=last-first+1;
	strcpy(buffer,name);
	strcat(buffer,".bft");
	fp=fopen(buffer,"wb");
	if(fp)
	{
		fwrite(fontname,32,1,fp);
		fwrite(&height,sizeof(long),1,fp);
		fwrite(&first,sizeof(short),1,fp);
		fwrite(&last, sizeof(short),1,fp);
		fwrite(&bytesperline,sizeof(long),1,fp);
		fwrite(&i,sizeof(long),1,fp);
		i=0;
		fwrite(&i,sizeof(long),1,fp);
		i=(last-first+1)*bytesperline*height;
		fwrite(&i,sizeof(long),1,fp);
		i=last-first+1;
		if(i)
			fwrite(chr,sizeof(CharStr),i,fp);
		i=(last-first+1)*bytesperline*height;
		if(i)
			fwrite(fontdata,i,1,fp);
		fclose(fp);
	}
}

void ImportFromRaw24(char *name)
{
	char buffer[200];
	short first, last;
	long x,y,height;
	char *token;
	long bytesperline;
	RGBType *mem;
	CharStr *chr;
	char fontname[32];
	long i;
	RECT *rct;
	FILE *fp;
	unsigned char *fontdata;

	strcpy(buffer,name);
	strcat(buffer,".rct");

	fp=fopen(buffer,"r");
	if(!fp)
		return;

	if(!(fgets(buffer,200,fp) > 0))
	{
		fclose(fp);
		return;
	}
	token=strtok(buffer,"\"");
	if(!token)
	{
		fclose(fp);
		return;
	}
	memset(fontname,0,32);
	strcpy(fontname,token);

	token=strtok(NULL," ,\n\t");
	if(token)
		first=atol(token);
	token=strtok(NULL," ,\n\t");
	if(token)
		last=atol(token);
	token=strtok(NULL," ,\n\t");
	if(token)
		bytesperline=atol(token);

	chr=new CharStr[last-first+1];
	memset(chr,0,sizeof(CharStr)*(last-first+1));
	rct=new RECT[last-first+1];
	memset(rct,0,sizeof(RECT)*(last-first+1));

	for(i=0;i<last-first+1;i++)
	{
		if(fgets(buffer,200,fp) > 0)
		{
			token=strtok(buffer," \t\n,");
			if(!token)
			{
				fclose(fp);
				return;
			}
			token=strtok(NULL," \t\n,");
			if(token)
				rct[i].left=atol(token);
			token=strtok(NULL," \t\n,");
			if(token)
				rct[i].top=atol(token);
			token=strtok(NULL," \t\n,");
			if(token)
				rct[i].right=atol(token);
			token=strtok(NULL," \t\n,");
			if(token)
				rct[i].bottom=atol(token);
			token=strtok(NULL," \t\n,");
			if(token)
				chr[i].lead=atol(token);
			token=strtok(NULL," \t\n,");
			if(token)
				chr[i].trail=atol(token);

			chr[i].w=rct[i].right;
		}
	}
	fclose(fp);

	mem=new RGBType[800*600];

	strcpy(buffer,name);
	strcat(buffer,".raw");

	fp=fopen(buffer,"rb");
	if(fp)
	{
		fread(mem,800*600*3,1,fp);
		fclose(fp);
	}
	else
		return;

	height=rct[0].bottom;
	fontdata=new unsigned char[(last-first+1)*bytesperline*height];
	memset(fontdata,0,(last-first+1)*bytesperline*height);

	for(i=0;i<(last-first+1);i++)
	{
		for(y=0;y<rct[i].bottom;y++)
			for(x=0;x<rct[i].right;x++)
				if(mem[(y + rct[i].top)*800+(x+rct[i].left)].r ||
				   mem[(y + rct[i].top)*800+(x+rct[i].left)].g ||
				   mem[(y + rct[i].top)*800+(x+rct[i].left)].b)
					fontdata[i*bytesperline*height + y*bytesperline + x/8] |= 1 << (x % 8);
	}

	i=last-first+1;
	strcpy(buffer,name);
	strcat(buffer,".bft");
	fp=fopen(buffer,"wb");
	if(fp)
	{
		fwrite(fontname,32,1,fp);
		fwrite(&height,sizeof(long),1,fp);
		fwrite(&first,sizeof(short),1,fp);
		fwrite(&last, sizeof(short),1,fp);
		fwrite(&bytesperline,sizeof(long),1,fp);
		fwrite(&i,sizeof(long),1,fp);
		i=0;
		fwrite(&i,sizeof(long),1,fp);
		i=(last-first+1)*bytesperline*height;
		fwrite(&i,sizeof(long),1,fp);
		i=last-first+1;
		if(i)
			fwrite(chr,sizeof(CharStr),i,fp);
		i=(last-first+1)*bytesperline*height;
		if(i)
			fwrite(fontdata,i,1,fp);
		fclose(fp);
	}
}

void main(int argc,char **argv)
{
	FILE *fp;
	long size;
	char filename[200];

	printf("RAW_FONT - Version 1.0 by Peter Ward - Convert a RAW file to my font format\n");
	strcpy(filename,argv[1]);
	strcat(filename,".raw");
	fp=fopen(filename,"rb");
	if(!fp)
	{
		printf("Can't open input file (%s)\n",filename);
		exit(0);
	}
	fseek(fp,0,SEEK_END);
	size=ftell(fp);
	fclose(fp);

	if(size/(800*600) == 1)
		ImportFromRaw8(argv[1]);
	else if(size/(800*600) == 2)
		ImportFromRaw16(argv[1]);
	else if(size/(800*600) == 3)
		ImportFromRaw24(argv[1]);
	else
		printf("Unsupported format\n");
}
