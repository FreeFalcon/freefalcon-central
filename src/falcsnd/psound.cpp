#include <windows.h>
#include <mmreg.h>
#include <process.h>
#include "f4thread.h"
#include "falclib.h"
#include "dsound.h"
#include "psound.h"
#include "talkio.h"
#include "grTypes.h"
#include "matrix.h"
#include "SoundFX.h"
//#include "sim\include\otwdrive.h"


#include "sim/include/simlib.h" // MLR needed for SetVelocity since objects set there Delta values per frame

//#define _USE_RES_MGR_ 1

#define CUSTOM_DOPPLER

#ifdef USE_SH_POOLS
MEM_POOL gSoundMemPool = NULL;
#endif

static int d3dcount;

#define FADE_IN_STEP (25)
#define FADE_OUT_STEP (15)

CSoundMgr *gSoundDriver=NULL;
extern bool g_bUse3dSound, g_bOldSoundAlg, g_bEnableDopplerSound,g_bSoundDistanceEffect;
extern float g_fSoundDopplerFactor,g_fSoundRolloffFactor; //,g_fSoundDopplerBlend; // MLR 12/3/2003 - Blend is obsolete

F4CSECTIONHANDLE* StreamCSection;

#ifdef _USE_RES_MGR_
	#include "cmpclass.h"
	extern "C"
	{
		#include "codelib/resources/reslib/src/resmgr.h"
	}

	#define FS_OPEN			RES_FOPEN
	#define FS_READ			RES_FREAD
	#define FS_CLOSE		RES_FCLOSE
	#define FS_HANDLE		FILE *
#else
	#define FS_OPEN			fopen
	#define FS_READ			fread
	#define FS_CLOSE		fclose
	#define FS_HANDLE		FILE *
#endif

// SORRY, STREAMING NOT ALLOWED in RESMGR (it decompressed the entire thing to a buffer)


static HANDLE StreamThreadID=0;
static unsigned PSoundThreadID=0;
static long BootVolume=0;

CSoundMgr::CSoundMgr()
{
	#ifdef USE_SH_POOLS
	if ( gSoundMemPool == NULL )
	{
		gSoundMemPool = MemPoolInit( 0 );
	}
	#endif
	StreamCSection=F4CreateCriticalSection("SoundMgr");
	MasterVolume=0;
	TotalSamples=0;
	TotalStreams=0;
	DSound=NULL;
	Primary=NULL;
	SampleList=NULL;
	DuplicateList=NULL;
	StreamList=NULL;
	StreamRunning=FALSE;
	use3d = FALSE;
	Ds3dListener = NULL;
	// to notify the thread thigns have changed
	signalEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	ShiAssert(signalEvent != NULL);
	CamPos.x=0;
	CamPos.y=0;
	CamPos.z=0;
	CamVelocity.x=0;
	CamVelocity.y=0;
	CamVelocity.z=0;
}

CSoundMgr::~CSoundMgr()
{
	F4DestroyCriticalSection(StreamCSection);
	StreamCSection = NULL; // JB 010108
	CloseHandle (signalEvent); // JPO
	signalEvent = NULL;
	#ifdef USE_SH_POOLS
	if ( gSoundMemPool != NULL )
	{
		MemPoolFree ( gSoundMemPool );
		gSoundMemPool = NULL;
	}
	#endif
}

// Sound Installation / Removal functions
//
// Priority:	DSSCL_NORMAL (Lowest)	- Lets any APP use DSound when in focus
//				DSSCL_PRIORITY			-
//				DSSCL_EXCLUSIVE			-
//				DSSCL_WRITEPRIMARY		-
// Use DSSCL_NORMAL for now...
//
BOOL CSoundMgr::InstallDSound(HWND hwnd,DWORD Priority,WAVEFORMATEX *fmt)
{
	HRESULT res;
	DSBUFFERDESC        dsbdesc;
//	DWORD Speakers;
//	DSCAPS dscaps;

	if(gSoundDriver)
	{
		if(DSound != NULL)
			return(FALSE);

		res = DirectSoundCreate(NULL, &DSound,NULL);
		if(res != DS_OK)
		{
			DSoundCheck(res);
			return(FALSE);
		}

		res = DSound->SetCooperativeLevel(hwnd, DSSCL_EXCLUSIVE );
		if(res != DS_OK)
		{
			DSound->Release();
			DSound = NULL;
			DSoundCheck(res);
			return(FALSE);
		}
    // Set up DSBUFFERDESC structure.
		memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); // Zero it out.
		dsbdesc.dwSize = sizeof(DSBUFFERDESC);
		dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLPAN;
		if (g_bUse3dSound) 
		{
		    dsbdesc.dwFlags |= DSBCAPS_CTRL3D | DSBCAPS_MUTE3DATMAXDISTANCE;
		}
    // Buffer size is determined by sound hardware.
		dsbdesc.dwBufferBytes = 0;
		dsbdesc.lpwfxFormat = NULL; // Must be NULL for primary buffers.

		res = DSound->CreateSoundBuffer(&dsbdesc, &Primary, NULL);
		if(res != DS_OK) { // JPO - no primary buffer, must mean no sound
			DSoundCheck(res);
			DSound->Release();
			DSound = NULL;
			return FALSE;
		}
		// Get listener interface
		if (g_bUse3dSound) 
		{
			if (FAILED(res = Primary->QueryInterface(IID_IDirectSound3DListener,
				(LPVOID *)&Ds3dListener))) 
			{
				DSoundCheck(res);
				use3d = FALSE;
				Ds3dListener = NULL;
			}
			else 
			{
				use3d = TRUE;
				res = Ds3dListener->SetDistanceFactor(0.3048f, DS3D_DEFERRED); // convert to feet
				// MLR 
#ifdef CUSTOM_DOPPLER
				Ds3dListener->SetDopplerFactor( 0, DS3D_DEFERRED);
#else
				Ds3dListener->SetDopplerFactor( g_fSoundDopplerFactor, DS3D_DEFERRED);
#endif
				Ds3dListener->SetRolloffFactor( g_fSoundRolloffFactor, DS3D_DEFERRED);
				if (FAILED(res)) DSoundCheck(res);
			}
		}

    
		if (!F4IsBadCodePtr((FARPROC) Primary)) // JB 010305 CTD
			res=Primary->SetFormat(fmt);
		
		if(res != DS_OK)
			DSoundCheck(res);
/*
		memset(&dscaps,0,sizeof(DSCAPS));
		dscaps.dwSize=sizeof(DSCAPS);
		res=DSound->GetCaps(&dscaps);
		if(res != DS_OK)
			DSoundCheck(res);

		res=lpNewDSBuf->GetFormat(fmt,sizeof(WAVEFORMATEX),&size);
		if(res != DS_OK)
			DSoundCheck(res);

		DSound->GetSpeakerConfig(&Speakers);

		switch(DSSPEAKER_CONFIG(Speakers))
		{
			case DSSPEAKER_HEADPHONE:
				res=1;
				break;
			case DSSPEAKER_MONO:
				res=2;
				break;
			case DSSPEAKER_QUAD:
				res=3;
				break;
			case DSSPEAKER_STEREO:
				res=4;
				break;
			case DSSPEAKER_SURROUND:
				res=5;
				break;
		}
*/

		res = DSound->SetCooperativeLevel(hwnd,Priority);

		//Primary->Play(0,0,DSBPLAY_LOOPING);
		if(res != DS_OK)
		{
			if (!F4IsBadCodePtr((FARPROC) Primary)) // JB 010305 CTD
				Primary->Release();
			Primary=NULL;
			DSound->Release();
			DSound = NULL;
			DSoundCheck(res);
			return(FALSE);
		}
		BootVolume=GetMasterVolume();
		return(TRUE);
	}
	return(FALSE);
}

void CSoundMgr::RemoveDSound()
{
    if(this != NULL)
    {
	if(DSound == NULL)
	    return;
	
	if(StreamList != NULL)
	{
	    RemoveAllStreams();
	}
	
	if(StreamThreadID)
	{
	    StreamRunning=FALSE;
	    NotifyThread();
	    while(StreamThreadID)
		Sleep(5);
	    CloseHandle(StreamThreadID);
	}
	
	if(SampleList != NULL)
	{
	    RemoveAllSamples();
	    SampleList=NULL;
	}
	
	SetMasterVolume(BootVolume);
	F4EnterCriticalSection(StreamCSection);
	
	if (Ds3dListener)
	    Ds3dListener->Release();
	Ds3dListener = NULL;
	if (Primary)
	    Primary->Release();
	Primary=NULL;
	DSound->Release();
	DSound = NULL;
	F4LeaveCriticalSection(StreamCSection);
    }
}

long CSoundMgr::SetMasterVolume(long NewVolume)
{
	long retval=MasterVolume;

	MasterVolume=NewVolume;
	Primary->SetVolume(NewVolume);
	return(retval);
}

long CSoundMgr::GetMasterVolume()
{
	Primary->GetVolume(&MasterVolume);
	return(MasterVolume);
}

// Fill a RIFF_FILE struct with the relevant info (similar to LoadRiff, be file is already loaded)
long CSoundMgr::FillRiffInfo(char *memory,RIFF_FILE *riff)
{
	char *ptr,*hdr;
	long size,datasize;

	if(!memory)
		return(0);

	ptr=memory;
	if(strncmp(ptr,"RIFF",4))
		return(0); // Unknown file type

	ptr+=4;

	datasize=*(long*)ptr;
	ptr+=sizeof(long);

	memset(riff,0,sizeof(RIFF_FILE));

	riff->data=ptr;

	if(ptr && !strncmp(ptr,"WAVE",4))
	{
		ptr+=4;
		while(ptr && !riff->Start && ptr < (riff->data + datasize))
		{
			hdr=ptr;
			ptr+=4;
			size=*(long*)ptr;
			ptr+=4;
			if(!strncmp(hdr,"fmt ",4))
				riff->Format=(WAVEFORMATEX*)ptr;
			else if(!strncmp(hdr,"fact",4))
				riff->NumSamples=*(long*)ptr;
			else if(!strncmp(hdr,"data",4))
			{
				riff->Start=ptr;
				riff->SampleLen=size;
			}
			ptr+=size;
		}
		if(riff->Start)
			return(riff->SampleLen);
	}
	return(0);
}

