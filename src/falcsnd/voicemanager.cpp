/* ------------------------------------------------------------------------

  VoiceManager.cpp
  
    Queues conversations and decompresses sound
	
	  Version 0.01
	  
		Written by Jim DiZoglio (x257)       (c) 1996 Spectrum Holobyte
		Rewritten by Dave Power (x4373)
		
------------------------------------------------------------------------ */

#include <windows.h>
#include <process.h>
#include "fsound.h"
#include "f4thread.h"
#include "debuggr.h"
#include "VoiceManager.h"
#include "conv.h"
#include "F4Find.h"
#include "soundgroups.h"
#include "vutypes.h"
#include "psound.h"
#include "sim\include\aircrft.h"
#include "sim\include\navsystem.h"
#include "sim\include\tacan.h"
#include "sim\include\otwdrive.h"
#include "playerop.h"
#include "flight.h"
#include "sim\include\simdrive.h"
#include "MsgInc\RadioChatterMsg.h"

void *map_file (char *filename,long bytestomap = 0);

extern int noUIcomms;

extern VU_TIME vuxGameTime;
extern void set_spinner3 (int);
VU_ID gVmPlayVU_ID;

extern char FalconSoundThrDirectory [];
extern int g_nSoundSwitchFix;

#ifdef USE_SH_POOLS
MEM_POOL	CONVERSATION::pool;
MEM_POOL	VM_BUFFLIST::pool;
MEM_POOL	VM_CONVLIST::pool;
#endif

VM_BUFFLIST		*voiceBufferQueue = NULL; 
VM_CONVLIST		*voiceChannelQueue[NUM_VOICE_CHANNELS] = {NULL};

enum{
	MSG_HOLD_TIME =  60000, //60 seconds 
	SILENCE_LEN   =  16000, //16 seconds
};


HANDLE			VMWakeEventHandle;
BOOL			killThread = FALSE;
HANDLE			hThread;

//FILE *debugFile = NULL;
//FILE *debugEndFile = NULL;

//int	global=0; //for debugging
//int MESGNUM = 0;

extern VoiceManager	*VM;
//extern "C"
//{
//	DWORD WINAPI VoiceManagementThread( LPVOID lpvThreadParm ) ;
//}

VoiceManager::VoiceManager( void )
{
    vmCriticalSection = NULL;
}

VoiceManager::~VoiceManager( void )
{
	VMCleanup();

#ifdef USE_SH_POOLS
	CONVERSATION::ReleaseStorage();
	VM_BUFFLIST::ReleaseStorage();
	VM_CONVLIST::ReleaseStorage();
#endif
}

BOOL VoiceManager::VMBegin( void )
{
	int	i;
	
#ifdef USE_SH_POOLS
	CONVERSATION::InitializeStorage();
	VM_BUFFLIST::InitializeStorage();
	VM_CONVLIST::InitializeStorage();
#endif

	// edg: create critical section for voicemanager
	if ( vmCriticalSection == NULL )
		vmCriticalSection = F4CreateCriticalSection("vmCritical");
	//This is where the decomp library is initialized
	lhspPtr = new LHSP;
	if(lhspPtr)
		lhspPtr->InitializeLHSP();
	//This is where the .tlk file is opened
	VoiceOpen();
	
	falconVoices = new FalcVoice[NUM_VOICE_CHANNELS];
	for ( i = 0; i < NUM_VOICE_CHANNELS; i++ )
	{
		falconVoices[i].CreateVoice();
		falconVoices[i].SetVoiceChannel( i );
		falconVoices[i].InitCompressionData(  );
		falconVoices[i].PlayVoices();
		PauseChannel(i);
		decompQueue[i].status = SLOT_IS_AVAILABLE;
		decompQueue[i].conversations = NULL;
		decompQueue[i].filter = 0;
		decompQueue[i].message = -1;
		decompQueue[i].priority = 0;
	};

	radiofilter[0] = rcfPackage1;
	radiofilter[1] = rcfTeam;
	currRadio = 0;
	
	CallVoiceThread();
	
	return( TRUE );
}

int VoiceManager::VoiceOpen( void )
{
	char	filename[MAX_PATH];
	
	sprintf(filename,"%s\\falcon.tlk",FalconSoundThrDirectory);
#if 0
	voiceMapPtr = (char *)map_file(filename);
#endif
	if (voiceMap.Open(filename) != TRUE)
	    ShiError("Can't open falcon.tlk");
	return TRUE;
}

void VoiceManager::CallVoiceThread( void )
{
	DWORD	dwIDThread;
	
	strcpy( VMWakeEventName, "VoiceWakeupCall" );
	VMWakeEventHandle = CreateEvent( NULL, FALSE, FALSE, VMWakeEventName );
	if ( !VMWakeEventHandle ) 
	{
		return;
	} 
	
	hThread = ( HANDLE ) _beginthreadex( NULL, 0, (unsigned int (__stdcall *)(void *)) VoiceManagementThread, 
		(LPVOID)falconVoices, 0, ( unsigned * ) &dwIDThread );
	
	if (hThread != NULL)
	{
// 2002-03-25 MN we need to set killThread to false if we opened a thread sucessfully because of theater switching, or we won't have voices
		killThread = false;	
		CloseHandle(hThread);
	}
	else
	{
		VMCleanup();
	}
}

