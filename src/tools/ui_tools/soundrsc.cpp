#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "rsrc.h"
#include "hash.h"

char FileList[200],OutputFile[200];

typedef struct
{
	char *data;				// actual file data (All except for 1st 8 bytes,free this)
	long Size;
	WAVEFORMATEX *Header;
	long Headersize;			// ptr to start of sample in data
} RIFF_FILE;

typedef struct
{
	SoundFmt Header;
	long Size;
	RIFF_FILE *Sound;
} SoundList;

C_Hash *SoundTable=NULL;

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
	SoundList *rec;

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

	rec=(SoundList*)SoundTable->GetFirst();
	while(rec)
	{
		rec->Header.offset=curpos;
		fwrite(rec->Sound->data,rec->Size,1,data);
		curpos+=rec->Size;
		fwrite(&rec->Header,sizeof(SoundFmt),1,header);
		rec=(SoundList*)SoundTable->GetNext();
	}

	fclose(header);
	fclose(data);
	InputSize=DataSize;
	OutputSize=HeaderSize+DataSize;
}

RIFF_FILE *LoadRiff(char *filename)
{
	RIFF_FILE *filedata;
	FILE *fp;
	char buffer[5];
	char *ptr,*hdr;
	long size,datasize,filesize,Done;

	fp=fopen(filename,"rb");
	if(!fp)
		return(NULL);

	fread(buffer,4,1,fp);
	buffer[4]=0;
	if(strcmp(buffer,"RIFF"))
		return(NULL); // Unknown file type

	fread(&datasize,sizeof(long),1,fp);

	filedata=new RIFF_FILE;
	memset(filedata,0,sizeof(RIFF_FILE));

	fseek(fp,0,SEEK_END);
	filesize=ftell(fp);
	fseek(fp,0,SEEK_SET);

	filedata->data=new char[filesize];
	fread(filedata->data,filesize,1,fp);
	fclose(fp);

	filedata->Size=filesize;

	ptr=filedata->data;
	ptr+=8;
	if(ptr && !strncmp(ptr,"WAVE",4))
	{
		ptr+=4;
		Done=0;
		while(ptr && ptr < (filedata->data + datasize) && !Done)
		{
			hdr=ptr;
			ptr+=4;
			size=*(long*)ptr;
			ptr+=4;
			if(!strncmp(hdr,"fmt ",4))
				filedata->Header=(WAVEFORMATEX*)ptr;
			if(!strncmp(hdr,"data",4))
			{
				filedata->Headersize=ptr - filedata->data;
				Done=1;
			}
			ptr+=size;
		}
	}
	else
	{
		if(filedata->data)
			delete filedata->data;
		delete filedata;
		filedata=NULL;
	}
	return(filedata);
}

char *SoundFlags[]=
{
	"S_BIT_NORMAL",
	"S_BIT_FINISH",
	"S_BIT_LOOP",
	"S_BIT_FADE_IN",
	"S_BIT_FADE_OUT",
	"S_BIT_EXCLUSIVE",
	NULL,
};

long SoundFlagValues[]=
{
	2,
	0,
	4,
	8,
	16,
	2048,
	0,
};

char *TODOList[]=
{
	NULL,
	"[LOADSOUND]",
	NULL,
};

enum
{
	LOAD_SOUND=1,
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

long FindFlag(char *token)
{
	int i;

	i=0;
	while(SoundFlags[i])
	{
		if(!stricmp(token,SoundFlags[i]))
			return(SoundFlagValues[i]);
		i++;
	}
	return(0);
}

void ProcessSoundLine(char buffer[])
{
	short sidx,eidx;
	short done=0;
	long TODO,flag;
	RIFF_FILE *WaveFile=NULL;
	SoundList *rec;
	char ID[32];
	

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
	if(TODO == LOAD_SOUND)
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

		// Find filename
		while(buffer[sidx] != '"' && buffer[sidx])
			sidx++;

		if(buffer[sidx] != '"')
			return;

		eidx=0;
		sidx++;
		while(buffer[sidx+eidx] != '"' && buffer[sidx+eidx] >= ' ')
			eidx++;

		if(buffer[sidx+eidx] != '"')
			return;

		buffer[sidx+eidx]=0;
		printf("Processing (%s)...",&buffer[sidx]);
		WaveFile=LoadRiff(&buffer[sidx]);

		if(!WaveFile)
		{
			printf("Failed\n");
			return;
		}

		printf("Adding (%s) (%1ld)\n",&buffer[sidx],WaveFile->Size);

		rec=new SoundList;
		memset(rec,0,sizeof(SoundList));

		rec->Sound=WaveFile;
		rec->Size=WaveFile->Size;
		rec->Header.Type=_RSC_IS_SOUND_;
		memcpy(rec->Header.ID,ID,32);
		rec->Header.headersize=WaveFile->Headersize;
		rec->Header.Channels=WaveFile->Header->nChannels;
		rec->Header.SoundType=WaveFile->Header->wFormatTag;

		sidx+=eidx+1;
		eidx=0;

		// Get flags
		while(!done)
		{
			while((buffer[sidx] == ',' || buffer[sidx] <= ' ') && buffer[sidx])
				sidx++;

			if(buffer[sidx] < ' ')
				done=1;
			else
			{
				while(buffer[sidx+eidx] > ' ' && buffer[sidx+eidx] != ',')
					eidx++;

				buffer[sidx+eidx]=0;

				if(eidx)
				{
					flag=FindFlag(&buffer[sidx]);
					rec->Header.flags |= flag;
				}
				else
					done=1;
				sidx +=eidx+1;
				eidx=0;
			}
		}
		HeaderSize+=sizeof(SoundFmt);
		DataSize+=rec->Size;
		SoundTable->Add(UniqueID++,rec);
	}
}

int PASCAL WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR  lpCmdLine, int nCmdShow)
{
	FILE *ifp;
	char buffer[220];

	if(!ParseCommandLine (lpCmdLine))
	{
		printf("Sound Resource - Version 1.0 - by Peter Ward\n\n");
		printf("Usage: SOUNDRSC [path]<soundrc.irc> [path]<output>\n");
		printf("    Sorry... ALL input MUST .WAV files (PCM or IMA ADPCM)\n");
		printf("    Only includes [LOADSOUND] Tokens\n");
		return(0);
	}

	ifp=fopen(FileList,"r");
	if(!ifp)
	{
		printf("Can't open soundrc.irc file (%s)\n",FileList);
		return(0);
	}

	TheTime=GetCurrentTime();

 	SoundTable=new C_Hash;
 	SoundTable->Setup(512);
 	SoundTable->SetFlags(HSH_REMOVE);

 	while(fgets(buffer,200,ifp) > 0)
 		ProcessSoundLine(buffer);
 	fclose(ifp);

 	SaveResource(OutputFile);
 	printf("Input (%1ld)... Output (%1ld)  %%%1ld\n",InputSize,OutputSize,OutputSize*100/InputSize);
	return(0);
}