// This function loads a WAVE file
// and Handles ALL types
// doesn't handle anything special...(because I don't know how
// to handle the special stuff, it does however skip it)
// but can load _PCM & _ADPCM format files properly
// 
RIFF_FILE *CSoundMgr::LoadRiff(char *filename)
{
	RIFF_FILE *filedata;
	FILE *fp;
	char buffer[5];
	char *ptr,*hdr;
	long size,datasize;

	fp=fopen(filename,"rb");
	if(!fp)
		return(NULL);

	fread(buffer,4,1,fp);
	buffer[4]=0;
	if(strcmp(buffer,"RIFF"))
		return(NULL); // Unknown file type

	fread(&datasize,sizeof(long),1,fp);

	#ifdef USE_SH_POOLS
	filedata= (RIFF_FILE *)MemAllocPtr( gSoundMemPool, sizeof(RIFF_FILE), 0 );
	#else
	filedata=new RIFF_FILE;
	#endif
	memset(filedata,0,sizeof(RIFF_FILE));

	#ifdef USE_SH_POOLS
	filedata->data= (char *)MemAllocPtr( gSoundMemPool, sizeof(char)*datasize, 0 );
	#else
	filedata->data=new char[datasize];
	#endif
	fread(filedata->data,datasize,1,fp);
	fclose(fp);

	ptr=filedata->data;
	if(ptr && !strncmp(ptr,"WAVE",4))
	{
		ptr+=4;
		while(ptr && !filedata->Start && ptr < (filedata->data + datasize))
		{
			hdr=ptr;
			ptr+=4;
			size=*(long*)ptr;
			ptr+=4;
			if(!strncmp(hdr,"fmt ",4))
				filedata->Format=(WAVEFORMATEX*)ptr;
			else if(!strncmp(hdr,"fact",4))
				filedata->NumSamples=*(long*)ptr;
			else if(!strncmp(hdr,"data",4))
			{
				filedata->Start=ptr;
				filedata->SampleLen=size;
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

// this function loads all the stuff upto the data section of
// a riff file, and returns the size of the header
long CSoundMgr::SkipRiffHeader(FILE *fp)
{
	char buffer[256];
	long size,totalsize,bytesread;

	fread(buffer,4,1,fp);
	if(strncmp(buffer,"RIFF",4))
		return(0);

	bytesread=4;
	fread(&totalsize,sizeof(long),1,fp);
	bytesread+=sizeof(long);
	fread(buffer,4,1,fp);
	bytesread+=4;
	if(strncmp(buffer,"WAVE",4))
		return(0); // unsupported format

	totalsize-=4;
	fread(buffer,4,1,fp);
	fread(&size,sizeof(long),1,fp);
	bytesread+=4+sizeof(long);
	totalsize-=8;
	while(totalsize > 0 && strncmp(buffer,"data",4))
	{
		while(size > 256)
		{
			fread(buffer,256,1,fp);
			bytesread+=256;
			size-=256;
			totalsize-=256;
		}
		fread(buffer,size,1,fp);
		bytesread+=size;
		totalsize-=size;
		fread(buffer,4,1,fp);
		fread(&size,sizeof(long),1,fp);
		bytesread+=4+sizeof(long);
		totalsize-=8;
	}
	if(!strncmp(buffer,"data",4))
		return(bytesread);
	return(0);
}

// this function loads all the stuff upto the data section of
// a riff file, and returns the size of the header
long CSoundMgr::SkipRiffHeader(HANDLE fp)
{
	char buffer[256];
	long size,totalsize,bytesread;
	DWORD br;

	ReadFile(fp,buffer,4,&br,NULL);
	if(strncmp(buffer,"RIFF",4))
		return(0);

	bytesread=4;
	ReadFile(fp,&totalsize,sizeof(long),&br,NULL);
	bytesread+=sizeof(long);
	ReadFile(fp,buffer,4,&br,NULL);
	if(strncmp(buffer,"WAVE",4))
		return(0); // unsupported format

	bytesread+=4;
	totalsize-=4;
	ReadFile(fp,buffer,4,&br,NULL);
	ReadFile(fp,&size,sizeof(long),&br,NULL);
	bytesread+=4+sizeof(long);
	totalsize-=8;
	while(totalsize > 0 && strncmp(buffer,"data",4))
	{
		while(size > 256)
		{
			ReadFile(fp,buffer,256,&br,NULL);
			bytesread+=256;
			size-=256;
			totalsize-=256;
		}
		ReadFile(fp,buffer,size,&br,NULL);
		bytesread+=size;
		totalsize-=size;
		ReadFile(fp,buffer,4,&br,NULL);
		ReadFile(fp,&size,sizeof(long),&br,NULL);
		bytesread+=4+sizeof(long);
		totalsize-=8;
	}
	if(!strncmp(buffer,"data",4))
		return(bytesread);
	return(0);
}

long CSoundMgr::LoadRiffFormat(char *filename,WAVEFORMATEX *Format,long *HeaderSize,long *SampleCount)
{
	HANDLE fp;
	long samplesize;

	fp=CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,
						  OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL);

	if(fp == INVALID_HANDLE_VALUE)
		return(0);

	samplesize=LoadRiffFormat(fp,Format,HeaderSize,SampleCount);
	CloseHandle(fp);

	return(samplesize);
}

// SampleCount is only set for IMA_ADPCM (if the riff has a "fact" record... it gets set)
// multiply by # bytes/sample to get output size in bytes (16bit stereo = 4 bytes/sample)
long CSoundMgr::LoadRiffFormat(HANDLE fp,WAVEFORMATEX *Format,long *HeaderSize,long *SampleCount)
{
	char buffer[256];
	long size,totalsize,bytesread;
	DWORD br;

	(*SampleCount)=0;
	ReadFile(fp,buffer,4,&br,NULL);
	bytesread=br;
	if(strncmp(buffer,"RIFF",4))
		return(0);

	ReadFile(fp,&totalsize,sizeof(long),&br,NULL);
	bytesread+=br;

	ReadFile(fp,buffer,4,&br,NULL);
	bytesread+=br;
	if(strncmp(buffer,"WAVE",4))
		return(0); // unsupported format

	ReadFile(fp,buffer,4,&br,NULL);
	bytesread+=br;
	ReadFile(fp,&size,sizeof(long),&br,NULL);
	bytesread+=br;
	while(bytesread < totalsize && strncmp(buffer,"data",4))
	{
		if(!strncmp(buffer,"fmt ",4))
		{
			ReadFile(fp,Format,min(sizeof(WAVEFORMATEX),size),&br,NULL);
			size-=br;
			bytesread+=br;
		}
		if(!strncmp(buffer,"fact",4))
		{
			ReadFile(fp,SampleCount,sizeof(long),&br,NULL);
			size-=br;
			bytesread+=br;
		}
		while(size > 256)
		{
			ReadFile(fp,buffer,256,&br,NULL);
			size-=br;
			bytesread+=br;
		}
		if(size)
		{
			ReadFile(fp,buffer,size,&br,NULL);
			bytesread+=br;
		}
		ReadFile(fp,buffer,4,&br,NULL);
		bytesread+=br;
		ReadFile(fp,&size,sizeof(long),&br,NULL);
		bytesread+=br;
	}
	if(bytesread < totalsize)
	{
		*HeaderSize=bytesread;
//		if(Format->wFormatTag == WAVE_FORMAT_IMA_ADPCM)
//			return((*SampleCount) * Format->nChannels * Format->wBitsPerSample/8);
//		else
		return(size);
	}
	return(0);
}

// Functions to get Sounds into Direct Sound

// Returns the ID which will be used for the sample
long CSoundMgr::LoadWaveFile(char *Filename,long Flags, SFX_DEF_ENTRY *sfx)
{
	char *mem;
	RIFF_FILE *newsnd;
	long NewID = SND_NO_HANDLE;
	DSBUFFERDESC dsbdesc;
	LPDIRECTSOUNDBUFFER lpNewDSBuf;
	DWORD Len;
	HRESULT hr;

	if(gSoundDriver)
	{
		newsnd=LoadRiff(Filename);
		if(newsnd)
		{
			if(!newsnd->Format)
			{
				if(newsnd->data)
					delete newsnd->data;
				delete newsnd;
				return(SND_NO_HANDLE);
			}
			if(newsnd->Format->wFormatTag == WAVE_FORMAT_PCM)
			{
				memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); // Zero it out.
				dsbdesc.dwSize = sizeof(DSBUFFERDESC);
				if (sfx) 
				{ // SFX specific - we have more control
					dsbdesc.dwFlags = DSBCAPS_CTRLVOLUME | 
						DSBCAPS_GETCURRENTPOSITION2;
											
					if (g_bOldSoundAlg == false)
						dsbdesc.dwFlags |= DSBCAPS_CTRLPOSITIONNOTIFY;
					
					if (g_bUse3dSound && (sfx->flags & SFX_FLAGS_3D)) 
					{
						dsbdesc.dwFlags |= DSBCAPS_CTRL3D | DSBCAPS_MUTE3DATMAXDISTANCE;
					}
					else
					{
						if (sfx->flags & SFX_FLAGS_PAN) 
							dsbdesc.dwFlags |= DSBCAPS_CTRLPAN;
					}
					
					if (sfx->flags & (SFX_FLAGS_FREQ | SFX_POS_EXTERN))  // MLR 12/22/2003 - External sounds must have the Freq cap so doppler effects can be applied.
						dsbdesc.dwFlags |= DSBCAPS_CTRLFREQUENCY;
					
					if ((sfx->flags & SFX_FLAGS_HIGH) == 0) // low priority sound
						dsbdesc.dwFlags |= DSBCAPS_LOCDEFER;
					
					
				} else 
				{ // other sounds have other requirements
				    dsbdesc.dwFlags =	DSBCAPS_CTRLPAN | 
										DSBCAPS_CTRLVOLUME | 
										DSBCAPS_CTRLFREQUENCY |
										DSBCAPS_GETCURRENTPOSITION2 ; // Need default controls (pan, volume, frequency).
				    if (g_bOldSoundAlg == false)
						dsbdesc.dwFlags |= DSBCAPS_CTRLPOSITIONNOTIFY;
				}
				dsbdesc.dwBufferBytes = newsnd->SampleLen;
				dsbdesc.lpwfxFormat = newsnd->Format;

			// Create buffer.
				hr = DSound->CreateSoundBuffer(&dsbdesc, &lpNewDSBuf, NULL);
				if(hr == DS_OK)
				{
					lpNewDSBuf->Lock(0,newsnd->SampleLen,(void**)&mem,&Len,NULL,NULL,NULL);
					memcpy(mem,newsnd->Start,Len);
					lpNewDSBuf->Unlock(mem,Len,NULL,NULL);
					NewID=AddSampleToMgr(100,newsnd->Format->nSamplesPerSec,0,lpNewDSBuf,Flags, sfx);
				}
				else
					DSoundCheck(hr);
				delete newsnd->data;
				delete newsnd;
				return(NewID);
			}
			else
			{
				MonoPrint("Unsupported file format\n");
				if(newsnd->data)
					delete newsnd->data;
				delete newsnd;
			}
		}
	}
	return(SND_NO_HANDLE);
}

long CSoundMgr::AddRawSample(WAVEFORMATEX *Header,char *Data, long size,long Flags)
{
	long NewID = SND_NO_HANDLE;
	DWORD Len;
	char *mem;
	DSBUFFERDESC dsbdesc;
	LPDIRECTSOUNDBUFFER lpNewDSBuf;
	HRESULT hr;

	if(gSoundDriver)
	{
// Set up DSBUFFERDESC structure.
		memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); // Zero it out.
		dsbdesc.dwSize = sizeof(DSBUFFERDESC);
		dsbdesc.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | 
		    DSBCAPS_CTRLFREQUENCY | 
		    DSBCAPS_GETCURRENTPOSITION2 ; // Need default controls (pan, volume, frequency).
		if (g_bOldSoundAlg == false)
		    dsbdesc.dwFlags |= DSBCAPS_CTRLPOSITIONNOTIFY;
		dsbdesc.dwBufferBytes = size;
		dsbdesc.lpwfxFormat = Header;

	// Create buffer.
		hr = DSound->CreateSoundBuffer(&dsbdesc, &lpNewDSBuf, NULL);
		if(hr == DS_OK)
		{
			lpNewDSBuf->Lock(0,size,(void**)&mem,&Len,NULL,NULL,NULL);
			memcpy(mem,Data,size);
			lpNewDSBuf->Unlock(mem,Len,NULL,NULL);
			NewID=AddSampleToMgr(100,Header->nSamplesPerSec,0,lpNewDSBuf,Flags, NULL);
		}
		return(NewID);
	}
	return(SND_NO_HANDLE);
}

void CSoundMgr::RemoveSample(long ID)
{
	SoundList *Cur,*Last;

	if(gSoundDriver)
	{
		if(SampleList == NULL)
			return;

		Cur=SampleList;

		while(Cur != NULL)
		{
			if(Cur->ID == ID)
			{
				SampleList=Cur->Next;
				delete Cur;
				Cur=SampleList;
			}
			else
				break;
		}

		Last=Cur;
		while(Cur != NULL)
		{
			if(Cur->ID == ID)
			{
				Last->Next=Cur->Next;
				delete Cur;
				Cur=Last;
			}
			Last=Cur;
			Cur=Cur->Next;
		}
	}
}

void CSoundMgr::RemoveDuplicateSample(long ID)
{
	SoundList *Cur,*Last;

	if(gSoundDriver)
	{
		if(DuplicateList == NULL)
			return;

		Cur=DuplicateList;

		while(Cur != NULL)
		{
			if(Cur->ID == ID)
			{
				DuplicateList=Cur->Next;
				delete Cur;
				Cur=DuplicateList;
			}
			else
				break;
		}

		Last=Cur;
		while(Cur != NULL)
		{
			if(Cur->ID == ID)
			{
				Last->Next=Cur->Next;
				delete Cur;
				Cur=Last;
			}
			Last=Cur;
			Cur=Cur->Next;
		}
	}
}

void CSoundMgr::RemoveAllSamples()
{
	SoundList *Cur, *Last;
	if (SampleList == NULL){
		return;
	}

	Cur=SampleList;
	while (Cur != NULL){
		Last=Cur;
		Cur=Cur->Next;
		delete Last;
	}
	SampleList=NULL;
	if (DuplicateList == NULL){
		return;
	}

	Cur=DuplicateList;
	while (Cur != NULL){
		Last=Cur;
		Cur=Cur->Next;
		delete Last;
	}
	DuplicateList=NULL;
}

// This is only called by the UI, so we'll tailor it to our needs
// Functions to Start/Stop/Modify Samples
BOOL CSoundMgr::PlaySample(long ID,long Flags)
{
	SoundList * Sample;
	HRESULT hr;
	if(gSoundDriver)
	{
		if(DSound)
		{
			Sample=FindSample(ID);
			if(Sample != NULL)
			{
				/*
				if(IsSamplePlaying(ID,0) && !(Flags & SND_OVERRIDE))
				{
					if(Sample->Flags & SND_EXCLUSIVE)
						return(FALSE);

					//Sample=AddDuplicateSample(Sample);

					// only play first sample;
					int i=0; //for(int i=0;i<Sample->DS3DBufferCount;i++)
					{
						if(Sample->Buf[i].DSoundBuffer)
						{
							Sample->Buf[i].DSoundBuffer->SetCurrentPosition(0);
							
							if(Flags & SND_LOOP_SAMPLE)
								hr = Sample->Buf[i].DSoundBuffer->Play(0,0,DSBPLAY_LOOPING);
							else
								hr = Sample->Buf[i].DSoundBuffer->Play(0,0,0);
							if (FAILED(hr))
							DSoundCheck(hr);
						}
					}

					return(TRUE);
				}
				else */
				{
					//if(Flags & SND_EXCLUSIVE)
					//	Sample->Flags = SND_EXCLUSIVE;
					// only play 1st sample
					int i=0; //for(int i=0;i<Sample->DS3DBufferCount;i++)
					{
						if(Sample->Buf[i].DSoundBuffer)
						{
							Sample->Buf[i].DSoundBuffer->SetCurrentPosition(0);
							//if(Flags & SND_LOOP_SAMPLE)
							if(Flags & SFX_POS_LOOPED)
								hr = Sample->Buf[i].DSoundBuffer->Play(0,0,DSBPLAY_LOOPING);
							else
								hr = Sample->Buf[i].DSoundBuffer->Play(0,0,0);
							if (FAILED(hr))
								DSoundCheck(hr);
						}
					}
					return(TRUE);
				}
			}
		}
	}
	return(FALSE);
}

// This is only called by the UI, we can tailor it to our needs
BOOL CSoundMgr::StopSample(long ID)
{
	SoundList * Sample;

	if(gSoundDriver)
	{
		if(DSound)
		{
			Sample=FindSample(ID);
			if(Sample != NULL)
			{
				for(int i=0;i<Sample->DS3DBufferCount;i++)
				{
					if(Sample->Buf[i].DSoundBuffer)
						Sample->Buf[i].DSoundBuffer->Stop();
				}
				return(TRUE);
			}
		}
	}
	return(FALSE);
}

BOOL CSoundMgr::StopAllSamples(void)
{
SoundList *Cur;

	Cur=SampleList;

	while(Cur != NULL)
	{
		StopSample (Cur->ID);
		Cur=Cur->Next;
	}

	Cur=DuplicateList;

	while(Cur != NULL)
	{
		StopSample (Cur->ID);
		Cur=Cur->Next;
	}

	return(TRUE);
}

BOOL CSoundMgr::SetSamplePitch(long ID, float NewPitch)
{
	/*
	SoundList * Sample;
	long Frequency;

	if(gSoundDriver)
	{
		if(DSound)
		{
			Sample=FindSample(ID);
			if(Sample != NULL)
			{
				Frequency=(long)(Sample->Frequency * NewPitch);
            Frequency = min ( max (DSBFREQUENCY_MIN, Frequency), DSBFREQUENCY_MAX);
				Sample->Buf[0].DSoundBuffer->SetFrequency(Frequency);
				return(TRUE);
			}
		}
	}
	return(FALSE);
	*/
	return(TRUE);
}

BOOL CSoundMgr::IsSamplePlaying(long ID, int UID)
{
	SoundList * Sample;
	DWORD status;

	if(gSoundDriver)
	{
		if(DSound)
		{
			Sample=FindSample(ID);
			
			if(Sample != NULL)
			{
				int i;

				for(i=0;i<Sample->DS3DBufferCount;i++)
				{
					if(Sample->Buf[i].uid == UID)
					{
						Sample->Buf[i].DSoundBuffer->GetStatus(&status);
						if(status & DSBSTATUS_PLAYING)
		 					return(TRUE);
					}
				}
			}
		}
	}
	return(FALSE);
}
int CSoundMgr::GetSampleVolume(long ID)
{
	SoundList * Sample;
	long Volume=DSBVOLUME_MIN;

	if(gSoundDriver)
	{
		if(DSound)
		{
			Sample=FindSample(ID);
			if(Sample != NULL)
				Sample->Buf[0].DSoundBuffer->GetVolume(&Volume);
		}
	}
	return(Volume);
}

BOOL CSoundMgr::SetSampleVolume(long ID, long Volume)
{
	SoundList * Sample;
	HRESULT hr;

	if(gSoundDriver)
	{
		if(DSound)
		{
			Sample=FindSample(ID);
			if(Sample != NULL)
			{
				//Sample->Volume=Volume;
				//if (Sample->is3d == FALSE) {
				    hr=Sample->Buf[0].DSoundBuffer->SetVolume(Volume);
				    if(hr != DS_OK)
					DSoundCheck(hr);
				//}
				return(TRUE);
			}
		}
	}
	return(FALSE);
}


#if 0
BOOL CSoundMgr::SetSamplePosition(long ID, float x, float y, float z, float vx, float vy, float vz)
{
    SoundList * Sample;
    HRESULT hr;
    
    if(gSoundDriver && DSound)
    {
	Sample=FindSample(ID);

	if(Sample != NULL && Sample->DSound3dBuffer)
	{
	    if (Sample->is3d == FALSE) {
		hr = Sample->DSound3dBuffer->SetMode(DS3DMODE_NORMAL, DS3D_DEFERRED );
		if(hr != DS_OK)
		    DSoundCheck(hr);
	    }
	    Sample->is3d = TRUE;
	    hr = Sample->DSound3dBuffer->SetPosition(x, y, z, DS3D_DEFERRED );
		if(g_bEnableDopplerSound) // MLR
		{
			
			
			if(g_fSoundDopplerBlend) 
			{
				// this blends the Velocity to smooth out transitions
				Sample->dx-=(Sample->dx-vx/SimLibMajorFrameTime)/g_fSoundDopplerBlend;
				Sample->dy-=(Sample->dy-vy/SimLibMajorFrameTime)/g_fSoundDopplerBlend;
				Sample->dz-=(Sample->dz-vz/SimLibMajorFrameTime)/g_fSoundDopplerBlend;
				Sample->DSound3dBuffer->SetVelocity(Sample->dx, Sample->dy, Sample->dz, DS3D_DEFERRED );
			}
		}
	    if(hr != DS_OK)
		DSoundCheck(hr);
	    return(TRUE);
	}
    }
    return(FALSE);
}
#endif
/***********************************************************************
	SetSamplePosition()

	this function puts the sound data in Buf, 
	it doesn't get sent to a DSound3DBuffer/DSoundBuffer
	until AssignSamples() below.
	
	Pass in postion and velocities.
 
	Since the sound system is only updated at 20hz (20fps)
	it's very likely that an object will call upon a specific LOOPING sound
	several times before it is actually updated.

	uid is used to prevent an object from playing the same LOOPING sound effect 
	multiple times per frame.

	the uid also allows for the object to play from the same DSoundBuffer 
	where possible.

    For non-looping sounds, I presume that each sound is invoked one time, 
	and not multiple times as looping sounds are.  Therefore each call to
	this function on a non-looping sound, should create another instance of
	that sound.

	Notes:
	  Buf[].distsq is set to -1 when the buffer is unused.
*************************************************************************/
BOOL CSoundMgr::SetSamplePosition(long ID, float x, float y, float z, float pitch, float vol, 
								  float vx, float vy, float vz, float dsq, int uid, int is3d)
{
	return 0;
#if 0
    SoundList * Sample;
	BOOL rv=FALSE;

//#define SNDLOG
#ifdef SNDLOG
	FILE *fp;

	if(fp=fopen("sndlog.txt","a+"))
	{
#endif
    if(gSoundDriver && DSound)
    {
		if(Sample=FindSample(ID))
		{			
			int i;
#ifdef SNDLOG
			fprintf(fp,"SSP - ID(%d %s) xyz(%.2f,%.2f,%.2f) dxyz(%.2f,%.2f,%.2f) pit(%.2f) vol(%.2f) dist(%.2f) uid(%d)\n",
			ID,
			(Sample->Sfx ? Sample->Sfx->fileName:"none"),
			x,y,z,vx,vy,vz,pitch,vol,dsq,uid);
#endif


			// we have to handle looped and non looped sounds a little differently
			//if(Sample->Flags & SND_LOOP_SAMPLE)
			if(Sample->Flags & SFX_POS_LOOPED)
			{
				for(i=0;i<Sample->DS3DBufferCount;i++)
				{
					if(Sample->Buf[i].uid==uid)
					{
						Sample->Buf[i].x=x;
						Sample->Buf[i].y=y;
						Sample->Buf[i].z=z;
						Sample->Buf[i].vx=vx;
						Sample->Buf[i].vy=vy;
						Sample->Buf[i].vz=vz;
						Sample->Buf[i].distsq=dsq;
						Sample->Buf[i].vol=vol;
						Sample->Buf[i].pitch=pitch;
						// Sample->Buf[best].uid=uid; // not needed
						Sample->Buf[i].Timer=vuxGameTime;
						Sample->Buf[i].Is3d=is3d;

						rv=TRUE;
						
#ifdef SNDLOG
						fprintf(fp,"  updated buffer %d (dsq=%f)\n",i,dsq);
#endif
						break;
						
					}
				}

				if(!rv) // didn't match a uid
				{
					int i, best=-1;
					float  bestdist=(float)0;
					
					// run thru the Positional data, if the new sound is closer than one of the others,
					// then replace one of them 
					// replace the one that's farthest out.
					for(i=0;i<Sample->DS3DBufferCount;i++)
					{
						//float dist=Sample->Buf[i].distsq - dsq;
						// if the buffers distance is farther, dist will be positive

						if(Sample->Buf[i].distsq < 0)
						{	// unused buffer found
							best=i;
							break; // leave now
						}

						if(Sample->Buf[i].distsq > dsq) 
						{	// new sound call is closer
							if(Sample->Buf[i].distsq > bestdist)
							{   // this Buf[i] is that farthest away, canidate for replacement.
								best=i;
								bestdist=Sample->Buf[i].distsq;
							}
						}

					}
					if(best>=0)
					{
	#ifdef SNDLOG
						fprintf(fp,"  assigned to buffer %d (dsq=%f)\n",best,dsq);
	#endif
						Sample->Buf[best].x=x;
						Sample->Buf[best].y=y;
						Sample->Buf[best].z=z;
						Sample->Buf[best].vx=vx;
						Sample->Buf[best].vy=vy;
						Sample->Buf[best].vz=vz;
						Sample->Buf[best].distsq=dsq;
						Sample->Buf[best].vol=vol;
						Sample->Buf[best].pitch=pitch;
						Sample->Buf[best].uid=uid;
						Sample->Buf[best].Timer=vuxGameTime;
						Sample->Buf[best].Is3d=is3d;

						rv=TRUE;
						//return(TRUE);	
					}
				}
			}
			else 
			{ // non looped
				// move to next buffer

				Sample->Cur3dBuffer=0;

				if(Sample->Cur3dBuffer>Sample->DS3DBufferCount)
				{
					Sample->Cur3dBuffer=0;
				}

#ifdef SNDLOG
						fprintf(fp,"  NONLOOPED - assigned to buffer %d (dsq=%f)\n",Sample->Cur3dBuffer,dsq);
#endif

				Sample->Buf[Sample->Cur3dBuffer].x=x;
				Sample->Buf[Sample->Cur3dBuffer].y=y;
				Sample->Buf[Sample->Cur3dBuffer].z=z;
				Sample->Buf[Sample->Cur3dBuffer].vx=vx;
				Sample->Buf[Sample->Cur3dBuffer].vy=vy;
				Sample->Buf[Sample->Cur3dBuffer].vz=vz;
				Sample->Buf[Sample->Cur3dBuffer].distsq=dsq;
				Sample->Buf[Sample->Cur3dBuffer].vol=vol;
				Sample->Buf[Sample->Cur3dBuffer].pitch=pitch;
				Sample->Buf[Sample->Cur3dBuffer].uid=uid;
				Sample->Buf[Sample->Cur3dBuffer].Timer=vuxGameTime;
				Sample->Buf[Sample->Cur3dBuffer].Is3d=is3d;

				rv=TRUE;
			}
		}
    }
	
#ifdef SNDLOG
	fclose(fp);
	}
#endif


    return(rv);
#endif
}


/**************************************************************
	This assigns the buffered sound calls to the DSoundBuffers, 
	Plays & Stops the buffers as needed and marks the buffers as
	unused for the next update.
***************************************************************/
void CSoundMgr::AssignSamples(void)
{
#if 0
    SoundList *S;
//    HRESULT hr; // We could check for errors, but wtf would I do?  Pop up a error dialog behind the 3d display like normal?

//#define SNDLOG

#ifdef SNDLOG
	FILE *fp;

	if(fp=fopen("sndlog.txt","a+"))
	{
		fprintf(fp,"AssignSamples()=========================================\n");
#endif

	
//#define _SNDDBG

#ifdef _SNDDBG
	MonoPrint("AssignSamples()=========================================\n");
#endif

    if(gSoundDriver && DSound)
    {
		S=SampleList;
		while(S)
		{
			{
				int i;

				for(i=0;i<S->DS3DBufferCount;i++)
				{
					if(S->Buf[i].DSoundBuffer)
					{
						if(S->Buf[i].distsq>=0)
						{
#ifdef SNDLOG
							fprintf(fp,"  Sample ID(%d %s) Buf[%d]  xyz(%.4f %.4f %.4f) | vxyz(%.4f %.4f %.4f) | Pitch(%f) | Vol(%.4f) | UID(%d) | Flags(%08x)\n",
								S->ID, 
								(S->Sfx ? S->Sfx->fileName:"none"),
								i,
								S->Buf[i].x, S->Buf[i].y, S->Buf[i].z,
								S->Buf[i].vx, S->Buf[i].vy, S->Buf[i].vz,
								S->Buf[i].pitch,
								S->Buf[i].vol,
								S->Buf[i].uid,
								S->Flags);
#endif
#ifdef _SNDDBG
							MonoPrint("%s - %d  Dist(%f)  Pitch(%f)  Vol(%.4f)  UID(%d)  Flags(%08x)\n",
								(S->Sfx ? S->Sfx->fileName:"none"),
								i,
								sqrt(S->Buf[i].distsq),
								S->Buf[i].pitch,
								S->Buf[i].vol,
								S->Buf[i].uid,
								S->Flags);
#endif


#ifdef CUSTOM_DOPPLER
							if(g_bEnableDopplerSound && 
							   S->Flags & SFX_POS_EXTERN) 
							{
								float d1,d2,xx,yy,zz,m;
								
								xx=S->Buf[i].x-CamPos.x;
								yy=S->Buf[i].y-CamPos.y;
								zz=S->Buf[i].z-CamPos.z;
								
								d1=(float)sqrt(xx*xx + yy*yy + zz*zz);
								
								xx=(S->Buf[i].x + S->Buf[i].vx) - (CamPos.x + CamVelocity.x);
								yy=(S->Buf[i].y + S->Buf[i].vy) - (CamPos.y + CamVelocity.y);
								zz=(S->Buf[i].z + S->Buf[i].vz) - (CamPos.z + CamVelocity.z);
								
								d2=(float)sqrt(xx*xx + yy*yy + zz*zz);
								
								m = ((d1 - d2) / (1100) ) * g_fSoundDopplerFactor;
								
								if(S->Flags & SFX_FLAGS_REVDOP)
									m=-m;
								
								// constrain to +/- mach 1
								if(m < -1.f)
								{  
									m=-1.f;
								}
								if(m > 1.f)
									m = 1.f;
								m+=1; // range is 0 to 2
								
								S->Buf[i].pitch *= m;
								
								// lower volume on sounds moving away
								if(m < 0)
									S->Buf[i].vol += m * 1000; // m is already negative
								
								
								// prevent the pitch from going below 0
								if(S->Buf[i].pitch < 0)
								{
									S->Buf[i].pitch = 0;
									S->Buf[i].vol = -10000; // MLR 12/3/2003 - Fixed, to -10000 not 0
								}	
							}
#endif


							if(S->Buf[i].DSound3dBuffer && S->Buf[i].Is3d)
							{   // sound is 3d
								S->Buf[i].DSound3dBuffer->SetMode( DS3DMODE_NORMAL, DS3D_DEFERRED );
#define DISTEFF_THRESHOLD (100 * 100)
								if(  g_bSoundDistanceEffect                && 
									S->Buf[i].distsq > DISTEFF_THRESHOLD   &&
									S->Flags & SFX_POS_LOOPED ) // MLR 12/3/2003 - Only applied to looping sounds
								{   // sounds lag behind high speed objects
									// only applied to external sounds
									float x,y,z;
									float s;
									
									s=(float)((S->Buf[i].distsq - DISTEFF_THRESHOLD) / (1100 * 1100)); // seconds it takes for the sound to get from the object, to the camera.
									// this had to be fudged a little so that objects real close wouldn't have the effect applied.
									
									x=S->Buf[i].x - S->Buf[i].vx * s;
									y=S->Buf[i].y - S->Buf[i].vy * s;
									z=S->Buf[i].z - S->Buf[i].vz * s;
									S->Buf[i].DSound3dBuffer->SetPosition(x, y, z, DS3D_DEFERRED );
									
								}
								else
								{
									S->Buf[i].DSound3dBuffer->SetPosition(S->Buf[i].x, S->Buf[i].y, S->Buf[i].z, DS3D_DEFERRED );
								}
								
#ifndef CUSTOM_DOPPLER
								if(g_bEnableDopplerSound)
								{
									if(S->Flags & SFX_FLAGS_REVDOP)
									{
										S->Buf[i].DSound3dBuffer->SetVelocity(-S->Buf[i].vx, -S->Buf[i].vy, -S->Buf[i].vz, DS3D_DEFERRED );
									}
									else
									{
										S->Buf[i].DSound3dBuffer->SetVelocity(S->Buf[i].vx, S->Buf[i].vy, S->Buf[i].vz, DS3D_DEFERRED );
									}
								}
#endif
								
							}
							else // non-external or non-3d sounds
							{ 
								// no 3d buffer == no 3d sound
								// we have to do volume and panning by hand.

								if(S->Buf[i].DSound3dBuffer)
								{
								  S->Buf[i].DSound3dBuffer->SetMode(DS3DMODE_DISABLE, DS3D_DEFERRED);
								}

								// MLR 12/4/2003 - We shouldn't be messing with internal volume sounds
								if(S->Flags & SFX_POS_EXTERN &&
								   S->Buf[i].Is3d )
								{   // we're here because there's on 3d buffer allocated.
									// we need to compute the volume by hand to simulate
									// attenuation over distance.

									float v;

									// scale v from 0 to 1 between min & max dist
									v=(S->Buf[i].distsq - S->Sfx->min3ddist) / (S->Sfx->maxDistSq - S->Sfx->min3ddist);

									// clamp result
									if(v<0) v=0; else if(v>1) v=1;

#ifdef SNDLOG
	  							    fprintf(fp,"  v %f  ",v);
#endif



									// this prolly ain't quite scientifically correct. :)
									// log(1)  = 0
									// log(10) = 1
                                    v=(float)log(v*9+1);

									// map v to maxVol -> minVol
									v=S->Sfx->maxVol - (S->Sfx->maxVol - S->Sfx->minVol) * v;
									S->Buf[i].vol+=v;
									if(S->Buf[i].vol<-10000)
									{
										S->Buf[i].distsq=-1; // Set to -1 so the sound will be not played/stoped
										//S->Buf[i].vol=-10000;
									}


#ifdef SNDLOG
	  							    fprintf(fp,"  Computed Vol %f - distsq %f  min3ddist %f  maxDistSq %f\n",S->Buf[i].vol,
										S->Buf[i].distsq, S->Sfx->min3ddist,S->Sfx->maxDistSq);
#endif
								}
								
							}
						}

						if(S->Buf[i].distsq>=0) // MLR 12/7/2003 - No need to play sounds that can't be heard
						{
							long Frequency;
							// set up dsoundbuffer
							if(S->Flags & SFX_FLAGS_FREQ)
							{
								Frequency=(long)(S->Frequency * S->Buf[i].pitch);

								Frequency = min ( Frequency, DSBFREQUENCY_MAX);
								Frequency = max ( DSBFREQUENCY_MIN, Frequency);
							}
							else
							{
								Frequency=(long)(S->Frequency);
							}

							// Play the sample
							if(S->Flags & SFX_POS_LOOPED)
							{
#ifdef SNDLOG
								fprintf(fp,"  Playing Looped\n");
#endif
								// loopy sounds
#ifdef _SNDDBG
	  							    MonoPrint("    Playing Looped - Vol:%f  Freq:%f", S->Buf[i].vol, Frequency);
#endif

								S->Buf[i].DSoundBuffer->SetFrequency(Frequency); // MLR 12/7/2003 - The freq & vol code was moved here
								S->Buf[i].DSoundBuffer->SetVolume((long)S->Buf[i].vol);
								S->Buf[i].DSoundBuffer->Play(0,0,DSBPLAY_LOOPING);
								S->Buf[i].distsq=-1; // mark buffer as unused for the next go around.
							}
							else
							{   // NON Looped sounds
								if(g_bSoundDistanceEffect && (S->Flags & SFX_POS_EXTERN))
								{   // delay external sounds 
									float time,     // Elapsed time since sound was created.
										  radiussq; // MLR 12/2/2003 - The radius from the sounds origin that the soundwave is currently at.

									// since sound radiates out in a sphere from the origin, we'll calculate the radius
									// that the sound has traveled, and see if the camera is inside the sphere.
									// if the camera is inside, the sound is played.
									// if not, the sound is delayed.

									time     = (float)((vuxGameTime - S->Buf[i].Timer) / 1000.0 + 1.0); // when time = 1 = 1sec
									radiussq = (1100 * 1100 * time * time);  // MLR 12/2/2003 - The radius from the sounds origin that the soundwave is currently at.

									float dx,dy,dz;
									
									dx=S->Buf[i].x-CamPos.x;
									dy=S->Buf[i].y-CamPos.y;
									dz=S->Buf[i].z-CamPos.z;

									S->Buf[i].distsq=dx*dx+dy*dy+dz*dz; // distance from camera to sounds origin

									// it would be more correct to only play the sound if the camera is very 
									// close to the radius.
									if(S->Buf[i].distsq < radiussq) // note comparing squared values
									{
#ifdef _SNDDBG
	  							    MonoPrint("    Playing NonLooped - Vol:%f  Freq:%f", S->Buf[i].vol, Frequency);
#endif

										S->Buf[i].DSoundBuffer->SetFrequency(Frequency); // MLR 12/7/2003 - The freq & vol code was moved here
										S->Buf[i].DSoundBuffer->SetVolume((long)S->Buf[i].vol);
										S->Buf[i].DSoundBuffer->SetCurrentPosition(0);
										S->Buf[i].DSoundBuffer->Play(0,0,0);
										S->Buf[i].distsq=-1;
#ifdef SNDLOG
										fprintf(fp,"  Playing Once\n");
#endif
									}
									else
									{
										if(S->Buf[i].distsq > S->Sfx->maxDistSq) // MLR 12/2/2003 - Terminate sounds that can't be heard because they've traveled to far.
										{
											// the sound has outlasted it's lifespan and could not be heard anymore.
											S->Buf[i].distsq=-1;
#ifdef SNDLOG
											fprintf(fp,"  Attenuated to death\n");
#endif
#ifdef _SNDDBG
	  										MonoPrint("    Killed");
#endif

										}
										else
										{
#ifdef _SNDDBG
	  										MonoPrint("    Playing Delayed");
#endif

											
#ifdef SNDLOG
											fprintf(fp,"  Delayed Play (distance effect - distsq=%f.4 time=%f.4)\n",	S->Buf[i].distsq ,  time * (1100 * 1100));
#endif
										}
									}
								}
								else
								{
#ifdef SNDLOG
									fprintf(fp,"  Playing Once\n");
#endif
#ifdef _SNDDBG
	  							    MonoPrint("    Playing Looped - Vol:%f  Freq:%f", S->Buf[i].vol, Frequency);
#endif

									S->Buf[i].DSoundBuffer->SetFrequency(Frequency);		 // MLR 12/22/2003 - added
									S->Buf[i].DSoundBuffer->SetVolume((long)S->Buf[i].vol);	 // MLR 12/18/2003 - Added this
									S->Buf[i].DSoundBuffer->SetCurrentPosition(0);
									S->Buf[i].DSoundBuffer->Play(0,0,0);
									S->Buf[i].distsq=-1;
								}
							}
						}
						else
						{
							if(S->Flags & SFX_POS_LOOPED && S->Buf[i].distsq==-1) // don't stop non-looping sounds, let them finish on thier own.
							{
								// MLR 1/21/2004 - Changed so that the sound is Stop()ed only if distsq is -1
								//                 which may have been causeing trouble with Direct Sound.
								//S->Buf[i].DSoundBuffer->SetVolume(-10000);
								S->Buf[i].DSoundBuffer->Stop();
								S->Buf[i].distsq=-2; 
							}
							else
							{
								S->Buf[i].distsq-=1.0;
							}

						}

					}

				}
			}
			S=S->Next;
		}
	}
	
#ifdef SNDLOG
	fclose(fp);
	}
#endif
#endif
	//return(TRUE);
}



// MLR: this only appears to make the sound non-3D, not disable soud output???
BOOL CSoundMgr::Disable3dSample(long ID)
{
    SoundList * Sample;
    if(gSoundDriver && DSound)
    {
		Sample=FindSample(ID);
		if(Sample != NULL)
		{
			if (Sample->is3d == TRUE)
			{
				int i;

				for(i=0;i<Sample->DS3DBufferCount;i++)
				{
					// 
					//Sample->Buf[i].DSound3dBuffer->SetMode(DS3DMODE_DISABLE, DS3D_DEFERRED);
				}
			}
			return TRUE;
		}
    }
    return FALSE;
}

BOOL CSoundMgr::SetSamplePan(long ID, long Direction)
{
	SoundList * Sample;
	HRESULT hr;

	if(gSoundDriver)
	{
		if(DSound)
		{
			Sample=FindSample(ID);
			if(Sample != NULL)
			{
				hr=Sample->Buf[0].DSoundBuffer->SetPan(Direction);
				if(hr != DS_OK)
					DSoundCheck(hr);
				return(TRUE);
			}
		}
	}
	return(FALSE);
}

long CSoundMgr::ConvertVolumeToDB(long Percentage)
{
	long Result;
	float WorkPerc;

	if(Percentage < 20)
		return(DSBVOLUME_MIN);
	if(Percentage >= 100)
		return(0);

	WorkPerc = (float)Percentage;
	Result=-(long)(4000.0f * (2-log10(WorkPerc)));

	return(Result);
}

long CSoundMgr::ConvertPanToDB(long Direction)
{
	long Result;
	float WorkDir;

	if(Direction < -80)
		return(-10000);
	if(Direction > 80)
		return(10000);

	WorkDir=(float)Direction;
	Result=(long)(4000.0f * log10(WorkDir));

	return(Result);
}

DWORD CSoundMgr::SampleStatus(SoundList *Sample)
{
	DWORD status;

	if(gSoundDriver)
	{
		if(Sample != NULL)
		{
			// if [0] aint playing, then none of them are playing.
			Sample->Buf[0].DSoundBuffer->GetStatus(&status);
			return(status);
		}
	}
	return(0);
}

SoundList *CSoundMgr::AddDuplicateSample(SoundList *Sample)
{
	SoundList *Cur,*New;
#ifdef USE_SH_POOLS
	HRESULT res;
#endif

	if(DuplicateList == NULL)
	{
		#ifdef USE_SH_POOLS
		New= (SoundList *)MemAllocPtr( gSoundMemPool, sizeof(SoundList), 0 );
		memcpy(New,Sample,sizeof(SoundList));
		New->Next=NULL;
		for(int i=0;i<<Sample->DS3DBufferCount;i++)
		{
			res=DSound->DuplicateSoundBuffer(Sample->Buf[0].DSoundBuffer,&New->Buf[i].DSoundBuffer);
		}

		#else
		New=new SoundList(Sample, DSound);
		#endif
		DuplicateList=New;
	}
	else
	{
		Cur=DuplicateList;

		if(Cur->ID == Sample->ID)
			if(!(SampleStatus(Cur) & DSBSTATUS_PLAYING))
				return(Cur);

		while(Cur->Next != NULL)
		{
			if(Cur->Next->ID == Sample->ID)
			{
				if(!(SampleStatus(Cur->Next) & DSBSTATUS_PLAYING))
					return(Cur->Next);
			}
			Cur=Cur->Next;
		}

//		New=(SoundList *)malloc(sizeof(SoundList));
		#ifdef USE_SH_POOLS
		New= (SoundList *)MemAllocPtr( gSoundMemPool, sizeof(SoundList), 0 );
		memcpy(New,Sample,sizeof(SoundList));
		New->Next=NULL;
		for(int i=0;i<Sample->DS3DBufferCount;i++)
		{
			res=DSound->DuplicateSoundBuffer(Sample->Buf[0].DSoundBuffer,&New->Buf[i].DSoundBuffer);
		}
		#else
		New=new SoundList(Sample, DSound);
		#endif
		Cur->Next=New;
	}
	return(New);
}

long CSoundMgr::AddSampleToMgr(long Volume,long Frequency,long Direction,IDirectSoundBuffer *NewSound,long Flags, SFX_DEF_ENTRY *sfx)
{
	SoundList *Cur,*New;

//	New=(SoundList *)malloc(sizeof(SoundList));
	#ifdef USE_SH_POOLS
	New= (SoundList *)MemAllocPtr( gSoundMemPool, sizeof(SoundList), 0 );
	#else
	New=new SoundList;
	#endif
	if(New == NULL)
		return(SND_NO_HANDLE);

	New->ID=TotalSamples+100;

	New->Volume=Volume;
	New->Frequency=Frequency;
	New->Direction=Direction;
	New->MaxDist=(float)sqrt(sfx->maxDistSq);
	New->MinDist=(float)sqrt(sfx->min3ddist);
	New->is3d = FALSE;
	New->Buf[0].DSoundBuffer=NewSound;
	New->Sfx   = sfx;
	New->Flags = sfx->flags;

	if(New->Flags & (SFX_FLAGS_VMS|SFX_POS_INSIDE)) // only allocate 1 soundobject for these types
	{
		New->DS3DBufferCount=1;
	}

	int i;
	for(i=0;i<New->DS3DBufferCount;i++)
	{
		HRESULT hr = S_OK;

		if(i>0)
			hr=DSound->DuplicateSoundBuffer(NewSound, &New->Buf[i].DSoundBuffer);

		/*  MLR 5/6/2004 - No need to allocate a 3D buffer 
		if (g_bUse3dSound && 
			New->Flags & SFX_FLAGS_3D)
		{
			// create buffer here
			//if (FAILED(hr))
			//	DSoundCheck(hr);
			hr = New->Buf[i].DSoundBuffer->QueryInterface(IID_IDirectSound3DBuffer, 
				(LPVOID *)&New->Buf[i].DSound3dBuffer); 
			if (FAILED(hr))
				DSoundCheck(hr);
			else 
			{
				d3dcount ++;
				
				hr = New->Buf[i].DSound3dBuffer->SetMode(DS3DMODE_DISABLE, DS3D_DEFERRED);
				if (FAILED(hr))
					DSoundCheck(hr);
				if ( sfx && 
					(sfx->flags & SFX_POS_EXTERN) && 
					(sfx->flags & SFX_FLAGS_3D)) // only make external 3d sounds 3d
				{
					float maxdist = (float)  sqrt(sfx->maxDistSq); 
					hr = New->Buf[i].DSound3dBuffer->SetMaxDistance(maxdist, DS3D_DEFERRED);
					if (FAILED(hr))
						DSoundCheck(hr);
					float mindist = sfx->min3ddist;
					if (mindist == 0)
						mindist = maxdist/20.0f;
					hr = New->Buf[i].DSound3dBuffer->SetMinDistance(mindist, DS3D_DEFERRED);
					if (FAILED(hr))
						DSoundCheck(hr);


					if(New->Flags & SFX_FLAGS_3D)
					{
						New->Buf[i].DSound3dBuffer->SetMode(DS3DMODE_NORMAL, DS3D_DEFERRED);
					}
				}
			}
		} 
		*/
	}

	New->Next=NULL;
//	if(Flags & SND_EXCLUSIVE)
//		New->Flags |= SND_EXCLUSIVE;
//	if(Flags & SFX_POSITIONAL)
//		New->Flags |= SND_USE_3D;
//	if(Flags & SFX_POS_LOOPED) // MLR 12/6/2003 - commented out
//		New->Flags |= SND_LOOP_SAMPLE; // MLR 12/6/2003 - 



	if(SampleList == NULL)
	{
		SampleList=New;
		TotalSamples++;
	}
	else
	{
		Cur=SampleList;

		while(Cur->Next != NULL)
			Cur=Cur->Next;

		Cur->Next=New;
		TotalSamples++;
	}
	return(New->ID);
}

SoundList * CSoundMgr::FindSample(long ID)
{
	SoundList *Cur;

	if(gSoundDriver)
	{
		Cur=SampleList;

		while(Cur != NULL)
		{
			if(Cur->ID == ID)
				return(Cur);
			Cur=Cur->Next;
		}
	}
	return(NULL);
}

// Streaming Code begins Here
long CSoundMgr::CreateStream(WAVEFORMATEX *Format,float StreamSeconds) // Quesize is in seconds
{
	long NewID = SND_NO_HANDLE;
	DSBUFFERDESC dsbdesc;
	LPDIRECTSOUNDBUFFER lpNewDSBuf;
	HRESULT hr;
	long Size;

	if(gSoundDriver && DSound)
	{
		Size = (long)(StreamSeconds * (float)(Format->nSamplesPerSec * (Format->wBitsPerSample / 8) * Format->nChannels));

		Size=(Size/8)*8;

// Set up DSBUFFERDESC structure.
		memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); // Zero it out.
		dsbdesc.dwSize = sizeof(DSBUFFERDESC);
		dsbdesc.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | 
		    DSBCAPS_CTRLFREQUENCY | 
		    DSBCAPS_GETCURRENTPOSITION2 ; // Need default controls (pan, volume, frequency).
		if (g_bOldSoundAlg == false)
		    dsbdesc.dwFlags |= DSBCAPS_CTRLPOSITIONNOTIFY;
		dsbdesc.dwBufferBytes = Size;
		dsbdesc.lpwfxFormat = Format;

// OW just to have some music during debugging hehe
#ifdef _DEBUG
	dsbdesc.dwFlags |= DSBCAPS_GLOBALFOCUS;
#endif

    // Create buffer.
		hr = DSound->CreateSoundBuffer(&dsbdesc, &lpNewDSBuf, NULL);
		if(hr != DS_OK)
			DSoundCheck(hr);
		if(hr == DS_OK)
		{
			NewID=AddStreamToMgr(0,Format,Size,lpNewDSBuf);
			hr=lpNewDSBuf->SetVolume(DSBVOLUME_MIN);
			if(hr != DS_OK)
				DSoundCheck(hr);
		}
		return(NewID);
	}
	return(SND_NO_HANDLE);
}

