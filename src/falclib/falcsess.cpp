/** @file FalcSess.cpp
* Kevin Klemmick, 6/97
* Falcon's session subclass
*/

#include "f4vu.h"
#include "vusessn.h"
#include "tchar.h"
#include "FalcSess.h"
#include "Squadron.h"
#include "Flight.h"
#include "ui/include/uicomms.h"
#include "classtbl.h"
#include "ThreadMgr.h"
#include "sim/include/SimBase.h"
#include "Campaign.h"
#include "ui/include/queue.h"
#include "ui/include/logbook.h"
#include "GameMgr.h"
#include "MsgInc/SendAircraftSlot.h"
#include "Sim/Include/Simdrive.h"
#include "Sim/Include/OTWdrive.h"

//sfr: added for checks
#include "InvalidBufferException.h"

#define ACE_KILL(x,y) (log(exp(x) + y))
#define ACE_DEATH(x,y) (log(exp(x) - (((x * (x - y))>x*0.5F)?(x * (x - y)):x*0.5F)))

extern void SetTimeCompression (int newComp);
extern void SetLabel (SimBaseClass* theObject);
extern void CheckForNewPlayer (FalconSessionEntity *session);
extern ulong gCompressTillTime;
extern int F4SessionType;
extern void UI_Refresh (void);
extern int gGameType;

using namespace std;

// constructors & destructor
FalconSessionEntity::FalconSessionEntity(ulong domainMask, char *callsign) : VuSessionEntity(domainMask, callsign)
{
	//name = new _TCHAR[_NAME_LEN_];
	//_stprintf(name,"Kevin");
	_stprintf(name,LogBook.NameWRank());
	name[_NAME_LEN_] = 0;
	//callSign = new _TCHAR[_CALLSIGN_LEN_];
	//_stprintf(callSign,"DeathPup");
	_stprintf(callSign,LogBook.Callsign());
	callSign[_CALLSIGN_LEN_] = 0;
	playerSquadron = FalconNullId;
	playerFlight = FalconNullId;
	playerEntity = FalconNullId;
	// sfr: smartpointers
	//playerEntityPtr = NULL;
	//playerSquadronPtr = NULL;
	//playerFlightPtr = NULL;
	AceFactor = LogBook.AceFactor();
	initAceFactor = LogBook.AceFactor();
	memset(kills,0,sizeof(kills));
	rating=0;
	voiceID = 0;
	missions=0;
	country = 255;
	aircraftNum = 255;
	pilotSlot = 255;
	assignedPlayerFlightPtr = NULL;
	assignedPilotSlot = 255;
	assignedAircraftNum = 255;
	latency = 10;
	samples = 10;
	bubbleRatio=1.0F;
	reqCompression = 0;
	unitDataSendBuffer = NULL;
	unitDataSendSet = 0;
	unitDataSendSize = 0;
	unitDataReceiveBuffer = NULL;
	unitDataReceiveSet = 0;
	memset(unitDataReceived,0,FS_MAXBLK/8);
	objDataSendBuffer = NULL;
	objDataSendSet = 0;
	objDataSendSize = 0;
	objDataReceiveBuffer = NULL;
	objDataReceiveSet = 0;
	memset(objDataReceived,0,FS_MAXBLK/8);
	SetEntityType((ushort)(F4SessionType+VU_LAST_ENTITY_TYPE));
	flyState = FLYSTATE_IN_UI;
}