int FilterMessage(CONVERSATION *node)
{
	int retval = FALSE;
	//DSP Hack for debugging
#ifdef _DEBUG
	//VM->radiofilter = rcfProx;
#endif

	if(!VM)
		return FALSE;

	if(!node || node->message == -1)
		return FALSE;

	if(FalconLocalSession->GetFlyState() != FLYSTATE_FLYING && SimDriver.RunningCampaign())
	{
		if(noUIcomms || FalconLocalSession->GetFlyState() != FLYSTATE_IN_UI)
			return FALSE;
		//else 
			//return TRUE;
	}

	//MI added check to make it only work for the radio we've selected
	//if(VM->Radio() == 0)
	//{
		switch(VM->radiofilter[0])
		{
		case rcfOff:
			retval = FALSE;
			break;
		case rcfFlight5:
		case rcfFlight1:
		case rcfFlight2:
		case rcfFlight3:
		case rcfFlight4:
			if(TOFROM_FLIGHT & node->filter)
				retval = TRUE;
			break;
		case rcfPackage5:
		case rcfPackage1:
		case rcfPackage2:
		case rcfPackage3:
		case rcfPackage4:
			if( (TO_PACKAGE & node->filter) || (node->filter & TOFROM_FLIGHT) )
				retval = TRUE;
			break;
		case rcfFromPackage:
			if( (TOFROM_PACKAGE & node->filter) || (node->filter & TOFROM_FLIGHT) )
				retval = TRUE;
			break;
		case rcfProx:
			if( (node->filter & TOFROM_FLIGHT) || ( (IN_PROXIMITY & node->filter) && ((node->filter & TO_TEAM) || (TO_PACKAGE & node->filter)) ) )
				retval = TRUE;
			break;
		case rcfTeam:
			if( (TO_TEAM & node->filter) || (node->filter & TOFROM_FLIGHT) || (TOFROM_PACKAGE & node->filter))
				retval = TRUE;
			break;
		case rcfAll:
			if( (TO_WORLD & node->filter) || (node->filter & TOFROM_FLIGHT) || (TOFROM_PACKAGE & node->filter) || (TO_TEAM & node->filter))
				retval = TRUE;
			break;
		case rcfTower:
			if(node->filter & TOFROM_FLIGHT)
				retval = TRUE;
			else if( (TOFROM_TOWER & node->filter) && gNavigationSys)
			{
				VU_ID	ATCId;
				gNavigationSys->GetAirbase(&ATCId);
				
				if(ATCId == node->from || ATCId == node->to)
					//return TRUE;
					retval = TRUE;
			}
			break;
		}
	//}

	//MI added check to make it only work for the radio we've selected
	//else if(VM->Radio() == 1)
	//{
	//if(!retval)
	//{
		switch(VM->radiofilter[1])
		{
		case rcfOff:
			retval = FALSE;
			break;
		case rcfFlight5:
		case rcfFlight1:
		case rcfFlight2:
		case rcfFlight3:
		case rcfFlight4:
		
			if(TOFROM_FLIGHT & node->filter)
				retval = TRUE;
			break;
		case rcfPackage5:
		case rcfPackage1:
		case rcfPackage2:
		case rcfPackage3:
		case rcfPackage4:
		
			if( (TO_PACKAGE & node->filter) || (node->filter & TOFROM_FLIGHT) )
				retval = TRUE;
			break;
		case rcfFromPackage:
			if( (TOFROM_PACKAGE & node->filter) || (node->filter & TOFROM_FLIGHT) )
				retval = TRUE;
			break;
		case rcfProx:
			if( (node->filter & TOFROM_FLIGHT) || ( (IN_PROXIMITY & node->filter) && ((node->filter & TO_TEAM) || (TO_PACKAGE & node->filter)) ) )
				retval = TRUE;
			break;
		case rcfTeam:
			if( (TO_TEAM & node->filter) || (node->filter & TOFROM_FLIGHT) || (TOFROM_PACKAGE & node->filter))
				retval = TRUE;
			break;
		case rcfAll:
			if( (TO_WORLD & node->filter) || (node->filter & TOFROM_FLIGHT) || (TOFROM_PACKAGE & node->filter) || (TO_TEAM & node->filter))
				retval = TRUE;
			break;
		case rcfTower:
			if(node->filter & TOFROM_FLIGHT)
				retval = TRUE;
			else if( (TOFROM_TOWER & node->filter) && gNavigationSys && gTacanList)
			{
				VU_ID	ATCId;
				gNavigationSys->GetAirbase(&ATCId);
				
				if(ATCId == node->from || ATCId == node->to)
					//return TRUE;
					retval = TRUE;
			}
			break;
		}
	//}
	//}
	return retval;
}