DWORD CSoundMgr::StreamStatus(SoundStream *Stream)
{
	DWORD status;

	if(gSoundDriver)
	{
		if(Stream != NULL)
		{
			Stream->DSoundBuffer->GetStatus(&status);
			return(status);
		}
	}
	return(0);
}

long CSoundMgr::GetStreamPlayTime(long ID)
{
	SoundStream *Stream;
	if(gSoundDriver)
	{
		Stream=FindStream(ID);
		if(Stream != NULL)
			return(Stream->BytesProcessed);//*1000/Stream->BytesPerSecond);
	}
	return(0);
}

BOOL CSoundMgr::IsStreamPlaying(long ID)
{
	SoundStream * Stream;
	DWORD status;

	if(gSoundDriver)
	{
		if(DSound)
		{
			Stream=FindStream(ID);
			if(Stream != NULL)
			{
				status=StreamStatus(Stream);
				if(status & DSBSTATUS_PLAYING)
					return(TRUE);
			}
		}
	}
	return(FALSE);
}


long CSoundMgr::SetStreamVolume(long ID, long Volume)
{
	SoundStream * Stream;
	HRESULT hr;
	long oldvol;

	if(gSoundDriver)
	{
		if(DSound)
		{
			Stream=FindStream(ID);
			if(Stream != NULL)
			{
				oldvol=Stream->Volume;
				Stream->Volume=Volume;
				if(!(Stream->Status & (SND_STREAM_FADE_IN|SND_STREAM_FADE_OUT|SND_STREAM_FADEDOUT))){
					hr=Stream->DSoundBuffer->SetVolume(Volume);
					if(hr != DS_OK)
						DSoundCheck(hr);
				}
				return(oldvol);
			}
		}
	}
	return(DSBVOLUME_MIN);
}


