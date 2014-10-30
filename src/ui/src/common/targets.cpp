// Targetting Code goes here
#include <windows.h>
#include "Graphics/Include/TimeMgr.h"
#include "Graphics/Include/imagebuf.h"
#include "Graphics/Include/renderow.h"
#include "Graphics/Include/RViewPnt.h"
#include "Graphics/Include/drawbrdg.h"
#include "Graphics/Include/drawplat.h"
#include "Graphics/Include/drawbsp.h"
#include "vu2.h"
#include "F4vu.h"
#include "team.h"
//#include "simbase.h"
//#include "simlib.h"
//#include "initdata.h"
//#include "simfiltr.h"
//#include "simdrive.h"
//#include "simfeat.h"
#include "vehicle.h"
#include "objectiv.h"
#include "division.h"
#include "feature.h"
#include "find.h"
#include "dispcfg.h"
#include "Graphics/Include/setup.h"
#include "Graphics/Include/loader.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "cmap.h"
#include "gps.h"
#include "cbsplist.h"
#include "c3dview.h"
#include "userids.h"
#include "textids.h"
#include "teamdata.h"
#include "classtbl.h"
#include "PtData.h"

#include "FalcLib/include/playerop.h" // OW

void CenterOnFeatureCB(long ID, short hittype, C_Base *control);
void SetBullsEye(C_Window *);
void SetSlantRange(C_Window *);
void SetHeading(C_Window *);
void PositionCamera(OBJECTINFO *Info, C_Window *win, long client);
void CloseAllRenderers(long openID);

extern C_Handler *gMainHandler;
extern C_TreeList *TargetTree;
extern C_3dViewer *gUIViewer;
extern OBJECTINFO Recon;
extern GlobalPositioningSystem *gGps;
extern VU_ID FeatureID;
extern long FeatureNo;

void PickFirstChildCB(long, short hittype, C_Base *control)
{
    TREELIST *parent, *item;
    C_Entity *cent;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    cent = (C_Entity*)control;

    if ( not cent)
        return;

    parent = cent->GetOwner();

    if (parent)
        return;

    FeatureID = cent->GetVUID();
    FeatureNo = 0;

    item = parent->Child;

    if (item)
        CenterOnFeatureCB(RECON_TREE, hittype, item->Item_);
}

