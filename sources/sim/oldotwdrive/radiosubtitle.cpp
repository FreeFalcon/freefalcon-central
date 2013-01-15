/*****************************************************************************/
//	File:			RadioSubTitle.cpp
//	Author:			Retro
//	Date:			Dec2003
//
//	Description:	This class enables subtitles for radio messages. Radio messages
//					are created in voicefilter.cpp in the soundthread and are displyed
//					in otwloop.cpp in the graphics thread.
//					One problem is that the order of created messages is not necessarily
//					the order in which they should be played, eg is the 'Tower, inbound'
//					reply from the Tower scheduled to play about 7 secs after the call,
//					the messages are howver generated simultaneously. Thus, a simple
//					FIFO like buffer is out of the question.
//					So this class implements a linked list (derived from AList.cpp out
//					of falclib) and inserts/removes messages according to their scheduled
//					playtime.
//					Works like this: on constucting the class a csv file with all the typed-up
//					radiomessiges is read in and placed in a big table. When a messages is
//					created (in voicefilter.cpp) it constructs the string out of the chunks
//					read from that csv file and a new node, containing this string, its
//					scheduled playtime and a colour representing the channel it originated from,
//					is placed into the linked list at the correct place (time-sorted, 'earliest'
//					message first).
//					The graphics loop then requests a sorted list of all inputstrings+the associated
//					colours - while doing this I also clean the linked list of messages that already
//					have been displayed long enough - 'old messages fall out of the front of the
//					linked list'
//
//	Issues:			I don´t have completely examined the multithreading problem, eg what happens
//					if the graphics loop requests a list of strings while the sound loop at the
//					same time adds a new message.. I guess I can´t use mutex thingies or such,
//					since delaying a thread is a big nono... hrmm
//					However I´ve never had a problem with this as of yet, and I´ve flown quite a
//					few heavy-duty ('radio-call-wise') mission.. of course, that doesn´t prove anything..
/*****************************************************************************/
#include "stdhdr.h"
#include "radiosubtitle.h"
#include "falcsnd\voicefilter.h"
#include <string.h>
#include <stdio.h>

#pragma warning (push,4)

CRITICAL_SECTION  RadioSubTitle::cs_radiosubtitle;

// default channel colours
#define TOFROM_FLIGHT_COLOUR		0xff00ff00	// green
#define TO_PACKAGE_COLOUR			0xff0000ff	// red
#define TOFROM_PACKAGE_COLOUR		0xff00ffff	// yellow
#define TO_TEAM_COLOUR				0xffff0000	// blue	- this is the guard channel, actually..
#define IN_PROXIMITY_COLOUR			0xffffff00	// cyan
#define TO_WORLD_COLOUR				0xff808080	// no idea, kinda dark grey :p
#define TOFROM_TOWER_COLOUR			0xff000000	// black
#define STANDARD_COLOUR				0xffffffff	// white

#define THE_INPUT_FILE_NAME			"F4Talk95v1-0-0.csv"

#define MAX_READ_LEN 2048	//	max size of a single line in the csv line. since that whole file is 300k,
							//	2MB should be enough I guess
#define SEPARATOR ','
#define SEP ","
#define QUOTAS '"'

//	#define WRITE_BACK_STRING_FILE
//	#define SHOW_FRAG_AND_TALKER___BUT_MEM_LEAK
//	#define DISPLAY_CHANNEL_NAME

#define THREADSTUFF	// gotta try out my shiny new critical section..

