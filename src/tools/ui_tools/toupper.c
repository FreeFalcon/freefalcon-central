#include <stdio.h>
#include <string.h>

char *mem;
long size;
FILE *fp;

void main(int argc,char **argv)
{
	long i;
	long Flag;

	printf("toUPPER ver 1.0 - Peter Ward - Convert to uppercase\n");
	printf("NOTE: Doesn't convert things inside double quotes\n");

	fp=fopen(argv[1],"rb");
	if(!fp)
	{
		printf("Can't open file (%s)\n",argv[1]);
		exit(0);
	}

	fseek(fp,0,SEEK_END);
	size=ftell(fp);
	fseek(fp,0,SEEK_SET);

	mem=(char*)malloc(size);
	if(!mem)
	{
		printf("Can't allocate %1ld bytes of memory\n",size);
		exit(0);
	}

	fread(mem,size,1,fp);
	fclose(fp);

	Flag=0;

	for(i=0;i<size;i++)
	{
		if(mem[i] == '"')
			Flag ^= 1;
		if(!Flag)
			mem[i]=toupper(mem[i]);
	}

	fp=fopen(argv[2],"wb");
	if(!fp)
	{
		printf("Can't create (%s)\n",argv[2]);
		exit(0);
	}
	fwrite(mem,size,1,fp);
	fclose(fp);
	exit(0);
}
