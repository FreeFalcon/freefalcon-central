#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <malloc.h>

char *Exe;

char Search1[]="SecretCodeGoesHere";
char Search2[]="ereHseoGedoCterceS";

char EncriptCode[18];

char Mask[]=   "Falcon 4.0 is Cool";

void main(int argc,char **argv)
{
	int fp;
	long length,i,j;
	int code1,code2;
	char newfile[80];

	if(argc != 4)
	{
		printf("Usage <filename> <output path> ""string""\n");
		printf("  make sure you put the string inside double quotes!!!!\n");
		return;
	}

	memset(EncriptCode,0,18);
	strncpy(EncriptCode,argv[3],18);

	for(i=0;i<18;i++)
		EncriptCode[i] ^= Mask[i];

	printf("Embeded value (in Hex) will be:[");
	for(i=0;i<18;i++)
		printf("%02x",EncriptCode[i]);
	printf("]\n");

	fp=open(argv[1],O_RDONLY|O_BINARY);
	if(fp == -1)
	{
		printf("Can't open (%s)... exitting\n",argv[1]);
		return;
	}

	length=filelength(fp);

	Exe=(char *)malloc(length);
	if(Exe == NULL)
	{
		printf("Can't allocate %1ld bytes to load (%s) into\b",length,argv[1]);
		close(fp);
		return;
	}
	read(fp,Exe,length);
	close(fp);

	code1=0;
	code2=0;

	for(i=0;(i<length-16) && (code1 == 0 || code2 == 0);i++)
	{
		if(!code1)
		{
			if(!strncmp(Search1,&Exe[i],18))
			{
				printf("%1ld\n",i);
				code1=1;
				for(j=0;j<18;j++)
					Exe[i+j]=EncriptCode[j];
			}
		}
		if(!code2)
		{
			if(!strncmp(Search2,&Exe[i],18))
			{
				code1=1;
				for(j=0;j<18;j++)
					Exe[i+j]=EncriptCode[17-j];
			}
		}
	}

	strcpy(newfile,argv[2]);
	strcat(newfile,argv[1]);

	fp=open(newfile,O_WRONLY|O_CREAT|O_BINARY);
	if(fp == -1)
	{
		printf("Can't open (%s) for output\n",newfile);
		return;
	}
	write(fp,Exe,length);
	close(fp);

	return;
}