// JPO - 3d interface
void CSoundMgr::SetCameraPostion(Tpoint *campos, Trotation *camrot, Tpoint *camvel, bool Reset)
{
	static int reset=1;
	static float olddoppler=-1,oldrolloff;

	CamPos.x=campos->x;
	CamPos.y=campos->y;
	CamPos.z=campos->z;

	CamVelocity.x=camvel->x;
	CamVelocity.y=camvel->y;
	CamVelocity.z=camvel->z;


    if (use3d == FALSE || Ds3dListener == NULL || StreamCSection == NULL || campos == NULL || camrot == NULL)
		return;

    F4EnterCriticalSection(StreamCSection);

    HRESULT hr = Ds3dListener->SetPosition(campos->x, campos->y, campos->z, DS3D_DEFERRED);
    if (FAILED(hr))
	DSoundCheck(hr);
    static const Tpoint upv = { 0, 0, 1 };
    static const Tpoint fwd = { 1, 0, 0 };


	Tpoint front, up;
    MatrixMult(camrot, &upv, &up);
    MatrixMult(camrot, &fwd, &front);
    // compute up vector
    // compute front vector;

    hr= Ds3dListener->SetOrientation(front.x, front.y, front.z, up.x, up.y, up.z, DS3D_DEFERRED);

	if(g_bEnableDopplerSound) // MLR 2003-10-17 ear candy
	{


#ifndef CUSTOM_DOPPLER
	    // we don't need this with the custom doppler code.
		Ds3dListener->SetVelocity(CamVelocity.x,CamVelocity.y,CamVelocity.z,DS3D_DEFERRED);

		if(g_fSoundDopplerFactor!=olddoppler)
		{
			Ds3dListener->SetDopplerFactor(g_fSoundDopplerFactor,DS3D_DEFERRED);
			olddoppler=g_fSoundDopplerFactor;
		}
#endif

		if(g_fSoundRolloffFactor!=oldrolloff)
		{
			Ds3dListener->SetRolloffFactor(g_fSoundRolloffFactor,DS3D_DEFERRED);
			oldrolloff=g_fSoundRolloffFactor;
		}
	}
//#define SNDLOG

#ifdef SNDLOG
	FILE *fp;

	if(fp=fopen("sndlog.txt","a+"))
	{
		fprintf(fp,"SetCameraPosition() Pos(%.2f,%.2f,%.2f) Vel(%.2f,%.2f,%.2f)\n",
			CamPos.x,CamPos.y,CamPos.z,CamVelocity.x,CamVelocity.y,CamVelocity.z);
		fclose(fp);
	}
#endif



    if (FAILED(hr))
	DSoundCheck(hr);
    hr = Ds3dListener->CommitDeferredSettings();
    if (FAILED(hr))
	DSoundCheck(hr);
    F4LeaveCriticalSection(StreamCSection);
}   