/*****************************************************************************/
//	
/*****************************************************************************/
RadioSubTitle::RadioSubTitle(const int MaximumMessageNum, const unsigned long TTL)
{

	if (MaximumMessageNum != 0)
		MaxMessageNum = MaximumMessageNum;
	else
		MaxMessageNum = 20;

	if (TTL != 0)
		messageTTL = TTL;
	else
		messageTTL = MESSAGE_TTL;

	theRadioChatterList = new AList();
	currentlyEditedNode = (SubTitleNode*)0;
	LinkedListCount = 0;

#ifndef DYNAMIC_LINE_NUM	// Retro 11Jan2004
	for (int i = 0; i < MAX_FRAG_NUM; i++)
	{
		theStrings[i] = 0;
	}
#endif	// DYNAMIC_LINE_NUM

	colour_Flight		= TOFROM_FLIGHT_COLOUR;
	colour_ToPackage	= TO_PACKAGE_COLOUR;
	colour_ToFromPackage= TOFROM_PACKAGE_COLOUR;
	colour_Team			= TO_TEAM_COLOUR;		// this is the guard channel
	colour_Proximity	= IN_PROXIMITY_COLOUR;
	colour_World		= TO_WORLD_COLOUR;
	colour_Tower		= TOFROM_TOWER_COLOUR;
	colour_Standard		= STANDARD_COLOUR;	// only used in default cases which (shouldn´t happen actually)

#ifdef DYNAMIC_LINE_NUM	// Retro 11Jan2004
	FragCount = CountLinesInFile(THE_INPUT_FILE_NAME);
	if (FragCount == 0)
	{
		throw Init_Error("No frag lines in subtitles input file");
	}
	else
	{
		theStrings = (csvLine**)calloc(sizeof(csvLine*),FragCount+100); // Retro 11Jan2004 - accounting for count-errors :p
		if (theStrings == (csvLine**)0)
		{
			throw Init_Error("Could not create strings");
		}
	}
#endif	// DYNAMIC_LINE_NUM

	if (!ReadNewFile(THE_INPUT_FILE_NAME))
	{
		throw Init_Error("Error reading the subtitles input file");
	}

#ifdef WRITE_BACK_STRING_FILE
	WriteOut();
#endif

	InitializeCriticalSection(&cs_radiosubtitle);
}

/*****************************************************************************/
//	
/*****************************************************************************/
RadioSubTitle::~RadioSubTitle(void)
{
	int i = 0;

	SubTitleNode* node = 0;
	do
	{
		delete(theRadioChatterList->RemHead());
		node = (SubTitleNode*)theRadioChatterList->GetHead();
	} while (node);

	delete(theRadioChatterList);
	theRadioChatterList = 0;

#ifndef DYNAMIC_LINE_NUM	// Retro 11Jan2004
	for (i = 0; i < MAX_FRAG_NUM; i++)
	{
		if (theStrings[i])
		{
			delete(theStrings[i]);
		}
	}
#else	// DYNAMIC_LINE_NUM
	if (theStrings)
	{
		while (theStrings[i])
		{
			delete(theStrings[i]);
			theStrings[i] = 0;
			i++;
		}
		free(theStrings);
		theStrings = 0;
	}
#endif	// DYNAMIC_LINE_NUM

	DeleteCriticalSection(&cs_radiosubtitle);
}

/*****************************************************************************/
//
/*****************************************************************************/
void RadioSubTitle::SetTTLAndMessageNum(const int MessageNum, const unsigned long TTL)
{
	if (MessageNum != 0)
		MaxMessageNum = MessageNum;
	else
		MaxMessageNum = 20;

	if (TTL != 0)
		messageTTL = TTL;
	else
		messageTTL = MESSAGE_TTL;
}

/*****************************************************************************/
//	wipes nodes in the list but not the list itself.
/*****************************************************************************/
void RadioSubTitle::ResetAll()
{
	SubTitleNode* node = 0;
	do
	{
		delete(theRadioChatterList->RemHead());
		node = (SubTitleNode*)theRadioChatterList->GetHead();
	} while (node);
	LinkedListCount = 0;
}

