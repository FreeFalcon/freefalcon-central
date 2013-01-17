#include <stdio.h>
#include <string.h>

char Mask[]=   "Falcon 4.0 is Cool";

void main(int argc,char **argv)
{
	char buffer[20];
	int i,val;

	if(argc != 2)
	{
		printf("Usage: decode [that long string of numbers you get from running addcode.exe]\n");
		return;
	}

	memset(buffer,0,20);

	for(i=0;i<strlen(argv[1]) && i < 36;i++)
	{
		if(!i % 2)
			val=0;
		val*=16;
		if(argv[1][i] >= '0' && argv[1][i] <= '9')
			val+=argv[1][i]-'0';
		else if(argv[1][i] >= 'a' && argv[1][i] <= 'f')
			val+=argv[1][i]-'a'+10;
		else if(argv[1][i] >= 'A' && argv[1][i] <= 'F')
			val+=argv[1][i]-'A'+10;

		if(i%2)
			buffer[i/2]=val;
	}

	for(i=0;i<18;i++)
		buffer[i] ^= Mask[i];

	printf("Encoded=[%s]\n",argv[1]);
	printf("Decoded=[%s]\n",buffer);
}






