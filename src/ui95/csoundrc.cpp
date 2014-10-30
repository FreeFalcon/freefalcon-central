#include <windows.h>
#include "falclib/include/fsound.h"
#include "falcsnd/psound.h"
#include "chandler.h"

#ifdef _UI95_PARSER_

enum
{
    CSND_NOTHING = 0,
    CSND_LOADSOUND,
    CSND_STREAMSOUND,
    CSND_SETLOOPBACK,
    CSND_SETLOOPCOUNT,
    CSND_LOADRESOURCE,
    CSND_LOADSTREAMRES,
};

char *C_Snd_Tokens[] =
{
    "[NOTHING]",
    "[LOADSOUND]",
    "[STREAMSOUND]",
    "[LOOPBACK]",
    "[LOOPCOUNT]",
    "[LOADRESOURCE]",
    "[LOADSTREAMRES]",
    0,
};

#endif // PARSER

C_Sound *gSoundMgr = NULL;

static WAVEFORMATEX MonoFormat =
{
    WAVE_FORMAT_PCM,
    1,
    22050,
    44100,
    2,
    16,
    0,
};

static WAVEFORMATEX StereoFormat =
{
    WAVE_FORMAT_PCM,
    2,
    22050,
    88200,
    4,
    16,
    0,
};


static void RemoveResCB(void *rec)
{
    C_Resmgr *res = (C_Resmgr*)rec;

    if (res)
    {
        res->Cleanup();
        delete res;
    }
}

static void CleanupSoundListCB(void *rec)
{
    SOUND_RES *record = (SOUND_RES*)rec;

    if (record)
    {
        record->Cleanup();
        delete record;
    }
}

C_Sound::C_Sound()
{
    SoundList_ = NULL;
    ResList_ = NULL;
    IDTable_ = NULL;
    Mono_ = SND_NO_HANDLE;
    Stereo_ = SND_NO_HANDLE;
}

C_Sound::~C_Sound()
{
    Cleanup();
}

void C_Sound::Setup()
{
    if (SoundList_ or ResList_)
        Cleanup();

    if (gSoundDriver)
    {
        Stereo_ = gSoundDriver->CreateStream(&StereoFormat, 1.0f);
        Mono_ = gSoundDriver->CreateStream(&MonoFormat, 1.0f);
    }
}

void C_Sound::Cleanup()
{
    if (Stereo_ not_eq SND_NO_HANDLE)
    {
        gSoundDriver->RemoveStream(Stereo_);
        Stereo_ = SND_NO_HANDLE;
    }

    if (Mono_ not_eq SND_NO_HANDLE)
    {
        gSoundDriver->RemoveStream(Mono_);
        Mono_ = SND_NO_HANDLE;
    }

    if (SoundList_)
    {
        SoundList_->Cleanup();
        delete SoundList_;
        SoundList_ = NULL;
    }

    if (ResList_)
    {
        ResList_->Cleanup();
        delete ResList_;
        ResList_ = NULL;
    }
}

void C_Sound::AddResSound(C_Resmgr *res)
{
    C_Hash *Index;
    C_HASHNODE *current;
    SOUND_RES *newentry;
    long curidx;
    SOUND_RSC *snd;

    if ( not res)
        return;

    Index = res->GetIDList();

    if ( not Index)
        return;

    if ( not SoundList_)
    {
        SoundList_ = new C_Hash;
        SoundList_->Setup(20);
        SoundList_->SetFlags(C_BIT_REMOVE);
        SoundList_->SetCallback(CleanupSoundListCB);
    }

    snd = (SOUND_RSC*)Index->GetFirst(&current, &curidx);

    while (snd)
    {
        if (snd->Header->Type == _RSC_IS_SOUND_)
        {
            newentry = new SOUND_RES;
            newentry->ID = IDTable_->FindTextID(snd->Header->ID);
            newentry->SoundID = SND_NO_HANDLE;
            newentry->flags = snd->Header->flags bitor SOUND_IN_RES;
            newentry->Volume = 0;
            newentry->LoopPoint = 0;
            newentry->Count = 0;
            newentry->Sound = snd;
            newentry->filename = NULL;

            SoundList_->Add(newentry->ID, newentry);
        }

        snd = (SOUND_RSC*)Index->GetNext(&current, &curidx);
    }
}

void C_Sound::AddResStream(C_Resmgr *res)
{
    C_Hash *Index;
    C_HASHNODE *current;
    SOUND_RES *newentry;
    long curidx;
    SOUND_RSC *snd;

    if ( not res)
        return;

    Index = res->GetIDList();

    if ( not Index)
        return;

    if ( not SoundList_)
    {
        SoundList_ = new C_Hash;
        SoundList_->Setup(20);
        SoundList_->SetFlags(C_BIT_REMOVE);
        SoundList_->SetCallback(CleanupSoundListCB);
    }

    snd = (SOUND_RSC*)Index->GetFirst(&current, &curidx);

    while (snd)
    {
        if (snd->Header->Type == _RSC_IS_SOUND_)
        {
            snd->Header->offset += 4 + sizeof(long);
            newentry = new SOUND_RES;
            newentry->ID = IDTable_->FindTextID(snd->Header->ID);
            newentry->SoundID = SND_NO_HANDLE;
            newentry->flags = snd->Header->flags bitor SOUND_IN_RES bitor SOUND_RES_STREAM;
            newentry->Volume = 0;
            newentry->LoopPoint = 0;
            newentry->Count = 0;
            newentry->Sound = snd;
            newentry->filename = NULL;

            SoundList_->Add(newentry->ID, newentry);
        }

        snd = (SOUND_RSC*)Index->GetNext(&current, &curidx);
    }
}

