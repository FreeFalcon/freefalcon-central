#ifndef	_VOICE_MGR_H_
#define _VOICE_MGR_H_

#ifndef BINARY_TOOL
#include "f4thread.h"
#include "sim\include\stdhdr.h"
#include "falcsnd\falcvoice.h"
#include "falcsnd\lhsp.h"
#include "FileMemMap.h"

class CONVERSATION
{
  public:
	short	message;			// id of this message
	char	status;				// flag to note if ok to fill with new conversation
	char	convIndex;			// Conversation Index is the token(file) to play
	char	sizeofConv;			// The number of tokens(files) in the conversation
	char	speaker;			// The person speaking the conversation
	char	priority;			// The conversations priority - for sorting the conversation queue
	char	interrupt;			// To overide the conversation queue - also used to kill voice
	char	channelIndex;		// the channel voice will play through
	VU_TIME playTime;			// when the message can be played 
	VU_ID	to;					// the VuEntity that the message was sent to, if any 
	VU_ID	from;				// the VuEntity that sent the message, if any 
	char	filter;				// at what filter level should this message be played?
	short	*conversations;		// The array of tokens(files) played to make up a conversation

#ifdef USE_SH_POOLS
  public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(CONVERSATION) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(CONVERSATION), 100, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif
};

#include "falcsnd\voicefilter.h"

extern VU_TIME vuxGameTime;
#endif

#define		NUM_TLK_FILES			4
#define		NUM_VOICE_CHANNELS		2 
#define		MULT_VOICE_CHANNELS		1
#define		MAX_NUM_AUDIO_BUFFERS	2
//#define		NUM_VOICES				14

// conversation interupt
enum{
QUEUE_CONV		= 1,
OVERRIDE_CONV	= 2,
};

// conversation priority
enum{
 ATTACK_PRIORITY,		
 NORMAL_PRIORITY,		
 MEDIUM_PRIORITY,
};

//decompQueue status
enum{
 SLOT_IS_AVAILABLE	=	-1,
 MESG_IS_AVAILABLE	=	0,
 MESG_IS_PROCESSING	=	1,
 ADD_SILENCE		=	2,
 SILENCE_ADDED		=	3,
};

//how to insert/sort
enum{
	SEARCH_AND_DESTROY	=	0,
	SORT_TIME			=	0,
	SORT_TIME_PRIORITY	=	1,
};


#define COMP_SIL_SIZE			0x6D60//0x3120	// Requested silence decompression from .tlk
#define COMP_LZSS_SIZE			0x6D60//0x3120	// Requested lzss decompression from .tlk

/* Write .tlk file */
#define WAV_TO_RAW				0
#define COMPRESS_RAW_FILES		1
#define TLK_HEADER_INFO			12
#define FLAG_SET(a,b)			((a) |= (b) )
#define FLAG_UNSET(a,b)			((a) &= ~(b))

#ifndef BINARY_TOOL

typedef struct VMBuffQueue {
	int		channel;
	int		buff;
} VMBuffQueue;



typedef struct VM_BUFFLIST {
	VMBuffQueue * node;     /* pointer to node data */
	
	struct VM_BUFFLIST * next;   /* next list node */
	struct VM_BUFFLIST * prev;   /* prev list node */

#ifdef USE_SH_POOLS
  public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(VM_BUFFLIST) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(VM_BUFFLIST), 4, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif

} VM_BUFFLIST;




typedef struct VM_CONVLIST {
	CONVERSATION *node;    /* pointer to node data */
	
	struct VM_CONVLIST * next;   /* next list node */
	struct VM_CONVLIST * prev;   /* prev list node */

#ifdef USE_SH_POOLS
  public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(VM_CONVLIST) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(VM_CONVLIST), 100, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif
} VM_CONVLIST;