DWORD WINAPI VoiceManagementThread( LPVOID lpvThreadParm ) 
{
	int					i, curChannel, curBuffer=0,sleep, breakin;
	VOICE_STREAM_BUFFER *outputBuf;
	DWORD				sleeptime;
	static VU_TIME oldtime = 0,waketime;
	int ticks = 0;
	ulong	holdTime;
	VU_ID playerID; // sfr: player ID	
	VM_CONVLIST *pVC,*best,*pVCnext;

	while( !killThread ){

		curChannel = 0;
		sleep = FALSE;
		waketime = 0;
		set_spinner3 (ticks++);

		F4EnterCriticalSection( VM->vmCriticalSection );
		playerID = (FalconLocalSession == NULL) ? FalconNullId : FalconLocalSession->GetPlayerEntityID();
		// lower channels have higher priority to play
		for ( i = NUM_VOICE_CHANNELS -1 ; i > -1 ; --i){
			// best is the best conversation to be played
			best = NULL;

			// we go through all conversations in channel list, trying to find best one
			// conversations are organized by time to play, so we only use an older one
			// if its priority is higher
			// priorities:
			// 1- from player
			// 2- to player
			// 3- priority field
			for (pVC = voiceChannelQueue[i]; pVC && (pVC->node->playTime <= vuxGameTime); pVC = pVCnext){
				// we need this to get next if current message gets removed
				pVCnext = pVC->next;

				// adjust conversation hold time based on priority
				if(pVC->node->priority){
					holdTime = MSG_HOLD_TIME/pVC->node->priority;
				}
				else{
					holdTime = MSG_HOLD_TIME;
				}

				// remove old conversation
				if (pVC->node->playTime + holdTime < vuxGameTime){
					// old message
					VM->VMListRemoveVCQ(&voiceChannelQueue[i], pVC);
				}
				// filter message and check if its best
				else if (FilterMessage(pVC->node)){
					if (!best){
						// no best yet, its the best
						best = pVC;
					}
					// sfr removing best test and merging 2 ifs in one
					else if(best->node->from == playerID){
						// best message is from player
						if (
							(pVC->node->from == playerID) && 
							(pVC->node->priority > best->node->priority)
						){
							// current is also and has higher priority, remove it and make new current one
							VM->VMListRemoveVCQ(&voiceChannelQueue[i], best);
							best = pVC;
						}
						else {
							// current has lower priority, remove it
							VM->VMListRemoveVCQ(&voiceChannelQueue[i], pVC);
						}
					}
					else if(best->node->to == playerID){
						// best is to player
						if (
							(pVC->node->to == playerID) && 
							// sfr: was < here!!
							(pVC->node->priority > best->node->priority)
						){
							// current too, but has higher priority
							// set as new best
							best = pVC;
						}
					}
					else if (
						(pVC->node->from == playerID) || 
						(pVC->node->to == playerID) ||
						(pVC->node->priority > best->node->priority)
					){
						// best is not from player nor to player and current is
						// or has highter priority
						best = pVC;
					}
				}
			}

			// sfr: this breakin logic needs to be reviewed! Its not usign same as best choice!!
			// now we check if the best conversation is enough to break a current ongoin one
			// breaking means we are interrupting an ongoing conversation to play a new one
			breakin = FALSE;
			if(SimDriver.GetPlayerAircraft() && (best != NULL)){	
				if(	
					(
						// message from us 
						(VM->decompQueue[i].from != FalconLocalSession->GetPlayerEntityID()) &&
						(best->node->priority == rpLifeThreatening) && 
						(VM->decompQueue[i].priority != rpLifeThreatening)
					) ||
					(
						best->node->from == FalconLocalSession->GetPlayerEntityID()
					) ||
					(
						(best && best->node->priority == rpLifeThreatening) && 
						(VM->decompQueue[i].priority != rpLifeThreatening)
					)
				){
					breakin = TRUE;
				}
			}

			// here we know conversation to be played (best) for the channel
			// and if it breaks an ongoing one (breakin)
			// sfr: it seems to me this is inverted, i <= curChannel
			if (i >= curChannel){
				//if there is a message queued from PlayRadioMessage and the appropriate decompQueue is available
				if (
					best &&  
					(VM->decompQueue[i].status == SLOT_IS_AVAILABLE  || breakin) && 
					!VM->falconVoices[curChannel].exitChannel 
				){
					// we have a message to be played with an available slot or 
					// we must break an ongoing conversation at the channel

					//if we're breaking in let's clean up first
					if(VM->decompQueue[i].status != SLOT_IS_AVAILABLE){
						delete [] VM->decompQueue[i].conversations;
						VM->decompQueue[i].conversations = NULL;
						VM->decompQueue[i].from = FalconNullId;
						VM->decompQueue[i].to = FalconNullId;
						VM->decompQueue[i].message = -1;
						// sfr: whats is this firing thing????
						if(best->node->message != rcFIRING){
							VM->falconVoices[i].BufferEmpty(0);
							VM->falconVoices[i].BufferEmpty(1);
						}
					}

					// this tells the Action Camera System whose current message is from
					gVmPlayVU_ID = best->node->from;

					sleep = TRUE;
					//VM->ResumeChannel(i);
					VM->falconVoices[i].silenceWritten = 0;
					// place buffers in channel buffer queue
					VM->falconVoices[i].PopVCAddQueue();
					
					//copy radio message info into decompQueue
					memcpy(&VM->decompQueue[i], best->node, sizeof(CONVERSATION));
					VM->decompQueue[i].conversations = new short[VM->decompQueue[i].sizeofConv];
					
					//copy the actual fragFiles needed into decompQueue->conversations
					memcpy(VM->decompQueue[i].conversations, best->node->conversations, sizeof(short)*VM->decompQueue[i].sizeofConv);
					
					//tell decompression routine that a message is available for this channel
					VM->decompQueue[i].status = MESG_IS_AVAILABLE;
					
					//remove the message we added from the voiceChannelQueue
					VM->VMListRemoveVCQ(&voiceChannelQueue[i], best);

					// delete the lower channels decomp queue 
					// since we are already the current channel, we can delete it... is this correct?
					// sfr: wtf is this for??????
					for(int j = 0;j<i;++j) {						
						delete [] VM->decompQueue[j].conversations;
						VM->decompQueue[j].from = FalconNullId;
						VM->decompQueue[j].to = FalconNullId;
						VM->decompQueue[j].message = -1;
						VM->decompQueue[j].conversations = NULL;
						VM->falconVoices[j].BufferEmpty(0);
						VM->falconVoices[j].BufferEmpty(1);
						VM->decompQueue[j].status = SLOT_IS_AVAILABLE;
					}				
				}

				// if we placed something in this channel, it will be the current channel
				if (VM->decompQueue[i].status != SLOT_IS_AVAILABLE){
					curChannel = i;
				}
			}
			else if (voiceChannelQueue[i] && (waketime > voiceChannelQueue[i]->node->playTime)){
				// set wake time for this channel event
				waketime = voiceChannelQueue[i]->node->playTime;
			}
		} // end for each channel
		F4LeaveCriticalSection(VM->vmCriticalSection);

		// at this point we have both channels set to
		// play a new message
		// continue an ongoing message
		// do nothing

		//if there are any buffers on the queue we need to fill them if we can and remove
		//them from the queue if there is nothing to process
		if ( voiceBufferQueue != NULL ){
			int buffChnl;
			bool leave = false;

			F4EnterCriticalSection(VM->vmCriticalSection);
			if(voiceBufferQueue->node){
				curBuffer = voiceBufferQueue->node->buff;
				buffChnl = voiceBufferQueue->node->channel;
				// sfr: it seems this get called twice (here and after the continue below)
				VM->falconVoices[buffChnl].BufferEmpty(curBuffer);
				if(curChannel != buffChnl){
					leave = true;
				}
			}
			else{
				// is this even possible??
				leave = true;
			}
			voiceBufferQueue = VM->VMListRemoveVMBQ(voiceBufferQueue);	

			if (leave){
				F4LeaveCriticalSection(VM->vmCriticalSection);
				continue; // begin thread loop again...
			}
			
			// sfr: already called above
			// VM->falconVoices[buffChnl].BufferEmpty(curBuffer);

			// message was made available by first part of loop, or we are done with
			// previous part of the conversation
			if (VM->decompQueue[curChannel].status == MESG_IS_AVAILABLE){
				VM->decompQueue[curChannel].status = MESG_IS_PROCESSING;
				// sfr: i find it weird we call resume before loading data...
				//VM->ResumeChannel(curChannel);
				//MonoPrint("Decompression started channel: %d message: %d index: %d  time: %d\n",curChannel,VM->decompQueue[curChannel].message, VM->decompQueue[curChannel].convIndex, VM->decompQueue[curChannel].playTime);
				//this is where the conversation index is incremented
				//this function just sets the data needed by ReadLHSP
				VM->LoadCompressionData(curChannel);
				// sfr: moved resume here...
				VM->ResumeChannel(curChannel);
			}
			
			//need to load up the decompression buffer
			//with the compressed data so DSOUND can get to it
			if ( VM->decompQueue[curChannel].status == MESG_IS_PROCESSING ){
				outputBuf = VM->falconVoices[curChannel].GetVoiceBuffer(curBuffer);
				
				//this is where the current buffer is loaded with the uncompressed
				//data so DSOUND can get it
				outputBuf->waveBufferWrite = VM->lhspPtr->ReadLHSPFile(
					VM->falconVoices[curChannel].voiceCompInfo ,
					&outputBuf->waveBuffer
				);
				
				//if something was written continue on with this conv index
				//else mark buffer empty and if it was the last index in the
				//conversation we need to delete the conversation and make the
				//decompQueue for this channel available
				
				outputBuf->dataInWaveBuffer = outputBuf->waveBufferWrite;
				outputBuf->waveBufferLen = outputBuf->waveBufferWrite;
				outputBuf->waveBufferRead = 0;

				VM->AddNoise(outputBuf, VM->decompQueue[curChannel].from, curChannel);

				// check if we read everything
				if (VM->falconVoices[curChannel].voiceCompInfo->bytesRead == VM->falconVoices[curChannel].voiceCompInfo->compFileLength){
					if (VM->decompQueue[curChannel].convIndex == VM->decompQueue[curChannel].sizeofConv){
						VM->decompQueue[curChannel].status = ADD_SILENCE;
						// last part of conversation read, add silence
						delete [] VM->decompQueue[curChannel].conversations;
						VM->decompQueue[curChannel].conversations = NULL;
						VM->decompQueue[curChannel].from = FalconNullId;
						VM->decompQueue[curChannel].to = FalconNullId;
						VM->decompQueue[curChannel].message = -1;
						// buffer is full now
						VM->falconVoices[curChannel].BufferManager( curBuffer );
					}
					else {
						// conversation index was incremented already during load compression data
						VM->decompQueue[curChannel].status = MESG_IS_AVAILABLE;
						// process next part of conversation
						VM->falconVoices[curChannel].BufferManager( curBuffer );
					}
				}
				// sfr: i think we dont need this else, since state is already that and buffer is filled
				/*else {
					VM->falconVoices[curChannel].BufferManager( curBuffer );
					VM->decompQueue[curChannel].status = MESG_IS_PROCESSING;
				}*/
			}
			// sfr: is this really an else??? why the first one is not?
			else if(VM->decompQueue[curChannel].status == ADD_SILENCE){
				if(VM->BuffersEmpty(curChannel)){
					VM->decompQueue[curChannel].status = SILENCE_ADDED;
					outputBuf = VM->falconVoices[curChannel].GetVoiceBuffer(curBuffer);
					//try to add a little silence at end of message
					memset(outputBuf->waveBuffer, SILENCE_KEY, SILENCE_LEN);
					outputBuf->waveBufferRead = 0; // sfr: zero read
					outputBuf->dataInWaveBuffer = outputBuf->waveBufferWrite = outputBuf->waveBufferLen = SILENCE_LEN;
					VM->falconVoices[curChannel].BufferManager(curBuffer);
					// sfr: resume channel again... it may have gotten inactive
					//VM->ResumeChannel(curChannel); 
				}
			}
			// sfr: again, else??
			else if(VM->decompQueue[curChannel].status == SILENCE_ADDED){
				if(VM->BuffersEmpty(curChannel)){
					VM->decompQueue[curChannel].status = SLOT_IS_AVAILABLE;
				}
			}

			// if the other buffer is not full, wake immediatly
			if (VM->falconVoices[curChannel].voiceBuffers[1 - curBuffer].status != BUFFER_FILLED){
				SetEvent( VMWakeEventHandle );
			}

			F4LeaveCriticalSection( VM->vmCriticalSection );
		}
		// all buffers consumed in queue
		// now we place each one back which is not filled and is not in queue
		// ie: buffer.status == BUFFER_NOT_IN_QUEUE
		else {
			for (i = 0; i < NUM_VOICE_CHANNELS; i++){
				if(VM->decompQueue[i].status != SLOT_IS_AVAILABLE){
					VM->falconVoices[i].PopVCAddQueue();
				}
			}
		}

		// sfr: organized logic here
		if (!sleep && waketime){
			sleeptime = waketime - vuxGameTime;//should be divided by time compression
		}
		else {
			//sleeptime = INFINITE;
			sleeptime = 2000;
		}
		WaitForSingleObject( VMWakeEventHandle, sleeptime );
	}

	return(0);
	lpvThreadParm;
}