FalconSessionEntity::FalconSessionEntity(VU_BYTE** stream, long *rem) : VuSessionEntity (stream, rem)
{
	uchar			size;

	memcpychk (&size, stream, sizeof (uchar), rem);				
	//name = new _TCHAR[size+1];
	memcpychk (name, stream, sizeof (_TCHAR)*size, rem);			
	name[size]=name[_NAME_LEN_]=0;
	memcpychk (&size, stream, sizeof (uchar), rem);				
	//callSign = new _TCHAR[size+1];
	memcpychk (callSign, stream, sizeof (_TCHAR)*size, rem);		
	callSign[size]=0;
	memcpychk (&playerSquadron, stream, sizeof (VU_ID), rem);		
	memcpychk (&playerFlight, stream, sizeof (VU_ID), rem);		
	memcpychk (&playerEntity, stream, sizeof (VU_ID), rem);		
	assignedPlayerFlightPtr = NULL;
	assignedPilotSlot = 255;
	assignedAircraftNum = 255;
	// KCK NOTE: These pointers will be referenced either if:
	// a) This session is inserted
	// b) This update is used to change the pointers of a current remote session
	// sfr: smartpointers
	//playerSquadronPtr = NULL;
	//playerFlightPtr = NULL;
	//playerEntityPtr = NULL;
	// Don't find them until we need them...
	//playerSquadronPtr = (Squadron) vuDatabase->Find(playerSquadron);
	//playerFlightPtr = (Flight) vuDatabase->Find(playerFlight);
	//playerEntityPtr = vuDatabase->Find(playerEntity);
	memcpychk (&AceFactor, stream, sizeof(float), rem);			
	memcpychk (&initAceFactor, stream, sizeof(float), rem);		
	memcpychk (&bubbleRatio, stream, sizeof (float), rem);		
	memcpychk (&country, stream, sizeof (uchar), rem);			
	memcpychk (&aircraftNum, stream, sizeof (uchar), rem);		
	memcpychk (&pilotSlot, stream, sizeof (uchar), rem);		
	memcpychk (&flyState, stream, sizeof (uchar), rem);			
	memcpychk (&reqCompression, stream, sizeof (short), rem);	
	memcpychk (kills, stream, sizeof(kills), rem);				
	memcpychk (&rating, stream, sizeof(uchar), rem);			
	memcpychk (&voiceID, stream, sizeof(uchar), rem);			
	memcpychk (&missions, stream, sizeof(ushort), rem);			
	latency = 10;
	samples = 10;
	unitDataSendBuffer = NULL;
	unitDataSendSet = 0;
	unitDataSendSize = 0;
	unitDataReceiveBuffer = NULL;
	unitDataReceiveSet = 0;
	memset(unitDataReceived,0,FS_MAXBLK/8);
	objDataSendBuffer = NULL;
	objDataSendSet = 0;
	objDataSendSize = 0;
	objDataReceiveBuffer = NULL;
	objDataReceiveSet = 0;
	memset(objDataReceived,0,FS_MAXBLK/8);

#if FINE_INT
	// fine interest
	unsigned char fc;
	memcpychk(&fc, stream, sizeof(fc), rem);
	if (fc > 0){
		VU_ID *ids = new VU_ID[fc];
		memcpychk(ids, stream, sizeof(VU_ID)*fc, rem);
		for (unsigned char i=0;i<fc;++i){			
			AddToFineInterest(static_cast<FalconEntity*>(vuDatabase->Find(ids[i])));
		}
		delete [] ids;
	}
#endif

}

FalconSessionEntity::FalconSessionEntity(FILE* filePtr) : VuSessionEntity (filePtr)
{
	MonoPrint ("FalconSessionEntity: This function is not supported!\n");
}

VU_ERRCODE FalconSessionEntity::InsertionCallback(void)
{
	// This entity was inserted, so we'd better make these calls for real:
	Squadron	ps = (Squadron) vuDatabase->Find(playerSquadron);
	Flight		pf = (Flight) vuDatabase->Find(playerFlight);
	FalconEntity*pe = (FalconEntity*)vuDatabase->Find(playerEntity);
	playerSquadronPtr.reset();// = NULL;				// Clear current pointers
	playerFlightPtr.reset();// = NULL;
	playerEntityPtr.reset();// = NULL;
	SetPlayerSquadron(ps);					// Force them to be reassigned and referenced
	SetPlayerFlight(pf);
	SetPlayerEntity(pe);
	SetPlayerSquadronID(playerSquadron);
	SetPlayerFlightID(playerFlight);
	SetPlayerEntityID(playerEntity);
	return VuSessionEntity::InsertionCallback();
}

FalconSessionEntity::~FalconSessionEntity(void)
{
	SetPlayerEntity(NULL);
	SetPlayerFlight(NULL);
	SetPlayerSquadron(NULL);
	if (unitDataSendBuffer){
		delete unitDataSendBuffer;
	}
	if (objDataSendBuffer){
		delete objDataSendBuffer;
	}
	if (unitDataReceiveBuffer){
		delete unitDataReceiveBuffer;
	}
	if (objDataReceiveBuffer){
		delete objDataReceiveBuffer;
	}
}

