/*
 * Machine Generated message creation function file.
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 14-December-1998 at 14:17:07
 * Generated from file EVENTS.XLS by MicroProse
 */

/* Include Files */
#include "mesg.h"
#include "cuievent.h"
#include "campstr.h"
#include "sim/include/simvudrv.h"

#include "msginc/damagemsg.h"
#include "msginc/weaponfiremsg.h"
#include "msginc/campweaponfiremsg.h"
#include "msginc/campmsg.h"
#include "msginc/simcampmsg.h"
#include "msginc/unitmsg.h"
#include "msginc/objectivemsg.h"
#include "msginc/unitassignmentmsg.h"
#include "msginc/sendcampaignmsg.h"
#include "msginc/timingmsg.h"
#include "msginc/camptaskingmsg.h"
#include "msginc/airtaskingmsg.h"
#include "msginc/gndtaskingmsg.h"
#include "msginc/navaltaskingmsg.h"
#include "msginc/teammsg.h"
#include "msginc/wingmanmsg.h"
#include "msginc/airaimodechange.h"
#include "msginc/missionrequestmsg.h"
#include "msginc/divertmsg.h"
#include "msginc/weathermsg.h"
#include "msginc/missileendmsg.h"
#include "msginc/awacsmsg.h"
#include "msginc/facmsg.h"
#include "msginc/atcmsg.h"
#include "msginc/deathmessage.h"
#include "msginc/campeventmsg.h"
#include "msginc/landingmessage.h"
#include "msginc/controlsurfacemsg.h"
#include "msginc/simdatatoggle.h"
#include "msginc/requestdogfightinfo.h"
#include "msginc/senddogfightinfo.h"
#include "msginc/requestaircraftslot.h"
#include "msginc/sendaircraftslot.h"
#include "msginc/graphicstextdisplaymsg.h"
#include "msginc/addsfxmessage.h"
#include "msginc/sendpersistantlist.h"
#include "msginc/sendobjdata.h"
#include "msginc/sendunitdata.h"
#include "msginc/requestcampaigndata.h"
#include "msginc/sendchatmessage.h"
#include "msginc/tankermsg.h"
#include "msginc/ejectmsg.h"
#include "msginc/trackmsg.h"
#include "msginc/campdatamsg.h"
#include "msginc/voicedatamsg.h"
#include "msginc/radiochattermsg.h"
#include "msginc/playerstatusmsg.h"
#include "msginc/laserdesignatemsg.h"
#include "msginc/atccmdmsg.h"
#include "msginc/dlinkmsg.h"
#include "msginc/requestobject.h"
#include "msginc/regenerationmsg.h"
#include "msginc/requestlogbook.h"
#include "msginc/sendlogbook.h"
#include "msginc/sendimage.h"
#include "msginc/falconflightplanmsg.h"
#include "msginc/simdirtydatamsg.h"
#include "msginc/campdirtydatamsg.h"
#include "msginc/campeventdatamsg.h"
#include "msginc/sendvcmsg.h"
#include "msginc/senduimsg.h"
#include "msginc/sendevalmsg.h"
#include "msginc/requestsimmoverposition.h"
#include "msginc/sendsimmoverposition.h"

//#define F4_DEBUG_COMMS

/*
 * vuxCreateMessage
 */
