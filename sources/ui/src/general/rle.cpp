#include <windows.h>

// Test Headers
//#include <stdio.h>
//#include <io.h>
//#include <fcntl.h>

#ifndef uchar
typedef unsigned char uchar;
#endif

long CompressRLE8Bit(uchar *Src,uchar *Dest,long srcsize)
{
	long Size;
	uchar *start;
	uchar data;
	uchar count;
	uchar run;

	start=Src;
	data=static_cast<uchar>((*Src)+1);	// Just make data != *Src
	srcsize;
	count=0xff;
	run=0;
	Size=0;

	while(srcsize)
	{
		if(*Src == data)
		{
			if(count)
			{
				*Dest++=count;
				Size++;
				while(count)
				{
					*Dest++=*start++;
					count--;
					Size++;
				}
			}
			run=1;
			count=1;
			while(*Src == data && count < 127 && srcsize)
			{
				count++;
				Src++;
				srcsize--;
			}
			*Dest++=static_cast<uchar>(count^0xff);
			*Dest++=data;
			Size+=2;
			run=0;
			count=0;
			start=Src;
			if(srcsize)
			{
				data=*Src++;
				srcsize--;
			}
		}
		else
		{
			run=0;
			data=*Src++;
			count++;
			srcsize--;
		}

		if(count == 127)
		{
			if(run)
			{
				*Dest++=static_cast<uchar>(count^0xff);
				*Dest++=data;
				Size+=2;
			}
			else
			{
				*Dest++=count;
				Size++;
				while(count)
				{
					*Dest++=*start++;
					count--;
					Size++;
				}
			}
			count=0;
			run=0;
		}
	}
	if(count)
	{
		if(run)
		{
			*Dest++=static_cast<uchar>(count^0xff);
			*Dest++=data;
			Size+=2;
		}
		else
		{
			*Dest++=count;
			Size++;
			while(count)
			{
				*Dest++=*start++;
				count--;
				Size++;
			}
		}
	}
	return(Size);
}

long DecompressRLE8Bit(uchar *Src,uchar *Dest,long Size)
{
	uchar count;
	long OutSize;
	OutSize=0;

	while(Size)
	{
		count=*Src++;
		Size--;
		if(count & 0x80)
		{
			count ^= 0xff;
			OutSize+=count;
			while(count)
			{
				*Dest++=*Src;
				count--;
			}
			Src++;
			Size--;
		}
		else
		{
			OutSize+=count;
			while(count && Size)
			{
				*Dest++=*Src++;
				count--;
				Size--;
			}
		}
	}
	return(OutSize);
}


long CompressRLE16Bit(WORD *Src,WORD *Dest,long srcsize)
{
	long Size;
	WORD *start;
	WORD data;
	WORD count;
	WORD run;

	start=Src;
	data=static_cast<unsigned short>((*Src)+1);
	srcsize;
	count=0xffff;
	run=0;
	Size=0;

	while(srcsize)
	{
		if(*Src == data)
		{
			if(count)
			{
				*Dest++=count;
				Size += sizeof(WORD);
				while(count)
				{
					*Dest++=*start++;
					count--;
					Size += sizeof(WORD);
				}
			}
			run=1;
			count=1;
			while(*Src == data && count < 0xFFFF && srcsize)
			{
				count++;
				Src++;
				srcsize -= sizeof(WORD);
			}
			*Dest++=static_cast<unsigned short>(count^0xFFFF);
			*Dest++=data;
			Size += 2*sizeof(WORD);
			run=0;
			count=0;
			start=Src;
			if(srcsize)
			{
				data=*Src++;
				srcsize -= sizeof(WORD);
			}
		}
		else
		{
			run=0;
			data=*Src++;
			count++;
			srcsize -= sizeof(WORD);
		}

		if(count == 0x7FFF)
		{
			if(run)
			{
				*Dest++=static_cast<unsigned short>(count^0xFFFF);
				*Dest++=data;
				Size += 2*sizeof(WORD);
			}
			else
			{
				*Dest++=count;
				Size += sizeof(WORD);
				while(count)
				{
					*Dest++=*start++;
					count--;
					Size += sizeof(WORD);
				}
			}
			count=0;
			run=0;
		}
	}
	if(count)
	{
		if(run)
		{
			*Dest++=static_cast<unsigned short>(count^0xFFFF);
			*Dest++=data;
			Size += 2*sizeof(WORD);
		}
		else
		{
			*Dest++=count;
			Size += sizeof(WORD);
			while(count)
			{
				*Dest++=*start++;
				count--;
				Size += sizeof(WORD);
			}
		}
	}
	return(Size);
}

long DecompressRLE16Bit(WORD *Src,WORD *Dest,long Size)
{
	WORD count;
	long OutSize=0;

	while(Size)
	{
		count=*Src++;
		Size -= sizeof(WORD);
		if(count & 0x8000)
		{
			count ^= 0xFFFF;
			OutSize += count*sizeof(WORD);
			while(count)
			{
				*Dest++=*Src;
				count--;
			}
			Src++;
			Size -= sizeof(WORD);
		}
		else
		{
			OutSize += count*sizeof(WORD);
			while(count && Size)
			{
				*Dest++=*Src++;
				count--;
				Size -= sizeof(WORD);
			}
		}
	}
	return(OutSize);
}

/* Test Code
void main(int argc,char **argv)
{
	int ifp,ofp;
	char *iBuf;
	char *oBuf;
	long iSize,oSize;

	if(argc != 3)
	{
		printf("Usage: test ifp ofp\n");
		exit(0);
	}

	ifp=open(argv[1],O_RDONLY|O_BINARY);
	iSize=filelength(ifp);
	iBuf=(char *)calloc(1,iSize);
	read(ifp,iBuf,iSize);
	close(ifp);

	oBuf=(char *)calloc(1,iSize*2);

	oSize=CompressRLE(iBuf,oBuf,iSize);
	memset(iBuf,0,iSize);

	iSize=DecompressRLE(oBuf,iBuf,oSize);
	ofp=open(argv[2],O_WRONLY|O_CREAT|O_BINARY);
	write(ofp,iBuf,iSize);
	close(ofp);

	free(iBuf);
	free(oBuf);

	exit(0);
}

*/