int FalconSessionEntity::Save(VU_BYTE** stream)
{
	uchar		size;

	VuSessionEntity::Save (stream);
	size = (uchar) _tcslen(name);
	memcpy (*stream, &size, sizeof (uchar));				*stream += sizeof (uchar);			
	memcpy (*stream, name, sizeof (_TCHAR)*size);			*stream += sizeof (_TCHAR)*size;	
	size = (uchar)_tcslen(callSign);
	memcpy (*stream, &size, sizeof (uchar));				*stream += sizeof (uchar);			
	memcpy (*stream, callSign, sizeof (_TCHAR)*size);		*stream += sizeof (_TCHAR)*size;	
	memcpy (*stream, &playerSquadron, sizeof (VU_ID));		*stream += sizeof (VU_ID);			
	memcpy (*stream, &playerFlight, sizeof (VU_ID));		*stream += sizeof (VU_ID);			
	memcpy (*stream, &playerEntity, sizeof (VU_ID));		*stream += sizeof (VU_ID);			
	memcpy (*stream, &AceFactor, sizeof(float));			*stream += sizeof (float);
	memcpy (*stream, &initAceFactor, sizeof(float));		*stream += sizeof (float);
	memcpy (*stream, &bubbleRatio, sizeof (float));			*stream += sizeof (float);
	memcpy (*stream, &country, sizeof (uchar));				*stream += sizeof (uchar);			
	memcpy (*stream, &aircraftNum, sizeof (uchar));			*stream += sizeof (uchar);			
	memcpy (*stream, &pilotSlot, sizeof (uchar));			*stream += sizeof (uchar);
	memcpy (*stream, &flyState, sizeof (uchar));			*stream += sizeof (uchar);
	memcpy (*stream, &reqCompression, sizeof (short));		*stream += sizeof (short);
	memcpy (*stream, kills, sizeof(kills));					*stream += sizeof (kills);
	memcpy (*stream, &rating, sizeof(uchar));				*stream += sizeof (uchar);
	memcpy (*stream, &voiceID, sizeof(uchar));				*stream += sizeof (uchar);
	memcpy (*stream, &missions, sizeof(ushort));			*stream += sizeof (ushort);
#if FINE_INT
	// fine interest
	unsigned char fc = static_cast<unsigned char>(fineInterestList.size());
	memcpy(*stream, &fc, sizeof(fc)); 
	*stream += sizeof(fc);
	if (fc > 0){
		// get ids from list
		VU_ID *ids = new VU_ID[fc];
		FalconEntityList::iterator it;
		unsigned int i=0;
		for (it=fineInterestList.begin();it!=fineInterestList.end();++it){
			ids[i++] = it->get()->Id();
		}
		unsigned int idsize = sizeof(VU_ID)*fc; 
		memcpy(*stream, ids, idsize);
		*stream += idsize;
		delete [] ids;
	}
#endif

	//reqCompression++;
	//MonoPrint("Sending info for session: %s - team %d - #%d\n", name, country,reqCompression);
	return SaveSize();
}

int FalconSessionEntity::Save(FILE* filePtr)
{
	MonoPrint ("FalconSessionEntity: This function is not supported!\n");
	ShiWarning("FalconSessionEntity: This function is not supported!\n");
	return 0;
	filePtr;
}

int FalconSessionEntity::SaveSize(void)
{
	int size,saveSize = VuSessionEntity::SaveSize();

	size = _tcslen(name); 
	saveSize += sizeof (uchar);			// name size
	saveSize += sizeof (_TCHAR)*size;	// name
	size = _tcslen(callSign);
	saveSize += sizeof (uchar);			// callsign size
	saveSize += sizeof (_TCHAR)*size;	// callsign
	saveSize += sizeof (VU_ID);			// playerSquadron
	saveSize += sizeof (VU_ID);			// playerFlight
	saveSize += sizeof (VU_ID);			// playerEntity
	saveSize += sizeof (float);			// AceFactor
	saveSize += sizeof (float);			// initAceFactor
	saveSize += sizeof (float);			// bubbleRatio
	saveSize += sizeof (uchar);			// country
	saveSize += sizeof (uchar);			// aircraftNum
	saveSize += sizeof (uchar);			// pilotSlot
	saveSize += sizeof (uchar);			// flyState
	saveSize += sizeof (short);			// reqCompression
	saveSize += sizeof (kills);			// kills
	saveSize += sizeof (uchar);			// rating
	saveSize += sizeof (uchar);			// voiceID
	saveSize += sizeof (ushort);		// missions
#if FINE_INT
	// sfr: we transmit as a list of IDs
	saveSize += sizeof(char) + (fineInterestList.size()*sizeof(VU_ID));
#endif
	return saveSize;
}