/*****************************************************************************/
//	creates a new message-node, marks this node as 'editable' to enable appending
//	of other bits
//	the messages are kept in a time-sorted linked list, starting with the oldest
//	this linked list gets cleaned of outdated messages in the getTimeSortedMessages()
//	function
/*****************************************************************************/
void RadioSubTitle::NewMessage(const int theTalker, const int theFrag, const unsigned long thePlayTime, const char theFilter)
{
#ifdef THREADSTUFF	// locking out the graphicsthread..
	EnterCriticalSection(&cs_radiosubtitle);
#endif

	currentlyEditedNode = new SubTitleNode();
	if (!currentlyEditedNode)
	{
		assert(false);
#ifdef THREADSTUFF
		LeaveCriticalSection(&cs_radiosubtitle);
#endif
		return;
	}

	LinkedListCount++;
	currentlyEditedNode->messageStartTime = thePlayTime;
	currentlyEditedNode->associatedChannel = FindChannelColour(theFilter);
	
#ifndef DISPLAY_CHANNEL_NAME
	char* rchunk = GetRadioChunk(theTalker, theFrag);
	if (rchunk)
	{
		OverWriteString(&currentlyEditedNode->theSpokenLine, rchunk);
	}
#else
	char* chan = FindChannelName(theFilter);
	char* rchunk = GetRadioChunk(theTalker, theFrag);
	if (rchunk)
	{
		char* sum = (char*)malloc(strlen(chan)+strlen(rchunk)+1);
		if (sum)
		{
			strcpy(sum,chan);
			strcat(sum,rchunk);

			OverWriteString(&currentlyEditedNode->theSpokenLine, sum);
			free(sum);
		}
	}
#endif

	SubTitleNode* index = (SubTitleNode*)theRadioChatterList->GetHead();
	while ((index)&&(index->messageStartTime < thePlayTime))
	{
		index = (SubTitleNode*)index->GetSucc();
	}
	if (index)
		currentlyEditedNode->InsertAfter(index);
	else
		theRadioChatterList->AddTail(currentlyEditedNode);

#ifdef THREADSTUFF
	LeaveCriticalSection(&cs_radiosubtitle);
#endif
}

/*****************************************************************************/
//	Appends a string chunk to an already existing message
/*****************************************************************************/
void RadioSubTitle::AddToMessage(const int theTalker, const int theFrag)
{
#ifdef THREADSTUFF	// locking out the graphicsthread..
	EnterCriticalSection(&cs_radiosubtitle);
#endif
	char* rchunk = GetRadioChunk(theTalker, theFrag);
	if ((rchunk)&&(currentlyEditedNode))
	{
		AppendToString(&currentlyEditedNode->theSpokenLine," ");
		AppendToString(&currentlyEditedNode->theSpokenLine,rchunk);
	}
#ifdef THREADSTUFF
	LeaveCriticalSection(&cs_radiosubtitle);
#endif
}

/*****************************************************************************/
//	should eventually look up the current string in the talkview csv file..
/*****************************************************************************/
char* RadioSubTitle::GetRadioChunk(const int theTalker, const int theFrag)
{
#ifdef SHOW_FRAG_AND_TALKER___BUT_MEM_LEAK
	if ((theStrings[theFrag])&&(theTalker < MAX_VOICE_NUM))
	{
		char* bla = (char*)malloc(strlen(theStrings[theFrag]->Voices[theTalker])+30);	// mem leak but it´s only for test anyway 
		if (bla)
		{
			char tmp[30];
			sprintf(tmp,"-> %i %i ",theTalker, theFrag);
			strcpy(bla,theStrings[theFrag]->Voices[theTalker]);
			strcat(bla,tmp);
			return bla;
		}
		else
		{
			return "bla";
		}
	}
	else
		return 0;
#else
#ifdef NDEBUG
	if ((theFrag < FragCount)&&(theStrings[theFrag])&&(theTalker < MAX_VOICE_NUM)&&!F4IsBadReadPtr(theStrings[theFrag], sizeof(csvLine)))
	{
		return theStrings[theFrag]->Voices[theTalker];
	}
#else
	if ((theFrag < FragCount)&&(theStrings[theFrag])&&(theTalker < MAX_VOICE_NUM))
	{
		if (F4IsBadReadPtr(theStrings[theFrag], sizeof(csvLine)))
		{
#pragma warning(disable:4127)
			ShiAssert(false);
#pragma warning(default:4127)
			return 0;
		}
		else
			return theStrings[theFrag]->Voices[theTalker];
	}
#endif
	else
		return 0;
#endif
}

