// ***************************************************************************
// ObjName.cpp
//
// Stuff used to deal with naming
// ***************************************************************************

#include <stdio.h>
#include <conio.h>
#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include "cmpglobl.h"
#include "ASearch.h"
#include "objectiv.h"
#include "name.h"
#include "f4find.h"
#include "Campaign.h"
#include "F4Thread.h"

//sfr: added for checks
#include "InvalidBufferException.h"

// ===============================
// Name Index
// ===============================

//char	NameTable[MAX_NAMES][MAX_NAME_LENGTH];

short	*NameIndex=NULL;
short	NameEntries=0;
char	NameFile[40];

_TCHAR	*NameStream=NULL;

// ===============================
// Global functions
// ===============================

void LoadNames (char* filename)
{
	//char *data, *data_ptr;
	
	CampaignData cd = ReadCampFile(filename,"idx");
	if (cd.dataSize == -1){
		return;
	}

	VU_BYTE *data_ptr =(VU_BYTE *) cd.data;
	long rem = cd.dataSize;

	sprintf(NameFile,filename);

	memcpychk(&NameEntries, &data_ptr, sizeof(short), &rem);
	
	if (NameIndex != NULL){
		delete NameIndex;
	}
	NameIndex = new short[NameEntries];
	memcpychk(NameIndex, &data_ptr, sizeof(short) * NameEntries, &rem);

	delete cd.data;
}

void LoadNameStream (void)
{
	FILE	*fp;

	if (NameStream){
		delete [] NameStream;
	}

	CampEnterCriticalSection();
	NameStream = new _TCHAR [NameIndex[NameEntries-1]];
	fp = OpenCampFile (NameFile, "wch", "rb");
	if (fp){
		fread(NameStream,sizeof(_TCHAR),NameIndex[NameEntries-1],fp);
		CloseCampFile(fp);
	}
	CampLeaveCriticalSection();
}

// Kinda tricky here. We need to save the .idx file, the .wch and the .txt to make this work
int SaveNames (char* filename)
	{
	FILE*			fp;
	int				i;
	_TCHAR			buffer[128];

	if ((fp = OpenCampFile (filename, "idx", "wb")) == NULL)
		return 0;
	fwrite(&NameEntries,sizeof(short),1,fp);
	fwrite(NameIndex,sizeof(short),NameEntries,fp);
	CloseCampFile(fp);

	if (NameStream)
		{
		if ((fp = OpenCampFile (filename, "wch", "wb")) == NULL)
			return 0;
		fwrite(NameStream,sizeof(_TCHAR),NameIndex[NameEntries-1],fp);
		CloseCampFile(fp);
		}

	if ((fp = OpenCampFile (filename, "txt", "wt")) == NULL)
		return 0;
	for (i=0; i<NameEntries-1; i++)
		{
		ReadNameString(i,buffer,127);
		fprintf(fp, "%d %s\n", i, buffer);
		}
	fprintf(fp,"-1");
	CloseCampFile (fp);
	
	return 1;
	}

void FreeNames (void)
	{
	if (NameIndex)
		{
		delete [] NameIndex;
		NameIndex = NULL;
		}
	if (NameStream)
		{
		delete [] NameStream;
		NameStream = NULL;
		}
	}

_TCHAR* ReadNameString (int sid, _TCHAR *wstr, unsigned int len)
	{
	FILE	*fp;
	unsigned int	size,rlen;

	if (!NameIndex) // JB 010731 CTD
		return wstr;

	ShiAssert(FALSE == F4IsBadReadPtr(NameIndex, sizeof *NameIndex * NameEntries)); // JPO CTD
	size = NameIndex[sid+1]-NameIndex[sid];
	rlen = size / sizeof(_TCHAR);
	if (rlen >= len)
		rlen = len - 1;

	if (NameStream)
		{
		// The file's loaded. Should only happen when our tool is running
		_sntprintf(wstr,rlen,&NameStream[NameIndex[sid]]);
		wstr[rlen] = 0;
		}
	else
		{
		if ((fp = OpenCampFile(NameFile,"wch","rb")) == NULL)
			return NULL;
		fseek(fp,NameIndex[sid],0);
		fread(wstr,sizeof(_TCHAR),rlen,fp);
		wstr[rlen] = 0;
		CloseCampFile(fp);
		}
	return wstr;
	}