// accessors
uchar FalconSessionEntity::GetTeam(void)
{
	return ::GetTeam(country);
}

// setters
void FalconSessionEntity::SetPlayerName (_TCHAR* pname)
{
	//	MonoPrint ("SetPlayerName\n");
	SetDirty ();
	/*if (name)
	  delete name;
	  name = new _TCHAR[_tcslen(pname)+1];*/
	_tcscpy(name,pname);
	name[_NAME_LEN_] = 0;
	if (gUICommsQ && Game())
	{
		gUICommsQ->Add(_Q_SESSION_UPDATE_,Id(),Game()->Id());
		UI_Refresh();
	}
}

void FalconSessionEntity::SetPlayerCallsign (_TCHAR* pcallsign)
{
	//	MonoPrint ("SetPlayerCallsign\n");
	SetDirty ();
	//if (callSign)
	//	delete callSign;
	//callSign = new _TCHAR[_tcslen(pcallsign)+1];
	_tcscpy(callSign,pcallsign);
	callSign[_CALLSIGN_LEN_] = 0;
	if (gUICommsQ && Game())
	{
		gUICommsQ->Add(_Q_SESSION_UPDATE_,Id(),Game()->Id());
		UI_Refresh();
	}
}

/*_TCHAR* FalconSessionEntity::GetPlayerCallsign(void){
	_TCHAR *aux = callSign;
	return aux; 
}*/

void FalconSessionEntity::SetPlayerSquadronID (VU_ID id)
{
	CampEnterCriticalSection ();
	playerSquadron = id;
	CampLeaveCriticalSection ();
}

void FalconSessionEntity::SetPlayerFlightID (VU_ID id)
{
	CampEnterCriticalSection ();
	playerFlight = id;
	CampLeaveCriticalSection ();
}

void FalconSessionEntity::SetPlayerEntityID (VU_ID id)
{
	CampEnterCriticalSection ();
	playerEntity = id;
	CampLeaveCriticalSection ();
}

void FalconSessionEntity::SetPlayerSquadron(SquadronClass* ent)
{
	if (playerSquadronPtr.get() == ent){ return; }
	CampEnterCriticalSection ();
	SetDirty ();

	SquadronClass *oldPlayerPtr = playerSquadronPtr.get();
	if (oldPlayerPtr){
		GameManager.CheckPlayerStatus(oldPlayerPtr);
		//VuDeReferenceEntity(oldPlayerPtr);
	}
	// sfr: smartpointer ref/deref
	playerSquadronPtr.reset(ent);
	if (playerSquadronPtr)
	{
		playerSquadron = playerSquadronPtr->Id();
		country = playerSquadronPtr->GetOwner();
		ShiAssert(country > 0 && country < 255);
		GameManager.CheckPlayerStatus(playerSquadronPtr.get());
		//VuReferenceEntity(playerSquadronPtr);
	}
	else {
		playerSquadron = FalconNullId;
	}

	if (gUICommsQ && Game())
	{
		gUICommsQ->Add(_Q_SESSION_UPDATE_,Id(),Game()->Id());
		UI_Refresh();
	}
	CampLeaveCriticalSection ();
}

void FalconSessionEntity::SetPlayerFlight (FlightClass* ent)
{
	if (playerFlightPtr.get() == ent){ return; }

	CampEnterCriticalSection ();
	SetDirty ();
	if (gCompressTillTime && IsLocal())
	{
		gCompressTillTime = 0;		// Cancle our current takeoff flight.
		SetTimeCompression(1);
	}
	FlightClass *oldPlayerPtr = playerFlightPtr.get();
	if (oldPlayerPtr){
		GameManager.CheckPlayerStatus(oldPlayerPtr);
		//VuDeReferenceEntity(oldPlayerPtr);
	}
	// sfr: smartpointer ref/deref
	playerFlightPtr.reset(ent);
	if (playerFlightPtr){
		playerFlight=playerFlightPtr->Id();
		GameManager.CheckPlayerStatus(playerFlightPtr.get());
		//VuReferenceEntity(playerFlightPtr);
	}
	else {
		playerFlight=FalconNullId;
	}

	if (gUICommsQ && Game()){
		gUICommsQ->Add(_Q_SESSION_UPDATE_,Id(),Game()->Id());
		UI_Refresh();
	}
	CampLeaveCriticalSection ();
}