BOOL C_Sound::LoadResource(long ID, char *filename)
{
    C_Resmgr *res;

    res = new C_Resmgr;
    res->Setup(ID, filename, IDTable_);

    if ( not res->Status())
    {
        res->Cleanup();
        delete res;
        return(FALSE);
    }

    res->LoadData();

    if ( not ResList_)
    {
        ResList_ = new C_Hash;
        ResList_->Setup(1);
        ResList_->SetFlags(C_BIT_REMOVE);
        ResList_->SetCallback(RemoveResCB);
    }

    ResList_->Add(ID, res);
    // make references to all sounds in resource
    AddResSound(res);
    return(TRUE);
}

BOOL C_Sound::LoadStreamResource(long ID, char *filename)
{
    C_Resmgr *res;

    res = new C_Resmgr;
    res->Setup(ID, filename, IDTable_);

    if ( not res->Status())
    {
        res->Cleanup();
        delete res;
        return(FALSE);
    }

    if ( not ResList_)
    {
        ResList_ = new C_Hash;
        ResList_->Setup(1);
        ResList_->SetFlags(C_BIT_REMOVE);
        ResList_->SetCallback(RemoveResCB);
    }

    ResList_->Add(ID, res);
    // make references to all sounds in resource
    AddResStream(res);
    return(TRUE);
}

BOOL C_Sound::LoadSound(long ID, char *file, long flags)
{
    long snd;
    SOUND_RES *newentry;

    if (GetSound(ID))
        return(FALSE);

    snd = F4LoadSound(file, flags/* bitand SND_EXCLUSIVE */); // MLR 12/6/2003 - SND_ flags are obsolete

    if (snd == NULL)
        return(FALSE);

    newentry = new SOUND_RES;
    newentry->ID = ID;
    newentry->SoundID = snd;
    newentry->flags = flags;
    newentry->Volume = 0;
    newentry->LoopPoint = 0;
    newentry->Count = 0;
    newentry->Sound = NULL;
#ifdef USE_SH_POOLS
    newentry->filename = (char*)MemAllocPtr(UI_Pools[UI_SOUND_POOL], sizeof(char) * (strlen(file) + 1), FALSE);
#else
    newentry->filename = new char [strlen(file) + 1];
#endif
    strcpy(newentry->filename, file);

    if ( not SoundList_)
    {
        SoundList_ = new C_Hash;
        SoundList_->Setup(20);
        SoundList_->SetFlags(C_BIT_REMOVE);
        SoundList_->SetCallback(CleanupSoundListCB);
    }

    SoundList_->Add(ID, newentry);
    return(TRUE);
}

BOOL C_Sound::StreamSound(long ID, char *file, long flags)
{
    SOUND_RES *newentry;

    if (GetSound(ID))
        return(FALSE);

    newentry = new SOUND_RES;
    newentry->ID = ID;
    newentry->SoundID = SND_NO_HANDLE;
    newentry->flags = SOUND_STREAM bitor flags;
    newentry->Volume = 0;
    newentry->LoopPoint = 0;
    newentry->Sound = NULL;
#ifdef USE_SH_POOLS
    newentry->filename = (char*)MemAllocPtr(UI_Pools[UI_SOUND_POOL], sizeof(char) * (strlen(file) + 1), FALSE);
#else
    newentry->filename = new char [strlen(file) + 1];
#endif
    strcpy(newentry->filename, file);

    if ( not SoundList_)
    {
        SoundList_ = new C_Hash;
        SoundList_->Setup(20);
        SoundList_->SetFlags(C_BIT_REMOVE);
        SoundList_->SetCallback(CleanupSoundListCB);
    }

    SoundList_->Add(ID, newentry);
    return(TRUE);
}

SOUND_RES *C_Sound::GetSound(long ID)
{
    SOUND_RES *cur;

    if ( not SoundList_)
        return(NULL);

    cur = (SOUND_RES*)SoundList_->Find(ID);
    return(cur);
}

void C_Sound::SetFlags(long ID, long flags)
{
    SOUND_RES *snd;

    snd = GetSound(ID);

    if (snd)
        snd->flags = flags;
}

long C_Sound::GetFlags(long ID)
{
    SOUND_RES *snd;

    snd = GetSound(ID);

    if (snd)
        return(snd->flags);

    return(0);
}

