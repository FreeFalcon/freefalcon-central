#ifndef _ATCCMDMSG_H
#define _ATCCMDMSG_H

#include "F4vu.h"
#include "FalcMesg.h"
#include "mission.h"
#pragma pack (1)
#include "InvalidBufferException.h"

/*
 * Message Type ATC Command
 */
class FalconATCCmdMessage : public FalconEvent
{
public:
    enum ATCCmdCode
    {
        TakePosition,
        EmergencyHold,
        Hold,
        Abort,
        ToFirstLeg,
        ToBase,
        ToFinal,
        OnFinal,
        ClearToLand,
        Landed,
        TaxiOff,
        EmerToBase,
        EmerToFinal,
        EmerOnFinal,
        EmergencyStop,
        TaxiBack,
        Taxi,
        Wait,
        HoldShort,
        PrepToTakeRunway,
        TakeRunway,
        Takeoff,
        Divert,
        Release,
        ExitSim
    };

    FalconATCCmdMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback = TRUE);
    FalconATCCmdMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
    ~FalconATCCmdMessage(void);
    virtual int Size() const
    {
        return sizeof(dataBlock) + FalconEvent::Size();
    };
    //sfr: changed to long *
    int Decode(VU_BYTE **buf, long *rem)
    {
        long init = *rem;
        FalconEvent::Decode(buf, rem);
        memcpychk(&dataBlock, buf, sizeof(dataBlock), rem);
        return init - *rem;
    };
    int Encode(VU_BYTE **buf)
    {
        int size;

        size = FalconEvent::Encode(buf);
        memcpy(*buf, &dataBlock, sizeof(dataBlock));
        *buf += sizeof(dataBlock);
        size += sizeof(dataBlock);
        return size;
    };
    class DATA_BLOCK
    {
    public:

        VU_ID from;
        unsigned int type;
        long rwtime;
        short rwindex;
        VU_ID follow;
    } dataBlock;

protected:
    int Process(uchar autodisp);
};
#pragma pack ()

#endif
