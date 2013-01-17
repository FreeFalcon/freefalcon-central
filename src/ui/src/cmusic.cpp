#include <windows.h>
#include "fsound.h"
#include <mmreg.h>
#include "chandler.h"
#include "cmusic.h"
#include "dsound.h"

C_Music *gMusic=NULL;

static BOOL PlayingInteractive=FALSE;
static BOOL FadeOutInteractive=FALSE;
long MusicStopped=0;

static WAVEFORMATEX stereo=
{
	WAVE_FORMAT_PCM,
	2,
	22050,
	88200,
	4,
	16,
	0,
};

void gMusicCallback(SOUNDSTREAM *Stream,int MessageID)
{
	if(!Stream || !gMusic)
		return;

	switch(MessageID)
	{
		case SND_MSG_FADE_IN_DONE:
			break;
		case SND_MSG_START_FADE:
			gMusic->ToggleStream();
			MusicStopped=GetCurrentTime();
			break;
		case SND_MSG_FADE_OUT_DONE:
			Stream->Status |= SND_STREAM_FADEDOUT;
			if(gMusic->GetFlags() == C_Music::MUSIC_PAUSE_FADE)
			{
				Stream->DSoundBuffer->Stop();
			}
			else if(gMusic->GetFlags() == C_Music::MUSIC_STOP)
			{
				gSoundDriver->StopStream(Stream->ID);
				if(gMusic->Queued())
					gMusic->PlayQ();
			}
			else
				gSoundDriver->StopStream(Stream->ID);
			break;
		case SND_MSG_STREAM_EOF:
			if(PlayingInteractive)
				gMusic->PlayNextInteractive();
			if(gMusic->Queued())
				gMusic->QNext(Stream);

			if(FadeOutInteractive)
			{
				Stream->Status |= SND_STREAM_FADE_OUT;
				Stream->FadeOut=Stream->FadeIn;
				FadeOutInteractive=FALSE;
				gMusic->ToggleStream();
				MusicStopped=GetCurrentTime();
			}
			break;
		case SND_MSG_STREAM_DONE:
			if(gMusic->Queued())
				gMusic->PlayQ();
			break;
	}
}

C_Music::C_Music()
{
	int i,j;

	RepeatCount_=0;
	GroupRepeat_=0;
	Section_=MUSIC_SLOW;
	Group_=0;
	CurPiece_=0;
	SectionCount_=0;
	GroupCount_=0;
	StreamUsed_=0;
	Music_=NULL;

	for(i=0;i<NUM_SECTIONS;i++)
		for(j=0;j<NUM_GROUPS;j++)
			Count_[i][j]=0;

	Sound_=NULL;
	Volume_=0;
	StreamID_[0]=SND_NO_HANDLE;
	StreamID_[1]=SND_NO_HANDLE;
	MusicFlags_=MUSIC_NOTHING;
	memset(Queue_,SND_NO_HANDLE,sizeof(long)*_MUSIC_QUEUE_SIZE_);
}

C_Music::~C_Music()
{
	if(StreamID_[0] || StreamID_[1] || Music_)
		Cleanup();
}

void C_Music::Setup(CSoundMgr *mngr)
{
	Sound_=mngr;
	if(!Music_)
	{
		Music_=new C_Hash;
		Music_->Setup(10);
	}
}

void C_Music::Cleanup()
{
	int i,j;
	if(Sound_ && StreamID_[0] != SND_NO_HANDLE || StreamID_[1] != SND_NO_HANDLE)
	{
		RemoveStream();
		StreamID_[0]=SND_NO_HANDLE;
		StreamID_[1]=SND_NO_HANDLE;
	}

	Section_=MUSIC_SLOW;
	Group_=0;
	CurPiece_=0;
	SectionCount_=0;
	GroupCount_=0;
	for(i=0;i<NUM_SECTIONS;i++)
		for(j=0;j<NUM_GROUPS;j++)
			Count_[i][j]=0;

	if(Music_)
	{
		Music_->Cleanup();
		delete Music_;
		Music_=NULL;
	}

}

