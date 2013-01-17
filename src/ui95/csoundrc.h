#ifndef _SOUND_RESOURCE_H_
#define _SOUND_RESOURCE_H_

enum
{
	SOUND_STREAM		= 0x00000001,
	SOUND_STOPONEXIT	= 0x00000002,
	SOUND_LOOP			= 0x00000004,
	SOUND_FADE_IN		= 0x00000008,
	SOUND_FADE_OUT		= 0x00000010,
	SOUND_IN_RES		= 0x10000000,
	SOUND_RES_STREAM	= 0x20000000,
};

class SOUND_RES
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		long  ID;
		long  flags;
		int   SoundID;
		long  Volume;
		long  LoopPoint;
		short Count;
		char *filename;
		SOUND_RSC *Sound;

		void Cleanup()
		{
			if(filename)
				delete filename;
			filename=NULL;
		}
};

class C_Sound
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		int		Stereo_;
		int		Mono_;
		C_Hash	*ResList_;
		C_Hash	*SoundList_;
		C_Hash	*IDTable_; // ptr to cParser's Token list

	public:
		C_Sound();
		C_Sound(char **stream);
		C_Sound(FILE *fp);
		~C_Sound();
		long Size();
		void Save(char **stream);
		void Save(FILE *fp);

		void Setup();
		void Cleanup();

		void SetIDTable(C_Hash *tbl) { IDTable_=tbl; }
		void AddResSound(C_Resmgr *res);
		void AddResStream(C_Resmgr *res);
		BOOL LoadSound(long ID,char *file,long flags);
		BOOL LoadResource(long ID,char *filename);
		BOOL LoadStreamResource(long ID,char *filename);
		BOOL StreamSound(long ID,char *file,long flags);
		SOUND_RES *GetSound(long ID);
		void SetFlags(long ID,long flags);
		long GetFlags(long ID);
		BOOL RemoveSound(long ID);
		BOOL PlaySound(long ID) { return(PlaySound(GetSound(ID))); }
		BOOL LoopSound(long ID) { return(LoopSound(GetSound(ID))); }
		BOOL StopSound(long ID) { return(StopSound(GetSound(ID))); }
		long SetVolume(long ID,long Volume) { return(SetVolume(GetSound(ID),Volume)); }
		BOOL PlaySound(SOUND_RES *Snd);
		BOOL LoopSound(SOUND_RES *Snd);
		BOOL StopSound(SOUND_RES *Snd);
		long SetVolume(SOUND_RES *,long Volume);
		long SetVolume(long Volume);
		void SetAllVolumes(long Volume);

#ifdef _UI95_PARSER_

		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *str,C_Handler *);
		void SaveText(HANDLE,C_Parser *)	{ ; }

#endif // PARSER
};

extern C_Sound *gSoundMgr;

#endif
