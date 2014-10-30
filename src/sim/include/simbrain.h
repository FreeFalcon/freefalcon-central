#ifndef _BASEBRAIN_H
#define _BASEBRAIN_H

#include "f4vu.h"

class SimObjectType;
class SimObjectLocalData;
class FalconEvent;

class BaseBrain
{
private:
    int flags;
    int skillLevel;

public:
    enum
    {
        GunFireFlag = 0x1,
        MslFireFlag = 0x2
    };
    BaseBrain(void);
    virtual ~BaseBrain(void);
    SimObjectType* targetPtr;
    SimObjectType* lastTarget;
    SimObjectLocalData* targetData;
    int isWing;
    float pStick, rStick, yPedal, throtl;
    virtual void ReceiveOrders(FalconEvent*) {};
    virtual void JoinFlight(void) {};
    virtual void SetLead(int) {};
    virtual void FrameExec(SimObjectType*, SimObjectType*) {};
    virtual void PostInsert(void) {};
    virtual void Sleep(void)
    {
        ClearTarget();
    };
    void SetTarget(SimObjectType* newTarget);
    void ClearTarget(void);
    void SetFlag(int val)
    {
        flags or_eq val;
    };
    void ClearFlag(int val)
    {
        flags and_eq compl val;
    };
    int IsSetFlag(int val)
    {
        return (flags bitand val ? TRUE : FALSE);
    };
    int SkillLevel(void)
    {
        return skillLevel;
    };
    void SetSkill(int newLevel)
    {
        skillLevel = newLevel;
    };

    virtual int IsTanker(void)
    {
        return FALSE;
    }
    virtual void InitBoom(void) {};
    virtual void CleanupBoom(void) {};
};

class FeatureBrain
{
public:
    FeatureBrain(void);
    virtual ~FeatureBrain(void);

    virtual void Exec(void) {};
};

#endif