void CSoundMgr::SetNotification(SoundStream *Stream)
{
    if (g_bOldSoundAlg) return;
    ShiAssert(Stream->notif != NULL);
    ShiAssert(Stream->lpDsNotify != NULL);
    ShiAssert((Stream->Status & SND_USE_THREAD) != 0);
    
    if ((Stream->Status & SND_USE_THREAD) == 0 || 
	Stream->notif == NULL || 
	Stream->lpDsNotify == NULL) return;

    // set up notifications so we can refill buffers - we'll use the same event for now
    DSBPOSITIONNOTIFY PositionNotify[2];
    DWORD dist = Stream->HalfSize;
    for (int i = 0; i < 2; i++) {
	PositionNotify[i].hEventNotify = Stream->notif;
	PositionNotify[i].dwOffset = i * dist;
    }    
    HRESULT hr = Stream->lpDsNotify->SetNotificationPositions(2, PositionNotify);
    if (hr != S_OK)
	DSoundCheck(hr);
}

long CSoundMgr::AddStreamToMgr(long Volume,WAVEFORMATEX *Header,long StreamSize,IDirectSoundBuffer *NewSound)
{
    SoundStream *Cur,*New;
    SECURITY_ATTRIBUTES ps;
    ShiAssert(NewSound != NULL);
    
#ifdef USE_SH_POOLS
    New= (SoundStream *)MemAllocPtr( gSoundMemPool, sizeof(SoundStream), 0 );
#else
    New=new SoundStream;
#endif
    if(New == NULL)
	return(SND_NO_HANDLE);
    
	New->ID=TotalStreams+50;
	New->Volume=Volume;
	New->CurFade=DSBVOLUME_MIN;
	New->FadeIn=DSBVOLUME_MIN;
	New->FadeOut=DSBVOLUME_MIN;
	New->Frequency=Header->nSamplesPerSec;
	New->BytesPerSecond=Header->nAvgBytesPerSec;
	New->Direction=0;//Direction;
	New->OriginalSize=StreamSize;
	New->BytesProcessed=0;
	New->LastPos=0;
	New->LoopOffset=0;
	New->LoopCount=-1;
	New->HalfSize=StreamSize/2;
	New->Size=New->HalfSize*2;
	New->Status=0;
	New->DSoundBuffer=NewSound;
	if (g_bOldSoundAlg == false) {
	    HRESULT hr = New->DSoundBuffer->QueryInterface(IID_IDirectSoundNotify, 
		(LPVOID *)&New->lpDsNotify); 
	    if (FAILED(hr) )
	    { 
		DSoundCheck(hr);
		New->lpDsNotify = NULL;
	    }
	    New->notif = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
	New->fp=INVALID_HANDLE_VALUE;
	New->StreamSize=0;
	#ifdef USE_SH_POOLS
	New->ImaInfo= (IMA_STREAM *)MemAllocPtr( gSoundMemPool, sizeof(IMA_STREAM), 0 );
	#else
	New->ImaInfo=new IMA_STREAM;
	#endif
	memset(New->ImaInfo,0,sizeof(IMA_STREAM));
	New->ImaInfo->type=Header->nChannels;
	New->ImaInfo->bufsize=Header->nBlockAlign * (StreamSize / (Header->nBlockAlign * 2));
	New->ImaInfo->srcbuffer=new char[New->ImaInfo->bufsize];
	// I do this, so I can use same stream for memory streaming (without losing buffer)
	New->ImaInfo->srcsize=New->ImaInfo->bufsize;
	New->ImaInfo->src=New->ImaInfo->srcbuffer;

	New->ImaInfo->sreadidx=0;
	New->ImaInfo->slen=0;
	New->ImaInfo->dlen=0;
	New->Callback=NULL;
	New->StreamMessage=NULL;
	New->me=NULL;
	New->Next=NULL;

	F4EnterCriticalSection(StreamCSection);
	if(StreamList == NULL)
	{
		StreamList=New;
		TotalStreams++;
		memset(&ps,0,sizeof(SECURITY_ATTRIBUTES));
		ps.nLength=sizeof(ps);
		ps.bInheritHandle=TRUE;

		StreamRunning=TRUE;
		StreamThreadID = (HANDLE)_beginthreadex( NULL, 0, StreamThread, this, 0, &PSoundThreadID );
		//SetThreadPriority(StreamThreadID,THREAD_PRIORITY_ABOVE_NORMAL);
	}
	else
	{
		Cur=StreamList;

		while(Cur->Next != NULL)
			Cur=Cur->Next;

		Cur->Next=New;
		TotalStreams++;
	}
	F4LeaveCriticalSection(StreamCSection);
	return(New->ID);
}

void CSoundMgr::SetMessageCallback(int ID,void (*cb)(SoundStream *,int))
{
	SoundStream *cur;

	cur=FindStream(ID);
	if(cur)
		cur->StreamMessage=cb;
}

void CSoundMgr::SetFadeIn(long ID,long volume)
{
	SoundStream *cur;

	cur=FindStream(ID);
	if(cur)
		cur->FadeIn=volume;
}

void CSoundMgr::SetFadeOut(long ID,long volume)
{
	SoundStream *cur;

	cur=FindStream(ID);
	if(cur)
		cur->FadeOut=volume;
}

void CSoundMgr::SetLoopCounter(long ID,long count)
{
	SoundStream *cur;

	cur=FindStream(ID);
	if(cur)
		cur->LoopCount= static_cast<short>(count);
}

void CSoundMgr::SetLoopOffset(long ID,DWORD offset)
{
	SoundStream *cur;

	cur=FindStream(ID);
	if(cur)
		cur->LoopOffset=offset;
}

SoundStream *CSoundMgr::FindStream(long ID)
{
	SoundStream *Cur;

	if(gSoundDriver)
	{
		Cur=StreamList;

		while(Cur != NULL)
		{
			if(Cur->ID == ID)
				return(Cur);
			Cur=Cur->Next;
		}
	}
	return(NULL);
}

void CSoundMgr::RestartStream(SoundStream *Stream)
{
	if(Stream->Status & SND_STREAM_FILE)
	{
		if(Stream->LoopOffset && Stream->LoopOffset < Stream->OriginalSize) // NOT supported for IMA_ADPCM
		{
			SetFilePointer(Stream->fp,Stream->HeaderOffset+Stream->LoopOffset,NULL,FILE_BEGIN);
			Stream->StreamSize=Stream->LoopOffset;
		}
		else
		{
			SetFilePointer(Stream->fp,Stream->HeaderOffset,NULL,FILE_BEGIN);
			if(Stream->ImaInfo)
			{
				Stream->ImaInfo->sidx=0;
				Stream->ImaInfo->sreadidx=-1;
				Stream->ImaInfo->count=0;
				Stream->ImaInfo->blockLength=0;
				Stream->ImaInfo->didx=0;
			}
			Stream->StreamSize=0;
		}
	}
	else if(Stream->Status & SND_STREAM_MEMORY)
	{
		Stream->memptr=Stream->startptr;
		Stream->StreamSize=0;
		if(Stream->ImaInfo)
		{
			Stream->ImaInfo->sidx=0;
			Stream->ImaInfo->sreadidx=-1;
			Stream->ImaInfo->count=0;
			Stream->ImaInfo->blockLength=0;
			Stream->ImaInfo->didx=0;
		}
	}
}

void CSoundMgr::SilenceStream(SoundStream *Stream,DWORD Buffer,DWORD Length)
{
	char *mem;
	DWORD Len;
	HRESULT hr;

	if(gSoundDriver)
	{
		if(Stream == NULL)
			return;

		if(Stream->DSoundBuffer)
		{
			hr=Stream->DSoundBuffer->Lock(Buffer,Length,(void**)&mem,&Len,NULL,NULL,NULL);
			if(hr == DSERR_BUFFERLOST)
			{
				Stream->DSoundBuffer->Restore();
				hr=Stream->DSoundBuffer->Lock(Buffer,Length,(void**)&mem,&Len,NULL,NULL,NULL);
			}
			if(Len && hr == DS_OK)
			{
				memset((char *)mem,0,Length);
				Stream->DSoundBuffer->Unlock(mem,Length,NULL,NULL);
			}
		}
	}
}

DWORD CSoundMgr::ReadStream(SoundStream *Stream,DWORD Buffer,DWORD Length)
{
	char *mem;
	DWORD Len;
	DWORD bytesread = 0;
	HRESULT hr;

	if(gSoundDriver)
	{
		if(Stream == NULL)
			return(0);

		if(Stream->DSoundBuffer)
		{
			hr=Stream->DSoundBuffer->Lock(Buffer,Length,(void**)&mem,&Len,NULL,NULL,NULL);
			if(hr == DSERR_BUFFERLOST)
			{
				Stream->DSoundBuffer->Restore();
				Stream->DSoundBuffer->Lock(Buffer,Length,(void**)&mem,&Len,NULL,NULL,NULL);
			}
			if(Len && hr == DS_OK)
			{
				if(Stream->Status & SND_STREAM_FILE)
				{
					if(Stream->Status & SND_IS_IMAADPCM)
					{
						bytesread=StreamIMAADPCM(Stream,mem,Len);
					}
					else
						ReadFile(Stream->fp,mem,Len,&bytesread,NULL);
					Stream->StreamSize += bytesread;

					if(bytesread < Length)
					{
						if(Stream->Status & SND_STREAM_LOOP && (Stream->LoopCount > 0 || Stream->LoopCount == -1))
						{
							RestartStream(Stream);
							if(Stream->Status & SND_IS_IMAADPCM)
							{
								bytesread=StreamIMAADPCM(Stream,((char *)(mem) + bytesread),Length-bytesread);
							}
							else
								ReadFile(Stream->fp,((char *)(mem) + bytesread),Length-bytesread,&bytesread,NULL);
							Stream->StreamSize+=bytesread;
							if(Stream->LoopCount > 0)
								Stream->LoopCount--;
							if(!Stream->LoopCount && Stream->FadeOut < Stream->Volume) // Do fade out
							{
								if(Stream->StreamMessage)
									(*Stream->StreamMessage)(Stream,SND_MSG_START_FADE);
								Stream->Status |= SND_STREAM_FADE_OUT;
							}
						}
						else
						{
							if(Stream->StreamMessage)
								(*Stream->StreamMessage)(Stream,SND_MSG_STREAM_EOF);
							if(Stream->Status & SND_STREAM_CONTINUE) // Set in callback to pass another stream
							{ // Kludge code used to string multiple files together
								Stream->Status ^= SND_STREAM_CONTINUE;
								RestartStream(Stream);
								if(Stream->Status & SND_IS_IMAADPCM)
								{
									bytesread=StreamIMAADPCM(Stream,((char *)(mem) + bytesread),Length-bytesread);
								}
								else
									ReadFile(Stream->fp,((char *)(mem) + bytesread),Length-bytesread,&bytesread,NULL);
								Stream->StreamSize+=bytesread;
							}
							else
								memset(((char *) (mem)+bytesread),0,Length-bytesread);
						}
					}
				}
				else if(Stream->Status & SND_STREAM_MEMORY)
				{
					if(Stream->Status & SND_IS_IMAADPCM)
					{
						bytesread=MemStreamIMAADPCM(Stream,mem,Len);
					}
					else
					{
						if((Stream->OriginalSize - Stream->StreamSize) >= Len)
						{
							memcpy(mem,Stream->memptr,Len);
							bytesread=Len;
							Stream->memptr = ((char *) (Stream->memptr) + bytesread);
						}
						else
						{
							memcpy(mem,Stream->memptr,Stream->OriginalSize - Stream->StreamSize);
							bytesread=Stream->OriginalSize - Stream->StreamSize;
							Stream->memptr = ((char *) (Stream->memptr) + bytesread);
						}
					}

					if(bytesread < 0)
						bytesread=0;

					Stream->StreamSize += bytesread;

					if(bytesread < Length)
					{
						if(Stream->Status & SND_STREAM_LOOP && (Stream->LoopCount > 0 || Stream->LoopCount == -1))
						{
							RestartStream(Stream);
							if(Stream->Status & SND_IS_IMAADPCM)
							{
								bytesread=MemStreamIMAADPCM(Stream,((char *)(mem) + bytesread),Length-bytesread);
							}
							else
							{
								if((Stream->OriginalSize - Stream->StreamSize) >= (Length - bytesread))
								{
									memcpy(mem,Stream->memptr,(Length - bytesread));
									Stream->memptr = ((char *) (Stream->memptr) + (Length - bytesread));
									bytesread=(Length - bytesread);
								}
								else
								{
									memcpy(mem,Stream->memptr,Stream->OriginalSize - Stream->StreamSize);
									Stream->memptr = ((char *) (Stream->memptr) + bytesread);
									bytesread=Stream->OriginalSize - Stream->StreamSize;
								}
							}
							Stream->StreamSize+=bytesread;
							if(Stream->LoopCount > 0)
								Stream->LoopCount--;
							if(!Stream->LoopCount && Stream->FadeOut < Stream->Volume) // Do fade out
							{
								if(Stream->StreamMessage)
									(*Stream->StreamMessage)(Stream,SND_MSG_START_FADE);
								Stream->Status |= SND_STREAM_FADE_OUT;
							}
						}
						else
							memset(((char *) (mem)+bytesread),0,Length-bytesread);
					}
					if(!(Stream->Status & SND_IS_IMAADPCM))
						Stream->memptr = ((char *)(Stream->startptr) + Stream->StreamSize);
				}
				else if(Stream->Status & SND_STREAM_CALLBACK)
				{
					bytesread=Stream->Callback(Stream->me,mem,Length);
					Stream->StreamSize+=bytesread;
					if(bytesread < Length)
                        memset(((char *) (mem)+bytesread),0,Length-bytesread);
				}
			}
			Stream->DSoundBuffer->Unlock(mem,Len,NULL,NULL);
		}
	}
	return(bytesread);
}

void CSoundMgr::PauseStream(long StreamID)
{
	if(gSoundDriver)
		StreamPause(FindStream(StreamID));
}

void CSoundMgr::FadeOutStream(long StreamID)
{
	if(gSoundDriver)
		StreamFadeOut(FindStream(StreamID));
}

void CSoundMgr::StopStream(long StreamID)
{
	if(gSoundDriver)
		StreamStop(FindStream(StreamID));
}

void CSoundMgr::StopStreamWithFade(long StreamID)
{
	if(gSoundDriver)
		StreamStopWithFade(FindStream(StreamID));
}

void CSoundMgr::ResumeStreamFadeIn(long StreamID)
{
	if(gSoundDriver)
		StreamResumeFadeIn(FindStream(StreamID));
}

void CSoundMgr::ResumeStream(long StreamID)
{
	if(gSoundDriver)
		StreamResume(FindStream(StreamID));
}

void CSoundMgr::StopAllStreams()
{
	SoundStream *cur;
	if(gSoundDriver)
	{
		cur=StreamList;
		while(cur != NULL)
		{
			StreamStop(cur);
			cur=cur->Next;
		}
	}
}

void CSoundMgr::StreamPause(SoundStream *Stream)
{
	if(!Stream)
		return;

	if(gSoundDriver)
	{
		Stream->DSoundBuffer->Stop();
		Stream->Status &= ~SND_USE_THREAD;
		NotifyThread();
	}
}

void CSoundMgr::StreamFadeOut(SoundStream *Stream)
{
	if(!Stream)
		return;

	if(gSoundDriver)
	{
		Stream->FadeOut=DSBVOLUME_MIN;
		Stream->Status |= SND_STREAM_FADE_OUT;
		Stream->Status &= ~SND_STREAM_FADE_IN;
		NotifyThread();
	}
}

void CSoundMgr::StreamResume(SoundStream *Stream)
{
	if(!Stream)
		return;

	if(gSoundDriver)
	{
	    HRESULT hr;
		Stream->Status |= SND_USE_THREAD;
		SetNotification(Stream);
		hr = Stream->DSoundBuffer->Play(0,0,DSBPLAY_LOOPING);
		if (FAILED(hr))
		    DSoundCheck(hr);

		Stream->Status &= ~SND_STREAM_FADEDOUT;
		NotifyThread();
	}
}

void CSoundMgr::StreamResumeFadeIn(SoundStream *Stream)
{
	if(!Stream)
		return;

	Stream->Status |= SND_STREAM_FADE_IN;
	Stream->Status &= ~(SND_STREAM_FADEDOUT|SND_STREAM_FADE_OUT);
	StreamResume(Stream);
}

void CSoundMgr::StreamStop(SoundStream *Stream)
{
	if(!Stream)
		return;

	if(gSoundDriver)
	{
		if(Stream->Status & SND_STREAM_FILE)
		{
			if(Stream->fp != INVALID_HANDLE_VALUE)
				CloseHandle(Stream->fp);
			Stream->fp = INVALID_HANDLE_VALUE;
		}
		Stream->Status &= ~SND_USE_THREAD;
		Stream->DSoundBuffer->Stop();
		NotifyThread();
	}
}

void CSoundMgr::StreamStopWithFade(SoundStream *Stream)
{
	if(!Stream)
		return;

	if(gSoundDriver)
	{
		if(Stream->Status & SND_STREAM_FILE)
		{
			Stream->Status |= SND_STREAM_FADE_OUT;
			Stream->FadeOut=DSBVOLUME_MIN;
			NotifyThread();
		}
	}
}

BOOL CSoundMgr::StartFileStream(long StreamID,char *filename,long Flags,long startoffset)
{
	SoundStream *Stream;
	WAVEFORMATEX Header;
	HRESULT hr;
	long size,NumSamples;

	Stream=FindStream(StreamID);
	if(Stream == NULL)
		return(FALSE);

// Stop previous Stream in Stream
	if(StreamStatus(Stream) & DSBSTATUS_PLAYING)
	{
		StreamStop(Stream);
	}

	Stream->fp=CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,
						  OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL);

	if(Stream->fp == INVALID_HANDLE_VALUE)
		return(FALSE);

	SetFilePointer(Stream->fp,startoffset,NULL,FILE_BEGIN);
	size=LoadRiffFormat(Stream->fp,&Header,&Stream->HeaderOffset,&NumSamples);
	Stream->HeaderOffset+=startoffset;

	if(!size)
	{
		CloseHandle(Stream->fp);
		Stream->fp=INVALID_HANDLE_VALUE;
		return(FALSE);
	}

	Stream->StreamSize=0;
	Stream->OriginalSize=size;
	Stream->BytesProcessed=0;
	Stream->LastPos=0;
	Stream->memptr=NULL;
	Stream->startptr=NULL;
	Stream->Status=1 | SND_STREAM_FILE;
	if(Header.wFormatTag == WAVE_FORMAT_IMA_ADPCM)
	{
		Stream->Status |= SND_IS_IMAADPCM;
		if(!Stream->ImaInfo)
			return(FALSE);
		Stream->ImaInfo->sidx=0;
		Stream->ImaInfo->count=0;
		Stream->ImaInfo->blockLength=0;
		Stream->ImaInfo->didx=0;
		Stream->ImaInfo->sreadidx=-1; // When ReadStream gets called... read entire buffer size (if -1)
		Stream->ImaInfo->slen=size;
		Stream->ImaInfo->dlen=NumSamples; // (2 bytes) since we only handle 16bit
		Stream->ImaInfo->src=Stream->ImaInfo->srcbuffer;
		Stream->ImaInfo->srcsize=Stream->ImaInfo->bufsize;
	}
	else
		Stream->Status &= ~SND_IS_IMAADPCM;

	if(Flags & SND_STREAM_LOOP)
		Stream->Status |= SND_STREAM_LOOP;
	if(Flags & SND_STREAM_FADE_IN)
	{
		Stream->Status |= SND_STREAM_FADE_IN;
		Stream->CurFade=Stream->FadeIn;
	}
	else
		Stream->CurFade=Stream->Volume;
	if(Flags & SND_STREAM_FADE_OUT)
		Stream->FadeOut=DSBVOLUME_MIN;
	else
		Stream->FadeOut=Stream->Volume;

	hr=Stream->DSoundBuffer->SetCurrentPosition(0);
	if(hr != DS_OK)
		DSoundCheck(hr);
	SilenceStream(Stream,0,Stream->Size);
	hr=Stream->DSoundBuffer->SetVolume(Stream->CurFade);
	if(hr != DS_OK)
		DSoundCheck(hr);
	Stream->StreamSize=0;
	ReadStream(Stream,0,Stream->Size);
	Stream->Status |= SND_USE_THREAD;
	SetNotification(Stream);
	hr=Stream->DSoundBuffer->Play(0,0,DSBPLAY_LOOPING);
	if(hr != DS_OK)
		DSoundCheck(hr);
	NotifyThread();

	return(TRUE);
}

