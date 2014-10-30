#include "stdhdr.h"
#include "hdigi.h"
#include "mesg.h"
#include "simbase.h"
#include "MsgInc/WingmanMsg.h"
#include "simveh.h"
#include "campBase.h"
#include "otwdrive.h"
//TJL 11/27/03
#include "helo.h"
#include "fcc.h"
#include "sms.h"
#include "Graphics/Include/drawBSP.h"

void HeliBrain::ReceiveOrders(FalconEvent* theEvent)
{
    FalconWingmanMsg *wingCommand = (FalconWingmanMsg *)theEvent;
    int goLead = FALSE;

    if ( not self->IsAwake())
        return;

    switch (wingCommand->dataBlock.command)
    {
        case FalconWingmanMsg::WMSpread:
        case FalconWingmanMsg::WMWedge:
        case FalconWingmanMsg::WMTrail:
        case FalconWingmanMsg::WMLadder:
        case FalconWingmanMsg::WMStack:
        case FalconWingmanMsg::WMResCell:
        case FalconWingmanMsg::WMBox:
        case FalconWingmanMsg::WMArrowHead:
        case FalconWingmanMsg::WMFluidFour:
            curFormation = wingCommand->dataBlock.command;

        case FalconWingmanMsg::WMRejoin:
            underOrders = FALSE;
            break;

        case FalconWingmanMsg::WMBreakRight:
            underOrders = TRUE;
            headingOrdered = self->Yaw() + 90.0F * DTR;
            altitudeOrdered = self->ZPos();
            curOrder = wingCommand->dataBlock.command;
            break;

        case FalconWingmanMsg::WMBreakLeft:
            underOrders = TRUE;
            headingOrdered = self->Yaw() - 90.0F * DTR;
            altitudeOrdered = self->ZPos();
            curOrder = wingCommand->dataBlock.command;
            break;

        case FalconWingmanMsg::WMAssignTarget:
            break;

        case FalconWingmanMsg::WMPosthole:
        case FalconWingmanMsg::WMPince:
        case FalconWingmanMsg::WMChainsaw:
            break;

        case FalconWingmanMsg::WMFree:
            goLead = TRUE;
            underOrders = FALSE;
            break;

        case FalconWingmanMsg::WMPromote:
            isWing --;

            if ( not isWing)
            {
                SetLead(TRUE);
                self->flightLead = self;
            }
            else
                self->flightLead = (HelicopterClass *)self->GetCampaignObject()->GetComponentLead();

            break;

        default:
            curFormation = FalconWingmanMsg::WMWedge;
            //MonoPrint ("Digi %d received bad order %d at\n", self->Id().num_,
            //wingCommand->dataBlock.command, SimLibElapsedTime);
            break;
    }

    if (goLead and isWing)
    {
        SetLead(TRUE);
    }
    else if (isWing)
    {
        SetLead(FALSE);
    }

    //MonoPrint ("Message received %d at %.2f\n",
    //self->Id().num_, SimLibElapsedTime);
}

void HeliBrain::CheckOrders(void)
{
    if (underOrders)
    {
        AddMode(FollowOrdersMode);
    }
}

void HeliBrain::FollowOrders(void)
{
    float desSpeed;
    int turnType;

    if (self->flightLead == self)
        Loiter();

    switch (curOrder)
    {
        case FalconWingmanMsg::WMBreakRight:
        case FalconWingmanMsg::WMBreakLeft:
            trackX = self->XPos() + 5000.0F * (float)cos(headingOrdered);
            trackY = self->YPos() + 5000.0F * (float)sin(headingOrdered);
            desSpeed = CORNER_SPEED;
            turnType = 1;
            break;

        case FalconWingmanMsg::WMPosthole:
            trackX = self->XPos();
            trackY = self->YPos();
            trackZ = 0.0F;
            desSpeed = CORNER_SPEED;
            turnType = 1;
            break;

        case FalconWingmanMsg::WMChainsaw:
            break;

        case FalconWingmanMsg::WMPince:
            break;
    }

    AutoTrack(100.0f);
}

void HeliBrain::FollowLead(void)
{
    Tpoint newpos;

    HeliBrain *leadBrain = NULL;

    float rng = 0.0F;
    float dx = 0.0F, dy = 0.0F, dz = 0.0F;
    float speedXY;

    if (self->OnGround())
    {
        self->UnSetFlag(ON_GROUND);
    }

    // RV - Biker - Stay on ground if lead does also
    if (self->flightLead->curWaypoint->GetWPFlags() bitand WPF_TAKEOFF  and 
        self->flightLead->curWaypoint->GetWPDepartureTime() > SimLibElapsedTime and 
        self->flightLead->curWaypoint->GetPrevWP() == NULL)
    {
        LevelTurn(0.0f, 0.0f, TRUE);
        MachHold(0.0f, 0.0f, FALSE);
        // RV - Biker - Extend landing gear
        ((DrawableBSP*)self->drawPointer)->SetSwitchMask(2, 1);
        return;
    }

    // FRB - Get the lead's WP altitude
    self->SetWPalt(self->flightLead->GetWPalt());

    // RV - Biker - Retract landing gear
    ((DrawableBSP*)self->drawPointer)->SetSwitchMask(2, 0);

    // maybe there is a variable with speed in XY plane already
    speedXY = sqrt(self->XDelta() * self->XDelta() + self->YDelta() * self->YDelta());

    if ((self->flightLead->hBrain->onStation == Landed or self->flightLead->hBrain->onStation == Landing) and speedXY <= 20.0f)
    {
        if (onStation < Arrived)
            onStation = Arrived;

        LandMe();
        return;
    }
    else
    {
        onStation = NotThereYet;
    }


    if (self->flightLead == self)
    {
        Loiter();
    }

    if (self->FCC->GetMasterMode() == FireControlComputer::AirGroundMissile)
    {
        self->GetFormationPos(&newpos.x, &newpos.y, &newpos.z);
        trackX = newpos.x + 8000.0F;
        trackY = newpos.y + 100.0F;
        trackZ = newpos.z + 100.0F;
        //MonoPrint ("AGM Formation\n");
    }
    else if (self->FCC->GetMasterMode() == FireControlComputer::AirGroundBomb or
             self->FCC->GetMasterMode() == FireControlComputer::AirGroundRocket)
    {
        self->GetFormationPos(&newpos.x, &newpos.y, &newpos.z);
        trackX = newpos.x + 2000.0F;
        trackY = newpos.y + 2000.0F;
        trackZ = newpos.z + 200.0F;
        //MonoPrint ("Rocket Formation\n");
    }
    else
    {
        self->GetFormationPos(&newpos.x, &newpos.y, &newpos.z);
        trackX = newpos.x;
        trackY = newpos.y;
        trackZ = newpos.z;

        dx = newpos.x - self->XPos();
        dy = newpos.y - self->YPos();
        dz = newpos.z - self->ZPos();
        rng = dx * dx + dy * dy + dz * dz;
    }

    AutoTrack(0.0f);
}
