#include <stdio.h>

void main(short argc,char **args)
{
	FILE *fp;
	char buffer[16];
	long count,i;

	fp=fopen(args[1],"rb");
	if(!fp)
		return;

	count=0;
	while(fread(buffer,16,1,fp) > 0)
	{
		for(i=0;i<16;i++)
		printf("%02X ",buffer[i]);
		printf("\n");
		count++;
		if(count > 20)
		{
			getch();
			count=0;
		}
	}
	fclose(fp);
	exit(0);
}