/*****************************************************************************/
//	Returns an array of structures containing a radiomessage and a colour
//	representing the channel it was sent on..
//	Also does housekeeping on the linked list (throws out outdated messages..)
/*****************************************************************************/
ColouredSubTitle** RadioSubTitle::GetTimeSortedMessages(const unsigned long theTime)
{

#ifdef THREADSTUFF	// locking out the soundthread
	EnterCriticalSection(&cs_radiosubtitle);
#endif
	// Get the oldest message in the list..
	SubTitleNode* node = (SubTitleNode*)theRadioChatterList->GetHead();
	if (!node)
	{
#ifdef THREADSTUFF
		LeaveCriticalSection(&cs_radiosubtitle);
#endif
		return 0;	// no messages to play..
	}
	
	// walking back in the list
	// we are not going to draw messages that have already outlived their TTL..
	// if they weren´t displayed by now, well, tough luck (and I betcha your FPS suck hairy donkey balls too)
	// these messages are going to be removed actually..
	// this is done for every message (node) till I find one that is still to be drawn - and as the list is
	// sorted by time, all following ones have also to be drawn so I break from the loop
	while ((node)&&(node->messageStartTime  + messageTTL < theTime))
	{
		SubTitleNode* nodeToDelete = (SubTitleNode*)theRadioChatterList->RemHead();
		if (nodeToDelete)
		{
			delete(nodeToDelete);
			LinkedListCount--;
		}
		node = (SubTitleNode*)theRadioChatterList->GetHead();
	}

	if (!node)
	{
#ifdef THREADSTUFF
		LeaveCriticalSection(&cs_radiosubtitle);
#endif
		 return 0;	// no messages to play.. (all messages were outdated)
	}

	// the calling routine has to delete that array..
	ColouredSubTitle** theMessages = (ColouredSubTitle**)calloc(sizeof(ColouredSubTitle*),LinkedListCount+1);
	if (!theMessages)
	{
#ifdef THREADSTUFF
		LeaveCriticalSection(&cs_radiosubtitle);
#endif
		return 0;
	}

	int i = 0;

	while (node)
	{
		// also do NOT draw are messages (nodes) that are scheduled to be played in da future
		// since the list is sorted I can actually break once this condition is met
//		if (theTime > node->messageStartTime + messageTTL)
		if (node->messageStartTime > theTime)
		{
			break;
		}
//		if (theTime > node->messageStartTime)
		if ((node->messageStartTime < theTime)&&(theTime < node->messageStartTime + messageTTL))
		{
			theMessages[i] = (ColouredSubTitle*)malloc(sizeof(ColouredSubTitle));
			if (theMessages[i])
			{
				theMessages[i]->theString = (char*)malloc(strlen(node->theSpokenLine)+1);
				strcpy(theMessages[i]->theString,node->theSpokenLine);
				theMessages[i]->theColour = node->associatedChannel;
				i++;
			}
		}
		node = (SubTitleNode*)node->GetSucc();

		// there´s a (user-set) limit on how many messages my be displayed, so we break out here
		// (even if there may be messages that still should be played)
		if (i > MaxMessageNum)
			break;
	}

#ifdef THREADSTUFF
	LeaveCriticalSection(&cs_radiosubtitle);
#endif

	return theMessages;
}