int VoiceManager::LoadCompressionData( int curChannel )
{
	int					playConv;
	
	/* Get Conversations file no. and index to next file no. */
	playConv = decompQueue[curChannel].conversations[decompQueue[curChannel].convIndex];
	decompQueue[curChannel].convIndex++;
	
	falconVoices[curChannel].InitCompressionFile();

#if 0 // jpo old code
	long				tlkBlock;
	tlkBlock = TLK_HEADER_INFO + ( sizeof( long ) * ( playConv ) );

	ShiAssert(tlkBlock >= 0);

	tlkBlock = *((unsigned long *)(voiceMapPtr + tlkBlock));
	falconVoices[curChannel].voiceCompInfo->fileLength = *( (unsigned long *)(voiceMapPtr + tlkBlock) );
	falconVoices[curChannel].voiceCompInfo->compFileLength = *( (unsigned long *)(voiceMapPtr + tlkBlock + sizeof(unsigned long)) );
	falconVoices[curChannel].voiceCompInfo->dataPtr = (voiceMapPtr + tlkBlock + sizeof(unsigned long) * 2);
#endif

	falconVoices[curChannel].voiceCompInfo->fileLength = voiceMap.GetFileLength(playConv);
	falconVoices[curChannel].voiceCompInfo->compFileLength = voiceMap.GetCompressedLength(playConv);
	falconVoices[curChannel].voiceCompInfo->dataPtr = voiceMap.GetDataPtr(playConv);
	return( playConv );
}

void VoiceManager::AddToConversationQueue( CONVERSATION *newConv )
{
	CONVERSATION *listConv;
	
	//MonoPrint("Adding Message To Queue: %d  Channel: %d\n", newConv->message, newConv->channelIndex);

	if ( newConv->channelIndex >= NUM_VOICE_CHANNELS )
	{
		delete [] newConv->conversations;
		return;
	}

	//if(decompQueue[newConv->channelIndex].from == newConv->from && decompQueue[newConv->channelIndex].message == newConv->message)
	//	return;
	
	// must change currConversation to a list for voice manager
	listConv = new CONVERSATION;
	ShiAssert(listConv);
	
	memcpy( listConv, newConv, sizeof( CONVERSATION ) );
	
	listConv->conversations = newConv->conversations;
	
	// if override and VCQ is not null, reomve all conv from channel
	//	falconVoices[newConv->channelIndex].status = listConv->interupt; //convStruct->interupt;
	//	if overRide, need to destroy current queue and start again
	
	listConv->status = SLOT_IS_AVAILABLE;
	
	if( listConv->interrupt == OVERRIDE_CONV )
	{		
		F4EnterCriticalSection( VM->vmCriticalSection );
		if ( voiceChannelQueue[newConv->channelIndex] != NULL )
		{
			voiceChannelQueue[newConv->channelIndex] = VMListDestroyVCQ( voiceChannelQueue[newConv->channelIndex] );
		}
		F4LeaveCriticalSection( VM->vmCriticalSection );
	}
		
	VM_CONVLIST *newnode = new VM_CONVLIST;
	newnode->node = listConv;
	newnode->next = NULL;
	newnode->prev = NULL;
	
	
	F4EnterCriticalSection( VM->vmCriticalSection );
	voiceChannelQueue[newConv->channelIndex] = VMConvListInsert( voiceChannelQueue[newConv->channelIndex],newnode,SORT_TIME);
	F4LeaveCriticalSection( VM->vmCriticalSection );
	
	SetEvent( VMWakeEventHandle );
	
	newConv->status = SLOT_IS_AVAILABLE;
}

