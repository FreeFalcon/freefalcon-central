#include <windows.h>
#include <tchar.h>
#include "f4vu.h"
#include "ui95\chandler.h" //for some reason my compiler insists upon this. DSP
#include "logbook.h"
#include "userids.h"
#include "remotelb.h"

void RemoteLBCleanupCB(void *rec)
{
	RemoteLB *lb;

	lb=(RemoteLB*)rec;
	lb->Cleanup();
	delete lb;
}

RemoteLB::RemoteLB()
{
	flags_=0;
	memset(&Pilot_,0,sizeof(Pilot_));
	Photo_=NULL;
	Patch_=NULL;
}

RemoteLB::~RemoteLB()
{
}

void RemoteLB::Cleanup()
{
	if(Photo_ && (flags_ & PHOTO_CLEANUP))
		delete Photo_;
	Photo_=NULL;
	if(Patch_ && (flags_ & PATCH_CLEANUP))
		delete Patch_;
	Patch_=NULL;
	flags_ &= ~(PHOTO_CLEANUP|PATCH_CLEANUP|PHOTO_READY|PATCH_READY);
}

void RemoteLB::SetPilotData(LB_PILOT *data)
{
	memcpy(&Pilot_,data,sizeof(Pilot_));
	flags_ |= PILOT_READY;
}

RemoteImage *RemoteLB::Receive(RemoteImage *Image,short packetno,short length,long offset,long size,uchar *data)
{
	RemoteImage *remotedata;
	short i;

	remotedata=Image;
	if(!remotedata)
	{
		remotedata=new RemoteImage;
		if(!remotedata)
			return(NULL);
		remotedata->flags=0;
		remotedata->Size=size;
		remotedata->numblocks=static_cast<short>((size/length)+1);
		remotedata->blockflag=new uchar[remotedata->numblocks];
		memset(remotedata->blockflag,0,remotedata->numblocks);
		remotedata->ImageData=new uchar[size];
	}
	if(offset < size)
	{
		memcpy(remotedata->ImageData+offset,data,min(length,size-offset));
		remotedata->blockflag[packetno]=1;
		i=0;
		while(i < remotedata->numblocks)
		{
			if(!remotedata->blockflag[i])
				i=static_cast<short>(remotedata->numblocks+1);
			else
				i++;
		}
		if(i == remotedata->numblocks)
			remotedata->flags |= IMAGE_READY;
	}
	return(remotedata);
}

void RemoteLB::ReceiveImage(uchar ID,short packetno,short length,long offset,long size,uchar *data)
{
	switch(ID)
	{
		case PILOT_IMAGE:
			Photo_=Receive(Photo_,packetno,length,offset,size,data);
			if(Photo_)
			{
				if(Photo_->flags & IMAGE_READY)
					flags_ |= PHOTO_READY;
				flags_ |= PHOTO_CLEANUP;
			}
			break;
		case PATCH_IMAGE:
			Patch_=Receive(Patch_,packetno,length,offset,size,data);
			if(Patch_)
			{
				if(Patch_->flags & IMAGE_READY)
					flags_ |= PATCH_READY;
				flags_ |= PATCH_CLEANUP;
			}
			break;
	}
}