void C_Music::CreateStream()
{
	if(!Sound_)
		return;

	if(StreamID_[0] == SND_NO_HANDLE)
		StreamID_[0]=Sound_->CreateStream(&stereo,2);
	if(StreamID_[1] == SND_NO_HANDLE)
		StreamID_[1]=Sound_->CreateStream(&stereo,2);
	Sound_->SetStreamVolume(StreamID_[0],Volume_);
	Sound_->SetStreamVolume(StreamID_[1],Volume_);
}

void C_Music::RemoveStream()
{
	if(Sound_)
	{
		if(StreamID_[0] != SND_NO_HANDLE)
			Sound_->RemoveStream(StreamID_[0]);
		if(StreamID_[1] != SND_NO_HANDLE)
			Sound_->RemoveStream(StreamID_[1]);
		StreamID_[0]=SND_NO_HANDLE;
		StreamID_[1]=SND_NO_HANDLE;
	}
}

void C_Music::Play(SOUND_RES *snd)
{
	long SND_FLAGS;
	char fname[MAX_PATH];

	if(Sound_)
	{
		if(StreamID_[0] == SND_NO_HANDLE || StreamID_[1] == SND_NO_HANDLE)
		{
			CreateStream();
			SetMessageCB(gMusicCallback);
		}
		SND_FLAGS=0;
		if(snd->flags & SOUND_LOOP)
			SND_FLAGS |= SND_STREAM_LOOP;
		if(snd->flags & SOUND_FADE_IN)
			SND_FLAGS |= SND_STREAM_FADE_IN;
		if(snd->flags & SOUND_FADE_OUT)
			SND_FLAGS |= SND_STREAM_FADE_OUT;
		if(snd->flags & SOUND_RES_STREAM)
		{
			strcpy(fname,snd->Sound->Owner->ResName());
			strcat(fname,".rsc");
			Sound_->StartFileStream(StreamID_[StreamUsed_],fname,SND_FLAGS,snd->Sound->Header->offset);
		}
		else
			Sound_->StartFileStream(StreamID_[StreamUsed_],snd->filename,SND_FLAGS);
	}
}

void C_Music::Stop()
{
	if(Sound_)
		Sound_->StopStream(StreamID_[StreamUsed_]);
	MusicFlags_=0;
}

void C_Music::FadeOut_Stop()
{
	if(Sound_)
	{
		MusicFlags_ = MUSIC_STOP;
		Sound_->StopStreamWithFade(StreamID_[StreamUsed_]);
	}
}

void C_Music::Pause()
{
	if(Sound_)
	{
		MusicFlags_=MUSIC_PAUSE;
		Sound_->PauseStream(StreamID_[StreamUsed_]);
	}
}

void C_Music::FadeOut_Pause()
{
	if(Sound_)
	{
		MusicFlags_=MUSIC_PAUSE_FADE;
		Sound_->FadeOutStream(StreamID_[StreamUsed_]);
	}
}

void C_Music::Resume()
{
	if(Sound_)
	{
		if(MusicFlags_ == MUSIC_PAUSE_FADE)
			Sound_->ResumeStreamFadeIn(StreamID_[StreamUsed_]);
		else if(MusicFlags_ == MUSIC_PAUSE)
			Sound_->ResumeStream(StreamID_[StreamUsed_]);
		MusicFlags_=MUSIC_NOTHING;
	}
}

void C_Music::SetLoopCount(int count)
{
	if(Sound_)
		Sound_->SetLoopCounter(StreamID_[StreamUsed_],count);
}

void C_Music::SetLoopPosition(long pos)
{
	if(Sound_)
		Sound_->SetLoopOffset(StreamID_[StreamUsed_],pos);
}

void C_Music::SetMessageCB(void (*cb)(SOUNDSTREAM *,int))
{
	if(Sound_)
	{
		Sound_->SetMessageCallback(StreamID_[0],cb);
		Sound_->SetMessageCallback(StreamID_[1],cb);
	}
}

void C_Music::SetVolume(long volume)
{
	Volume_=volume;
	if(Sound_)
	{
		if(StreamID_[0] != SND_NO_HANDLE)
			Sound_->SetStreamVolume(StreamID_[0],volume);
		if(StreamID_[1] != SND_NO_HANDLE)
			Sound_->SetStreamVolume(StreamID_[1],volume);
	}
}