//aargh wrapper function
void ResetVoices(void)
{
	if (VM)
	{
		VM->VMResetVoices();
	}
}

void VoiceManager::VMResetVoice( int channel )
{
	F4EnterCriticalSection( vmCriticalSection );
	
	int i;

	if ( voiceBufferQueue != NULL )
	{
		voiceBufferQueue = VMListDestroyVBQ( voiceBufferQueue );
		voiceBufferQueue = NULL;
	}

	for(i = 0; i < NUM_VOICE_CHANNELS; i++)
	{
		if(i != channel)
			falconVoices[1 - channel].PopVCAddQueue();
	}

	decompQueue[channel].status = SLOT_IS_AVAILABLE;
	if(decompQueue[channel].conversations != NULL)
	{
		delete [] decompQueue[channel].conversations;
		decompQueue[channel].conversations	= NULL;
		decompQueue[channel].from				= FalconNullId;
		decompQueue[channel].message			= -1;
	}
	

	if ( voiceChannelQueue[channel] != NULL )
	{
		voiceChannelQueue[channel] = VMListDestroyVCQ( voiceChannelQueue[channel] );
		voiceChannelQueue[channel] = NULL;
	}
	if ( falconVoices )
		falconVoices[channel].ResetBufferStatus();

	for(i = 0; i<MAX_VOICE_BUFFERS;i++)
	{
		F4EnterCriticalSection( falconVoices[channel].voiceBuffers[i].criticalSection );
		memset(falconVoices[channel].voiceBuffers[i].waveBuffer, SILENCE_KEY, 8000 );
		F4LeaveCriticalSection(falconVoices[channel].voiceBuffers[i].criticalSection);

		falconVoices[channel].voiceBuffers[i].status = BUFFER_FILLED;
		falconVoices[channel].PopVCAddQueue();
	}
	falconVoices[channel].exitChannel = FALSE;

	F4LeaveCriticalSection( vmCriticalSection );
}

void VoiceManager::VMResetVoices( void )
{
	int	i;
	
	VMSilenceVoices();
	
	if ( voiceBufferQueue != NULL )
	{
		F4EnterCriticalSection( vmCriticalSection );
		voiceBufferQueue = VMListDestroyVBQ( voiceBufferQueue );
		voiceBufferQueue = NULL;
		F4LeaveCriticalSection( vmCriticalSection );
	}
	
	for ( i = 0; i < NUM_VOICE_CHANNELS; i++ )
	{
		F4EnterCriticalSection( vmCriticalSection );

		decompQueue[i].status = SLOT_IS_AVAILABLE;
		decompQueue[i].message			= -1;
		decompQueue[i].from				= FalconNullId;
		if(decompQueue[i].conversations != NULL)
		{
			delete [] decompQueue[i].conversations;
			decompQueue[i].conversations = NULL;
		}
		

		if ( voiceChannelQueue[i] != NULL )
		{
			voiceChannelQueue[i] = VMListDestroyVCQ( voiceChannelQueue[i] );
			voiceChannelQueue[i] = NULL;
		}
		if ( falconVoices )
			falconVoices[i].ResetBufferStatus();

		F4LeaveCriticalSection( vmCriticalSection );

		for(int j = 0; j<MAX_VOICE_BUFFERS;j++)
		{
			F4EnterCriticalSection( falconVoices[i].voiceBuffers[j].criticalSection );
			memset(falconVoices[i].voiceBuffers[j].waveBuffer, SILENCE_KEY, 8000 );
			falconVoices[i].voiceBuffers[j].status = BUFFER_FILLED;
			F4LeaveCriticalSection(falconVoices[i].voiceBuffers[j].criticalSection);

			falconVoices[i].PopVCAddQueue();
		}
		if (g_nSoundSwitchFix & 0x02) // I assume that garbage in this after theater switch
			falconVoices[i].exitChannel = FALSE; // can cause voices to fail to play...
	}
	radiofilter[0] = rcfPackage1;
	radiofilter[1] = rcfTeam;

	VMHearVoices();
}
/*
void VoiceManager::VMResumeVoiceStreams( void )
{
	int	i;
	
	if ( falconVoices )
	{
		for ( i = 0; i < NUM_VOICE_CHANNELS; i++ )
		{
			falconVoices[i].FVResumeVoiceStreams();
		}
	}
}*/

void VoiceManager::VMHearVoices( void )
{
	int i;
	for ( i = 0; i < NUM_VOICE_CHANNELS; i++ )
		falconVoices[i].UnsilenceVoices(i + COM1_SOUND_GROUP);
}

void VoiceManager::VMHearChannel( int channel )
{
	if(channel < NUM_VOICE_CHANNELS && channel >= 0 )
		falconVoices[channel].UnsilenceVoices(channel + COM1_SOUND_GROUP);
}

void VoiceManager::VMSilenceVoices( void )
{
	int i;
	for ( i = 0; i < NUM_VOICE_CHANNELS; i++ )
		falconVoices[i].SilenceVoices();
}

void VoiceManager::VMSilenceChannel( int channel )
{	
	if(channel < NUM_VOICE_CHANNELS && channel >= 0 )
		falconVoices[channel].SilenceVoices();
}
void VoiceManager::VMCleanup( void )
{
	int i;

	killThread = TRUE;
	SetEvent( VMWakeEventHandle );
	
	F4EnterCriticalSection( vmCriticalSection );
	if ( voiceBufferQueue != NULL )
	{
		voiceBufferQueue = VMListDestroyVBQ( voiceBufferQueue );
		voiceBufferQueue = NULL;
	}

	for ( i = 0; i < NUM_VOICE_CHANNELS; i++ )
	{
		if ( voiceChannelQueue[i] != NULL )
		{
			voiceChannelQueue[i] = VMListDestroyVCQ( voiceChannelQueue[i] );
			voiceChannelQueue[i] = NULL;
		}

		if(VM->decompQueue[i].conversations != NULL)
		{
			delete [] VM->decompQueue[i].conversations;
			VM->decompQueue[i].conversations = NULL;
		}
	}
	if ( falconVoices != NULL )
		delete [] falconVoices;
	
	falconVoices = NULL;	
	delete lhspPtr;
	
	F4LeaveCriticalSection( vmCriticalSection );

	// edg: destroy critical section for voicemanager
	if ( vmCriticalSection != NULL )
	{
		F4DestroyCriticalSection( vmCriticalSection );
		vmCriticalSection = NULL;
	}
	voiceMap.Close();
}