class TlkFile : public FileMemMap {
    long Index2Data(int tlkind) { return TLK_HEADER_INFO + sizeof(long) * tlkind; };
    long GetFragIndex(int tlkind) { 
	BYTE *data = GetData(Index2Data(tlkind), sizeof (long));
	ShiAssert(data != NULL);
	return data ? *(long *)data : 0;
    };
    struct TlkBlock {
	unsigned long filelen;
	unsigned long compressedlen;
	char data[1]; // more in practice
    };
public:
    unsigned long GetFileLength(int tlkind);
    unsigned long GetCompressedLength(int tlkind);
    char *GetDataPtr(int tlkind);
};
#define NEWVOICE

class VoiceManager
{
friend  DWORD WINAPI VoiceManagementThread( LPVOID lpvThreadParm );

public:
//	char			*voiceMapPtr; // JPO - removed
	TlkFile			voiceMap;
	LHSP			*lhspPtr;
	char			VMWakeEventName[30];
	FalcVoice		*falconVoices;
	CONVERSATION	decompQueue[NUM_VOICE_CHANNELS];
	int				radiofilter[2];
	int				currRadio;
	F4CSECTIONHANDLE*	vmCriticalSection;
	
				VoiceManager			( void );
				~VoiceManager			( void );
	BOOL		VMBegin					( void );
	void		CallVoiceThread			( void );
	int			LoadCompressionData		( int currChannel);
	void		AddToConversationQueue	( CONVERSATION *newConv );
//	void		InitCompressionBuffer	( compression_buf_t *compBuf );
	void		VMResetVoices			( void );
	void		VMResetVoice			( int channel );
	void		VMResumeVoiceStreams	( void );
	void		VMCleanup				( void );
	void		VMHearVoices			( void );
	void		VMHearChannel			( int channel);
	void		VMSilenceVoices			( void );
	void		VMSilenceChannel		( int channel );
	void		VMAddBuffToQueue		( int channel, int buffer );
	//should be either SORT_TIME_PRIORITY or SORT_TIME
	VM_CONVLIST *VMConvListInsert		( VM_CONVLIST *list, VM_CONVLIST *newnode,int insertType );
	VM_BUFFLIST *VMBuffListAppend		( VM_BUFFLIST *list, VMBuffQueue *node );
	void		VMListRemoveVCQ			( VM_CONVLIST **list, VM_CONVLIST *node);
	VM_BUFFLIST *VMListRemoveVMBQ		( VM_BUFFLIST *list );
	VM_BUFFLIST *VMListPopVMBQ			( VM_BUFFLIST *list );
	void		VMDeleteNode			( VMBuffQueue* vmNode );
	VM_BUFFLIST *VMListSearchVMBQ		( VM_BUFFLIST *list, int channelNum, int	searchType );
	int			ListCheckChannelNum		( void *node_a, int channelNum );
	VM_CONVLIST *VMListDestroyVCQ		( VM_CONVLIST *list );
	VM_BUFFLIST *VMListDestroyVBQ		( VM_BUFFLIST *list );
	void		ChangeRadioFreq			( int radiofilter, int radio );
	int			GetRadioFreq			( int radio);
	void		ForwardCycleFreq		( int radio );
	void		BackwardCycleFreq		( int radio );
	int			ResumeChannel			( int channel );
	int			PauseChannel			( int channel );
	int			BuffersEmpty			( int channel );
	int			VoiceOpen				(void);
	int			IsChannelDone			( int channel );
	void		AddNoise				(VOICE_STREAM_BUFFER *buffer, VU_ID from, int channel);
	void		RemoveRadioCalls		( VU_ID dead );
	void		RemoveDuplicateMessages ( VU_ID from, VU_ID to, int msgid); //this removes similar messages from the queues
	int			IsMessagePlaying		( VU_ID from, VU_ID to, int msgid);	//returns true if a the same message with the same to
																			// and from is playing

	void		SetChannelVolume(int channel, int volume);
	int			Radio					(void)				{return currRadio;}
	void		SetRadio				(int radio)			{currRadio = radio;}
	int			VoiceHandle				( int channel )		{return falconVoices[channel].FalcVoiceHandle;}
};

extern VoiceManager	*VM;
#endif

#endif
