#include "stdhdr.h"
#include "simveh.h"
#include "digi.h"
#include "wingorder.h"
#include "simdrive.h"
#include "Aircrft.h"
#include "campbase.h"
#include "object.h"

void DigitalBrain::CommandFlight(void)
{
    VU_ID targetId = FalconNullId;

    // Do i have wingmen?
    if (self->GetCampaignObject()->NumberOfComponents() > 1)
    {
        // If target changed, tell the flight
        // 2002-03-08 MODIFIED BY S.G. If in WaypointMode it means we don't care so why should our wing care but flag us as having a target switch so if we ever get out of WaypointMode, send the target to our wings
        if (targetPtr)
        {
            if (((moreFlags bitand KeepTryingAttack) or lastTarget == NULL or (lastTarget and (targetPtr->BaseData() not_eq lastTarget->BaseData()))))
            {
                if (curMode not_eq WaypointMode)
                {
                    moreFlags and_eq compl KeepTryingAttack;
                    moreFlags and_eq compl KeepTryingRejoin;

                    // KCK: I added the GetCampaignObject() check because BaseData() was observed to be a MissileClass
                    // But why would we be targetting a missile class?
                    // LR that would be because it is tracking us w/ it's radar, and we need to run away.
                    // In that case, we should either send the flight after the parent, or more realistically,
                    // just respond defensivly.
                    if (targetPtr->BaseData()->IsSim())
                    {
                        if ( not targetPtr->BaseData()->IsWeapon())
                            targetId = ((SimBaseClass*)targetPtr->BaseData())->GetCampaignObject()->Id();
                    }
                    else
                        targetId = targetPtr->BaseData()->Id();

                    if (targetId not_eq FalconNullId)
                    {

                        if (SimLibElapsedTime > mLastOrderTime + 5000)
                        {
                            mLastOrderTime = SimLibElapsedTime;
                            AiSendCommand(self, FalconWingmanMsg::WMAssignTarget, AiFlight, targetId);
                            AiSendCommand(self, FalconWingmanMsg::WMShooterMode, AiFlight, targetId);
                        }
                    }
                }
                else
                    moreFlags or_eq KeepTryingAttack;
            }
        }
        else if ((moreFlags bitand KeepTryingRejoin) or lastTarget and targetPtr == NULL) // 2002-03-08 MODIFIED BY S.G. keep trying to rejoin until it can
        {
            int usComponents = self->GetCampaignObject()->NumberOfComponents();
            int i;
            bool stillengaging = false;
            AircraftClass *flightMember = NULL;

            // Get the flight aircrafts (once per call instead of once per target querried)
            for (i = 0; i < usComponents; i++)
            {
                flightMember = (AircraftClass *)self->GetCampaignObject()->GetComponentEntity(i);

                if (flightMember and (flightMember->IsDigital() or flightMember->IsLocal()))
                {
                    if (flightMember->DBrain() and flightMember->DBrain()->GetAGDoctrine() not_eq AGD_NONE)
                    {
                        stillengaging = true;
                        break;
                    }
                }
            }

            if ( not stillengaging and not threatPtr) // If we are threatened, call the wingmen back regardless what they do
            {
                AiSendCommand(self, FalconWingmanMsg::WMRejoin, AiFlight, FalconNullId);
                AiSendCommand(self, FalconWingmanMsg::WMCoverMode, AiFlight, FalconNullId);

                moreFlags and_eq compl KeepTryingAttack;
                moreFlags and_eq compl KeepTryingRejoin;
            }
            else
                moreFlags or_eq KeepTryingRejoin;
        }
    }
}
