#ifndef _C_MUSIC_MANAGER_H_
#define _C_MUSIC_MANAGER_H_

#ifndef _SOUND_LIB_
#include "falcsnd\psound.h"
#endif

#ifndef gSoundDriver
extern CSoundMgr *gSoundDriver;
#endif

#define _MUSIC_QUEUE_SIZE_ (10)

class C_Music
{
	public:
		enum
		{
			MUSIC_NOTHING=0,
			MUSIC_PAUSE,
			MUSIC_PAUSE_FADE,
			MUSIC_STOP,

			// Interactive stuff
			MUSIC_SLOW=0,
			MUSIC_FAST,
			NUM_SECTIONS,

			NUM_GROUPS=10,

			PLAY_IN_GROUP=4,
			PLAY_IN_SECTION=5,
		};

	protected:
		// Stuff for interactive music
		short		StreamUsed_;
		short		GroupRepeat_;
		short		RepeatCount_;
		long		Section_;
		long		Group_;
		long		CurPiece_;
		long		SectionCount_;
		long		GroupCount_;
		long		Count_[NUM_SECTIONS][NUM_GROUPS];
		C_Hash		*Music_;
		// End of stuff for interactive music
		
		long Volume_;
		int  StreamID_[2];
		long MusicFlags_;
		long Queue_[_MUSIC_QUEUE_SIZE_];
		CSoundMgr   *Sound_;

		SOUND_RES *Current_;

	public:
		C_Music();
		~C_Music();

		void Setup(CSoundMgr *mngr);
		void Cleanup();
		long GetFlags() { return(MusicFlags_); }
		BOOL Queued() { if(Queue_[0] != SND_NO_HANDLE) return(TRUE); return(FALSE); }
		void CreateStream();
		void RemoveStream();
		void Play(SOUND_RES *snd);
		void Stop();
		void FadeOut_Stop();
		void Pause();
		void FadeOut_Pause();
		void Resume();
		void SetVolume(long Volume);
		void SetLoopCount(int count);
		void SetLoopPosition(long pos);
		void SetMessageCB(void (*cb)(SOUNDSTREAM *,int));
		void SetFadeOut(long vol);
		void AddQ(long ID);
		void ClearQ();
		void PlayQ();
		void QNext(SOUNDSTREAM *Stream); // NEVER CALL THIS FUNCTION

		SOUND_RES *GetCurrent() { return(Current_); }

		void ToggleStream() { StreamUsed_=(short)(1-StreamUsed_); }

		// Interactive Music stuff
		void AddInteractiveMusic(long Section,long Group,long MusicID);
		void StartInteractive(long Section,long Group);
		void PlayNextInteractive(); // Called by Callback
};

extern C_Music *gMusic;

#endif