void VoiceManager::VMAddBuffToQueue( int channel, int buffer )
{
	VMBuffQueue *vmBuffer = NULL;
	
	vmBuffer = new VMBuffQueue;
	
	if ( vmBuffer )
	{
		vmBuffer->channel = channel;
		vmBuffer->buff = buffer;
		
		F4EnterCriticalSection( vmCriticalSection );
		voiceBufferQueue = VMBuffListAppend( voiceBufferQueue, vmBuffer );
		F4LeaveCriticalSection( vmCriticalSection );
	}
}

VM_CONVLIST *VoiceManager::VMConvListInsert( VM_CONVLIST *list,VM_CONVLIST *newnode,int insType)
{
	if(!newnode)
		return list;
	
	VM_CONVLIST *prev = NULL;
	VM_CONVLIST *cur = list;
	
	

	switch(insType)
	{
	case SORT_TIME:
		while(cur && (newnode->node->playTime >= cur->node->playTime ) )
		{
			prev=cur;
			cur=cur->next;
		}
		if(!prev)
		{
			newnode->next = list;
			if(list)
				list->prev = newnode;
			return newnode;
		}
		newnode->next = cur;
		newnode->prev = prev;
		prev->next = newnode;
		if(cur)
			cur->prev = newnode;		
		return list;	
		
		break;
	case SORT_TIME_PRIORITY:
		while(cur && (newnode->node->playTime >= cur->node->playTime)  )
		{
			prev=cur;
			cur=cur->next;
		}
		while(cur && (newnode->node->priority <= cur->node->priority) &&
			(newnode->node->playTime == cur->node->playTime) )
		{
			prev=cur;
			cur=cur->next;
		}
		
		if(!prev)
		{
			newnode->next = list;
			if(list)
				list->prev = newnode;
			return newnode;
		}
		newnode->next = cur;
		newnode->prev = prev;
		prev->next = newnode;
		if(cur)
			cur->prev = newnode;
		
		return list;
		break;
	default:
		return list;
	}
}

VM_BUFFLIST *VoiceManager::VMBuffListAppend( VM_BUFFLIST *list, VMBuffQueue *node )
{
	VM_BUFFLIST * newnode;
	VM_BUFFLIST * tmpPtr = list;
	VM_BUFFLIST * newlist = list;
	
	newnode = new VM_BUFFLIST;
	
	newnode->node = node;
	newnode->next = NULL;
	newnode->prev = NULL;
	
	F4EnterCriticalSection( vmCriticalSection );
	
	if ( tmpPtr != NULL )
	{
		while( tmpPtr->next != NULL )
			tmpPtr = tmpPtr->next;
		
		newnode->prev = tmpPtr;
		tmpPtr->next = newnode;
		
	}
	else
	{
		newlist = newnode;
	}
	

	F4LeaveCriticalSection( vmCriticalSection );
	
	return( newlist );
}

void VoiceManager::RemoveDuplicateMessages ( VU_ID from, VU_ID to, int msgid)
{
	VM_CONVLIST *pVC,*pVCnext;

	F4EnterCriticalSection( vmCriticalSection );

	for (int i = 0 ; i < NUM_VOICE_CHANNELS ; i++ )
	{
		pVCnext = NULL;
		pVC = voiceChannelQueue[i];

		while(pVC)
		{
			pVCnext = pVC->next;
			// Fix for Weapons Call
			if(pVC->node->from == from && pVC->node->to == to && pVC->node->message == msgid && pVC->node->message != rcDAMREPORT && pVC->node->message != rcWEAPONSCHECKRSP)
				VMListRemoveVCQ(&voiceChannelQueue[i], pVC);
			pVC = pVCnext;
		}
	}

	F4LeaveCriticalSection( vmCriticalSection );
}

int VoiceManager::IsMessagePlaying ( VU_ID from, VU_ID to, int msgid)
{
	int retval = FALSE;

	F4EnterCriticalSection( vmCriticalSection );

	for (int i = 0 ; i < NUM_VOICE_CHANNELS ; i++ )
	{
		if( decompQueue[i].from == from &&  decompQueue[i].to == to && decompQueue[i].message == msgid)
		{
			retval = TRUE;
			break;
		}
	}

	F4LeaveCriticalSection( vmCriticalSection );

	return retval;
}

//always call from inside vmCriticalSection
void VoiceManager::VMListRemoveVCQ( VM_CONVLIST **list, VM_CONVLIST *node )
{
	VM_CONVLIST		*curr, *next, *prev;

	//F4EnterCriticalSection( vmCriticalSection );
	
	if ( !*list )
		return;
	
	if(*list == node)
		*list = node->next;

	curr = node;
	prev = curr->prev;
	if(prev)
		prev->next = curr->next;
	next = curr->next;
	if ( next )
		next->prev = prev;

#ifdef DAVE_DBG
	//MonoPrint("Message removed from channel: %d message: %d\n", curr->node->channelIndex, curr->node->message);
#endif

	if ( curr->node )
	{
		delete [] curr->node->conversations;
		curr->node->conversations = NULL;
		delete curr->node;
		curr->node = NULL;
		
		delete curr;
		curr = NULL;
	}

	//F4LeaveCriticalSection( vmCriticalSection );
	return;
}

//removes the first entry in the list and returns the new list
VM_BUFFLIST *VoiceManager::VMListPopVMBQ( VM_BUFFLIST *list )
{
	VM_BUFFLIST	*next;
	
	if ( !list )
		return NULL;
	
	next = list->next;
	if(next)
		next->prev = NULL;
	
	delete list->node;
	list->node = NULL;
	
	delete list;
	list = NULL;

	return( next );
}

//removes the seleted entry from the list and returns the next pointer
// in case the first entry was removed
VM_BUFFLIST *VoiceManager::VMListRemoveVMBQ( VM_BUFFLIST *list )
{
	VMBuffQueue	*vmbqNode;
	VM_BUFFLIST	*curr, *next;
	
	if ( !list )
		return NULL;
	
	curr = list;
	next = list->next;
	if(next)
		next->prev = curr->prev;
	
	if ( curr->node )
	{
		vmbqNode = curr->node;
		delete vmbqNode;
		
		if ( curr->prev )
			curr->prev->next = curr->next;
		delete curr;
		curr = NULL;
	}
	
	return( next );
}

