#include <windows.h>
#include <tchar.h>
#include "f4vu.h"
#include "ui95/chandler.h" //for some reason my compiler insists upon this. DSP
#include "logbook.h"
#include "userids.h"
#include "remotelb.h"

void RemoteLBCleanupCB(void *rec)
{
    RemoteLB *lb;

    lb = (RemoteLB*)rec;
    lb->Cleanup();
    delete lb;
}

RemoteLB::RemoteLB()
{
    flags_ = 0;
    memset(&Pilot_, 0, sizeof(Pilot_));
    Photo_ = NULL;
    Patch_ = NULL;
}

RemoteLB::~RemoteLB()
{
}

void RemoteLB::Cleanup()
{
    if (Photo_ and (flags_ bitand PHOTO_CLEANUP))
        delete Photo_;

    Photo_ = NULL;

    if (Patch_ and (flags_ bitand PATCH_CLEANUP))
        delete Patch_;

    Patch_ = NULL;
    flags_ and_eq compl (PHOTO_CLEANUP bitor PATCH_CLEANUP bitor PHOTO_READY bitor PATCH_READY);
}

void RemoteLB::SetPilotData(LB_PILOT *data)
{
    memcpy(&Pilot_, data, sizeof(Pilot_));
    flags_ or_eq PILOT_READY;
}

RemoteImage *RemoteLB::Receive(RemoteImage *Image, short packetno, short length, long offset, long size, uchar *data)
{
    RemoteImage *remotedata;
    short i;

    remotedata = Image;

    if ( not remotedata)
    {
        remotedata = new RemoteImage;

        if ( not remotedata)
            return(NULL);

        remotedata->flags = 0;
        remotedata->Size = size;
        remotedata->numblocks = static_cast<short>((size / length) + 1);
        remotedata->blockflag = new uchar[remotedata->numblocks];
        memset(remotedata->blockflag, 0, remotedata->numblocks);
        remotedata->ImageData = new uchar[size];
    }

    if (offset < size)
    {
        memcpy(remotedata->ImageData + offset, data, min(length, size - offset));
        remotedata->blockflag[packetno] = 1;
        i = 0;

        while (i < remotedata->numblocks)
        {
            if ( not remotedata->blockflag[i])
                i = static_cast<short>(remotedata->numblocks + 1);
            else
                i++;
        }

        if (i == remotedata->numblocks)
            remotedata->flags or_eq IMAGE_READY;
    }

    return(remotedata);
}

void RemoteLB::ReceiveImage(uchar ID, short packetno, short length, long offset, long size, uchar *data)
{
    switch (ID)
    {
        case PILOT_IMAGE:
            Photo_ = Receive(Photo_, packetno, length, offset, size, data);

            if (Photo_)
            {
                if (Photo_->flags bitand IMAGE_READY)
                    flags_ or_eq PHOTO_READY;

                flags_ or_eq PHOTO_CLEANUP;
            }

            break;

        case PATCH_IMAGE:
            Patch_ = Receive(Patch_, packetno, length, offset, size, data);

            if (Patch_)
            {
                if (Patch_->flags bitand IMAGE_READY)
                    flags_ or_eq PATCH_READY;

                flags_ or_eq PATCH_CLEANUP;
            }

            break;
    }
}