BOOL C_Sound::PlaySound(SOUND_RES *Snd)
{
    long SND_FLAGS;

    if (Snd == NULL)
        return(FALSE);

    gSoundDriver->StopStream(Mono_);
    gSoundDriver->StopStream(Stereo_);

    if (Snd->flags bitand SOUND_IN_RES)
    {
        if (Snd->Sound)
        {
            if (Snd->flags bitand SOUND_RES_STREAM)
            {
                if (Snd->Sound->Header->Channels == 2) // Stereo
                    Snd->Sound->Stream(Stereo_);
                else // Mono
                    Snd->Sound->Stream(Mono_);
            }
            else
            {
                if (Snd->Sound->Header->Channels == 2) // Stereo
                    Snd->Sound->Play(Stereo_);
                else // Mono
                    Snd->Sound->Play(Mono_);
            }
        }
    }
    else if (Snd->flags bitand SOUND_STREAM)
    {
        // Get header... stream in Mono_ or Stereo_ stream
        SND_FLAGS = 0;

        if (Snd->flags bitand SOUND_LOOP)
            SND_FLAGS or_eq SND_STREAM_LOOP;

        Snd->SoundID = F4StartStream(Snd->filename, SND_FLAGS);
        F4SetStreamVolume(Snd->SoundID, Snd->Volume);
    }
    else
    {
        F4PlaySound(Snd->SoundID);
        F4SetVolume(Snd->SoundID, Snd->Volume);
    }

    return(TRUE);
}

BOOL C_Sound::LoopSound(SOUND_RES *Snd)
{
    long SND_FLAGS;

    if (Snd == NULL)
        return(FALSE);

    gSoundDriver->StopStream(Mono_);
    gSoundDriver->StopStream(Stereo_);

    if (Snd->flags bitand SOUND_IN_RES)
    {
        if (Snd->Sound)
        {
            if (Snd->Sound->Header->Channels == 2) // Stereo
                Snd->Sound->Loop(Stereo_);
            else // Mono
                Snd->Sound->Loop(Mono_);
        }
    }
    else if (Snd->flags bitand SOUND_STREAM)
    {
        // Get header... stream in Mono_ or Stereo_ stream
        SND_FLAGS = SND_STREAM_LOOP;
        Snd->SoundID = F4StartStream(Snd->filename, SND_FLAGS);
        F4SetStreamVolume(Snd->SoundID, Snd->Volume);
    }
    else
    {
        F4LoopSound(Snd->SoundID);
        F4SetVolume(Snd->SoundID, Snd->Volume);
    }

    return(TRUE);
}

BOOL C_Sound::StopSound(SOUND_RES *Snd)
{
    if (Snd == NULL)
        return(FALSE);

    gSoundDriver->StopStream(Mono_);
    gSoundDriver->StopStream(Stereo_);
    return(TRUE);
}

BOOL C_Sound::RemoveSound(long ID)
{
    if (SoundList_)
        SoundList_->Remove(ID);

    // F4FreeSound(&cur->SoundID);
    return(FALSE);
}

long C_Sound::SetVolume(SOUND_RES *, long Volume)
{
    gSoundDriver->SetStreamVolume(Stereo_, Volume);
    return(gSoundDriver->SetStreamVolume(Mono_, Volume));
}

long C_Sound::SetVolume(long Volume)
{
    gSoundDriver->SetStreamVolume(Stereo_, Volume);
    return(gSoundDriver->SetStreamVolume(Mono_, Volume));
}

void C_Sound::SetAllVolumes(long Volume)
{
    /* cur=Root_;
     while(cur not_eq NULL)
     {
     cur->Volume=Volume;
     if(cur->flags bitand SOUND_STREAM)
     F4SetStreamVolume(cur->SoundID,Volume);
     else
     F4SetVolume(cur->SoundID,Volume);
     cur=cur->Next;
     }*/
    gSoundDriver->SetStreamVolume(Stereo_, Volume);
    gSoundDriver->SetStreamVolume(Mono_, Volume);
}

#ifdef _UI95_PARSER_
short C_Sound::LocalFind(char *token)
{
    short i = 0;

    while (C_Snd_Tokens[i])
    {
        if (strnicmp(token, C_Snd_Tokens[i], strlen(C_Snd_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Sound::LocalFunction(short ID, long P[], _TCHAR *str, C_Handler *)
{
    SOUND_RES *snd;

    switch (ID)
    {
        case CSND_LOADSOUND:
            LoadSound(P[0], str, P[1] bitor P[2] bitor P[3] bitor P[4] bitor P[5] bitor P[6]);
            break;

        case CSND_STREAMSOUND:
            StreamSound(P[0], str, P[1] bitor P[2] bitor P[3] bitor P[4] bitor P[5] bitor P[6]);
            break;

        case CSND_SETLOOPBACK:
            snd = GetSound(P[0]);

            if (snd)
                snd->LoopPoint = P[1];

            break;

        case CSND_SETLOOPCOUNT:
            snd = GetSound(P[0]);

            if (snd)
                snd->Count = static_cast<short>(P[1]); 

            break;

        case CSND_LOADRESOURCE:
            LoadResource(P[0], str);
            break;

        case CSND_LOADSTREAMRES:
            LoadStreamResource(P[0], str);
            break;
    }
}

#endif // PARSER
