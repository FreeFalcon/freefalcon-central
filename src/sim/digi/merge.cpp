#include "stdhdr.h"
#include "digi.h"
#include "simveh.h"
#include "object.h"
#include "airframe.h"
#include "aircrft.h"
#include "fakerand.h"

void DigitalBrain::MergeCheck(void)
{
    float breakRange;

    /*-----------*/
    /* no target */
    /*-----------*/
    if (targetPtr == NULL)
    {
        return;
    }

    /*-------*/
    /* entry */
    /*-------*/
    if (curMode not_eq MergeMode)
    {
        if (-self->ZPos() > 3000 and targetData->range <= (1000) and targetData->ata < 45.0f * DTR and fabs(self->Pitch()) < 45.0F * DTR)
        {
            float dx, dy;

            dx = targetPtr->BaseData()->XPos() - self->XPos();
            dy = targetPtr->BaseData()->YPos() - self->YPos();

            // Max range when on target nose, 0 if a stern chase
            breakRange = ((targetPtr->BaseData()->GetKias() * self->GetKias())) * //me123
                         (1.0F - targetData->ataFrom / (180.0F * DTR)) *
                         (1.0F - targetData->ataFrom / (180.0F * DTR));

            if (dx * dx + dy * dy < breakRange and targetData->ataFrom < 45.0F * DTR)
                AddMode(MergeMode);
        }
    }
    /*------*/
    /* exit */
    /*------*/
    else if (curMode == MergeMode)
    {
        if (-self->ZPos() > 3000 and SimLibElapsedTime < mergeTimer)
            AddMode(MergeMode);
        else
            mergeTimer = 0;
    }
}

void DigitalBrain::MergeManeuver(void)
{
    int mnverFlags;
    float eDroll;
    float curRoll = self->Roll();

    /*-----------*/
    /* no target */
    /*-----------*/
    if (targetPtr == NULL)
    {
        return;
    }

    // Pick bank angle on first pass
    if (lastMode not_eq MergeMode)
    {
        // Mil power except for Vertical;

        mergeTimer = SimLibElapsedTime + 3 * SEC_TO_MSEC;//me123 from 5
        mnverFlags = maneuverClassData[self->CombatClass()].flags;

        switch (mnverFlags bitand (CanLevelTurn bitor CanSlice bitor CanUseVertical))
        {
            case CanLevelTurn:
                if ((mnverFlags bitand CanOneCircle) and (self->GetKias() < cornerSpeed))//me123
                {
                    // One Circle, turn away from the target
                    newroll = (targetData->az > 0.0F ? -90.0F * DTR : 90.0F * DTR);
                    MachHold(cornerSpeed, self->GetKias(), FALSE);
                }
                else
                {
                    // Two Circle, turn towards the target
                    newroll = (targetData->az > 0.0F ? 90.0F * DTR : -90.0F * DTR);
                    MachHold(cornerSpeed, self->GetKias(), TRUE);
                }

                break;

            case CanSlice:
                if (curRoll > 0.0F)
                {
                    newroll = 135.0F * DTR;
                    MachHold(cornerSpeed, self->GetKias(), FALSE); //me123
                }
                else
                {
                    newroll = -135.0F * DTR;
                    MachHold(cornerSpeed, self->GetKias(), FALSE); //me123
                }

                break;

            case CanUseVertical:
                newroll = 0.0F;
                MachHold(cornerSpeed, self->GetKias(), TRUE);
                // Full burner for the pull
                break;

            case CanLevelTurn bitor CanSlice:

                // level turn or slice?
                if ((self->GetKias() > cornerSpeed) and -self->ZPos() > 3000.0f)//me123
                {
                    // Level Turn
                    if ((mnverFlags bitand CanOneCircle) and (self->GetKias() < cornerSpeed))
                    {
                        // One Circle, turn away from the target
                        newroll = (targetData->az > 0.0F ? -90.0F * DTR : 90.0F * DTR);
                        MachHold(0.7f * cornerSpeed, self->GetKias(), FALSE);//me123 addet *0.4
                    }
                    else
                    {
                        // Two Circle, turn towards the target
                        newroll = (targetData->az > 0.0F ? 90.0F * DTR : -90.0F * DTR);
                        MachHold(cornerSpeed, self->GetKias(), TRUE);
                    }
                }
                else
                {
                    if (curRoll > 0.0F)
                    {
                        newroll = 135.0F * DTR;
                        MachHold(cornerSpeed, self->GetKias(), FALSE);
                    }
                    else
                    {
                        newroll = -135.0F * DTR;
                        MachHold(cornerSpeed, self->GetKias(), FALSE);
                    }
                }

                break;

            case CanLevelTurn bitor CanUseVertical:

                // level turn or vertical?
                if (self->GetKias() < cornerSpeed * 1.2) //me123
                {
                    // Level Turn
                    if ((mnverFlags bitand CanOneCircle) and (self->GetKias() < cornerSpeed))
                    {
                        // One Circle, turn away from the target
                        newroll = (targetData->az > 0.0F ? -90.0F * DTR : 90.0F * DTR);
                        MachHold(0.7f * cornerSpeed, self->GetKias(), FALSE);
                    }
                    else
                    {
                        // Two Circle, turn towards the target
                        newroll = (targetData->az > 0.0F ? 90.0F * DTR : -90.0F * DTR);
                        MachHold(cornerSpeed, self->GetKias(), TRUE);
                    }
                }
                else
                {
                    newroll = 0.0F;
                    MachHold(cornerSpeed, self->GetKias(), TRUE);
                    // Full burner for the pull
                }

                break;

            case CanSlice bitor CanUseVertical:

                // slice or vertical?
                if ((self->GetKias() < cornerSpeed) and -self->ZPos() > 3000.0f) //me123
                {
                    if (curRoll > 0.0F)
                    {
                        newroll = 135.0F * DTR;
                        MachHold(cornerSpeed, self->GetKias(), FALSE);
                    }
                    else
                    {
                        newroll = -135.0F * DTR;
                        MachHold(cornerSpeed, self->GetKias(), FALSE);
                    }
                }
                else
                {
                    newroll = 0.0F;
                    MachHold(cornerSpeed, self->GetKias(), TRUE);
                    // Full burner for the pull
                }

                break;

            case CanLevelTurn bitor CanSlice bitor CanUseVertical:

                // slice, level turn, or vertical?
                if ((self->GetKias() < cornerSpeed * 0.7) and -self->ZPos() > 3000.0f) //me123
                {
                    if ((mnverFlags bitand CanOneCircle))
                    {
                        // One Circle, turn away from the target
                        newroll = (targetData->az > 0.0F ? -135.0F * DTR : 135.0F * DTR);
                        MachHold(0.7f * cornerSpeed, self->GetKias(), FALSE);
                    }
                    else
                    {
                        // Two Circle, turn towards the target
                        newroll = (targetData->az > 0.0F ? 135.0F * DTR : -135.0F * DTR);
                        MachHold(cornerSpeed, self->GetKias(), TRUE);
                    }
                }
                else if ((self->GetKias() < cornerSpeed * 1.2))//me123
                {
                    // Level Turn
                    if ((mnverFlags bitand CanOneCircle) and (self->GetKias() < cornerSpeed))
                    {
                        // One Circle, turn away from the target
                        newroll = (targetData->az > 0.0F ? -90.0F * DTR : 90.0F * DTR);
                        MachHold(0.7f * cornerSpeed, self->GetKias(), FALSE);
                    }
                    else
                    {
                        // Two Circle, turn towards the target
                        newroll = (targetData->az > 0.0F ? 90.0F * DTR : -90.0F * DTR);
                        MachHold(cornerSpeed, self->GetKias(), TRUE);
                    }
                }
                else
                {
                    newroll = 0.0F;
                    MachHold(cornerSpeed * 1.2f, self->GetKias(), TRUE);
                    // Full burner for the pull
                }

                break;
        }
    }

    eDroll = newroll - self->Roll();
    SetYpedal(0.0F);
    SetRstick(eDroll * 2.0F * RTD);
    SetPstick(maxGs, maxGs, AirframeClass::GCommand);
    SetMaxRoll(newroll * RTD);
    SetMaxRollDelta(eDroll * RTD);

}

