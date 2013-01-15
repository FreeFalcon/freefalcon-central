#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <malloc.h>

char *Exe;

char Search2[]="ereHseoGedoCterceS";

void main(int argc,char **argv)
{
	int fp;
	long length,i,j;
	int code1,code2;

	if(argc != 1)
	{
		printf("Usage: patchf4       from the falcon4 directory\n");
		return;
	}

	fp=open("falcon4.exe",O_RDONLY|O_BINARY);
	if(fp == -1)
	{
		printf("Can't find falcon4.exe\n");
		printf("Make sure you run this patch in the directory where you installed falcon 4.0\n");
		return;
	}

	length=filelength(fp);

	Exe=(char *)malloc(length);
	if(Exe == NULL)
	{
		printf("Can't allocate %1ld bytes to load (%s) into\b",length,"falcon4.exe");
		close(fp);
		return;
	}
	read(fp,Exe,length);
	close(fp);

	code1=0;

	for(i=0;(i<length-16) && (code1 == 0);i++)
	{
		if(!code1)
		{
			if(!strncmp(Search2,&Exe[i],18))
			{
				code2=1;
				for(j=0;j<18;j++)
					Exe[i+j]=Exe[1495024l+17-j];
			}
		}
	}

	fp=open("falcon4.exe",O_WRONLY|O_CREAT|O_BINARY);
	if(fp == -1)
	{
		printf("Can't save (falcon4.exe) for output\n");
		return;
	}
	write(fp,Exe,length);
	close(fp);

	free(Exe);

	fp=open("falcdebg.exe",O_RDONLY|O_BINARY);
	if(fp == -1)
	{
		printf("Can't find falcon4.exe\n");
		printf("Make sure you run this patch in the directory where you installed falcon 4.0\n");
		return;
	}

	length=filelength(fp);

	Exe=(char *)malloc(length);
	if(Exe == NULL)
	{
		printf("Can't allocate %1ld bytes to load (%s) into\b",length,"falcdebg.exe");
		close(fp);
		return;
	}
	read(fp,Exe,length);
	close(fp);

	code2=0;

	for(i=0;(i<length-16) && (code2 == 0);i++)
	{
		if(!code2)
		{
			if(!strncmp(Search2,&Exe[i],18))
			{
				code2=1;
				for(j=0;j<18;j++)
					Exe[i+j]=Exe[2344376l+17-j];
			}
		}
	}

	fp=open("falcdebg.exe",O_WRONLY|O_CREAT|O_BINARY);
	if(fp == -1)
	{
		printf("Can't open (%s) for output\n","falcdebg.exe");
		return;
	}
	write(fp,Exe,length);
	close(fp);
	free(Exe);
	return;
}







