#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "rsrc.h"
#include "hash.h"

char FileList[200],OutputFile[200];

typedef struct
{
	FlatFmt Header;
	long Size;
	char *Data;
} FlatList;

C_Hash *FlatTable=NULL;

long TheTime;

long HeaderSize=0;
long DataSize=0;

char InternalName[200];
char Label[32];
long UniqueID=1;
char Line[300];
long InputSize=0;
long OutputSize=0;

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
		}
		else
		{
			switch(expecttype)
			{
				case 0: // No compile errors
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

void SaveResource(char *filename)
{
	FILE *header,*data;
	long curpos;
	char outname[200];
	FlatList *rec;

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

	rec=(FlatList*)FlatTable->GetFirst();
	while(rec)
	{
		rec->Header.offset=curpos;
		fwrite(rec->Data,rec->Size,1,data);
		curpos+=rec->Size;
		fwrite(&rec->Header,sizeof(FlatFmt),1,header);
		rec=(FlatList*)FlatTable->GetNext();
	}

	fclose(header);
	fclose(data);
	InputSize=DataSize;
	OutputSize=HeaderSize+DataSize;
}

char *TODOList[]=
{
	NULL,
	"[LOADFLAT]",
	NULL,
};

enum
{
	LOAD_FLAT=1,
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

void ProcessLine(char buffer[])
{
	short sidx,eidx;
	short done=0;
	long TODO;
	FlatList *rec;
	char ID[32];
	FILE *fp;
	

	sidx=0;
	eidx=0;

	// Find Token
	while(buffer[sidx+eidx] != ',' && buffer[sidx+eidx] > ' ' && buffer[sidx+eidx] != '#')
		eidx++;

	if(!eidx)
		return;

	buffer[eidx]=0;
	TODO=FindTODO(&buffer[sidx]);
	if(!TODO)
		return;

	sidx=eidx+1;
	eidx=0;

	// Handle Token
	if(TODO == LOAD_FLAT)
	{
		// Find ID
		while((buffer[sidx] <= ' ' || buffer[sidx] == ',') && buffer[sidx])
			sidx++;

		if(!buffer[sidx] || buffer[sidx] == '#')
			return;

		eidx=0;
		while(buffer[sidx+eidx] > ' ' && buffer[sidx+eidx] != ',')
			eidx++;

		if(!eidx)
			return;

		memset(ID,0,32);
		memcpy(ID,&buffer[sidx],min(eidx,32));

		sidx+=eidx+1;

		eidx=0;

		// Find filename
		while(buffer[sidx] != '"' && buffer[sidx])
			sidx++;

		if(buffer[sidx] != '"')
			return;

		sidx++;
		while(buffer[sidx+eidx] != '"' && buffer[sidx+eidx])
			eidx++;

		if(buffer[sidx+eidx] != '"')
			return;

		buffer[sidx+eidx]=0;
		printf("Processing (%s)...",&buffer[sidx]);

		fp=fopen(&buffer[sidx],"rb");
		if(fp)
		{
			rec=new FlatList;
			rec->Header.Type=_RSC_IS_FLAT_;
			strncpy(rec->Header.ID,ID,32);

			fseek(fp,0,SEEK_END);
			rec->Size=ftell(fp);
			rec->Header.size=rec->Size;
			fseek(fp,0,SEEK_SET);
			rec->Data=new char[rec->Size];

			fread(rec->Data,rec->Size,1,fp);
			fclose(fp);
			
			printf("Adding (%s) (%1ld)\n",&buffer[sidx],rec->Size);

			HeaderSize+=sizeof(FlatFmt);
			DataSize+=rec->Size;
			FlatTable->Add(UniqueID++,rec);
		}
		sidx+=eidx+1;
		eidx=0;
	}
}

int PASCAL WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR  lpCmdLine, int nCmdShow)
{
	FILE *ifp;
	char buffer[220];

	if(!ParseCommandLine (lpCmdLine))
	{
		printf("Flat Resource - Version 1.0 - by Peter Ward\n\n");
		printf("Usage: FLATRSC [path]<soundrc.irc> [path]<output>\n");
		printf("    Sorry... ALL input MUST .WAV files (PCM or IMA ADPCM)\n");
		printf("    Only includes [LOADFLAT] Tokens\n");
		return(0);
	}

	ifp=fopen(FileList,"r");
	if(!ifp)
	{
		printf("Can't open soundrc.irc file (%s)\n",FileList);
		return(0);
	}

	TheTime=GetCurrentTime();

 	FlatTable=new C_Hash;
 	FlatTable->Setup(512);
 	FlatTable->SetFlags(HSH_REMOVE);

 	while(fgets(buffer,200,ifp) > 0)
 		ProcessLine(buffer);
 	fclose(ifp);

 	SaveResource(OutputFile);
 	printf("Input (%1ld)... Output (%1ld)  %%%1ld\n",InputSize,OutputSize,OutputSize*100/InputSize);
	return(0);
}