void DigitalBrain::AccelCheck(void)
{
    //Leon, if you are in waypoint mode, or loiter mode, it may be desired to fly at less than corner speed
    //this is only important in combat
    if (nextMode >= MergeMode and nextMode <= BVREngageMode and nextMode not_eq GroundAvoidMode)
    {
        if ((self->Pitch() > 50.0F * DTR and self->GetKias() < cornerSpeed * 0.4F) or//me123 150kias
            (self->Pitch() > 0.0F * DTR and self->GetKias() < cornerSpeed * 0.35F))//me123 100 GetKias
        {
            AddMode(AccelMode);
        }
        else if (curMode == AccelMode and self->GetKias() < cornerSpeed * 0.447F and 
                 self->Pitch() > 0.0F * DTR and self->GetKias())//me123 180kias
        {
            AddMode(AccelMode);
        }
    }
}

void DigitalBrain::AccelManeuver(void)
{
    float eDroll;
    float tmp;

    //MonoPrint ("Accelmaneuver");
    // Choose plane?
    if ((self->Roll()) >= 0)
    {
        tmp = 170.0F * DTR;
        //   MonoPrint (">0");
    }
    else
    {
        tmp = -170.0F * DTR;
        //   MonoPrint ("<0");
    }

    eDroll = tmp - self->Roll();
    SetYpedal(0.0F);
    SetRstick(eDroll * 2 * RTD);
    MachHold(cornerSpeed, self->GetKias(), FALSE);

    if (fabs(eDroll * RTD) > 10.0F)
        SetPstick(0.0F, maxGs, AirframeClass::GCommand);
    else
        SetPstick(4.0F, maxGs, AirframeClass::GCommand);

    SetMaxRoll(170.0F);
}