BOOL CSoundMgr::StartMemoryStream(long StreamID,RIFF_FILE *wave,long Flags)
{
	SoundStream *Stream;
	HRESULT hr;
	long size;

	Stream=FindStream(StreamID);
	if(Stream == NULL)
		return(FALSE);

// Stop previous Stream in Stream
	if(StreamStatus(Stream) & DSBSTATUS_PLAYING)
	{
		StreamStop(Stream);
	}

	size=wave->SampleLen;

	Stream->fp=NULL; // should be closed (if open) by StreamStop()
	Stream->StreamSize=0;
	Stream->OriginalSize=size;
	Stream->BytesProcessed=0;
	Stream->LastPos=0;
	Stream->memptr=NULL;
	Stream->startptr=NULL;
	Stream->Status=1 | SND_STREAM_MEMORY;
	if(wave->Format->wFormatTag == WAVE_FORMAT_IMA_ADPCM)
	{
		Stream->Status |= SND_IS_IMAADPCM;
		Stream->ImaInfo->sidx=0;
		Stream->ImaInfo->count=0;
		Stream->ImaInfo->blockLength=0;
		Stream->ImaInfo->didx=0;
		Stream->ImaInfo->sreadidx=0; // When ReadStream gets called... read entire buffer size (if -1)
		Stream->ImaInfo->slen=size;
		Stream->ImaInfo->dlen=wave->NumSamples; // (2 bytes) since we only handle 16bit
		Stream->ImaInfo->src=wave->Start;
		Stream->ImaInfo->srcsize=size;
	}
	else
		Stream->Status &= ~SND_IS_IMAADPCM;
	if(Flags & SND_STREAM_LOOP)
		Stream->Status |= SND_STREAM_LOOP;
	if(Flags & SND_STREAM_FADE_IN)
	{
		Stream->Status |= SND_STREAM_FADE_IN;
		Stream->CurFade=Stream->FadeIn;
	}
	else
		Stream->CurFade=Stream->Volume;
	if(Flags & SND_STREAM_FADE_OUT)
		Stream->FadeOut=DSBVOLUME_MIN;
	else
		Stream->FadeOut=DSBVOLUME_MAX;

	hr=Stream->DSoundBuffer->SetCurrentPosition(0);
	if(hr != DS_OK)
		DSoundCheck(hr);
	SilenceStream(Stream,0,Stream->Size);
	hr=Stream->DSoundBuffer->SetVolume(Stream->CurFade);
	if(hr != DS_OK)
		DSoundCheck(hr);
	Stream->StreamSize=0;
	ReadStream(Stream,0,Stream->Size);
	Stream->Status |= SND_USE_THREAD;
	SetNotification(Stream);
	hr=Stream->DSoundBuffer->Play(0,0,DSBPLAY_LOOPING);
	if(hr != DS_OK)
		DSoundCheck(hr);
	NotifyThread();

	return(TRUE);
}

