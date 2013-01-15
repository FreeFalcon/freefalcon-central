#ifndef _VOICE_MAP_H_
#define _VOICE_MAP_H_

// extra support for selecting voices

class VoiceMapper {
    // active stuff
    int totalvoices;
    unsigned int *voiceflags;

    // initialisation stuff
    static const unsigned int default_voices[];
    static const int max_default_voices;
/*JAM 19Sep03    static const struct namemap {
	char *name;
	unsigned int id;
    } Names[];*/
public:
	//JAM 19Sep03 - Fixes MSVC7 compile errors
	struct namemap
	{
		char *name;
		unsigned int id;
	};
	//JAM
	int GetNextVoice(int start, int type, int side);
    VoiceMapper() : totalvoices(0), voiceflags(0) {};
    ~VoiceMapper();
	int PickVoice(int type, int side);
	void SetMyVoice(int n);
	void SetVoiceCount(int n);
	void LoadVoices();
    enum {
	// types of thing
	VOICE_NONE = 0,
	VOICE_PILOT = 0x1,
	    VOICE_ATC = 0x2,
	    VOICE_FAC = 0x4,
	    VOICE_AWACS = 0x8,
	// where
	VOICE_SIDE_BASE = 0x10,
	VOICE_SIDE1 = VOICE_SIDE_BASE << 0,
	VOICE_SIDE2 = VOICE_SIDE_BASE << 1,
	VOICE_SIDE3 = VOICE_SIDE_BASE << 2,
	VOICE_SIDE4 = VOICE_SIDE_BASE << 3,
	VOICE_SIDE5 = VOICE_SIDE_BASE << 4,
	VOICE_SIDE6 = VOICE_SIDE_BASE << 5,
	VOICE_SIDE7 = VOICE_SIDE_BASE << 6,
	VOICE_SIDE8 = VOICE_SIDE_BASE << 7,
	VOICE_SIDE_ALL = VOICE_SIDE1 | VOICE_SIDE2 | VOICE_SIDE3 | VOICE_SIDE4 |
	VOICE_SIDE5 | VOICE_SIDE6 | VOICE_SIDE7,

	VOICE_SELF = VOICE_SIDE8 << 8,
	VOICE_SIDE_UNK = -1,
    };
    
private:
	unsigned int LookupName(const char *name);
};

extern VoiceMapper g_voicemap;
#endif
