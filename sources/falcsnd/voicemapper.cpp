#include <windows.h>
#include "VoiceMapper.h"
#include "sim\include\stdhdr.h"
#include "fakerand.h"

extern FILE* OpenCampFile (char *filename, char *ext, char *mode);
extern void CloseCampFile (FILE *fp);

// single instance of the voicemapper

VoiceMapper g_voicemap;

// JPO
// hold load of support routines for assigning voices to things
// idea is to generalise the F4 thing, so certain voices
// can stick to certain things.

const unsigned int  VoiceMapper::default_voices[] = {
    VOICE_AWACS | VOICE_PILOT | VOICE_SIDE_ALL, // 0
    VOICE_AWACS | VOICE_PILOT | VOICE_SIDE_ALL, // 1
    VOICE_AWACS | VOICE_PILOT | VOICE_SIDE_ALL, // 2
    VOICE_AWACS | VOICE_PILOT | VOICE_SIDE_ALL, // 3
    VOICE_AWACS | VOICE_PILOT | VOICE_SIDE_ALL, // 4
    VOICE_AWACS | VOICE_PILOT | VOICE_SIDE_ALL, // 5
    VOICE_AWACS | VOICE_PILOT | VOICE_SIDE_ALL, // 6
    VOICE_AWACS | VOICE_PILOT | VOICE_SIDE_ALL, // 7
    VOICE_AWACS | VOICE_PILOT | VOICE_SIDE_ALL, // 8
    VOICE_AWACS | VOICE_PILOT | VOICE_SIDE_ALL, // 9
    VOICE_AWACS | VOICE_PILOT | VOICE_SIDE_ALL, // 10
    VOICE_AWACS | VOICE_PILOT | VOICE_SIDE_ALL, // 11
    VOICE_ATC | VOICE_SIDE_ALL, // 12
    VOICE_ATC | VOICE_SIDE_ALL,	// 13
};
const int  VoiceMapper::max_default_voices = sizeof(default_voices) / sizeof(default_voices[0]);

//JAM 19Sep03 - Fixes MSVC7 compile errors
static VoiceMapper::namemap Names[] = {
	{ "atc", VoiceMapper::VOICE_ATC},
    { "awacs", VoiceMapper::VOICE_AWACS},
    { "fac", VoiceMapper::VOICE_FAC},
    { "pilot", VoiceMapper::VOICE_PILOT},
    { "all", VoiceMapper::VOICE_PILOT|VoiceMapper::VOICE_ATC|VoiceMapper::VOICE_AWACS|VoiceMapper::VOICE_FAC},
    { "any", VoiceMapper::VOICE_SIDE_ALL},
    { "1", VoiceMapper::VOICE_SIDE1},
    { "2", VoiceMapper::VOICE_SIDE2},
    { "3", VoiceMapper::VOICE_SIDE3},
    { "4", VoiceMapper::VOICE_SIDE4},
    { "5", VoiceMapper::VOICE_SIDE5},
    { "6", VoiceMapper::VOICE_SIDE6},
    { "7", VoiceMapper::VOICE_SIDE7},
    { "8", VoiceMapper::VOICE_SIDE8},
    { NULL, 0},
};
/*
const struct VoiceMapper::namemap {
    char *name;
    unsigned int id;
} VoiceMapper::Names[] = {
    { "atc", VOICE_ATC},
    { "awacs", VOICE_AWACS},
    { "fac", VOICE_FAC},
    { "pilot", VOICE_PILOT},
    { "all", VOICE_PILOT|VOICE_ATC|VOICE_AWACS|VOICE_FAC},
    { "any", VOICE_SIDE_ALL},
    { "1", VOICE_SIDE1},
    { "2", VOICE_SIDE2},
    { "3", VOICE_SIDE3},
    { "4", VOICE_SIDE4},
    { "5", VOICE_SIDE5},
    { "6", VOICE_SIDE6},
    { "7", VOICE_SIDE7},
    { "8", VOICE_SIDE8},
    { NULL, 0},
};*/
//JAM 19Sep03

VoiceMapper::~VoiceMapper()
{
    if (voiceflags)
	delete voiceflags;
}

// read in the camp file definitions
void VoiceMapper::LoadVoices()
{
    // default stuff
    FILE *fp = OpenCampFile("voicerange", "dat", "r");
    if (fp == NULL) { // just load defaults
	ShiAssert(totalvoices >= max_default_voices);
	for (int i = 0; i < max_default_voices; i++)
	    voiceflags[i] = default_voices[i];
	return;
    }
    memset (voiceflags, VOICE_NONE, sizeof *voiceflags * totalvoices);

    char buf[1024];
    char type[100];
    int voice;
    char side[100];
    while(fgets (buf, sizeof buf, fp)) {
	if (buf[0] == '\n' || buf[0] == '#' || buf[0] == ';')
	    continue; // comment
	if (sscanf(buf, "%d %99s %99s", &voice, &type, &side) != 3)
	    continue;
	ShiAssert(voice < totalvoices);
	if (voice >= totalvoices)
	    continue;
	int vtype = LookupName(type);
	vtype |= LookupName(side);
	voiceflags[voice] |= vtype;
    }

    CloseCampFile(fp);
}

unsigned int VoiceMapper::LookupName(const char *name)
{
    const struct namemap *mp;

    for (mp = Names; mp->name != NULL; mp ++) {
	if (stricmp(mp->name, name) == 0)
	    return mp->id;
    }
    return 0;
}

void VoiceMapper::SetVoiceCount(int n)
{
    totalvoices = n;
    if (voiceflags)
	delete []voiceflags;
    voiceflags = new unsigned int[n];
    memset (voiceflags, VOICE_NONE, sizeof *voiceflags * n);
}


void VoiceMapper::SetMyVoice(int n)
{
    ShiAssert(n <= totalvoices);
    voiceflags[n] = VOICE_SELF;
}

// choose a voice at random that has the right attributes
int VoiceMapper::PickVoice(int type, int side)
{
    ShiAssert(type > 0 && type < VOICE_SIDE_BASE);
    ShiAssert((side >= 0 && side <= 7) || side == VOICE_SIDE_UNK);
    unsigned int match = type;
    if (side != VOICE_SIDE_UNK)
	match |= (VOICE_SIDE_BASE << side);
    float chance;
    int selected = 0;
    int recno = 0;
    for (int i = 0; i < totalvoices; i++) {
	if ((voiceflags[i] & match) != match)
	    continue;
	recno ++;
	chance = 1.0f / recno;
	if (chance > PRANDFloatPos())
	    selected = i;
    }
    return selected;
}

int VoiceMapper::GetNextVoice(int start, int type, int side)
{
    ShiAssert(type > 0 && type < VOICE_SIDE_BASE);
    ShiAssert(side >= 0 && side <= 7);
    unsigned int match = type | (VOICE_SIDE_BASE << side);
    int selected = 0;
    int recno = 0;
    for (int i = 0; i < totalvoices; i++) {
	start = (start + 1) % totalvoices;
	if ((voiceflags[start] & match) == match)
	    return start;
    }
    return 0;
}