void C_Music::AddQ(long ID)
{
	int i;

	i=0;
	while(Queue_[i] != SND_NO_HANDLE && i < _MUSIC_QUEUE_SIZE_)
		i++;

	if(i == _MUSIC_QUEUE_SIZE_)
		return;

	Queue_[i]=ID;
	PlayingInteractive=FALSE;
}

void C_Music::ClearQ()
{
	memset(Queue_,SND_NO_HANDLE,sizeof(long)*_MUSIC_QUEUE_SIZE_);
	PlayingInteractive=FALSE;
}

void C_Music::PlayQ()
{
	SOUND_RES *snd;
	int i;

	if(!Sound_)
		return;

	if(Sound_->IsStreamPlaying(StreamID_[StreamUsed_]))
		return;

	do
	{
		snd=gSoundMgr->GetSound(Queue_[0]);
		if(snd)
		{
			if(StreamID_[StreamUsed_] == SND_NO_HANDLE)
			{
				CreateStream();
				SetMessageCB(gMusicCallback);
			}
			SetLoopPosition(snd->LoopPoint);
			SetLoopCount(snd->Count);
			Play(snd);
			MusicStopped=0;
		}
		for(i=1;i<_MUSIC_QUEUE_SIZE_;i++)
			Queue_[i-1]=Queue_[i];
		Queue_[_MUSIC_QUEUE_SIZE_-1]=SND_NO_HANDLE;
	} while(!snd && Queue_[0] != SND_NO_HANDLE);
}

// HELLA HUGE KLUDGE to String 2 or more WAVE files together
void C_Music::QNext(SOUNDSTREAM *Stream)
{
	SOUND_RES *snd;
	WAVEFORMATEX Header;
	long i,size,NumSamples;
	char fname[MAX_PATH];

	if(!Sound_)
		return;

	do
	{
		snd=gSoundMgr->GetSound(Queue_[0]);
		if(snd)
		{
			if(Stream->fp != INVALID_HANDLE_VALUE)
			{
				CloseHandle(Stream->fp);
				Stream->fp=INVALID_HANDLE_VALUE;
			}
			if(snd->flags & SOUND_IN_RES)
			{
				strcpy(fname,snd->Sound->Owner->ResName());
				strcat(fname,".rsc");
				Stream->fp=CreateFile(fname,GENERIC_READ,FILE_SHARE_READ,NULL,
									  OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL);
			}
			else
			{
				Stream->fp=CreateFile(snd->filename,GENERIC_READ,FILE_SHARE_READ,NULL,
									  OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL);
			}
			if(Stream->fp != INVALID_HANDLE_VALUE)
			{
				if(snd->Sound && snd->flags & SOUND_RES_STREAM)
				{
					SetFilePointer(Stream->fp,snd->Sound->Header->offset,NULL,FILE_BEGIN);
				}
				size=Sound_->LoadRiffFormat(Stream->fp,&Header,&Stream->HeaderOffset,&NumSamples);
				if(snd->Sound && snd->flags & SOUND_RES_STREAM)
				{
					Stream->HeaderOffset += snd->Sound->Header->offset;
				}
				if(Header.wFormatTag == WAVE_FORMAT_IMA_ADPCM)
				{
					if(!Stream->ImaInfo)
					{
						Stream->Status |= SND_IS_IMAADPCM;
						Stream->ImaInfo=new IMA_STREAM;
						memset(Stream->ImaInfo,0,sizeof(IMA_STREAM));
						Stream->ImaInfo->type=Header.nChannels;
						Stream->ImaInfo->srcsize=1024 * 40;
						if(Stream->ImaInfo->srcsize > size)
							Stream->ImaInfo->srcsize = size;
						Stream->ImaInfo->src=new char[Stream->ImaInfo->srcsize];
						Stream->ImaInfo->sreadidx=-1; // When ReadStream gets called... read entire buffer size (if -1)
						Stream->ImaInfo->slen=size;
						Stream->ImaInfo->dlen=NumSamples; // (2 bytes) since we only handle 16bit
					}
					else
					{
						Stream->ImaInfo->sreadidx=-1; // When ReadStream gets called... read entire buffer size (if -1)
						Stream->ImaInfo->slen=size;
						Stream->ImaInfo->sidx=0;
						Stream->ImaInfo->didx=0;
						Stream->ImaInfo->dlen=NumSamples; // (2 bytes) since we only handle 16bit
						Stream->ImaInfo->count=0;
						Stream->ImaInfo->blockLength=0;
					}
				}
				else
				{
					Stream->Status &= ~SND_IS_IMAADPCM;
					if(Stream->ImaInfo)
					{
						if(Stream->ImaInfo->src)
							delete Stream->ImaInfo->src;
						delete Stream->ImaInfo;
						Stream->ImaInfo=NULL;
					}
				}
				Stream->Status |= SND_STREAM_CONTINUE | SND_STREAM_LOOP;
				if(!(snd->flags & SOUND_LOOP))
					Stream->Status ^= SND_STREAM_LOOP;
				Stream->LoopOffset=snd->LoopPoint;
				Stream->LoopCount=snd->Count;
				if(snd->flags & SOUND_FADE_OUT)
				{
					Stream->FadeOut=DSBVOLUME_MIN;
					MusicFlags_=MUSIC_STOP;
				}
				else
					Stream->FadeOut=SND_MAX_VOLUME;
			}
			else
				snd=NULL;
		}
		for(i=1;i<_MUSIC_QUEUE_SIZE_;i++)
			Queue_[i-1]=Queue_[i];
		Queue_[_MUSIC_QUEUE_SIZE_-1]=SND_NO_HANDLE;
	} while(!snd && Queue_[0] != SND_NO_HANDLE);
}