void FalconSessionEntity::SetAssignedPlayerFlight (FlightClass* ent)
{
	FlightClass *oldPlayerPtr;

	if (assignedPlayerFlightPtr == ent)
	{
		return;
	}

	CampEnterCriticalSection ();
	//	MonoPrint ("SetAssignedPlayerFlight\n");
	SetDirty ();

	oldPlayerPtr = assignedPlayerFlightPtr;

	assignedPlayerFlightPtr = ent;

	if (oldPlayerPtr)
	{
		VuDeReferenceEntity(oldPlayerPtr);
	}

	if (assignedPlayerFlightPtr)
	{
		VuReferenceEntity(assignedPlayerFlightPtr);
	}
	CampLeaveCriticalSection ();
}

void FalconSessionEntity::SetPlayerEntity(FalconEntity* ent)
{
	if (ent == playerEntityPtr){
		return; 
	}

	CampEnterCriticalSection ();

	// Update cameras if local (remote cameras are sent to us via the update/create event)
	if (IsLocal() && ent){
		// KCK: Clear any previous camera and snap to this new entity.
		ClearCameras();	
		AttachCamera(ent);
	}

	VuEntity *oldPlayerPtr = playerEntityPtr.get();
	// sfr: we must reset player entity here, otherwise the CheckPlayerStatus below will fail
	// since it checks the playerEntityPtr
	playerEntityPtr.reset(ent);

	if (oldPlayerPtr){
		GameManager.CheckPlayerStatus((FalconEntity*)oldPlayerPtr);
		//VuDeReferenceEntity(oldPlayerPtr);
#if 0//NEW_SERVER_VIEWPOINT
		if (!IsLocal() && Game()->IsLocal()){
			// viewpoint for the session if remote
			OTWDriver.RemoveViewpoint(this);
		}
#endif
	}

	if (playerEntityPtr){
		playerEntity = playerEntityPtr->Id();
		GameManager.CheckPlayerStatus(playerEntityPtr.get());
		//VuReferenceEntity(playerEntityPtr);
		int newcountry = playerEntityPtr->GetCountry();
		ShiAssert(newcountry > 0 && newcountry < 255);
		if (newcountry > 0 && newcountry < 255){
			country = (uchar)newcountry;
		}
#if 0//NEW_SERVER_VIEWPOINT
		if (!IsLocal() && Game()->IsLocal()){
			// viewpoint for the session if remote
			OTWDriver.AddViewpoint(this);
		}
#endif
	}
	else{
		playerEntity = FalconNullId;
		flyState = FLYSTATE_DEAD;
	}

	SetDirty();
	CampLeaveCriticalSection();
}

// This is an extention to the above - if we need to distinguish between countries
void FalconSessionEntity::SetCountry (uchar c)
{
	//	MonoPrint ("SetCountry\n");
	SetDirty ();
	country = c;
	ShiAssert(country > 0);
	if (gUICommsQ && Game())
	{
		gUICommsQ->Add(_Q_SESSION_UPDATE_,Id(),Game()->Id());
		UI_Refresh();
	}
}

void FalconSessionEntity::SetAceFactor (float af)
{
	//	MonoPrint ("SetAceFactor\n");
	SetDirty ();
	AceFactor = af;
}

void FalconSessionEntity::SetInitAceFactor (float af)
{
	//	MonoPrint ("SetInitAceFactor\n");
	SetDirty ();
	initAceFactor = af;
}

void FalconSessionEntity::SetAceFactorKill(float opponent)
{
	//	MonoPrint ("SetAceFactorKill\n");
	SetDirty ();
	/*	double Raw;

		Raw = exp((double)AceFactor) + opponent;

		AceFactor = log(Raw);

#define ACE_KILL(x,y) (log(exp(x) + y))
	 */

	AceFactor = (float)ACE_KILL(((double)AceFactor),((double)opponent));

	if(AceFactor < 1.0f)
		AceFactor=1.0f;
}