void VoiceManager::VMDeleteNode( VMBuffQueue* vmNode )
{	
	delete vmNode;
}
/*
int VoiceManager::VMListCount( VMLIST * list)
{
VMLIST	*curr;
int		i;

  for( i = 0, curr = list; curr; i++, curr = curr->prev );
  
	return( i );
}*/

VM_BUFFLIST *VoiceManager::VMListSearchVMBQ( VM_BUFFLIST *list, int channelNum, int searchType )
{
	VM_BUFFLIST	*l;
	
	if ( !list )
		return NULL;
	
	for( l = list; list; list = list->next )
	{
		switch ( searchType )
		{
		case SEARCH_AND_DESTROY:
			if ( !ListCheckChannelNum( list->node, channelNum ) )
			{
				list = VMListRemoveVMBQ( list );
				if ( !list )
					return NULL;
			}
			break;
		};
	}
	return( list );
}

int VoiceManager::ListCheckChannelNum( void *node_a, int channelNum )
{
	VMBuffQueue	*channel_a;
	
	if ( !node_a )
		return NULL;
	
	channel_a = ( VMBuffQueue * ) node_a;
	
	if( channel_a->channel == channelNum )
		return 0;
	
	return 1;
}

// believe I need to change the code to search through prev, not next
// also need to make sure I have null checks

//AAGh!! it's a bubble sort that's using the prev? pointer
/*
VMLIST *VoiceManager::VMListSort( VMLIST **list, int	sortType )
{
VMLIST	**parent_a;
VMLIST	**parent_b;
BOOL	sortCritMet = FALSE;

  for( parent_a = list; *parent_a; parent_a = &(*parent_a)->prev )
  {
		if ( !(*parent_a)->prev )
		break;
		for( parent_b = &(*parent_a)->prev; *parent_b; parent_b = &(*parent_b)->prev )
		{
		sortCritMet = FALSE;
		switch ( sortType )
		{
		case SORT_PRIORITY:
		if( VMListSortPriority(*parent_a, *parent_b) > 0 )
		sortCritMet = TRUE;
		break;
		case SORT_TIME:
		if( VMListSortTime(*parent_a, *parent_b) > 0 )
		sortCritMet = TRUE;
		break;
		};
		
		  if ( sortCritMet )
		  {
		  VMLIST *swap_a, *swap_a_child;
		  VMLIST *swap_b, *swap_b_child;
		  
			swap_a = *parent_a;
			swap_a_child = (*parent_a)->prev;
			swap_b  = *parent_b;
			swap_b_child = (*parent_b)->prev;
			
			  (*parent_a)->prev = swap_b_child;
			  (*parent_a) = swap_b;
			  
				if( swap_b == swap_a_child )
				{
				(*parent_a)->prev = swap_a;
				parent_b = &(*parent_a)->prev;
				}
				else 
				{
				(*parent_b)->prev = swap_a_child;
				(*parent_b) = swap_a;
				}
				}
				}
				}
				
				  return( *list );
}*/
/*
int VoiceManager::VMListSortPriority( VMLIST *parent_a, VMLIST *parent_b )
{
CONVERSATION	*list_a, *list_b;

  list_a = ( CONVERSATION * )parent_a->node;
  list_b = ( CONVERSATION * )parent_b->node;
  
	if ((!list_a)||(!list_b))
	return NULL;
	
	  if( list_a->priority < list_b->priority )
	  return 1;
	  
		return 0;
}*/
/*
int VoiceManager::VMListSortTime( VMLIST *parent_a, VMLIST *parent_b )
{
CONVERSATION	*list_a, *list_b;

  list_a = ( CONVERSATION * )parent_a->node;
  list_b = ( CONVERSATION * )parent_b->node;
  
	if ((!list_a)||(!list_b))
	return NULL;
	
	  if( (list_a->playTime) < (list_b->playTime ) )
	  return 1;
	  
		return 0;
}*/
/*
VMLIST *VoiceManager::VMListDestroyVCQ( VMLIST *list )
{
CONVERSATION	*vmVCNode;
VMLIST			*curr, *prev;

  if ( !list )
		return NULL;
		
		  curr = list;
		  prev = list->prev;
		  
			while ( curr )
			{
			if ( !curr->node )
			break;
			
			  vmVCNode = ( CONVERSATION * )curr->node;
			  
				delete [] vmVCNode->conversations;
				delete vmVCNode;
				if ( curr->prev )
				curr->prev->next = NULL;
				delete curr;
				
				  curr = prev;
				  }
				  list = curr;
				  return( list );
}*/

VM_CONVLIST *VoiceManager::VMListDestroyVCQ( VM_CONVLIST *list )
{
	CONVERSATION	*vmVCNode;
	VM_CONVLIST		*curr, *next;
	
	if ( !list )
		return NULL;
	// JPO - go FORWARDS through the list stupid!
	for (curr = list; curr; curr = next)
	{
	    next = curr->next;
	    if ( !curr->node )
		break;
	    
	    vmVCNode = curr->node;
	    
	    delete [] vmVCNode->conversations;
	    delete vmVCNode;
	    delete curr;
	}
	list = curr;
	return( list );
}

VM_BUFFLIST *VoiceManager::VMListDestroyVBQ( VM_BUFFLIST *list )
{
	VMBuffQueue	*vmbqNode;
	VM_BUFFLIST	*curr, *next;
	
	if ( !list )
	    return NULL;
	
	// JPO - go FORWARDS through the list stupid!
	for (curr = list; curr; curr = next)
	{
	    next = curr->next;
	    if ( !curr->node )
		break;
	    
	    vmbqNode = curr->node;
	    delete vmbqNode;
	    delete curr;
	}
	list = curr;
	return( list );
}


int VoiceManager::GetRadioFreq(int radio)
{
	if (this)
	{
		if(radio >= 0 && radio <= 1)
			return radiofilter[radio];

		return radiofilter[0];
	}
	else
	{
		return 0;
	}
}


void VoiceManager::ForwardCycleFreq( int radio )
{
	if(radio < 0 || radio > 1 || !this) {
		return;
	}

	if(radiofilter[radio] == rcfTower) {
		ChangeRadioFreq( rcfOff, radio );
	}
	else {
		ChangeRadioFreq( radiofilter[radio] + 1, radio );
	}
}


void VoiceManager::BackwardCycleFreq( int radio )
{
	if(radio < 0 || radio > 1 || !this) {
		return;
	}

	if(radiofilter[radio] == rcfOff) {
		ChangeRadioFreq( rcfTower, radio );
	}
	else {
		ChangeRadioFreq( radiofilter[radio] - 1, radio );
	}
}