BOOL CSoundMgr::StartMemoryStream(long StreamID,char *Data,long size)
{
	SoundStream *Stream;

	Stream=FindStream(StreamID);

	if(Stream == NULL)
		return(FALSE);

// Stop previous Stream in Stream
	if(StreamStatus(Stream) & DSBSTATUS_PLAYING)
	{
		StreamStop(Stream);
	}

	Stream->StreamSize=0;
	Stream->OriginalSize=size;
	Stream->BytesProcessed=0;
	Stream->LastPos=0;
	Stream->memptr=Data;
	Stream->startptr=Data;
	Stream->Status=1 | SND_STREAM_MEMORY;
	ReadStream(Stream,0,Stream->Size);
	Stream->DSoundBuffer->SetCurrentPosition(0);
	Stream->Status |= SND_USE_THREAD;
	SetNotification(Stream);
	Stream->DSoundBuffer->Play(0,0,DSBPLAY_LOOPING);
	NotifyThread();

	return(TRUE);
}

BOOL CSoundMgr::StartCallbackStream(long StreamID,void *classptr,DWORD (*cb)(void *me,char *mem,DWORD Len))
{
	SoundStream *Stream;

	Stream=FindStream(StreamID);

	if(Stream == NULL)
		return(FALSE);

// Stop previous Stream in Stream
	if(StreamStatus(Stream) & DSBSTATUS_PLAYING)
	{
		StreamStop(Stream);
	}

    Stream->StreamSize=0;
	Stream->OriginalSize=0;
	Stream->BytesProcessed=0;
	Stream->LastPos=0;
	Stream->memptr=NULL;
	Stream->startptr=NULL;
	Stream->me=classptr;
	Stream->Callback=cb;
	Stream->Status=1 | SND_STREAM_CALLBACK;
	ReadStream(Stream,0,Stream->Size);
	Stream->DSoundBuffer->SetCurrentPosition(0);
	Stream->Status |= SND_USE_THREAD;
	SetNotification(Stream);
	Stream->DSoundBuffer->Play(0,0,DSBPLAY_LOOPING);
	NotifyThread();

	return(TRUE);
}

void CSoundMgr::RemoveStream(long ID)
{
	SoundStream *Cur,*Last;

	if(gSoundDriver == NULL)
		return;

	F4EnterCriticalSection(StreamCSection); // JPO - lock before test!
	if(StreamList == NULL) {
	    F4LeaveCriticalSection(StreamCSection);
	    return;
	}


	if(StreamList->ID == ID)
	{
		Last=StreamList;
		StreamList=StreamList->Next;
		delete Last;
	}
	else
	{
		Cur=StreamList;
		while(Cur->Next)
		{
			if(Cur->Next->ID == ID)
			{
				Last=Cur->Next;
				Cur->Next=Cur->Next->Next;
				delete Last;
			}
			else
				Cur=Cur->Next;
		}
	}
	F4LeaveCriticalSection(StreamCSection);
	NotifyThread();
}

void CSoundMgr::RemoveAllStreams()
{
	SoundStream *Cur,*Last;

	if(gSoundDriver == NULL)
		return;

	if(StreamList == NULL)
		return;

	F4EnterCriticalSection(StreamCSection);

	Cur=StreamList;

	while(Cur != NULL)
	{
		Last=Cur;
		Cur=Cur->Next;
		delete Last;
	}
	StreamList=NULL;
	F4LeaveCriticalSection(StreamCSection);
	NotifyThread();
}

unsigned int __stdcall CSoundMgr::StreamThread(void *myself)
{
   ((CSoundMgr *)myself)->ThreadHandler();
   return (0);
}


// routine does two things
// 1. Builds up the array of event switches we will block on
// 2. Checks to see if for any reason we can't do the block, because
//    of constraints on whats going on.
BOOL CSoundMgr::BuildObjectList(HANDLE hArray[], int *nHandles, SoundStream *slist[])
{
    ShiAssert(signalEvent != NULL);
    SoundStream *Stream;
    int count = 0;
    slist[count] = NULL;
    hArray[count++] = signalEvent;
    *nHandles = 1; // set here in case we have to exit in a rush
    for (Stream=StreamList; Stream != NULL; Stream=Stream->Next) {
		if((Stream->Status & SND_USE_THREAD) == 0){
			// not played on a thread
			continue;
		}
		if(Stream->Status & SND_STREAM_DONE){
			// we just don't care
			continue;
		}
		// any of the following we have to do more carefully.
		if(Stream->Status & (SND_STREAM_PAN_LT|SND_STREAM_PAN_RT|SND_STREAM_FADE_IN|SND_STREAM_FADE_OUT)){
			return FALSE;
		}
		if (Stream->notif == NULL){
			return FALSE; // how did this slip through??
		}
		hArray[count] = Stream->notif;
		slist[count] = Stream;
		++count;
		if (count >= MAXIMUM_WAIT_OBJECTS){
			// too much going on
			return FALSE;
		}
    }
    *nHandles = count;
	return g_bOldSoundAlg ? FALSE : TRUE;
}

// JPO - try and make some sense of this routine.
void CSoundMgr::ThreadHandler()
{
    SoundStream *Stream;
    static int dtime = 10;
    DWORD timer;
    HANDLE hArray[MAXIMUM_WAIT_OBJECTS];
    SoundStream *sstreams[MAXIMUM_WAIT_OBJECTS];
    int nHandles;
    bool scanall = false;

	// JPO basically there are 3 things we need to do in this loop.
    // 1. Check for buffers getting empty, and refill them
    // 2. Check for panning effects
    // 3. Check for fade effects.
    do{
		scanall = true;
		// RV - RED - Recoded in a decent way... with SLEEP() alwasy done
		//if (g_bOldSoundAlg) Sleep(dtime);
		//else {
		Sleep(dtime);
		if (!g_bOldSoundAlg){
			F4EnterCriticalSection(StreamCSection);
			if (BuildObjectList(hArray, &nHandles, sstreams) == FALSE) timer = dtime; // we have to go the slow route
			// we could set this to like 5 mins maybe to keep things going
			else timer = INFINITE; 

			F4LeaveCriticalSection(StreamCSection);
			DWORD result = WaitForMultipleObjects(nHandles, hArray, FALSE, timer);
			switch(result) {
				case WAIT_OBJECT_0: // something new has happened, rescan
									break;
				
				case WAIT_FAILED:	MonoPrint("Wait failed in CSoundMgr::TheadHandler\n");
									break;
				
				case WAIT_TIMEOUT:	break;

				default:			if (result > WAIT_OBJECT_0 && result < (WAIT_OBJECT_0 + nHandles)) {
										Stream = sstreams[result-WAIT_OBJECT_0];
										//MonoPrint("Sound triggered on stream %x\n", Stream);
										F4EnterCriticalSection(StreamCSection);
										// check it is still valid
										for (SoundStream *sp=StreamList; sp; sp=sp->Next) if (Stream == sp)ProcessStream(Stream);
										F4LeaveCriticalSection(StreamCSection);
										scanall = false;
									}
									break;
			}
		}

		// RV - RED - Thread no more valid....
		if(this == NULL)_endthreadex(0);

		if (scanall) {
#ifdef _DEBUG
		    //loopcount ++;
#endif
			F4EnterCriticalSection(StreamCSection);
			// ok - so we loop through all streams, looking for things to do.
			for (Stream=StreamList; Stream != NULL; Stream=Stream->Next){
				if((Stream->Status & SND_USE_THREAD) == 0) continue;			// not played on a thread
				if(Stream->Status & SND_STREAM_DONE){
					StreamStop(Stream);
					if(Stream->StreamMessage)
					(*Stream->StreamMessage)(Stream,SND_MSG_STREAM_DONE);
					continue;
				}
				ProcessStream(Stream);
			}
			F4LeaveCriticalSection(StreamCSection);
		}
    } while(StreamRunning);
    StreamThreadID=0;
    _endthreadex(0);
}