void FalconSessionEntity::SetAceFactorDeath(float opponent)
{
	//	MonoPrint ("SetAceFactorDeath\n");
	SetDirty ();
	/*	double Raw,temp;
		Raw = exp((double)AceFactor);

		temp = AceFactor * (AceFactor - opponent);

		Raw = Raw - ((temp>AceFactor)?temp:AceFactor);
		AceFactor = log(Raw);

#define ACE_DEATH(x,y) (log(exp(x) - (((x * (x - y))>x*0.5F)?(x * (x - y)):x*0.5F)))
	 */

	//AceFactor = ACE_DEATH(((double)AceFactor),((double)opponent));
	AceFactor = (float)ACE_DEATH(((double)AceFactor), ACE_KILL(((double)opponent),((double)initAceFactor)));

	if(AceFactor < 1.0f)
		AceFactor=1.0f;
}

void FalconSessionEntity::SetAircraftNum (uchar an)
{
	//	MonoPrint ("SetAircraftNum\n");
	SetDirty ();
	aircraftNum = an;
}

void FalconSessionEntity::SetPilotSlot (uchar ps)
{
	//	MonoPrint ("SetPilotSlot\n");
	SetDirty ();
	pilotSlot = ps;

}

void FalconSessionEntity::SetAssignedAircraftNum (uchar an)
{
	//	MonoPrint ("SetAssignedAircraftNum\n");
	SetDirty ();
	assignedAircraftNum = an;
}

void FalconSessionEntity::SetAssignedPilotSlot (uchar ps)
{
	//	MonoPrint ("SetAssignedPilotSlot\n");
	SetDirty ();
	assignedPilotSlot = ps;
}

void FalconSessionEntity::SetFlyState(uchar fs){
	flyState = fs;
	SetDirty();
}

void FalconSessionEntity::SetReqCompression (short rc)
{
	if (reqCompression != rc)
	{
		reqCompression = rc; 
		DoFullUpdate();
		ResyncTimes();
	}
}	

//#define		CAMPAIGN_BUBBLE_MULTIPLIER	3.0F

// Returns:	0 if not in bubble
//			1 if in bubble
int FalconSessionEntity::InSessionBubble (FalconEntity* ent, float bubble_multiplier)
{
	float	ent_bubble_range;
	int		i;

	if (CameraCount() == 0 || bubble_multiplier < 0.01F){
		// No camera - so we have no bubble, or a rediculously small multiplier
		return FALSE;
	}

	// Get the entity's bubble range
	if (ent->IsObjective()){
		ent_bubble_range = ent->EntityType()->bubbleRange_;			// We don't adjust objective's bubble
	}
	else if (ent->IsFlight() && gGameType == game_Dogfight){
		// KCK HACK: Basically, we want to always keep flights in our bubble in dogfight
		// sfr: is this necessary??
		return TRUE;
	}
	else {
		ent_bubble_range = ent->EntityType()->bubbleRange_ * bubbleRatio;
	}

	for (i=0; i < CameraCount(); i++){
		VuEntity *camera = GetCameraEntity(i);
		if (camera){
			float xdist,ydist,dsqu;
			float rsqu, range = ent_bubble_range * camera->EntityType()->fineUpdateMultiplier_ * bubble_multiplier;
			rsqu = range*range;
			// KCK NOTE: Estimate distances in one second's time - Opposite vectors assume convergence
			xdist = (float)fabs(ent->XPos() - camera->XPos()) - (float)fabs(ent->XDelta() - camera->XDelta());
			ydist = (float)fabs(ent->YPos() - camera->YPos()) - (float)fabs(ent->YDelta() - camera->YDelta());
			dsqu = (xdist*xdist) + (ydist*ydist);
			if (dsqu < rsqu){
				return TRUE;
			}
		}
	}
	return FALSE;
}

void FalconSessionEntity::DoFullUpdate (void)
{
	//	MonoPrint ("FalconSessionEntity::DoFullUpdate\n");
	VuEvent *event = new VuFullUpdateEvent(this, vuGlobalGroup);
	event->RequestReliableTransmit ();
	VuMessageQueue::PostVuMessage(event);
	ClearDirty ();
}