#ifdef WRITE_BACK_STRING_FILE
/*****************************************************************************/
//	
/*****************************************************************************/
void RadioSubTitle::WriteOut(void)
{
	FILE* fp = fopen("Talk_output.csv","wt");
	if (fp)
	{
#ifndef DYNAMIC_LINE_NUM	// Retro 11Jan2004
		for (int j = 0; j < MAX_FRAG_NUM; j++)
		{
			if (theStrings[j])
			{
				for (int i = 0; i < MAX_VOICE_NUM; i++)
				{
					if (theStrings[j]->Voices[i])
					{
						fprintf(fp,"%s",theStrings[j]->Voices[i]);
					}
					fprintf(fp,SEP);
				}
				fprintf(fp,"\n");
			}
		}
#else
		if (theStrings)
		{
			while(theStrings[i])
			{
				for (int i = 0; i < MAX_VOICE_NUM; i++)
				{
					if (theStrings[j]->Voices[i])
					{
						fprintf(fp,"%s",theStrings[j]->Voices[i]);
					}
					fprintf(fp,SEP);
				}
				fprintf(fp,"\n");
			}
		}
#endif	// DYNAMIC_LINE_NUM
		fclose(fp);
	}
}
#endif	// WRITE_BACK_STRING_FILE

/*****************************************************************************/
//	hmm.. actually ranking is not important ? indeed it is :/
/*****************************************************************************/
unsigned long RadioSubTitle::FindChannelColour(const char theChannel)
{
	if (theChannel & TOFROM_TOWER)
		return colour_Tower;
	else if (theChannel & TOFROM_FLIGHT)
		return colour_Flight;
	else if (theChannel & TO_PACKAGE)
		return colour_ToPackage;
	else if (theChannel & TOFROM_PACKAGE)
		return colour_ToFromPackage;
	else if (theChannel & TO_TEAM)
		return colour_Team;
	else if (theChannel & IN_PROXIMITY)
		return colour_Proximity;
	else if (theChannel & TO_WORLD)
		return colour_World;
	else
		return colour_Standard;
}

/*****************************************************************************/
//	
/*****************************************************************************/
char* RadioSubTitle::FindChannelName(const char theChannel)
{
	if (theChannel & TOFROM_TOWER)
		return "[Tower] ";
	else if (theChannel & TOFROM_FLIGHT)
		return "[Flight] ";
	else if (theChannel & TO_PACKAGE)
		return "[To Package] ";
	else if (theChannel & TOFROM_PACKAGE)
		return "[To/From Package] ";
	else if (theChannel & TO_TEAM)
		return "[Guard] ";
	else if (theChannel & IN_PROXIMITY)
		return "[Proximity] ";
	else if (theChannel & TO_WORLD)
		return "[Broadcast] ";
	else
		return "[Unknown] ";
}

/*****************************************************************************/
//	
/*****************************************************************************/
void RadioSubTitle::SetChannelColours(	unsigned long flight, unsigned long toPackage, unsigned long ToFromPackage,
										unsigned long Team, unsigned long Proximity, unsigned long World,
										unsigned long Tower, unsigned long Standard)
{
	if (flight != 0)		colour_Flight		= flight;
	if (toPackage != 0)		colour_ToPackage	= toPackage;
	if (ToFromPackage != 0)	colour_ToFromPackage= ToFromPackage;
	// 'Team' is the guard channel
	if (Team != 0)			colour_Team			= Team;
	if (Proximity != 0)		colour_Proximity	= Proximity;
	if (World != 0)			colour_World		= World;
	if (Tower != 0)			colour_Tower		= Tower;
	if (Standard != 0)		colour_Standard		= Standard;
}