void VoiceManager::ChangeRadioFreq( int filter, int radio )
{
	F4EnterCriticalSection( VM->vmCriticalSection );

	if(radio >= 0 && radio <= 1)
		radiofilter[radio] = filter;

	for(int i = 0; i < NUM_VOICE_CHANNELS; i++ )
	{
		if(!FilterMessage(&VM->decompQueue[i]))
		{
			delete [] VM->decompQueue[i].conversations;
			VM->decompQueue[i].conversations = NULL;
			VM->decompQueue[i].status = SLOT_IS_AVAILABLE;
			VM->falconVoices[i].BufferEmpty( 0 );
			VM->falconVoices[i].BufferEmpty( 1 );
		}
	}

	F4LeaveCriticalSection( VM->vmCriticalSection );
}


int	VoiceManager::IsChannelDone( int channel )
{
	if (gSoundDriver)
	{
		return !gSoundDriver->IsStreamPlaying(VM->falconVoices[channel].FalcVoiceHandle);
	}

	return FALSE;
}

int	VoiceManager::ResumeChannel( int channel )
{
	if(gSoundDriver)
	{
		if(!gSoundDriver->IsStreamPlaying(VM->falconVoices[channel].FalcVoiceHandle) )
		{
			gSoundDriver->ResumeStream(VM->falconVoices[channel].FalcVoiceHandle);
		}
		return TRUE;
	}
	return FALSE;
}

int	VoiceManager::PauseChannel( int channel )
{
	if(gSoundDriver)
	{
		gSoundDriver->PauseStream(VM->falconVoices[channel].FalcVoiceHandle);
		return TRUE;
	}
	return FALSE;
}


int	VoiceManager::BuffersEmpty( int channel )
{
	return ( 
		VM->falconVoices[channel].voiceBuffers[0].status == BUFFER_NOT_IN_QUEUE &&
		VM->falconVoices[channel].voiceBuffers[1].status == BUFFER_NOT_IN_QUEUE
	);
}

void VoiceManager::SetChannelVolume(int channel, int volume)
{
	if ( channel >= 0 && channel < NUM_VOICE_CHANNELS && !falconVoices[channel].exitChannel )
	{
		F4SetStreamVolume(	falconVoices[channel].FalcVoiceHandle, volume);
	}
}

void VoiceManager::AddNoise(VOICE_STREAM_BUFFER *streamBuffer, VU_ID from, int channel)
{
	unsigned long i; 
	int level=255, minLevel=253, volume, nonoise;
	VuEntity *fromEnt = NULL;
	float dist, dx, dy , dz;
	SimBaseClass *ownship = OTWDriver.GraphicsOwnship();

	Flight	awacs = NULL;
	Flight	flight = NULL;
	nonoise = FALSE;

	if(SimDriver.GetPlayerEntity())
	{
		flight = (Flight)SimDriver.GetPlayerEntity()->GetCampaignObject();
		if(flight)
		{
			// AWACS/FAC callsign
			awacs = flight->GetFlightController();
			if (awacs && awacs->Id() == from)
				nonoise = TRUE;

		}
	}

	fromEnt = vuDatabase->Find(from);
	if(fromEnt && ownship && fromEnt != SimDriver.GetPlayerEntity() && !nonoise && SimDriver.InSim())
	{
		dx = fromEnt->XPos() - ownship->XPos();
		dy = fromEnt->YPos() - ownship->YPos();
		dz = fromEnt->ZPos() - ownship->ZPos();
		dist = (float)sqrt(dx*dx + dy*dy + dz*dz);

		dist = min(dist, MAX_RADIO_RANGE - 1.0F);

		minLevel = FloatToInt32(253 - dist/MAX_RADIO_RANGE * 50.0F);	
		
		volume = FloatToInt32(max(-10000,PlayerOptions.GroupVol[COM1_SOUND_GROUP + channel] - dist/MAX_RADIO_RANGE*2000));
		SetChannelVolume(channel, volume);
	}


	unsigned char  *pos = streamBuffer->waveBuffer;

	for(i = 0; i < streamBuffer->dataInWaveBuffer; i++)
	{
		if(!(i%50))
			level = minLevel - rand()%4 - rand()%4;

		if(*pos > level)
			*pos = (uchar)level;

		//if(level < 235)
		//	*pos += (1 - rand()%3);

		pos++;
	}
}

void VoiceManager::RemoveRadioCalls	( VU_ID dead )
{
	VM_CONVLIST *pVC, *pVCnext;
	int i;

	F4EnterCriticalSection( VM->vmCriticalSection );
	for ( i = 0 ; i < NUM_VOICE_CHANNELS ; i++ )
	{
		pVC = voiceChannelQueue[i];

		while(pVC)
		{
			pVCnext = pVC->next;
			if(pVC->node->from == dead)
				VMListRemoveVCQ(&voiceChannelQueue[i], pVC);
			pVC = pVCnext;
		}

		if( decompQueue[i].from == dead )
		{
			decompQueue[i].status = SLOT_IS_AVAILABLE;
			decompQueue[i].from = FalconNullId;
			decompQueue[i].to = FalconNullId;
			delete [] decompQueue[i].conversations;
			decompQueue[i].conversations = NULL;
			falconVoices[i].BufferEmpty( 0 );
			falconVoices[i].BufferEmpty( 1 );
		}
	}
	F4LeaveCriticalSection( VM->vmCriticalSection );

}

unsigned long TlkFile::GetFileLength(int tlkind)
{
    long blockind = GetFragIndex(tlkind);
    ShiAssert(blockind > 0);
    struct TlkBlock *tblock;
    tblock = (struct TlkBlock *) GetData(blockind, sizeof *tblock);
    ShiAssert(tblock != NULL);
    if (tblock == NULL) return 0;
    return tblock->filelen;
}

unsigned long TlkFile::GetCompressedLength(int tlkind)
{
    long blockind = GetFragIndex(tlkind);
    ShiAssert(blockind > 0);
    struct TlkBlock *tblock;
    tblock = (struct TlkBlock *) GetData(blockind, sizeof *tblock);
    ShiAssert(tblock != NULL);
    if (tblock == NULL) return 0;
    return tblock->compressedlen;
}

char *TlkFile::GetDataPtr(int tlkind)
{
    long blockind = GetFragIndex(tlkind);
    ShiAssert(blockind > 0);
    struct TlkBlock *tblock;
    tblock = (struct TlkBlock *) GetData(blockind, sizeof *tblock);
    ShiAssert(tblock != NULL);
    if (tblock == NULL) return 0;
    return tblock->data;
}