/*****************************************************************************/
//	File:			RadioSubTitle.h
//	Author:			Retro
//	Date:			Dec2003
//	
//	Description:	Please see the .cpp
/*****************************************************************************/
#define MESSAGE_TTL 0x4e20 //20 secs 0x1350	// about 5 seconds (value is in ms)

#define DYNAMIC_LINE_NUM	// Retro 11Jan2004 - no static array for subtitles any more

#define MAX_VOICE_NUM 14

#ifndef DYNAMIC_LINE_NUM	// Retro 11Jan2004
	#define MAX_FRAG_NUM 3000
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "AList.h"

struct ColouredSubTitle {
	char* theString;
	unsigned long theColour;
	ColouredSubTitle() { theString = 0; theColour = 0; }
	~ColouredSubTitle() { free(theString); theString = 0; }
};

class RadioSubTitle {
public:
	RadioSubTitle(const int MessageNum, const unsigned long TTL);
	~RadioSubTitle(void);

	ColouredSubTitle** GetTimeSortedMessages(const unsigned long theTime);

	void ResetAll();

	void NewMessage(const int theTalker, const int theFrag, const unsigned long thePlayTime, const char theFilter);
	void AddToMessage(const int theTalker, const int theFrag);

	void SetTTLAndMessageNum(const int MessageNum, const unsigned long TTL);

	void SetChannelColours(	unsigned long flight, unsigned long toPackage, unsigned long ToFromPackage,
							unsigned long Team, unsigned long Proximity, unsigned long World,
							unsigned long Tower, unsigned long Standard);

	void SetChannelColours( char* flight, char* toPackage, char* ToFromPackage,
							char* Team, char* Proximity, char* World,
							char* Tower, char* Standard);

	struct Init_Error {
		const char* theReason;
		Init_Error(const char* reason) { theReason = reason; }
	};

	static CRITICAL_SECTION	cs_radiosubtitle;
private:
	AList* theRadioChatterList;

	class SubTitleNode : public ANode {
	public:
		char* theSpokenLine;
		unsigned long messageStartTime;
		unsigned long associatedChannel;
		SubTitleNode() { messageStartTime = 0; associatedChannel = 0; theSpokenLine = 0; }
		~SubTitleNode() { free(theSpokenLine); theSpokenLine = 0; }
	};

	SubTitleNode* currentlyEditedNode;

	unsigned int LinkedListCount;

	typedef struct csvLine {
		int Fragment;	// actually this could be the index in the array I guess ???
		int VoiceCount;
		char* Voices[MAX_VOICE_NUM];
		char* Summary;
		int Eval;
		csvLine()
		{
			for (int i = 0; i < MAX_VOICE_NUM; i++)
			{
				Voices[i] = 0;
			}
			Summary = 0;
			Fragment = VoiceCount = Eval = 0;
		}
		~csvLine()
		{
			for (int i = 0; i < MAX_VOICE_NUM; i++)
			{
				if (Voices[i])
				{
					free(Voices[i]);
					Voices[i] = 0;
				}
			}
			if (Summary)
			{
				free(Summary);
				Summary = 0;
			}
		}
	} csvLine_t;

#ifndef DYNAMIC_LINE_NUM	// Retro 11Jan2004
	csvLine_t* theStrings[MAX_FRAG_NUM];
#else
	csvLine_t** theStrings;
	int CountLinesInFile(const char* theFileName);
	int FragCount;
#endif	// DYNAMIC_LINE_NUM

	bool ReadNewFile(const char* theFileName);
	void breakDownLine(csvLine_t* theTextString, char* theLine, const int theLength);
	void HandleChunk(csvLine_t* theTextString, char* theChunk, int* ChunkCount);

	char* GetRadioChunk(const int theTalker, const int theFrag);
	void WriteOut(void);
	unsigned long FindChannelColour(const char theChannel);
	char* FindChannelName(const char theChannel);

	inline void AppendToString(char** theOldOne, const char* theNewOne);
	inline void OverWriteString(char** theOldOne, const char* theNewOne);

	int messageTTL;
	int MaxMessageNum;
	unsigned long colour_Flight;
	unsigned long colour_ToPackage;
	unsigned long colour_ToFromPackage;
	unsigned long colour_Team;		// this is the guard channel
	unsigned long colour_Proximity;
	unsigned long colour_World;
	unsigned long colour_Tower;
	unsigned long colour_Standard;	// only used in default cases which (shouldn´t happen actually)
};

extern RadioSubTitle* radioLabel;