VuMessage* VuxCreateMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target)
{
    VuMessage* retval = NULL;

#ifdef F4_DEBUG_COMMS
    MonoPrint("Received Message type: %d - ", type);
#endif

    switch (type)
    {
    case DamageMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("DamageMsg\n");
#endif
        retval = new FalconDamageMessage(type, senderid, target);
        break;

    case WeaponFireMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("WeaponFireMsg\n");
#endif
        retval = new FalconWeaponsFire(type, senderid, target);
        break;

    case CampWeaponFireMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("CampWeaponFireMsg\n");
#endif
        retval = new FalconCampWeaponsFire(type, senderid, target);
        break;

    case CampMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("CampMsg\n");
#endif
        retval = new FalconCampMessage(type, senderid, target);
        break;

    case SimCampMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("SimCampMsg\n");
#endif
        retval = new FalconSimCampMessage(type, senderid, target);
        break;

    case UnitMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("UnitMsg\n");
#endif
        retval = new FalconUnitMessage(type, senderid, target);
        break;

    case ObjectiveMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("ObjectiveMsg\n");
#endif
        retval = new FalconObjectiveMessage(type, senderid, target);
        break;

    case UnitAssignmentMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("UnitAssignmentMsg\n");
#endif
        retval = new FalconUnitAssignmentMessage(type, senderid, target);
        break;

    case SendCampaignMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("SendCampaignMsg\n");
#endif
        retval = new FalconSendCampaign(type, senderid, target);
        break;

    case TimingMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("TimingMsg\n");
#endif
        retval = new FalconTimingMessage(type, senderid, target);
        break;

    case CampTaskingMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("CampTaskingMsg\n");
#endif
        retval = new FalconCampTaskingMessage(type, senderid, target);
        break;

    case AirTaskingMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("AirTaskingMsg\n");
#endif
        retval = new FalconAirTaskingMessage(type, senderid, target);
        break;

    case GndTaskingMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("GndTaskingMsg\n");
#endif
        retval = new FalconGndTaskingMessage(type, senderid, target);
        break;

    case NavalTaskingMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("NavalTaskingMsg\n");
#endif
        retval = new FalconNavalTaskingMessage(type, senderid, target);
        break;

    case TeamMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("TeamMsg\n");
#endif
        retval = new FalconTeamMessage(type, senderid, target);
        break;

    case WingmanMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("WingmanMsg\n");
#endif
        retval = new FalconWingmanMsg(type, senderid, target);
        break;

    case AirAIModeChange:
#ifdef F4_DEBUG_COMMS
        MonoPrint("AirAIModeChange\n");
#endif
        retval = new AirAIModeMsg(type, senderid, target);
        break;

    case MissionRequestMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("MissionRequestMsg\n");
#endif
        retval = new FalconMissionRequestMessage(type, senderid, target);
        break;

    case DivertMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("DivertMsg\n");
#endif
        retval = new FalconDivertMessage(type, senderid, target);
        break;

    case WeatherMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("WeatherMsg\n");
#endif
        retval = new FalconWeatherMessage(type, senderid, target);
        break;

    case MissileEndMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("MissileEndMsg\n");
#endif
        retval = new FalconMissileEndMessage(type, senderid, target);
        break;

    case AWACSMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("AWACSMsg\n");
#endif
        retval = new FalconAWACSMessage(type, senderid, target);
        break;

    case FACMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("FACMsg\n");
#endif
        retval = new FalconFACMessage(type, senderid, target);
        break;

    case ATCMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("ATCMsg\n");
#endif
        retval = new FalconATCMessage(type, senderid, target);
        break;

    case DeathMessage:
#ifdef F4_DEBUG_COMMS
        MonoPrint("DeathMessage\n");
#endif
        retval = new FalconDeathMessage(type, senderid, target);
        break;

    case CampEventMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("CampEventMsg\n");
#endif
        retval = new FalconCampEventMessage(type, senderid, target);
        break;

    case LandingMessage:
#ifdef F4_DEBUG_COMMS
        MonoPrint("LandingMessage\n");
#endif
        retval = new FalconLandingMessage(type, senderid, target);
        break;

    case ControlSurfaceMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("ControlSurfaceMsg\n");
#endif
        retval = new FalconControlSurfaceMsg(type, senderid, target);
        break;

    case SimDataToggle:
#ifdef F4_DEBUG_COMMS
        MonoPrint("SimDataToggle\n");
#endif
        retval = new FalconSimDataToggle(type, senderid, target);
        break;

    case RequestDogfightInfo:
#ifdef F4_DEBUG_COMMS
        MonoPrint("RequestDogfightInfo\n");
#endif
        retval = new UI_RequestDogfightInfo(type, senderid, target);
        break;

    case SendDogfightInfo:
#ifdef F4_DEBUG_COMMS
        MonoPrint("SendDogfightInfo\n");
#endif
        retval = new UI_SendDogfightInfo(type, senderid, target);
        break;

    case RequestAircraftSlot:
#ifdef F4_DEBUG_COMMS
        MonoPrint("RequestAircraftSlot\n");
#endif
        retval = new UI_RequestAircraftSlot(type, senderid, target);
        break;

    case SendAircraftSlot:
#ifdef F4_DEBUG_COMMS
        MonoPrint("SendAircraftSlot\n");
#endif
        retval = new UI_SendAircraftSlot(type, senderid, target);
        break;

    case GraphicsTextDisplayMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("GraphicsTextDisplayMsg\n");
#endif
        retval = new GraphicsTextDisplay(type, senderid, target);
        break;

    case AddSFXMessage:
#ifdef F4_DEBUG_COMMS
        MonoPrint("AddSFXMessage\n");
#endif
        retval = new FalconAddSFXMessage(type, senderid, target);
        break;

    case SendPersistantList:
#ifdef F4_DEBUG_COMMS
        MonoPrint("SendPersistantList\n");
#endif
        retval = new FalconSendPersistantList(type, senderid, target);
        break;

    case SendObjData:
#ifdef F4_DEBUG_COMMS
        MonoPrint("SendObjData\n");
#endif
        retval = new FalconSendObjData(type, senderid, target);
        break;

    case SendUnitData:
#ifdef F4_DEBUG_COMMS
        MonoPrint("SendUnitData\n");
#endif
        retval = new FalconSendUnitData(type, senderid, target);
        break;

    case RequestCampaignData:
#ifdef F4_DEBUG_COMMS
        MonoPrint("RequestCampaignData\n");
#endif
        retval = new FalconRequestCampaignData(type, senderid, target);
        break;

    case SendChatMessage:
#ifdef F4_DEBUG_COMMS
        MonoPrint("SendChatMessage\n");
#endif
        retval = new UI_SendChatMessage(type, senderid, target);
        break;

    case TankerMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("TankerMsg\n");
#endif
        retval = new FalconTankerMessage(type, senderid, target);
        break;

    case EjectMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("EjectMsg\n");
#endif
        retval = new FalconEjectMessage(type, senderid, target);
        break;

    case TrackMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("TrackMsg\n");
#endif
        retval = new FalconTrackMessage(type, senderid, target);
        break;

    case CampDataMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("CampDataMsg\n");
#endif
        retval = new FalconCampDataMessage(type, senderid, target);
        break;

    case VoiceDataMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("VoiceDataMsg\n");
#endif
        retval = new FalconVoiceDataMessage(type, senderid, target);
        break;

    case RadioChatterMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("RadioChatterMsg\n");
#endif
        retval = new FalconRadioChatterMessage(type, senderid, target);
        break;

    case PlayerStatusMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("PlayerStatusMsg\n");
#endif
        retval = new FalconPlayerStatusMessage(type, senderid, target);
        break;

    case LaserDesignateMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("LaserDesignateMsg\n");
#endif
        retval = new FalconLaserDesignateMsg(type, senderid, target);
        break;

    case ATCCmdMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("ATCCmdMsg\n");
#endif
        retval = new FalconATCCmdMessage(type, senderid, target);
        break;

    case DLinkMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("DLinkMsg\n");
#endif
        retval = new FalconDLinkMessage(type, senderid, target);
        break;

    case RequestObject:
#ifdef F4_DEBUG_COMMS
        MonoPrint("RequestObject\n");
#endif
        retval = new FalconRequestObject(type, senderid, target);
        break;

    case RegenerationMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("RegenerationMsg\n");
#endif
        retval = new FalconRegenerationMessage(type, senderid, target);
        break;

    case RequestLogbook:
#ifdef F4_DEBUG_COMMS
        MonoPrint("RequestLogbook\n");
#endif
        retval = new UI_RequestLogbook(type, senderid, target);
        break;

    case SendLogbook:
#ifdef F4_DEBUG_COMMS
        MonoPrint("SendLogbook\n");
#endif
        retval = new UI_SendLogbook(type, senderid, target);
        break;

    case SendImage:
#ifdef F4_DEBUG_COMMS
        MonoPrint("SendImage\n");
#endif
        retval = new UI_SendImage(type, senderid, target);
        break;

    case FalconFlightPlanMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("FalconFlightPlanMsg\n");
#endif
        retval = new FalconFlightPlanMessage(type, senderid, target);
        break;

    case SimDirtyDataMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("SimDirtyDataMsg\n");
#endif
        retval = new SimDirtyData(type, senderid, target);
        break;

    case CampDirtyDataMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("CampDirtyDataMsg\n");
#endif
        retval = new CampDirtyData(type, senderid, target);
        break;

    case CampEventDataMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("CampEventDataMsg\n");
#endif
        retval = new CampEventDataMessage(type, senderid, target);
        break;

    case SendVCMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("SendVCMsg\n");
#endif
        retval = new FalconSendVC(type, senderid, target);
        break;

    case SendUIMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("SendUIMsg\n");
#endif
        retval = new UISendMsg(type, senderid, target);
        break;

    case SendEvalMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("SendEvalMsg\n");
#endif
        retval = new SendEvalMessage(type, senderid, target);
        break;

            // sfr: added position messages
    case RequestSimMoverPositionMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("RequestSimMoverPosition\n");
#endif
        retval = new RequestSimMoverPosition(senderid, target);
        break;

    case SendSimMoverPositionMsg:
#ifdef F4_DEBUG_COMMS
        MonoPrint("SendSimMoverPosition\n");
#endif
        retval = new SendSimMoverPosition(senderid, target);
        break;

#if 0

        case SimPositionUpdateMsg:
#ifdef F4_DEBUG_COMMS
            MonoPrint("SimPositionUpdateMsg\n");
#endif
            retval = new SimPositionUpdateEvent(type, senderid, target);
            break;

        case SimRoughPositionUpdateMsg:
#ifdef F4_DEBUG_COMMS
            MonoPrint("SimRoughPositionUpdateMsg\n");
#endif
            retval = new SimRoughPositionUpdateEvent(type, senderid, target);
            break;
#endif
    }

    return retval;
}