/*****************************************************************************/
//	Inputs should be null-terminated strings that can be read as hex-number
//	eg "0xFF00FF00"
/*****************************************************************************/
void RadioSubTitle::SetChannelColours(	char* flight, char* toPackage, char* ToFromPackage,
										char* Team, char* Proximity, char* World,
										char* Tower, char* Standard)
{
	unsigned long flightCol = 0;
	unsigned long toPackageCol = 0;
	unsigned long ToFromPackageCol = 0;
	unsigned long TeamCol = 0;
	unsigned long ProximityCol = 0;
	unsigned long WorldCol = 0;
	unsigned long TowerCol = 0;
	unsigned long StandardCol = 0;

	unsigned long temp = 0;

	if (flight)
		if (sscanf(flight,"%x",&temp) == 1)
			flightCol = temp;
	if (toPackage)
		if (sscanf(toPackage,"%x",&temp) == 1)
			toPackageCol = temp;
	if (ToFromPackage)
		if (sscanf(ToFromPackage,"%x",&temp) == 1)
			ToFromPackageCol = temp;
	if (Team)
		if (sscanf(Team,"%x",&temp) == 1)
			TeamCol = temp;
	if (Proximity)
		if (sscanf(Proximity,"%x",&temp) == 1)
			ProximityCol = temp;
	if (World)
		if (sscanf(World,"%x",&temp) == 1)
			WorldCol = temp;
	if (Tower)
		if (sscanf(Tower,"%x",&temp) == 1)
			TowerCol = temp;
	if (Standard)
		if (sscanf(Standard,"%x",&temp) == 1)
			StandardCol = temp;

	SetChannelColours(	flightCol,toPackageCol,ToFromPackageCol,
						TeamCol,ProximityCol,WorldCol,
						TowerCol,StandardCol);
}

/*****************************************************************************/
//	
/*****************************************************************************/
inline void RadioSubTitle::AppendToString(char** theOldOne, const char* theNewOne)
{
	if ((!theOldOne)||(!*theOldOne)||(!theNewOne))
	{
		assert(false);
		return;
	}

	int oldLen = strlen(*theOldOne);
	int newLen = strlen(theNewOne);

	char* temp = (char*)malloc(oldLen+newLen+1);
	if (temp)
	{
		strcpy(temp,*theOldOne);
		strcat(temp,theNewOne);

		free(*theOldOne);
		*theOldOne = temp;
	}
}

/*****************************************************************************/
//	
/*****************************************************************************/
inline void RadioSubTitle::OverWriteString(char** theOldOne, const char* theNewOne)
{
	if (!theNewOne)
	{
		assert(false);
		return;
	}

	if (theOldOne)
	{
		if (*theOldOne)
		{
		free(*theOldOne);
		*theOldOne = 0;
		}
	}

	*theOldOne = (char*)malloc(strlen(theNewOne)+1);
	if (*theOldOne)
	{
		strcpy(*theOldOne,theNewOne);
	}
}

/*****************************************************************************/
//	
/*****************************************************************************/
void RadioSubTitle::HandleChunk(csvLine_t* theTextString, char* theChunk, int* ChunkCount)
{
	assert(theTextString);
	assert(ChunkCount);

	// 'theChunk' is often NULL, happens when the csv reads ',,'

	int ChunkIndex = *ChunkCount;

	switch (ChunkIndex)
	{
	case 0:	// frag #
		if (theChunk)
		{
			theTextString->Fragment = atoi(theChunk);
			free(theChunk);		// Retro 3Jan2004 - gotta close that mem leak.. oops
			theChunk = 0;
		}
		break;
	case 1:	// maxvoice #
		if (theChunk)
		{
			theTextString->VoiceCount = atoi(theChunk);
			free(theChunk);		// Retro 3Jan2004 - gotta close that mem leak.. oops
			theChunk = 0;
		}
		break;
	case 2: // the actual voices
	case 3: case 4: case 5:case 6: case 7: case 8: case 9:
	case 10: case 11: case 12: case 13: case 14: case 15:
		theTextString->Voices[ChunkIndex-2] = theChunk;
		break;
	case 16:	// summary
		theTextString->Summary = theChunk;
		break;
	case 17:	// eval
		if (theChunk)
		{
			theTextString->Eval = atoi(theChunk);
			free(theChunk);		// Retro 3Jan2004 - gotta close that mem leak.. oops
			theChunk = 0;
		}
		break;
	default:	// shoudn´t happen
		assert(false);
		break;
	}

	ChunkIndex++;
	*ChunkCount = ChunkIndex;
}