// This is a pain in the ass. Figure it out if you can, monkey boy.
int AddName (_TCHAR *name)
	{
	int		i,nid=0,len,lastoffset=0,offset=0,movesize;
	_TCHAR	*newstream;

	len = _tcslen(name);
	// Load our wch file if we don't already have it in memory
	if (!NameStream)
		LoadNameStream();

	// Find a free spot
	for (i=2; i<NameEntries && !nid; i++)
		{
		// And entry is free if it has a zero (or negative) length
		if (NameIndex[i+1] - NameIndex[i] <= 0)
			{
			offset = lastoffset;
			nid = i;
			}
		if (NameIndex[i])
			lastoffset = NameIndex[i];
		}
	if (nid)
		{
		// Found a free spot, insert this string
		if (!NameIndex[nid])
			NameIndex[nid] = (short)offset;
		for (i=nid+1; i<NameEntries; i++)
			NameIndex[i] += len;
		movesize = NameIndex[NameEntries-1] - NameIndex[nid+1];
		}
	// Otherwise tack on a new entry
	else
		{
		short	*TmpIdx;
		nid = NameEntries;
		// Reallocate our memory
		TmpIdx = new short[NameEntries+1];
		memcpy(TmpIdx,NameIndex,sizeof(short)*NameEntries);
		TmpIdx[nid+1] = (short)(TmpIdx[nid] + len);
		delete [] NameIndex;
		NameIndex = TmpIdx;
		NameEntries++;
		movesize = 0;
		}
	
	// Update our name stream
	newstream = new _TCHAR [NameIndex[NameEntries-1]];
	memcpy(newstream,NameStream,sizeof(_TCHAR)*NameIndex[nid]);
	if (movesize)
		memcpy(&newstream[NameIndex[nid+1]],&NameStream[NameIndex[nid]],movesize);
	memcpy(&newstream[NameIndex[nid]],name,len);
	delete [] NameStream;
	NameStream = newstream;
	return nid;
	}

int SetName (int nameid, _TCHAR *name)
	{
	// It's actually easier to remove our name and add a new one
	if (nameid)
		RemoveName(nameid);
	return AddName(name);
	}
	
int FindName (_TCHAR* name)
	{
	int		i;
	_TCHAR	entry[128];

	for (i=0; i<NameEntries; i++)
		{
		ReadNameString(i,entry,127);
		if (strcmp(name,entry) == 0)
			return i;
		}
	return 0;
	}

void RemoveName (int nid)
	{
	int		movesize,i,len;
	_TCHAR	*tmp;

	if (nid < 2)
		return;

	// Load our wch file if we don't already have it in memory
	if (!NameStream)
		LoadNameStream();

	movesize = NameIndex[NameEntries-1] - NameIndex[nid+1];
	tmp = new _TCHAR[movesize];
	memcpy (tmp,&NameStream[NameIndex[nid+1]],movesize);
	memcpy (&NameStream[NameIndex[nid]],tmp,movesize);
	delete [] tmp;

	len = NameIndex[nid+1] - NameIndex[nid];
	for (i=nid+1; i<NameEntries; i++)
		NameIndex[i] -= len;
	}

void RemoveName (_TCHAR* name)
	{
	int		nid;

	nid = FindName(name);
	RemoveName(nid);
	}

/*
int InitNames (void)
	{
	sprintf(NameTable[0],"Nowhere");
	sprintf(NameTable[1],"New");
	NameEntries = 2;
	return 1;
	}

int LoadNames (char* name)
	{
	FILE*	fp;

	if ((fp = OpenCampFile (name, "nam", "rb")) == NULL)
		return 0;
	fread(&NameEntries,sizeof(short),1,fp);
	fread(NameTable,MAX_NAME_LENGTH,NameEntries,fp);
	fclose(fp);
	return 1;
	}

int SaveNames (char* name)
	{
	FILE*			fp;
	int				i;

	if ((fp = OpenCampFile (name, "nam", "wb")) == NULL)
		return 0;
	fwrite(&NameEntries,sizeof(short),1,fp);
	fwrite(NameTable,MAX_NAME_LENGTH,NameEntries,fp);
	fclose(fp);

	// Save these off as text
	if ((fp = OpenCampFile (name, "txt", "wt")) == NULL)
		return 0;
	for (i=0; i<NameEntries; i++)
		fprintf(fp, "%d %s\n", i, NameTable[i]);
	fprintf("-1");
	fclose(fp);
	
	return 1;
	}

char* GetName (int nameid)
	{
	return NameTable[nameid];
	}

int AddName (char* name)
	{
	int			i,nid=0;

	for (i=2; i<NameEntries && !nid; i++)
		{
		if (NameTable[i][0] == 0 || !strcmp(NameTable[i],"<None>"))
			nid = i;
		}
	if (!nid && NameEntries < MAX_NAMES)
		{
		nid = NameEntries;
		NameEntries++;
		}
	if (nid < 2)
		return 0;

	sprintf(NameTable[nid],name);
	return nid;
	}

void SetName (int nameid, char* name)
	{
	sprintf(NameTable[nameid],name);
	}
	
int FindName (char* name)
	{
	int		i;

	for (i=0; i<NameEntries; i++)
		{
		if (strcmp(name,NameTable[i]) == 0)
			return i;
		}
	return 0;
	}

void RemoveName (char* name)
	{
	int		nid;

	nid = FindName(name);
	NameTable[nid][0] = 0;
*/