VU_ERRCODE FalconSessionEntity::Handle(VuFullUpdateEvent *event)
{
	FalconSessionEntity* tmpSess = (FalconSessionEntity*)(event->expandedData_.get());
	short	dirty=0;
	unsigned int size;

	if (IsLocal()){
		return 0;
	}

	// Copy in new data
	if (!name || strcmp(name,tmpSess->name) != 0){
		dirty |= 0x0001;
		size = _tcslen(tmpSess->name);
		/*if (name)
		  delete name;
		  name = new _TCHAR[size+1];*/
		memcpy (name, tmpSess->name, size);
		name[size]=name[_NAME_LEN_] = 0;
	}
	if (!callSign || strcmp(callSign,tmpSess->callSign) != 0){
		dirty |= 0x0002;
		size = _tcslen(tmpSess->callSign);
		/*if (callSign)
		  delete callSign;
		  callSign = new _TCHAR[size+1];*/
		memcpy (callSign, tmpSess->callSign, size);
		callSign[_CALLSIGN_LEN_] = callSign[size]=0;
	}

	AceFactor = tmpSess->AceFactor;
	initAceFactor = tmpSess->initAceFactor;
	voiceID = tmpSess->voiceID;
	rating = tmpSess->rating;

	// KCK: This will ALWAYS dirty the session if the player is in
	// a flight in another game.. Probably not good. If he's in 
	// a flight in this game, the playerFlightPtr should dirty the
	// session just dandily..
	//	if(playerFlight != tmpSess->playerFlight)
	//		dirty |= 0x0004;
	if(country != tmpSess->country){
		dirty |= 0x0008;
	}
	if(aircraftNum != tmpSess->aircraftNum){
		dirty |= 0x0010; 
	}
	if(pilotSlot != tmpSess->pilotSlot){
		dirty |= 0x0020;
	}

	tmpSess->playerSquadronPtr.reset((Squadron) vuDatabase->Find(tmpSess->playerSquadron));
	tmpSess->playerFlightPtr.reset((Flight) vuDatabase->Find(tmpSess->playerFlight));
	tmpSess->playerEntityPtr.reset(static_cast<FalconEntity*>(vuDatabase->Find(tmpSess->playerEntity)));

	if(playerSquadronPtr != tmpSess->playerSquadronPtr){
		dirty |= 0x0040;
		SetPlayerSquadron(tmpSess->playerSquadronPtr.get());
	}

	if (playerFlightPtr != tmpSess->playerFlightPtr){
		dirty |= 0x0080;
		SetPlayerFlight(tmpSess->playerFlightPtr.get());
	}

	if (playerEntityPtr != tmpSess->playerEntityPtr){
		dirty |= 0x0100;
		SetPlayerEntity(tmpSess->playerEntityPtr.get());
	}

	SetPlayerSquadronID(tmpSess->playerSquadron);
	SetPlayerFlightID(tmpSess->playerFlight);
	SetPlayerEntityID(tmpSess->playerEntity);

	tmpSess->playerSquadronPtr.reset();// = NULL;				// Clear temporary pointers
	tmpSess->playerFlightPtr.reset();// = NULL;
	tmpSess->playerEntityPtr.reset();// = NULL;

	memcpy(&country, &tmpSess->country, sizeof (uchar));		
	memcpy(&aircraftNum, &tmpSess->aircraftNum, sizeof (uchar));	
	memcpy(&pilotSlot, &tmpSess->pilotSlot, sizeof (uchar));
	memcpy(&flyState, &tmpSess->flyState, sizeof (uchar));
	memcpy(kills,tmpSess->kills,sizeof(kills));

	// Tell the flight to hold short if this guy is coming into the sim.
	// sfr: taking this out. This prevents flights from taking off
	//if (flyState == FLYSTATE_LOADING && playerFlightPtr && playerFlightPtr->IsLocal())
	//	playerFlightPtr->SetFalcFlag(FEC_HOLDSHORT);

	memcpy(&reqCompression, &tmpSess->reqCompression, sizeof (short));
	if (tmpSess->Game() && (Game() != tmpSess->Game())){
		JoinGame(tmpSess->Game());
	}

	//	MonoPrint("Got info for session: %s - team %d - #%d\n", name, country, reqCompression);

	if (gUICommsQ && (dirty & 0x00ff) && Game()) {
		gUICommsQ->Add(_Q_SESSION_UPDATE_,Id(),Game()->Id());
		UI_Refresh();
	}
	else {
		CheckForNewPlayer(this);
	}

	// KCK: if we're the host, check to see if Assigned aircraft is different than the one
	// the session thinks it has and correct any errors by sending an SendAircraftSlot message
	if (FalconLocalGame && Game() == FalconLocalGame && FalconLocalGame->IsLocal()){
		if (
			assignedAircraftNum != aircraftNum || 
			assignedPilotSlot != pilotSlot || 
			assignedPlayerFlightPtr != playerFlightPtr
		){
			//Flight					flight = GetAssignedPlayerFlight();
			MonoPrint ("Invalid Session Info %d:%d %d:%d %08x:%08x\n",
						assignedAircraftNum, aircraftNum,
						assignedPilotSlot, pilotSlot,
						assignedPlayerFlightPtr, playerFlightPtr);
			/*			
						if (flight)
						{
						msg = new UI_SendAircraftSlot(flight->Id(), FalconLocalGame);
						msg->dataBlock.team = flight->GetTeam();
						}
						else
						{
						msg = new UI_SendAircraftSlot(FalconNullId, FalconLocalGame);
						msg->dataBlock.team = 255;
						}
						msg->dataBlock.requesting_session = Id();
						msg->dataBlock.host_id = FalconLocalSession->Id();
						msg->dataBlock.game_id = FalconLocalGame->Id();
						msg->dataBlock.game_type = FalconLocalGame->GetGameType();
						msg->dataBlock.got_pilot_slot = assignedPilotSlot;
						msg->dataBlock.got_slot = assignedAircraftNum;
						msg->dataBlock.got_pilot_skill = 0;
						if (msg->dataBlock.got_pilot_slot != NO_PILOT)
						msg->dataBlock.result = REQUEST_RESULT_SUCCESS;
						else
						msg->dataBlock.result = REQUEST_RESULT_DENIED;
			//FalconSendMessage(msg,TRUE); - may be causing a drop to flyby cam - RH
			 */
		}
	}	

#if FINE_INT
	// insert each entity in ours
	fineInterestList.clear();
	FalconEntityList::iterator it;
	for (it = tmpSess->fineInterestList.begin(); it != tmpSess->fineInterestList.end(); ++it){
		AddToFineInterest(it->get());
	}
#endif


	return (VuSessionEntity::Handle(event));
}

