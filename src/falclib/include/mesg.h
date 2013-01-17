#ifndef _FALCON_MESSAGE_TYPES_H
#define _FALCON_MESSAGE_TYPES_H

/*
 * Required Include Files
 */
#include "falclib.h"



/*
 * Shared data arrays
 */
extern int MsgNumElements[];   /* Number of elements in each message */
extern int FalconMsgIdStr[];   /* Array of Message name strings      */
extern int* FalconMsgElementStr[];  /* Array of element name arrays  */
extern int* FalconMsgElementTypes[];/* Array of element type arrays  */
extern int* FalconMsgEnumStr[];        /* Array of enum name arrays  */
extern char* TheEventStrings[];       /* List of event desc strings  */

/*
 * Message Types
 */
enum FalconMsgID {
   DamageMsg = VU_LAST_EVENT + 1,      // 21
   WeaponFireMsg,                      // 22
   CampWeaponFireMsg,                  // 23
   CampMsg,                            // 24
   SimCampMsg,                         // 25
   UnitMsg,                            // 26
   ObjectiveMsg,                       // 27
   UnitAssignmentMsg,                  // 28
   SendCampaignMsg,                    // 29
   TimingMsg,                          // 30
   CampTaskingMsg,                     // 31
   AirTaskingMsg,                      // 32
   GndTaskingMsg,                      // 33
   NavalTaskingMsg,                    // 34
   TeamMsg,                            // 35
   WingmanMsg,                         // 36
   AirAIModeChange,                    // 37
   MissionRequestMsg,                  // 38
   DivertMsg,                          // 39
   EmptyMsg,						   // 40
   WeatherMsg,                         // 41
   MissileEndMsg,                      // 42
   AWACSMsg,                           // 43
   FACMsg,                             // 44
   ATCMsg,                             // 45
   DeathMessage,                       // 46
   CampEventMsg,                       // 47
   LandingMessage,                     // 48
   ControlSurfaceMsg,                  // 49
   SimDataToggle,                      // 50
   RequestDogfightInfo,                // 51
   SendDogfightInfo,                   // 52
   RequestAircraftSlot,                // 53
   SendAircraftSlot,                   // 54
   GraphicsTextDisplayMsg,             // 55
   AddSFXMessage,                      // 56
   SendPersistantList,                 // 57
   SendObjData,                        // 58
   SendUnitData,                       // 59
   RequestCampaignData,                // 60
   SendChatMessage,                    // 61
   TankerMsg,                          // 62
   EjectMsg,                           // 63
   TrackMsg,                           // 64
   CampDataMsg,                        // 65
   VoiceDataMsg,                       // 66
   RadioChatterMsg,                    // 67
   PlayerStatusMsg,                    // 68
   LaserDesignateMsg,                  // 69
   ATCCmdMsg,                          // 70
   DLinkMsg,                           // 71
   RequestObject,                      // 72
   RegenerationMsg,                    // 73
   RequestLogbook,                     // 74
   SendLogbook,                        // 75
   SendImage,                          // 76
   FalconFlightPlanMsg,                // 77
   SimDirtyDataMsg,                    // 78
   CampDirtyDataMsg,                   // 79
   CampEventDataMsg,                   // 80
   SendVCMsg,                          // 81
   SendUIMsg,                          // 82
   SendEvalMsg,                        // 83
   SimPositionUpdateMsg,               // 84
   SimRoughPositionUpdateMsg,          // 85
   RequestSimMoverPositionMsg,         // 86
   SendSimMoverPositionMsg,            // 87
   LastFalconEvent                     // ...
   };



#endif

