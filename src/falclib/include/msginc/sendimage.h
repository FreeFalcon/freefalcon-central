#ifndef _SENDIMAGE_H
#define _SENDIMAGE_H
#include <cstdint>

#include "f4vu.h"
#include "falcmesg.h"
#include "mission.h"

#include "invalidbufferexception.h"

#pragma pack(1)

/*
 * Message Type Send Image
 */
class UI_SendImage : public FalconEvent
{
public:
    UI_SendImage(VU_ID entityId, VuTargetEntity *target,
                 VU_BOOL loopback = TRUE);
    UI_SendImage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
    ~UI_SendImage(void);
    int Size(void)
    {
        int size = FalconEvent::Size();
        size += sizeof(VU_ID) + sizeof(uchar) + sizeof(short) + sizeof(short) +
                sizeof(int32_t) + sizeof(int32_t) + dataBlock.blockSize;
        return (size);
    }

    //sfr: changed to long *
    int Decode(VU_BYTE **buf, long *rem)
    {
        long init = *rem;

        FalconEvent::Decode(buf, rem);
        memcpychk(&dataBlock.fromID, buf, sizeof(VU_ID), rem);
        memcpychk(&dataBlock.typeID, buf, sizeof(uchar), rem);
        memcpychk(&dataBlock.blockNo, buf, sizeof(short), rem);
        memcpychk(&dataBlock.blockSize, buf, sizeof(short), rem);
        memcpychk(&dataBlock.offset, buf, sizeof(int32_t), rem);
        memcpychk(&dataBlock.size, buf, sizeof(int32_t), rem);
        dataBlock.data = new uchar[dataBlock.blockSize];
        memcpychk(dataBlock.data, buf, dataBlock.blockSize, rem);
        return init - *rem;
    };
    int Encode(VU_BYTE **buf)
    {
        int size;

        size = FalconEvent::Encode(buf);
        memcpy(*buf, &dataBlock.fromID, sizeof(VU_ID));
        *buf += sizeof(VU_ID);
        size += sizeof(VU_ID);
        memcpy(*buf, &dataBlock.typeID, sizeof(uchar));
        *buf += sizeof(uchar);
        size += sizeof(uchar);
        memcpy(*buf, &dataBlock.blockNo, sizeof(short));
        *buf += sizeof(short);
        size += sizeof(short);
        memcpy(*buf, &dataBlock.blockSize, sizeof(short));
        *buf += sizeof(short);
        size += sizeof(short);
        memcpy(*buf, &dataBlock.offset, sizeof(int32_t));
        *buf += sizeof(int32_t);
        size += sizeof(int32_t);
        memcpy(*buf, &dataBlock.size, sizeof(int32_t));
        *buf += sizeof(int32_t);
        size += sizeof(int32_t);
        memcpy(*buf, dataBlock.data, dataBlock.blockSize);
        *buf += dataBlock.blockSize;
        size += dataBlock.blockSize;
        return size;
    };
    class DATA_BLOCK
    {
    public:
        VU_ID fromID;
        uchar typeID;
        short blockNo;
        short blockSize;
        int32_t offset;
        int32_t size;
        uchar *data;
    } dataBlock;

protected:
    int Process(uchar autodisp);
};
#pragma pack()

#endif
