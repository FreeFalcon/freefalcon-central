#ifndef _FSOUND_H
#define _FSOUND_H


#define MLR_NEWSNDCODE // enable code changes throughout source tree



#pragma warning (push)
#pragma warning (disable : 4201)
#include <mmsystem.h>
#pragma warning (pop)

#include "Graphics/include/grtypes.h"
//#include "../vu2/include/vuentity.h"

class VuEntity;


#ifdef __cplusplus
extern "C"
{
#endif



struct Tpoint;
struct Trotation;
struct SfxDef;

// KCK: Used to convert flight and wing callnumbers to the correct CALLNUMBER eval value
#define VF_SHORTCALLSIGN_OFFSET		45
#define	VF_FLIGHTNUMBER_OFFSET		36

int  F4LoadFXSound(char filename[], long Flags, struct SfxDef *sfx);
int  F4LoadSound(char filename[], long Flags);
int  F4LoadRawSound (int flags, char *data, int len);
int  F4IsSoundPlaying (int soundIdx, int UID=0);
int  F4GetVolume (int soundIdx);
void F4FreeSound(int* sound);
void F4PlaySound (int soundIdx);
void F4LoopSound (int soundIdx);
void F4StopSound (int soundIdx);
void ExitSoundManager (void);
void F4SoundStop (void);
void F4SoundStart (void);
void F4PitchBend (int soundIdx, float PitchMultiplier); // new frequency = orignal freq * PitchMult
void F4PanSound (int soundIdx, int Direction); // dBs -10000 to 10000 (0=Center)
long F4SetVolume(int soundIdx,int Volume); // dBs -10000 -> 0
//int F4CreateStream(int Flags); // NOT Necessary (OBSOLETE)
int F4CreateStream(WAVEFORMATEX *fmt,float seconds);
long F4StreamPlayed(int StreamID);
void F4RemoveStream(int StreamID);
int F4StartStream(char *filename,long flags=0);
int F4StartRawStream(int StreamID,char *Data,long size);
BOOL F4StartCallbackStream(int StreamID,void *ptr,DWORD (*cb)(void *,char *,DWORD));
void F4StopStream(int StreamID);
long F4SetStreamVolume(int StreamID,long volume);
void F4HearVoices();
void F4SilenceVoices();
void F4StopAllStreams();
void F4ChatToggleXmitReceive( void );
// MLR 2003-11-05
void F4ReloadSFX(void); // dump and reload sfx table - useful for developing the sound table.

void F4SoundEntering3d(void); // MLR 12/13/2003 - called when OTW is going to the 3d world
void F4SoundLeaving3d(void); // MLR 12/13/2003 - called when OTW is leaving the 3d world



/*
** Sound Groups
*/
/*
#define FX_SOUND_GROUP					0
#define ENGINE_SOUND_GROUP				1
#define SIDEWINDER_SOUND_GROUP			2
#define RWR_SOUND_GROUP					3
#define NUM_SOUND_GROUPS				4
*/
#include "SoundGroups.h"

void F4SetGroupVolume(int group,int vol); // dBs -10000 -> 0

extern long gSoundFlags; // defined in top of fsound

enum
{
	FSND_SOUND=0x00000001,
	FSND_REPETE=0x00000002, // Pete's BRA bearing voice stuff
};


// for positional sound effects stuff
// MLR 2003-10-17 added vx/y/z for doppler effect
void F4SoundFXSetPos( int sfxId, int override, float x, float y, float z, float pscale, float volume = 0.0F, int uid=0);
void F4SoundFXSetDist( int sfxId, int override, float volume, float pscale );
//void F4SoundFXSetCamPos( float x, float y, float z ); // obsolete JPO
void F4SoundFXSetCamPosAndOrient(Tpoint *campos, Trotation *camrot, Tpoint *camvel); // MLR 12/2/2003 - Added velocity (back) to this function
void F4SoundFXPositionDriver( unsigned int begFrame, unsigned int endFrame );
void F4SoundFXInit( void );
void F4SoundFXEnd( void );
BOOL F4SoundFXPlaying(int sfxId, int UID=0);

#ifdef __cplusplus
}
#endif

class SimBaseClass;

#include "FalcLib/include/alist.h"

// this object is used to maintain the positional & velocity values and to supply an interface to the sound code.
class F4SoundPos : public ANode
{
	friend class mlrVoiceHandle;
	friend class mlrVoice;
	public:
	  F4SoundPos();
	  ~F4SoundPos();
		void UpdatePos( SimBaseClass *owner );
	  void UpdatePos( float x, float y, float z, float vx, float vy, float vz);
	  void Sfx(int SfxID, int subid=0, float pscale=1.0, float vol=0);
	  void Sfx(int SfxID, int subid, float pscale, float vol, float x, float y, float z);
	  void SfxRel(int SfxID, int subid, float pscale, float vol, float x, float y, float z);
		void SfxRel(int SfxID, int subid, float pscale, float vol, Tpoint &lpos);
	  bool IsPlaying(int SfxID, int subid=0);
	  void Purge(void);
		void PositionalData(void);
		static void OTWViewChanged( int boolean ) { otwViewChange = boolean; };
	private:
		static int otwViewChange;
	
	  int uid;
	  Tpoint pos,vel,velVec,relPos;
		Trotation orientation;
		float velocity;
		float distance; // to camera
		int inMachShadow; // if true, I can't be heard
		int sonicBoom;

		AList soList;
		int inPurgeList;
		SimBaseClass *platform; // MLR 5/16/2004 - 
};

#ifdef _INC_WINDOWS
int  InitSoundManager (HWND hWnd, int mode, char *falconDataDir);
#endif

#endif
