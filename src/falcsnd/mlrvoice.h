#include <dsound.h>
#include "soundfx.h"
#include "alist.h"

/* 

  ownership heirarchy.

VuEntity
   |
   +-F4SoundPos - positional data
        |                 API interface
		+-mlrVoiceHandle -> mlrVoice -> mlrBuffer
		+-SoundHandleNode -> mlrVoice


  */

class mlrVoiceManager;
class mlrVoice;
class mlrVoiceHandle;

class mlrVoiceHandle : public ANode
{
	friend mlrVoice;
public:
	mlrVoiceHandle(class F4SoundPos *Owner, int SfxID, int UID);
	~mlrVoiceHandle();

	void Play(float Pscale, float Vol, float X, float Y, float Z, float VX, float VY, float VZ);
	bool IsPlaying(void);
	void Stop(void);

	bool AreYou(int SfxId, int UserID);

private:
	mlrVoice	*voice; // my baby
	F4SoundPos  *SPos;   // my baby's daddy

	int   sfxid;
	int   userid;
	SFX_DEF_ENTRY *sfx;
public:
	int   lastPlayTime; // Delete Time
};




extern mlrVoiceManager gVoiceManager;

class mlrVoiceManager
{
public:
	AList ExecList; // sorted
	AList PlayList; // unsorted
	AList HoldList; // unsorted

	mlrVoiceManager();
	~mlrVoiceManager();

	void Exec(Tpoint *campos, Trotation *camrot, Tpoint *camvel);
	void StopAll(void);
	void ReleaseAll(void);
	void Lock(void);
	void Unlock(void);

	Tpoint listenerFront;
	Tpoint listenerUp;
	Tpoint listenerPosition;
	Tpoint listenerVelocity;

private:
	F4CSECTIONHANDLE*    mlrVoiceSection;	// Thread critical section information
	void MovePlay2Hold(void);
};



class mlrVoice : public ANode
{
	friend class CSoundMgr;
	friend class mlrVoiceManager; // we're buddies!
public:

	mlrVoice(mlrVoiceHandle *owner);
	~mlrVoice();
	void Play(float PScale, float Vol, float X, float Y, float Z, float VX, float VY, float VZ);
	void Pause(void);
	bool IsPlaying(void);
	void Stop(void);
	enum mlrVoiceStatus { VSHOLD, VSSTART, VSPLAYING, VSPAUSED, VSSTOP };

	mlrVoiceStatus status;
	// ANode virtual, used to prioritize the queue list
	int CompareWith(ANode *n);

private:
	mlrVoiceHandle *owner;
	SFX_DEF_ENTRY *sfx;

	float priority;

	void PreExec(void); // used to intialize vol levels based on in/out of pit and set priority.
	void Exec(void);
	bool AllocateBuffers(void);
	void ReleaseBuffers(void);

	bool OK;

	// these should only be set in the constructor.
	IDirectSoundBuffer	  *DSoundBuffer;
	LPDIRECTSOUND3DBUFFER  DSound3dBuffer;

	float x,y,z;    // the last emmitting position;
	float vx,vy,vz; // velocities
	float pscale;
	int   freq;
	float initvol,vol;
	float distsq;
	int   startTime;
	int autodelete;
	int is3d;

};