/*****************************************************************************/
// set start pointer

// check for "
// if yes, look for next "

// look for SEPERATOR
// set end pointer

// copy chunk, hand it to handleroutiner

// advance pointer
/*****************************************************************************/
void RadioSubTitle::breakDownLine(csvLine_t* theTextString, char* theLine, const int theLength)
{
	assert(theTextString);
	assert(theLine);

	// set start pointer
	char* start = theLine;
	char* end = theLine;
	int len = 0;
	int i = 0;
	bool inDoubleQuotes = false;

	int ChunkCount = 0;

	do
	{
		if (end == 0)
		{
			assert(false);
			break;
		}

		// check for "
		// if yes, look for next "
		if (*end == QUOTAS/*'"'*/)
		{
			do
			{
				end++;
			} while (*end != QUOTAS/*'"'*/);
			inDoubleQuotes = true;
		}

		// look for SEPERATOR
		// set end pointer
		if ((*end == SEPARATOR/*','*/)||(*end == '\0'))
		{
			if (end > start)
			{
				if (!inDoubleQuotes)
				{
					len = end - start + 1;
					char* tmp = (char*)malloc(len);
					if (tmp)
					{
						strncpy(tmp,start,len-1);
						tmp[len-1] = '\0';
						// copy chunk, hand it to handleroutiner
						HandleChunk(theTextString,tmp,&ChunkCount);
					}
					start = end+1;
				}
				else
				{
					len = end - start - 2 + 1;
					char* tmp = (char*)malloc(len);
					if (tmp)
					{
						strncpy(tmp,start+1,len-1);
						tmp[len-1] = '\0';
						// copy chunk, hand it to handleroutiner
						HandleChunk(theTextString,tmp,&ChunkCount);
					}
					start = end+1;
					inDoubleQuotes = false;
				}
			}
			else if (start == end)
			{
				// copy chunk, hand it to handleroutiner
				HandleChunk(theTextString,0,&ChunkCount);
				start = end+1;
			}
			else
			{
				assert(false);
			}

			if (*end == '\0')
			{
				break;
			}
		}
		i++;
		// advance pointer
		end++;
	} while (i < theLength);

//	assert(ChunkCount-1 == theTextString->VoiceCount);
}

/*****************************************************************************/
//	
/*****************************************************************************/
bool RadioSubTitle::ReadNewFile(const char* theFileName)
{

	FILE* fp = fopen(theFileName,"rt");
	if (fp)
	{
		char tmp[MAX_READ_LEN];
		int j = 0;

		while (fgets(&tmp[0],MAX_READ_LEN,fp) != NULL)
		{
			if (j == 0)
			{
				j++;
				continue;	// ignoring the first line
			}

			tmp[strlen(tmp)-1] = 0;	// getting rid of that damn newline

			theStrings[j-1] = new csvLine_t;
			breakDownLine(theStrings[j-1],tmp,strlen(tmp));
			j++;
		}
		fclose(fp);
		return true;
	}
	else
	{
	 	return false;
	}
}

#ifdef DYNAMIC_LINE_NUM		// Retro 11Jan2004
/*****************************************************************************/
//	bwhahaaha.. counts lines in a file.. lol I guess this isn´t very elegant
/*****************************************************************************/
int RadioSubTitle::CountLinesInFile(const char* theFileName)
{
	FILE* fp = fopen(theFileName,"rt");
	if (fp)
	{
		char tmp[MAX_READ_LEN];
		int linecount = 0;

		while (fgets(&tmp[0],MAX_READ_LEN,fp) != NULL)
		{
			linecount++;
		}
		fclose(fp);
		return linecount;
	}
	return 0;
}
#endif	// DYNAMIC_LINE_NUM
#pragma warning (pop)
