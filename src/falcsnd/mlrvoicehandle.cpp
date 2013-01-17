#if 1
#include <windows.h>
#include "fsound.h"
#include "falclib.h"
#include "alist.h"
#include "mlrVoice.h"

extern unsigned long    vuxRealTime;


/* 

  ownership heirarchy.

VuEntity
   |
   +-F4SoundPos - VU object interface
        |                
		+-mlrVoiceHandle - handle to mlrVoice :)
		    +-> mlrVoice - Basic voice processing (doppler, volume)
			      +-> mlrChannel - interface to API(DSound)? 
		+-mlrVoiceHandle -> mlrVoice

mlrVoice are kept in 2 lists own by the gVoiceManager.


  */

/*

AList SoundQueue;
AList OutofQueue;

*/


mlrVoiceHandle::mlrVoiceHandle(F4SoundPos *Owner, int SfxID, int UserID)
{
	memset(this,0,sizeof(mlrVoiceHandle));
	SPos=Owner;
	sfxid  = SfxID;
	userid = UserID;
	sfx=&SFX_DEF[ sfxid ];

	voice = new mlrVoice(this);
}

mlrVoiceHandle::~mlrVoiceHandle(){
	if(voice){
		delete voice;
	}
}

void mlrVoiceHandle::Stop(){
	if(voice){
		voice->Stop();
	}
}

bool mlrVoiceHandle::IsPlaying(){
	if(voice){
		return (voice->IsPlaying());
	}
	return(0);
}

extern bool g_bNoSound, gSoundManagerRunning;
extern Tpoint CamPos;

void mlrVoiceHandle::Play(float pscale, float volume, float x, float y, float z, float vx, float vy, float vz)
{
	if(voice)
	{
		lastPlayTime = vuxRealTime;
		voice->Play(pscale, volume, x, y, z, vx, vy, vz);
	}

	if(sfx->LinkedSoundID && sfx->LinkedSoundID!=sfxid) 
	{	// there's an another sound associated with this sound	
		SPos->Sfx(sfx->LinkedSoundID, userid, pscale, volume, x, y, z);
	}
}

bool mlrVoiceHandle::AreYou(int SfxId, int UserID) 
{ 
	return(SfxId == sfxid && UserID == userid);
}

#endif

