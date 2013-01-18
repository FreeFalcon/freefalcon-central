#include <windows.h>
#include "chandler.h"
#include "FalcSnd/psound.h"

void *FLAT_RSC::GetData()
{
    if (!Owner || !Header)
        return(NULL);

    return(Owner->GetData() + Header->offset);
    return(NULL);
}

BOOL SOUND_RSC::Play(int StreamID)
{
    long SndFlags;
    char *snddata;
    RIFF_FILE RiffHeader;

    if (!Owner)
        return(FALSE);

    if (!Owner->GetData())
        return(FALSE);

    SndFlags = 0;

    if (Header->flags & SOUND_LOOP)
        SndFlags |= SND_STREAM_LOOP;

    snddata = Owner->GetData() + Header->offset;

    if (gSoundDriver->FillRiffInfo(snddata, &RiffHeader))
        return(gSoundDriver->StartMemoryStream(StreamID, &RiffHeader, SndFlags));

    return(FALSE);
}

BOOL SOUND_RSC::Loop(int StreamID)
{
    long SndFlags;
    char *snddata;
    RIFF_FILE RiffHeader;

    if (!Owner)
        return(FALSE);

    if (!Owner->GetData())
        return(FALSE);

    SndFlags = SND_STREAM_LOOP;

    snddata = Owner->GetData() + Header->offset;

    if (gSoundDriver->FillRiffInfo(snddata, &RiffHeader))
        return(gSoundDriver->StartMemoryStream(StreamID, &RiffHeader, SndFlags));

    return(FALSE);
}

BOOL SOUND_RSC::Stream(int StreamID)
{
    long SndFlags;
    char fname[MAX_PATH];

    if (!Owner)
        return(FALSE);

    SndFlags = 0;

    if (Header->flags & SOUND_LOOP)
        SndFlags |= SND_STREAM_LOOP;

    strcpy(fname, Owner->ResName());
    strcat(fname, ".rsc");
    return(gSoundDriver->StartFileStream(StreamID, fname, SndFlags, Header->offset));
}

