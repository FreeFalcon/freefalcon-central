#include "MsgInc/SimDirtyDataMsg.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"


SimDirtyData::SimDirtyData(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (SimDirtyDataMsg, FalconEvent::SimThread, entityId, target, loopback)
{
	dataBlock.data = NULL;
	dataBlock.size = 0;
}

SimDirtyData::SimDirtyData(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (SimDirtyDataMsg, FalconEvent::SimThread, senderid, target)
{
	dataBlock.data = NULL;
	dataBlock.size = 0;
	type;
}

SimDirtyData::~SimDirtyData(void)
{
	if (dataBlock.data)
	{
		delete dataBlock.data;
	}

	dataBlock.data = NULL;
	dataBlock.size = 0;
}

int SimDirtyData::Size (void) const {
    ShiAssert(dataBlock.size >= 0);
	return (FalconEvent::Size() + sizeof(ushort) + dataBlock.size);
}

int SimDirtyData::Decode (VU_BYTE **buf, long *rem)
{
	long int init = *rem;

	FalconEvent::Decode(buf, rem);
	memcpychk(&dataBlock.size, buf, sizeof(ushort), rem);
    ShiAssert(dataBlock.size >= 0);
	dataBlock.data = new uchar[dataBlock.size];
	memcpychk(dataBlock.data, buf, dataBlock.size, rem);

	int size = init - *rem;
	ShiAssert (size == Size());

	return size;
}

int SimDirtyData::Encode (VU_BYTE **buf)
{
	int size;

    ShiAssert(dataBlock.size >= 0);
	size = FalconEvent::Encode (buf);
	memcpy (*buf, &dataBlock.size, sizeof(ushort));				*buf += sizeof(ushort);			size += sizeof(ushort);	
	memcpy (*buf, dataBlock.data, dataBlock.size);				*buf += dataBlock.size;			size += dataBlock.size;		

	ShiAssert (size == Size());

	return size;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int SimDirtyData::Process(uchar autodisp)
{
	FalconEntity *ent;

	
	ent = static_cast<FalconEntity*>(vuDatabase->Find(EntityId ()));

	if (!ent || autodisp){
		return 0;
	}

	// sfr: removing this. some dirty data stuff come after unit has changed owner
	// specially low priority data
	// Only accept data on remote entities
	//if (!ent->IsLocal())
	//{
		VU_BYTE *data = dataBlock.data;
		
		//sfr: was ent->DecodeDirty (&data);
		long rem = dataBlock.size;
		ent->DecodeDirty (&data, &rem);
	//}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