// Interactive stuff
void C_Music::AddInteractiveMusic(long Section,long Group,long MusicID)
{
	long ID;

	if(!Music_)
		return;

	ID=Count_[Section][Group] | (Section << 16) | (Group << 8);

	if(Music_->Find(ID))
		return;

	Music_->Add(ID,(void*)MusicID);

	Count_[Section][Group]++;
}

void C_Music::StartInteractive(long Section,long Group)
{
	long ID,MusicID;

	RepeatCount_=1;

	ID=(rand() % Count_[Section][Group]) | (Section << 16) | (Group << 8);

	MusicID=(long)Music_->Find(ID);
	if(MusicID)
	{
		Section_=Section;
		Group_=Group;
		GroupRepeat_= static_cast<short>(2 + rand() % 4);
		CurPiece_=ID & 0xff;
		SectionCount_=0;
		GroupCount_=1;
		// Start it playing
		AddQ(MusicID);
		PlayQ();
		PlayingInteractive=TRUE;
	}
}

void C_Music::PlayNextInteractive()
{
	long ID,MusicID,newvalue;

	if(RepeatCount_ > 48)
		return;

	// this should be called by callback ONLY
	GroupCount_++;
	RepeatCount_++;
	if(GroupCount_ > GroupRepeat_)
	{
		GroupRepeat_= static_cast<short>(1 + rand() % 6);
		GroupCount_=1;
		newvalue=rand() % 5;
		if(newvalue == Group_)
			newvalue=(Group_+1)%5;
		Group_=newvalue;

		SectionCount_++;
		if(SectionCount_ > PLAY_IN_SECTION)
		{
			SectionCount_=0;
			Section_++;
			if(Section_ > MUSIC_FAST)
				Section_=MUSIC_SLOW;
		}
	}
	ID=(rand() % Count_[Section_][Group_]);
	if(ID == CurPiece_)
		ID=(CurPiece_+1)%Count_[Section_][Group_];
		
	ID |= (Section_ << 16) | (Group_ << 8);

	MusicID=(long)Music_->Find(ID);
	if(MusicID)
	{
		CurPiece_=ID & 0xff;
		// Start it playing
		AddQ(MusicID);
		if(RepeatCount_ > 48)
			FadeOutInteractive=TRUE;
			
		PlayingInteractive=TRUE;
	}
}