C_Entity *BuildObjective(Objective obj)
{
    C_Resmgr *res;
    IMAGE_RSC *rsc;
    C_Entity *newinfo;
    _TCHAR buffer[200];
    ObjClassDataType *ObjPtr;
    int type = 0;

    ObjPtr = obj->GetObjectiveClassData();

    if (ObjPtr == NULL)
        return(NULL);

    // Create new parent class
    newinfo = new C_Entity;
    newinfo->Setup(obj->GetCampID(), C_TYPE_ITEM);
    newinfo->SetWH(500, 37);
    newinfo->SetFlagBitOff(C_BIT_USEBGFILL);
    newinfo->InitEntity();

    newinfo->SetVUID(obj->Id());
    newinfo->SetUserNumber(0, obj->GetTeam());

    newinfo->SetOperational(obj->GetObjectiveStatus());

    res = gImageMgr->GetImageRes(TeamColorIconIDs[TeamInfo[obj->GetTeam()]->GetColor()][0]);

    if (res)
    {
        rsc = (IMAGE_RSC*)res->Find(ObjPtr->IconIndex);

        if (rsc and rsc->Header->Type == _RSC_IS_IMAGE_)
            type = rsc->Header->h;
        else
            type = 0;
    }
    else
        rsc = NULL;

    if (type > 35)
        newinfo->SetH(type + 2);
    else
        type = 35;

    // Set Icon Image
    newinfo->SetIcon(15, static_cast<short>(type / 2), rsc);

    // Set Name
    obj->GetName(buffer, 40, TRUE);
    newinfo->SetName(35, 10, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    // Set # bitand Airplane type
    _stprintf(buffer, "%1d%% %s", newinfo->GetOperational(), gStringMgr->GetString(TXT_OPERATIONAL));
    newinfo->SetStatus(300, 10, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    return(newinfo);
}

C_Feature *BuildFeature(Objective obj, long featureID, Tpoint *)
{
    C_Feature *feat;
    long TextID;
    FeatureClassDataType* fc;
    long classID;

    classID = obj->GetFeatureID(featureID);

    if (classID)
    {
        fc = GetFeatureClassData(classID);

        // RV - Biker - Don't add trees to target list
        if ( not fc or fc->Flags bitand FEAT_VIRTUAL or fc->Flags bitand FEAT_NO_HITEVAL)
            return(NULL);

        feat = new C_Feature;
        feat->Setup(obj->GetCampID() << 16 bitor featureID, 0);
        feat->SetFlagBitOff(C_BIT_USEBGFILL);
        feat->SetColor(0xc0c0c0, 0x00ff00);
        feat->InitEntity();
        feat->SetName(25, 0, fc->Name);

        switch (obj->GetFeatureStatus(featureID))
        {
            case 0:
                TextID = TXT_NO_DAMAGE;
                break;

            case 1:
                TextID = TXT_REPAIRED;
                break;

            case 2:
                TextID = TXT_DAMAGED;
                break;

            case 3:
                TextID = TXT_DESTROYED;
                break;

            default:
                TextID = TXT_NO_DAMAGE;
                break;
        }

        feat->SetStatus(280, 0, TextID);

        if (obj->GetFeatureValue(featureID) < 10)
            TextID = TXT_VERYLOW;
        else if (obj->GetFeatureValue(featureID) < 25)
            TextID = TXT_LOW;
        else if (obj->GetFeatureValue(featureID) < 40)
            TextID = TXT_MEDIUM;
        else if (obj->GetFeatureValue(featureID) < 50)
            TextID = TXT_HIGH;
        else
            TextID = TXT_VERYHIGH;

        feat->SetValue(390, 0, TextID);
        feat->SetCallback(CenterOnFeatureCB);
        feat->SetVUID(obj->Id());
        feat->SetFeatureID(featureID);
        feat->SetFeatureValue(static_cast<uchar>(obj->GetFeatureValue(featureID)));
        return(feat);
    }

    return(NULL);
}

C_Entity *BuildUnitParent(Unit unit)
{
    C_Resmgr *res;
    IMAGE_RSC *rsc;
    C_Entity *newinfo;
    _TCHAR buffer[200];
    UnitClassDataType *UnitPtr;
    long type = 0;

    UnitPtr = unit->GetUnitClassData();

    if (UnitPtr == NULL)
        return(NULL);

    // Create new parent class
    newinfo = new C_Entity;
    newinfo->Setup(unit->GetCampID(), C_TYPE_ITEM);
    newinfo->SetWH(500, 37);
    newinfo->SetFlagBitOff(C_BIT_USEBGFILL);
    newinfo->InitEntity();

    newinfo->SetVUID(unit->Id());
    newinfo->SetUserNumber(0, unit->GetTeam());

    if (unit->GetFullstrengthVehicles())
    {
        if (unit->IsFlight())
        {
            short planecount;
            Flight flt = (Flight)unit;
            planecount = 0;

            while (flt->plane_stats[planecount] not_eq AIRCRAFT_NOT_ASSIGNED and planecount < PILOTS_PER_FLIGHT)
                planecount++;

            if (planecount)
                newinfo->SetOperational(static_cast<uchar>(unit->GetTotalVehicles() * 100 / planecount));
            else
                newinfo->SetOperational(0);
        }
        else
            newinfo->SetOperational(static_cast<uchar>(unit->GetTotalVehicles() * 100 / unit->GetFullstrengthVehicles()));
    }
    else
        newinfo->SetOperational(0);

    res = gImageMgr->GetImageRes(TeamColorIconIDs[TeamInfo[unit->GetTeam()]->GetColor()][0]);

    if (res)
    {
        rsc = (IMAGE_RSC*)res->Find(UnitPtr->IconIndex);

        if (rsc and rsc->Header->Type == _RSC_IS_IMAGE_)
            type = rsc->Header->h;
        else
            type = 0;
    }
    else
        rsc = NULL;

    if (type > 35)
        newinfo->SetH(type + 2);
    else
        type = 35;

    // Set Icon Image
    newinfo->SetIcon(15, static_cast<short>(type / 2), rsc);

    // Set Name
    // 2002-02-21 ADDED BY S.G. 'Fog of war code'. If an enemy flight and not identified and not editing a TE, change its label to 'Bandit'
    if ( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT) and unit->IsFlight() and gGps->GetTeamNo() >= 0 and unit->GetTeam() not_eq gGps->GetTeamNo() and not unit->GetIdentified(static_cast<uchar>(gGps->GetTeamNo())))
        _stprintf(buffer, "Bandit");
    else
        // END OF ADDED SECTION 2002-02-21
        unit->GetName(buffer, 40, FALSE);

    newinfo->SetName(35, 10, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    // Set # bitand Airplane type
    _stprintf(buffer, "%1d%% %s", newinfo->GetOperational(), gStringMgr->GetString(TXT_OPERATIONAL));
    newinfo->SetStatus(300, 10, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    return(newinfo);
}
C_Feature *BuildUnit(Unit un, long vehno, long vehid, Tpoint *)
{
    C_Feature *veh;
    long TextID;
    VehicleClassDataType *vc;
    long classID;

    classID = un->GetVehicleID(vehno);

    if (classID)
    {
        vc = GetVehicleClassData(classID);
        veh = new C_Feature;
        veh->Setup(un->GetCampID() << 16 bitor vehid, 0);
        veh->SetFlagBitOff(C_BIT_USEBGFILL);
        veh->SetColor(0xc0c0c0, 0x00ff00);
        veh->InitEntity();

        // 2002-02-21 ADDED BY S.G. 'Fog of war code'. If an enemy flight and not identified and not editing a TE, change its label to 'Bandit'
        if ( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT) and un->IsFlight() and gGps->GetTeamNo() >= 0 and un->GetTeam() not_eq gGps->GetTeamNo() and not un->GetIdentified(static_cast<uchar>(gGps->GetTeamNo())))
            veh->SetName(25, 0, "Bandit");
        else
            // END OF ADDED SECTION 2002-02-21
            veh->SetName(25, 0, vc->Name);

        TextID = TXT_NO_DAMAGE;
        veh->SetStatus(280, 0, TextID);
        TextID = TXT_VERYHIGH;
        veh->SetValue(390, 0, TextID);
        veh->SetCallback(CenterOnFeatureCB);
        veh->SetVUID(un->Id());
        veh->SetFeatureID(vehno);
        veh->SetFeatureValue(100);
        return(veh);
    }

    return(NULL);
}

void AddUnitToTargetTree(Unit unit)
{
    Tpoint objPos;
    TREELIST *item, *parent;
    C_Entity *recon_ent;
    C_Feature *veh;

    if (gGps->GetTeamNo() >= 0 and unit->GetTeam() not_eq gGps->GetTeamNo())
        if ( not unit->GetSpotted(static_cast<uchar>(gGps->GetTeamNo())) and not unit->IsFlight())
            return;

    if (TargetTree)
    {
        SimInitDataClass simdata;
        int v, vehs, motiontype, inslot, visType;
        VehicleID classID;
        VehicleClassDataType *vc;

        // Check for possible problems
        if (unit->IsFlight() and not unit->ShouldDeaggregate())
            return;

        // Now add all the vehicles
        recon_ent = BuildUnitParent(unit);

        if (recon_ent)
            recon_ent->SetCallback(PickFirstChildCB);

        parent = TargetTree->CreateItem(unit->GetCampID(), C_TYPE_MENU, recon_ent);

        if (parent)
        {
            TargetTree->AddItem(TargetTree->GetRoot(), parent);

            if (recon_ent)
                recon_ent->SetOwner(parent);
        }

        // 2002-02-21 ADDED BY S.G. 'Fog of war code'. If an enemy flight and not identified and not editing a TE, don't break it down by vehicle so it can't be reconed either NOTE THE '!' IN FRONT OF THE WHOLE STATEMENT TO REVERSE IT
        if ( not ( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT) and unit->IsFlight() and gGps->GetTeamNo() >= 0 and unit->GetTeam() not_eq gGps->GetTeamNo() and not unit->GetIdentified(static_cast<uchar>(gGps->GetTeamNo()))))
        {
            // END OF ADDED DATA 2002-02-21
            simdata.vehicleInUnit = -1;

            for (v = 0; v < VEHICLE_GROUPS_PER_UNIT; v++)
            {
                vehs = unit->GetNumVehicles(v);
                classID = unit->GetVehicleID(v);
                inslot = 0;

                while (vehs and classID)
                {
                    vc = GetVehicleClassData(classID);
                    simdata.campBase = unit;
                    simdata.campSlot = v;
                    simdata.inSlot = inslot;
                    simdata.descriptionIndex = classID + VU_LAST_ENTITY_TYPE;
                    simdata.flags = vc->Flags;
                    // Set the position initially to the unit's real position
                    simdata.x = unit->XPos();
                    simdata.y = unit->YPos();
                    simdata.z = unit->ZPos();
                    // Now query for any offsets
                    motiontype = unit->GetVehicleDeagData(&simdata, FALSE);
                    objPos.x = simdata.x;
                    objPos.y = simdata.y;
                    objPos.z = simdata.z;
                    visType = Falcon4ClassTable[classID].visType[VIS_NORMAL];
                    gUIViewer->LoadDrawableUnit(unit->GetCampID() << 16 bitor (v << 8) bitor (vehs + 1), visType, &objPos, simdata.heading, Falcon4ClassTable[classID].vuClassData.classInfo_[VU_DOMAIN], Falcon4ClassTable[classID].vuClassData.classInfo_[VU_TYPE], Falcon4ClassTable[classID].vuClassData.classInfo_[VU_STYPE]);

                    veh = BuildUnit(unit, v, (v << 8) bitor vehs + 1, &objPos);

                    if (veh)
                    {
                        item = TargetTree->CreateItem(unit->GetCampID() << 16 bitor (v << 8) bitor (vehs + 1), C_TYPE_ITEM, veh);

                        if (item)
                        {
                            TargetTree->AddChildItem(parent, item);
                            veh->SetOwner(item);
                        }
                    }

                    vehs--;
                    inslot++;
                }
            }
        }
    }
}

#include "../../../Sim/Include/Atcbrain.h" // 2002-02-28 S.G.

void AddObjectiveToTargetTree(Objective obj)
{
    short f, fid;
    VehicleID classID;
    Falcon4EntityClassType* classPtr;
    float x, y, z;
    FeatureClassDataType* fc;
    ObjClassDataType* oc;
    BSPLIST *drawptr;
    Tpoint objPos;
    TREELIST *item, *parent;
    C_Entity *recon_ent;
    C_Feature *feat;
    BSPLIST *Parent;
    short ShowAllFeatures = 0;

    if (TargetTree)
    {
        recon_ent = BuildObjective(obj);

        if (recon_ent)
            recon_ent->SetCallback(PickFirstChildCB);

        parent = TargetTree->CreateItem(obj->GetCampID(), C_TYPE_MENU, recon_ent);

        if (parent)
        {
            TargetTree->AddItem(TargetTree->GetRoot(), parent);
            recon_ent->SetOwner(parent);
        }

        Parent = NULL;

        if (obj->GetType() == TYPE_CITY or obj->GetType() == TYPE_TOWN or obj->GetType() == TYPE_VILLAGE)
            ShowAllFeatures = 1;

        oc = obj->GetObjectiveClassData();
        fid = oc->FirstFeature;

        for (f = 0; f < oc->Features; f++, fid++)
        {
            classID = static_cast<short>(obj->GetFeatureID(f));

            if (classID)
            {
                fc = GetFeatureClassData(classID);

                if ( not fc or fc->Flags bitand FEAT_VIRTUAL)
                    continue;

                obj->GetFeatureOffset(f, &y, &x, &z);
                objPos.x = x + obj->XPos();
                objPos.y = y + obj->YPos();
                objPos.z = z;
                classPtr = &Falcon4ClassTable[fc->Index];

                if (classPtr not_eq NULL)
                {
                    drawptr = gUIViewer->LoadDrawableFeature(obj->GetCampID() << 16 bitor f, obj, f, fid, classPtr, fc, &objPos, Parent);

                    if (drawptr not_eq NULL)
                    {
                        // 2002-02-28 ADDED BY S.G. If runway, adjust texture so runway number is accurate
                        if (Falcon4ClassTable[fc->Index].vuClassData.classInfo_[VU_TYPE] == TYPE_RUNWAY and Falcon4ClassTable[fc->Index].vuClassData.classInfo_[VU_STYPE] == STYPE_RUNWAY_NUM)
                        {
                            ShiAssert(obj->brain);

                            if (obj->brain)
                            {
                                // index = obj->GetComponentIndex(this);
                                int texIdx = obj->brain->GetRunwayTexture(f);
                                ((DrawableBSP*)drawptr->object)->SetTextureSet(texIdx);
                            }
                        }

                        // END OF ADDED SECTION 2002-02-28
                        Parent = drawptr->owner;

                        if (obj->GetFeatureValue(f) or ShowAllFeatures)
                        {
                            ((DrawableObject*)drawptr)->GetPosition(&objPos);

                            feat = BuildFeature(obj, f, &objPos);

                            if (feat)
                            {
                                item = TargetTree->CreateItem(obj->GetCampID() << 16 bitor f, C_TYPE_ITEM, feat);

                                if (item)
                                {
                                    TargetTree->AddChildItem(parent, item);
                                    feat->SetOwner(item);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void GetGroundUnitsNear(float x, float y, float range)
{
    VuListIterator myit(AllRealList);
    Unit un;
    float deltax, deltay;

    un = GetFirstUnit(&myit);

    while (un not_eq NULL)
    {
        deltax = x - un->XPos();
        deltay = y - un->YPos();

        if (deltax < 0) deltax = -deltax;

        if (deltay < 0) deltay = -deltay;

        // KCK: I made the following change here. Not sure what was intended
        // if((deltax < range bitand deltay < range) and not un->IsSquadron())
        if (deltax < range and deltay < range and not un->IsSquadron())
            AddUnitToTargetTree(un);

        un = GetNextUnit(&myit);
    }
}

void GetObjectivesNear(float x, float y, float range)
{
    VuListIterator myit(AllObjList);
    Objective Obj;
    float deltax, deltay;

    Obj = GetFirstObjective(&myit);

    while (Obj not_eq NULL)
    {
        deltax = x - Obj->XPos();
        deltay = y - Obj->YPos();

        if (deltax < 0) deltax = -deltax;

        if (deltay < 0) deltay = -deltay;

        // KCK: I made the following change here. Not sure what was intended
        // if(deltax < range bitand deltay < range)
        if (deltax < range and deltay < range)
            AddObjectiveToTargetTree(Obj);

        Obj = GetNextObjective(&myit);
    }
}

void ReconArea(float x, float y, float range)
{
    C_Window *win;
    TREELIST *parent, *item;

    SetCursor(gCursors[CRSR_WAIT]);
    win = gMainHandler->FindWindow(RECON_WIN);

    if (win)
    {
        CloseAllRenderers(RECON_WIN);

        if (TargetTree)
            TargetTree->DeleteBranch(TargetTree->GetRoot());

        if (gUIViewer)
        {
            gUIViewer->Cleanup();
            delete gUIViewer;
        }

        gUIViewer = new C_3dViewer;
        gUIViewer->Setup();
        gUIViewer->Viewport(win, 0); // use client 0 for this window

        Recon.Heading = 0.0f;
        Recon.Pitch = 70.0f;
        Recon.Distance = 4000.0f;
        Recon.Direction = 0.0f;

        Recon.MinDistance = 250.0f;
        Recon.MaxDistance = 30000.0f;
        Recon.MinPitch = 5;
        Recon.MaxPitch = 90;
        Recon.CheckPitch = TRUE;

        Recon.PosX = x;
        Recon.PosY = y;
        Recon.PosZ = -4000;

        GetObjectivesNear(x, y, range);
        GetGroundUnitsNear(x, y, range);

        SetBullsEye(win);
        SetSlantRange(win);
        SetHeading(win);

        gUIViewer->SetPosition(Recon.PosX, Recon.PosY, Recon.PosZ);
        gUIViewer->InitOTW(30.0f, FALSE);

        // OW
#if 1

        if (gUIViewer->GetRendOTW())
        {
            // gUIViewer->GetRendOTW()->SetTerrainTextureLevel( PlayerOptions.TextureLevel() );

            gUIViewer->GetRendOTW()->SetDitheringMode(PlayerOptions.HazingOn());

            if (FalconDisplay.theDisplayDevice.IsHardware())
            {
                gUIViewer->GetRendOTW()->SetFilteringMode(PlayerOptions.FilteringOn());
                // gUIViewer->GetRendOTW()->SetAlphaMode(PlayerOptions.AlphaOn());
                gUIViewer->GetRendOTW()->SetHazeMode(PlayerOptions.HazingOn());
                // gUIViewer->GetRendOTW()->SetSmoothShadingMode( PlayerOptions.GouraudOn() );
            }

            else
            {
                gUIViewer->GetRendOTW()->SetFilteringMode(false);
                // gUIViewer->GetRendOTW()->SetAlphaMode(false);
                gUIViewer->GetRendOTW()->SetHazeMode(false);
                // gUIViewer->GetRendOTW()->SetSmoothShadingMode(false);
            }

            gUIViewer->GetRendOTW()->SetObjectDetail(PlayerOptions.ObjectDetailLevel());
            gUIViewer->GetRendOTW()->SetObjectTextureState(TRUE);//PlayerOptions.ObjectTexturesOn());
        }

#endif

        gUIViewer->AddAllToView();

        win->ScanClientAreas();
        win->RefreshWindow();

        TheLoader.WaitLoader();
        PositionCamera(&Recon, win, 0);

        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
    }

    win = gMainHandler->FindWindow(RECON_LIST_WIN);

    if (win)
    {
        if (TargetTree)
            TargetTree->RecalcSize();

        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
    }

    if (TargetTree)
    {
        parent = TargetTree->GetRoot();

        if (parent)
        {
            if (parent->Type_ == C_TYPE_MENU)
            {
                item = parent->Child;

                if (item)
                    CenterOnFeatureCB(RECON_TREE, C_TYPE_LMOUSEUP, item->Item_);
            }
            else if (parent->Type_ == C_TYPE_ITEM)
                CenterOnFeatureCB(RECON_TREE, C_TYPE_LMOUSEUP, parent->Item_);
        }
    }

    SetCursor(gCursors[CRSR_F16]);
}

void BuildTargetList(float x, float y, float range)
{
    TREELIST *parent, *item;
    C_Window *win;

    win = gMainHandler->FindWindow(RECON_WIN);

    if (win)
    {
        CloseAllRenderers(RECON_WIN);

        if (TargetTree)
            TargetTree->DeleteBranch(TargetTree->GetRoot());

        if (gUIViewer)
        {
            gUIViewer->Cleanup();
            delete gUIViewer;
        }

        gUIViewer = new C_3dViewer;
        gUIViewer->Setup();
        gUIViewer->Viewport(win, 0); // use client 0 for this window

        Recon.Heading = 0.0f;
        Recon.Pitch = 70.0f;
        Recon.Distance = 1000.0f;
        Recon.Direction = 0.0f;

        Recon.MinDistance = 250.0f;
        Recon.MaxDistance = 30000.0f;
        Recon.MinPitch = 5;
        Recon.MaxPitch = 90;
        Recon.CheckPitch = TRUE;

        Recon.PosX = x;
        Recon.PosY = y;
        Recon.PosZ = -4000;

        GetObjectivesNear(x, y, range);
        GetGroundUnitsNear(x, y, range);

        SetBullsEye(win);
        SetSlantRange(win);
        SetHeading(win);

        gUIViewer->SetPosition(Recon.PosX, Recon.PosY, Recon.PosZ);
        gUIViewer->InitOTW(30.0f, FALSE);
        gUIViewer->AddAllToView();

        win->ScanClientAreas();
        win->RefreshWindow();

        PositionCamera(&Recon, win, 0);
    }

    win = gMainHandler->FindWindow(RECON_LIST_WIN);

    if (win)
    {
        if (TargetTree)
            TargetTree->RecalcSize();

        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
    }

    if (TargetTree)
    {
        parent = TargetTree->GetRoot();

        if (parent)
        {
            if (parent->Type_ == C_TYPE_MENU)
            {
                item = parent->Child;

                if (item)
                    CenterOnFeatureCB(RECON_TREE, C_TYPE_LMOUSEUP, item->Item_);
            }
            else if (parent->Type_ == C_TYPE_ITEM)
                CenterOnFeatureCB(RECON_TREE, C_TYPE_LMOUSEUP, parent->Item_);
        }
    }
}

void BuildSpecificTargetList(VU_ID targetID)
{
    C_Window *win;
    CampEntity ent;
    TREELIST *parent, *item;

    ent = (CampEntity)vuDatabase->Find(targetID);

    if ( not ent)
        return;

    win = gMainHandler->FindWindow(RECON_WIN);

    if (win)
    {
        CloseAllRenderers(RECON_WIN);

        if (TargetTree)
            TargetTree->DeleteBranch(TargetTree->GetRoot());

        if (gUIViewer)
        {
            gUIViewer->Cleanup();
            delete gUIViewer;
        }

        gUIViewer = new C_3dViewer;
        gUIViewer->Setup();
        gUIViewer->Viewport(win, 0); // use client 0 for this window

        Recon.Heading = 0.0f;
        Recon.Pitch = 70.0f;
        Recon.Distance = 1000.0f;
        Recon.Direction = 0.0f;

        Recon.MinDistance = 150.0f;
        Recon.MaxDistance = 30000.0f;
        Recon.MinPitch = 5;
        Recon.MaxPitch = 90;
        Recon.CheckPitch = TRUE;

        if (ent->IsObjective())
            AddObjectiveToTargetTree((Objective)ent);

        if (ent->IsUnit())
            AddUnitToTargetTree((Unit)ent);

        Recon.PosX = ent->XPos();
        Recon.PosY = ent->YPos();
        Recon.PosZ = -4000;

        SetBullsEye(win);
        SetSlantRange(win);
        SetHeading(win);

        gUIViewer->SetPosition(Recon.PosX, Recon.PosY, Recon.PosZ);
        gUIViewer->InitOTW(30.0f, FALSE);
        gUIViewer->AddAllToView();

        win->ScanClientAreas();
        win->RefreshWindow();

        PositionCamera(&Recon, win, 0);
    }

    win = gMainHandler->FindWindow(RECON_LIST_WIN);

    if (win)
    {
        if (TargetTree)
            TargetTree->RecalcSize();

        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
    }

    if (TargetTree)
    {
        parent = TargetTree->GetRoot();

        if (parent)
        {
            if (parent->Type_ == C_TYPE_MENU)
            {
                item = parent->Child;

                if (item)
                    CenterOnFeatureCB(RECON_TREE, C_TYPE_LMOUSEUP, item->Item_);
            }
            else if (parent->Type_ == C_TYPE_ITEM)
                CenterOnFeatureCB(RECON_TREE, C_TYPE_LMOUSEUP, parent->Item_);
        }
    }
}