void CSoundMgr::ProcessStream(SoundStream *Stream)
{
    DWORD Pos,Dummy,bytesread;
    // either we are in the 1st or 2nd half of the buffer.
    HRESULT hr=Stream->DSoundBuffer->GetCurrentPosition(&Pos,&Dummy);
    if(!(Stream->Status & SND_STREAM_PART2)){
		// if we have moved beyond the half way stage, we fill up
		// the first half of the buffer.
		// sfr: added =, was only >
		if(Pos >= Stream->HalfSize){
			bytesread=ReadStream(Stream,0,Stream->HalfSize);
			if(!bytesread){
				if(Stream->Status & SND_STREAM_FINAL)
				{
					Stream->Status &= ~SND_STREAM_FINAL;
					Stream->Status |= SND_STREAM_DONE;
				}
				else{
					Stream->Status |= SND_STREAM_FINAL;
				}
			}
			Stream->Status ^= SND_STREAM_PART2;
		}
    }
    else{
		//MonoPrint("ProcessStream in pt2, pos %d half %d\n", Pos, Stream->HalfSize);
		if(Pos < Stream->HalfSize){
			bytesread=ReadStream(Stream,Stream->HalfSize,Stream->HalfSize);
			if(!bytesread){
				if(Stream->Status & SND_STREAM_FINAL){
					Stream->Status &= ~SND_STREAM_FINAL;
					Stream->Status |= SND_STREAM_DONE;
				}
				else{
					Stream->Status |= SND_STREAM_FINAL;
				}
			}
			Stream->Status ^= SND_STREAM_PART2;
		}
    }
    
	if( Stream->LastPos <= Pos ){
		Stream->BytesProcessed += ( Pos - Stream->LastPos );
	}
	else{
		Stream->BytesProcessed += ( Pos + ( Stream->Size - Stream->LastPos ) );
	}
    Stream->LastPos = Pos;
    
    
    if(Stream->Status & SND_STREAM_PAN_LT)
    {
	Stream->Direction-=FADE_OUT_STEP;
	if(Stream->Direction <= -10000)
	{
	    Stream->Direction=-10000;
	    Stream->Status ^= SND_STREAM_PAN_LT;
	    if(Stream->Status & SND_STREAM_PAN_CIR)
		Stream->Status |= SND_STREAM_PAN_RT;
	}
	Stream->DSoundBuffer->SetPan(Stream->Direction);
    }
    else if(Stream->Status & SND_STREAM_PAN_RT)
    {
	Stream->Direction+=FADE_OUT_STEP;
	if(Stream->Direction >= 10000)
	{
	    Stream->Direction=10000;
	    Stream->Status ^= SND_STREAM_PAN_RT;
	    if(Stream->Status & SND_STREAM_PAN_CIR)
		Stream->Status |= SND_STREAM_PAN_LT;
	}
	Stream->DSoundBuffer->SetPan(Stream->Direction);
    }
    
    if(Stream->Status & SND_STREAM_FADE_IN)
    {
	if(Stream->CurFade < SND_MIN_VOLUME)
	    Stream->CurFade=SND_MIN_VOLUME;
	else
	    Stream->CurFade += FADE_IN_STEP;
	if(Stream->CurFade >= Stream->Volume)
	{
	    Stream->CurFade=Stream->Volume;
	    Stream->Status ^= SND_STREAM_FADE_IN;
	    if(Stream->StreamMessage)
		(*Stream->StreamMessage)(Stream,SND_MSG_FADE_IN_DONE);
	}
	Stream->DSoundBuffer->SetVolume(Stream->CurFade);
    }
    else if(Stream->Status & SND_STREAM_FADE_OUT)
    {
	if(Stream->CurFade > SND_MIN_VOLUME)
	    Stream->CurFade -= FADE_OUT_STEP;
	else
	    Stream->CurFade=DSBVOLUME_MIN;
	if(Stream->CurFade <= Stream->FadeOut)
	{
	    Stream->CurFade=Stream->FadeOut;
	    Stream->Status ^= SND_STREAM_FADE_OUT;
	    if(Stream->StreamMessage)
		(*Stream->StreamMessage)(Stream,SND_MSG_FADE_OUT_DONE);
	}
	Stream->DSoundBuffer->SetVolume(Stream->CurFade);
    }
}

void CSoundMgr::DSoundCheck(HRESULT hr)
{
#if 0 // MLR 2003-10-21 Nothing like reporting cryptic messages that the user can't do a thing about.
	switch(hr)
	{
		case DS_OK:
			break;
// The call failed because resources (such as a priority level)
// were already being used by another caller.
		case DSERR_ALLOCATED:
//			MessageBox(NULL,"DSERR_ALLOCATED","CSoundMgr",MB_OK);
			MonoPrint("DSERR_ALLOCATED");
			break;
// The control (vol,pan,etc.) requested by the caller is not available.
		case DSERR_CONTROLUNAVAIL:
//			MessageBox(NULL,"DSERR_CONTROLUNAVAIL","CSoundMgr",MB_OK);
			MonoPrint("DSERR_CONTROLUNAVAIL");
			break;
// An invalid parameter was passed to the returning function
		case DSERR_INVALIDPARAM:
//			MessageBox(NULL,"DSERR_INVALIDPARAM","CSoundMgr",MB_OK);
			MonoPrint("DSERR_INVALIDPARAM");
			break;
// This call is not valid for the current state of this object
		case DSERR_INVALIDCALL:
//			MessageBox(NULL,"DSERR_INVALIDCALL","CSoundMgr",MB_OK);
			MonoPrint("DSERR_INVALIDCALL");
			break;
// An undetermined error occured inside the DirectSound subsystem
		case DSERR_GENERIC:
//			MessageBox(NULL,"DSERR_GENERIC","CSoundMgr",MB_OK);
			MonoPrint("DSERR_GENERIC");
			break;
// The caller does not have the priority level required for the function to
// succeed.
		case DSERR_PRIOLEVELNEEDED:
			MonoPrint("DSERR_PRIOLEVELNEEDED");
//			MessageBox(NULL,"DSERR_PRIOLEVELNEEDED","CSoundMgr",MB_OK);
			break;
// Not enough free memory is available to complete the operation
		case DSERR_OUTOFMEMORY:
//			MessageBox(NULL,"DSERR_OUTOFMEMORY","CSoundMgr",MB_OK);
			MonoPrint("DSERR_OUTOFMEMORY");
			break;
// The specified WAVE format is not supported
		case DSERR_BADFORMAT:
//			MessageBox(NULL,"DSERR_BADFORMAT","CSoundMgr",MB_OK);
			MonoPrint("DSERR_BADFORMAT");
			break;
// The function called is not supported at this time
		case DSERR_UNSUPPORTED:
//			MessageBox(NULL,"DSERR_UNSUPPORTED","CSoundMgr",MB_OK);
			MonoPrint("DSERR_UNSUPPORTED");
			break;
// No sound driver is available for use
		case DSERR_NODRIVER:
//			MessageBox(NULL,"DSERR_NODRIVER","CSoundMgr",MB_OK);
			MonoPrint("DSERR_NODRIVER");
			break;
// This object is already initialized
		case DSERR_ALREADYINITIALIZED:
//			MessageBox(NULL,"DSERR_ALREADYINITIALIZED","CSoundMgr",MB_OK);
			MonoPrint("DSERR_ALREADYINITIALIZED");
			break;
// This object does not support aggregation
		case DSERR_NOAGGREGATION:
//			MessageBox(NULL,"DSERR_NOAGGREGATION","CSoundMgr",MB_OK);
			MonoPrint("DSERR_NOAGGREGATION");
			break;
// The buffer memory has been lost, and must be restored.
		case DSERR_BUFFERLOST:
//			MessageBox(NULL,"DSERR_BUFFERLOST","CSoundMgr",MB_OK);
			MonoPrint("DSERR_BUFFERLOST");
			break;
// Another app has a higher priority level, preventing this call from
// succeeding.
		case DSERR_OTHERAPPHASPRIO:
			MessageBox(NULL,"DSERR_OTHERAPPHASPRIO","CSoundMgr",MB_OK);
			MonoPrint("DSERR_OTHERAPPHASPRIO");
			break;
// This object has not been initialized
		case DSERR_UNINITIALIZED:
//			MessageBox(NULL,"DSERR_UNINITIALIZED","CSoundMgr",MB_OK);
			MonoPrint("DSERR_UNINITIALIZED");
			break;

// The requested COM interface is not available
		case DSERR_NOINTERFACE:
			MessageBox(NULL,"DSERR_NOINTERFACE","CSoundMgr",MB_OK);
			MonoPrint("DSERR_NOINTERFACE");
			break;
	}
#endif
}

// JPO SoundStream support routines
// we could do a lot more encapsulation here
SoundStream::SoundStream()
{
    DSoundBuffer = NULL;
    notif = NULL;
    lpDsNotify = NULL;
    ImaInfo = NULL;
}

SoundStream::~SoundStream()
{
    if (lpDsNotify)
	lpDsNotify->Release();
    if (DSoundBuffer)
	DSoundBuffer->Release();
    if (notif != NULL)
	CloseHandle(notif);
    if (ImaInfo) {
	if (ImaInfo->srcbuffer)
	    delete ImaInfo->srcbuffer;
	delete ImaInfo;
    }
}

SoundList::SoundList()
{
    	ID = 0;
	Volume = 0;
	Frequency = 0;
	Direction = 0;
	Flags = 0;
	DS3DBufferCount=DS3DBUFFERMAX;
	Cur3dBuffer=0;
	//DSoundBuffer = NULL;
	int i;
	for(i=0;i<DS3DBufferCount;i++)
	{
		Buf[i].uid=0;
		Buf[i].distsq=-1; // set so that the sound doesn't get played errantly when you enter the 3d world.
		Buf[i].DSoundBuffer   = NULL;
		Buf[i].DSound3dBuffer = NULL;
	}
	is3d = FALSE;
	Next = NULL;

}

// MLR: doesn't copy 3d sound buffer!
SoundList::SoundList(SoundList *copy, IDirectSound *DSound)
{
    ShiAssert(FALSE == F4IsBadReadPtr(DSound, sizeof *DSound));
    //ShiAssert(FALSE == F4IsBadReadPtr(copy->DSoundBuffer, sizeof *copy->DSoundBuffer));
    ID = copy->ID;
    Volume = copy->Volume;
    Frequency = copy->Frequency;
    Direction = copy->Direction;
	DS3DBufferCount=copy->DS3DBufferCount;
	Cur3dBuffer=0;
    Flags = copy->Flags;
    is3d = FALSE;
	for(int i=0;i<DS3DBufferCount;i++)
	{
		Buf[i]=copy->Buf[i];
		HRESULT hr = DSound->DuplicateSoundBuffer(copy->Buf[0].DSoundBuffer, &Buf[i].DSoundBuffer);
		Buf[i].DSound3dBuffer=0;
		
		if (hr != S_OK)
		CSoundMgr::DSoundCheck(hr);
	}
    Next = NULL;
}

SoundList::~SoundList()
{
    //if (DSoundBuffer)
	//DSoundBuffer->Release();
	int i;

	for(i=0;i<DS3DBufferCount;i++)
	{
		if (Buf[i].DSoundBuffer)
		Buf[i].DSoundBuffer->Release();
		
		if (Buf[i].DSound3dBuffer)
		Buf[i].DSound3dBuffer->Release();
	}
}

// ---------- All below: MLR -----------

FILE *sofp=0;
int sofp_ref=0;

LPDIRECTSOUNDBUFFER CSoundMgr::LoadWaveFile(char *Filename, SFX_DEF_ENTRY *sfx)
{
	char *mem;
	RIFF_FILE *newsnd;
	long NewID = SND_NO_HANDLE;
	DSBUFFERDESC dsbdesc;
	LPDIRECTSOUNDBUFFER lpNewDSBuf;
	DWORD Len;
	HRESULT hr;

	if(gSoundDriver)
	{
		newsnd=LoadRiff(Filename);
		if(newsnd)
		{
			if(!newsnd->Format)
			{
				if(newsnd->data)
					delete newsnd->data;
				delete newsnd;
				return(0);
			}
			if(newsnd->Format->wFormatTag == WAVE_FORMAT_PCM)
			{
				memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); // Zero it out.
				dsbdesc.dwSize = sizeof(DSBUFFERDESC);
				
				dsbdesc.dwFlags = DSBCAPS_CTRLVOLUME | 
									DSBCAPS_GETCURRENTPOSITION2;
										
				//if (g_bOldSoundAlg == false)
				// 	dsbdesc.dwFlags |= DSBCAPS_CTRLPOSITIONNOTIFY;
				
				if(g_bUse3dSound && (sfx->flags & SFX_FLAGS_3D)) 
				{
					dsbdesc.dwFlags |= DSBCAPS_CTRL3D | DSBCAPS_MUTE3DATMAXDISTANCE;
				}
				else
				{
					if (sfx->flags & SFX_FLAGS_PAN) 
						dsbdesc.dwFlags |= DSBCAPS_CTRLPAN;
				}
				
				if (sfx->flags & SFX_FLAGS_FREQ) 
					dsbdesc.dwFlags |= DSBCAPS_CTRLFREQUENCY;
				
				if ((sfx->flags & SFX_FLAGS_HIGH) == 0) // low priority sound
					dsbdesc.dwFlags |= DSBCAPS_LOCDEFER;
					
				dsbdesc.dwBufferBytes = newsnd->SampleLen;
				dsbdesc.lpwfxFormat = newsnd->Format;

			// Create buffer.
				hr = DSound->CreateSoundBuffer(&dsbdesc, &lpNewDSBuf, NULL);
				if(hr == DS_OK)
				{
					lpNewDSBuf->Lock(0,newsnd->SampleLen,(void**)&mem,&Len,NULL,NULL,NULL);
					memcpy(mem,newsnd->Start,Len);
					lpNewDSBuf->Unlock(mem,Len,NULL,NULL);
				}
				else
					DSoundCheck(hr);
				delete newsnd->data;
				delete newsnd;
				return(lpNewDSBuf);
			}
			else
			{
				MonoPrint("Unsupported file format\n");
				if(newsnd->data)
					delete newsnd->data;
				delete newsnd;
			}
		}
	}
	return(0);
}

//#define SOLOG