void FalconSessionEntity::UpdatePlayer (void)
{
	SquadronClass *squadron_ptr;
	FlightClass *flight_ptr;
	FalconEntity *entity_ptr;
	squadron_ptr = (Squadron) vuDatabase->Find(playerSquadron);
	flight_ptr = (Flight) vuDatabase->Find(playerFlight);

	if ((!flight_ptr) && (playerFlight != vuNullId)){
		static int now, last_time;
		now = GetTickCount ();
		if (now - last_time > 5000)	{
			last_time = now;
			MonoPrint ("Flight is not found : %08x%08x\n", playerFlight);
			VuGetRequest *msg = new VuGetRequest(playerFlight, this);
			FalconSendMessage(msg);
		}
	}

	entity_ptr = static_cast<FalconEntity*>(vuDatabase->Find(playerEntity));

	if (squadron_ptr != playerSquadronPtr){
		SetPlayerSquadron(squadron_ptr);
	}

	if (flight_ptr != playerFlightPtr){
		SetPlayerFlight(flight_ptr);
	}

	if (entity_ptr != playerEntityPtr){
		SetPlayerEntity(entity_ptr);
		if (entity_ptr){
			SetLabel((SimBaseClass*)entity_ptr);
		}
	}
}

#if FINE_INT

// sfr fine interest stuff
bool FalconSessionEntity::AddToFineInterest(FalconEntity *entity, bool silent){
	if ((entity == NULL) || (fineInterestList.size() == FALCSESS_MAX_FINE_INTEREST)){
		return false;
	}
	fineInterestList.push_back(FalconEntityBin(entity));
	if (!silent){ SetDirty(); }
	return true;
}

/** removes unit from fine interest list. Will be updated normally */
bool FalconSessionEntity::RemoveFromFineInterest(const FalconEntity *entity, bool silent){
	for (FalconEntityList::iterator it = fineInterestList.begin();it!=fineInterestList.end();++it){
		FalconEntity *e = it->get();
		if (entity->Id() == e->Id()){
			fineInterestList.erase(it);
			if (!silent){ SetDirty(); }
			return true;
		}
	}
	return false;
}

/** clears fine interest list */
void FalconSessionEntity::ClearFineInterest(bool silent){
	fineInterestList.clear();
	if (!silent){ SetDirty(); }
}


/** checks if a unit is in fine interest list */
bool FalconSessionEntity::HasFineInterest(const FalconEntity *entity) const {
	for (FalconEntityList::const_iterator it = fineInterestList.begin();it!=fineInterestList.end();++it){
		FalconEntity *e = it->get();
		if (entity->Id() == e->Id()){
			return true;
		}
	}
	return false;
}


#endif

