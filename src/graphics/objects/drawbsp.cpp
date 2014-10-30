/***************************************************************************\
    DrawBSP.cpp
    Scott Randolph
    May 3, 1996

    Derived class to handle interaction with the BSP object library.
\***************************************************************************/
#include "StateStack.h"
#include "PalBank.h"
#include "Matrix.h"
#include "TimeMgr.h"
#include "TOD.h"
#include "RenderOW.h"
#include "DrawBSP.h"
#include "DrawPNT.h"
#include "FalcLib/include/PlayerOp.h"
#include "FalcLib/include/dispopts.h"

#include "Graphics/DXEngine/DXTools.h"
#include "Graphics/DXEngine/DXEngine.h"

//JAM 09Dec03
#include "RealWeather.h"

#ifdef USE_SH_POOLS
MEM_POOL DrawableBSP::pool;
#endif

BOOL DrawableBSP::drawLabels = FALSE; // Shared by ALL drawable BSPs
extern int g_nNearLabelLimit; // JB 000807
extern bool g_bSmartScaling; // JB 010112
extern bool g_bDrawBoundingBox;
extern bool g_bLabelRadialFix;
extern bool g_bLabelShowDistance;
extern BOOL renderACMI;


/***************************************************************************\
    Initialize a container for a BSP object to be drawn
\***************************************************************************/
DrawableBSP::DrawableBSP(int ID, const Tpoint *pos, const Trotation *rot, float s)
    : DrawableObject(s), instance(ID)
{
    // Initialize our member variables
    id = ID;
    label[0] = 0;
    labelLen = 0;
    drawClassID = BSP;
    inhibitDraw = FALSE;

    // Record our position
    position = *pos;

    // Ask the object library what size this object is
    radius = instance.Radius();

    // Store our rotation matrix
    orientation = *rot;
}



/***************************************************************************\
    Remove an instance of a BSP object.
\***************************************************************************/
DrawableBSP::~DrawableBSP(void)
{
    ShiAssert(id >= 0);

    // HACK
    // This check should go as soon as Drawable2D stops inheriting from
    // this class.
    // if (id < 0)  return;

    // Mark this id as having been released
    id = -id;
}


/***************************************************************************\
    Update the position and orientation of this object.
\***************************************************************************/
void DrawableBSP::Update(const Tpoint *pos, const Trotation *rot)
{
    ShiAssert(id >= 0);

    ShiAssert( not _isnan(position.x));
    // Update the location of this object
    position.x = pos->x;
    position.y = pos->y;

    if (GetClass() not_eq GroundVehicle)
        position.z = pos->z;

    orientation = *rot;
}


/***************************************************************************\
    Attach an object as a child (no further updates need to be done on it)
\***************************************************************************/
void DrawableBSP::AttachChild(DrawableBSP *child, int slotNumber)
{
    ShiAssert(id >= 0);
    ShiAssert(child);
    ShiAssert(slotNumber >= 0);
    ShiAssert(slotNumber < instance.ParentObject->nSlots);
    ShiAssert((instance.SlotChildren) and (instance.SlotChildren[slotNumber] == NULL));

    // THIS IS A HACK TO TOLERATE OBJECTS WHICH DON'T YET HAVE SLOTS
    // THIS SHOULD BE REMOVED IN THE LATE BETA AND SHIPPING VERSIONS
    if ( not instance.SlotChildren) return;

    if (slotNumber >= instance.ParentObject->nSlots) return;

    if ( not child) return;

    instance.SetSlotChild(slotNumber, &child->instance);
}


void MonPrint(char *, ...);

/***************************************************************************\
    Force the given child slot to be empty (the child becomes a peer)
\***************************************************************************/
void DrawableBSP::DetachChild(DrawableBSP *child, int slotNumber)
{
    ShiAssert(id >= 0);
    ShiAssert(child);
    ShiAssert(slotNumber >= 0);
    ShiAssert(slotNumber < instance.ParentObject->nSlots);
    ShiAssert((instance.SlotChildren) and (instance.SlotChildren[slotNumber] == &child->instance));

    Tpoint offset;
    Tpoint pos;

    // THIS IS A HACK TO TOLERATE OBJECTS WHICH DON'T YET HAVE SLOTS
    // THIS SHOULD BE REMOVED IN THE LATE BETA AND SHIPPING VERSIONS
    if (slotNumber >= instance.ParentObject->nSlots)
    {
        return;
    }

    if (( not instance.SlotChildren) or (instance.SlotChildren[slotNumber] not_eq &child->instance))
    {
        //(*(int*)0) = 0;
        return;
    }

    // Get the childs offset from the parent in object space
    GetChildOffset(slotNumber, &offset);

    // Rotate the offset into world space and add the parents position
    pos.x = orientation.M11 * offset.x + orientation.M12 * offset.y + orientation.M13 * offset.z + position.x;
    pos.y = orientation.M21 * offset.x + orientation.M22 * offset.y + orientation.M23 * offset.z + position.y;
    pos.z = orientation.M31 * offset.x + orientation.M32 * offset.y + orientation.M33 * offset.z + position.z;

    // Update the child's location
    child->position = pos;

    // Give the child the same orientation as the parent
    // (We used to set scale as well, but that cause a few problems, so we'll leave
    //  that up to the application...)
    child->orientation = orientation;

    // Stop drawing the child object as an attachment
    instance.SetSlotChild(slotNumber, NULL);
}



/***************************************************************************
    Return the object space location of the slot connect point indicated
***************************************************************************/
void DrawableBSP::GetChildOffset(int slotNumber, Tpoint *offset)
{
    ShiAssert(id >= 0);
    ShiAssert(slotNumber >= 0);
    ShiAssert(slotNumber < instance.ParentObject->nSlots);

    // THIS IS A HACK TO TOLERATE OBJECTS WHICH DON'T YET HAVE SLOTS
    // THIS SHOULD BE REMOVED IN THE LATE BETA AND SHIPPING VERSIONS
    if (slotNumber >= instance.ParentObject->nSlots)  return;

    *offset = instance.ParentObject->pSlotAndDynamicPositions[slotNumber];
    offset->x *= scale;
    offset->y *= scale;
    offset->z *= scale;
}


/***************************************************************************
    Set the angle control values for a degree of freedom in the model
***************************************************************************/
void DrawableBSP::SetDOFangle(int DOF, float radians)
{
    ShiAssert(id >= 0);

    // THIS IS A HACK TO TOLERATE OBJECTS WHICH DON'T YET HAVE DOFS
    // THIS SHOULD BE REMOVED IN THE LATE BETA AND SHIPPING VERSIONS
    if (DOF >= instance.ParentObject->nDOFs)  return;

    ShiAssert(DOF < instance.ParentObject->nDOFs);
    instance.DOFValues[DOF].rotation = radians;
}
float DrawableBSP::GetDOFangle(int DOF)
{
    ShiAssert(id >= 0);

    // THIS IS A HACK TO TOLERATE OBJECTS WHICH DON'T YET HAVE DOFS
    // THIS SHOULD BE REMOVED IN THE LATE BETA AND SHIPPING VERSIONS
    if (DOF >= instance.ParentObject->nDOFs)  return 0.0f;

    ShiAssert(DOF < instance.ParentObject->nDOFs);
    return instance.DOFValues[DOF].rotation;
}


/***************************************************************************\
    Set the offset control values for a degree of freedom in the model
\***************************************************************************/
void DrawableBSP::SetDOFoffset(int DOF, float offset)
{
    ShiAssert(id >= 0);

    // THIS IS A HACK TO TOLERATE OBJECTS WHICH DON'T YET HAVE DOFS
    // THIS SHOULD BE REMOVED IN THE LATE BETA AND SHIPPING VERSIONS
    if (DOF >= instance.ParentObject->nDOFs)  return;

    ShiAssert(DOF < instance.ParentObject->nDOFs);

    if (DOF < instance.ParentObject->nDOFs)
        instance.DOFValues[DOF].translation = offset;
}
float DrawableBSP::GetDOFoffset(int DOF)
{
    ShiAssert(id >= 0);

    // THIS IS A HACK TO TOLERATE OBJECTS WHICH DON'T YET HAVE DOFS
    // THIS SHOULD BE REMOVED IN THE LATE BETA AND SHIPPING VERSIONS
    if (DOF >= instance.ParentObject->nDOFs)  return 0.0f;

    ShiAssert(DOF < instance.ParentObject->nDOFs);
    return instance.DOFValues[DOF].translation;
}


/***************************************************************************\
    Set the offset from home postion for the named dynamic vertex.
\***************************************************************************/
void DrawableBSP::SetDynamicVertex(int vertID, float dx, float dy, float dz)
{
    // THIS IS A HACK TO TOLERATE OBJECTS WHICH DON'T YET HAVE DYANAMIC VERTS
    // THIS SHOULD BE REMOVED IN THE LATE BETA AND SHIPPING VERSIONS
    if (vertID >= instance.ParentObject->nDynamicCoords)  return;

    ShiAssert(vertID < instance.ParentObject->nDynamicCoords);
    instance.SetDynamicVertex(vertID, dx, dy, dz);
}
void DrawableBSP::GetDynamicVertex(int vertID, float *dx, float *dy, float *dz)
{
    // THIS IS A HACK TO TOLERATE OBJECTS WHICH DON'T YET HAVE DYANAMIC VERTS
    // THIS SHOULD BE REMOVED IN THE LATE BETA AND SHIPPING VERSIONS
    if (vertID >= instance.ParentObject->nDynamicCoords)
    {
        *dx = *dy = *dz = 0.0f;
        return;
    }

    ShiAssert(vertID < instance.ParentObject->nDynamicCoords);
    instance.GetDynamicVertex(vertID, dx, dy, dz);
}

// RV - Biker
void DrawableBSP::GetDynamicCoords(int vertID, float *dx, float *dy, float *dz)
{
    if (vertID >= instance.ParentObject->nDynamicCoords)
    {
        *dx = *dy = *dz = 0.0f;
        return;
    }

    ShiAssert(vertID < instance.ParentObject->nDynamicCoords);
    instance.GetDynamicCoords(vertID, dx, dy, dz);
}


/**************************************************************************
    Set one of the switch control masks in the model
***************************************************************************/
void DrawableBSP::SetSwitchMask(int switchNumber, UInt32 mask)
{
    ShiAssert(id >= 0);

    // THIS IS A HACK TO TOLERATE OBJECTS WHICH DON'T YET HAVE DOFS
    // THIS SHOULD BE REMOVED IN THE LATE BETA AND SHIPPING VERSIONS
    if (switchNumber >= instance.ParentObject->nSwitches)  return;

    ShiAssert(switchNumber < instance.ParentObject->nSwitches);

    if (switchNumber < instance.ParentObject->nSwitches)
        instance.SwitchValues[switchNumber] = mask;
}
UInt32 DrawableBSP::GetSwitchMask(int switchNumber)
{
    ShiAssert(id >= 0);

    // THIS IS A HACK TO TOLERATE OBJECTS WHICH DON'T YET HAVE DOFS
    // THIS SHOULD BE REMOVED IN THE LATE BETA AND SHIPPING VERSIONS
    if (switchNumber >= instance.ParentObject->nSwitches)  return 0;

    ShiAssert(switchNumber < instance.ParentObject->nSwitches);

    if (switchNumber < instance.ParentObject->nSwitches)
        return instance.SwitchValues[switchNumber];
    else return 0;
}


/**************************************************************************
    Store the labeling information for this object instance.
***************************************************************************/
void DrawableBSP::SetLabel(char *labelString, DWORD color)
{
    ShiAssert(strlen(labelString) < sizeof(label));

    strncpy(label, labelString, 31);
    label[31] = 0;
    labelColor = color;
    labelLen = VirtualDisplay::ScreenTextWidth(labelString) >> 1;
}


/***************************************************************************\
** Determine if a line segment (origin + vector) intersects bounding box
** for the BSP.
**  Return TRUE if so and the collision point.
** Algo from GGEms I
\***************************************************************************/
BOOL DrawableBSP::GetRayHit(const Tpoint *from, const Tpoint *vector, Tpoint *collide, float boxScale)
{
    Tpoint origin = {0.0F}, vec = {0.0F};
    Tpoint pos = {0.0F};
    int i = 0;
    float  *minBp = NULL, *maxBp = NULL, *orgp = NULL, *vecp = NULL, *collp = NULL;
    enum {LEFT, RIGHT, MIDDLE} quadrant[3] = {LEFT};
    float t = 0.0F, tMax = 0.0F;
    float minB[3] = {0.0F}, maxB[3] = {0.0F};
    float candidatePlane[3] = {0.0F};
    int whichPlane = 0;
    BOOL inside = TRUE;

    // First we transform the origin and vector into object space (since that's easier than rotating the box)
    pos.x = from->x - position.x;
    pos.y = from->y - position.y;
    pos.z = from->z - position.z;
    origin.x =  pos.x * orientation.M11 +     pos.y * orientation.M21 +     pos.z * orientation.M31;
    origin.y =  pos.x * orientation.M12 +     pos.y * orientation.M22 +     pos.z * orientation.M32;
    origin.z =  pos.x * orientation.M13 +     pos.y * orientation.M23 +     pos.z * orientation.M33;
    vec.x = vector->x * orientation.M11 + vector->y * orientation.M21 + vector->z * orientation.M31;
    vec.y = vector->x * orientation.M12 + vector->y * orientation.M22 + vector->z * orientation.M32;
    vec.z = vector->x * orientation.M13 + vector->y * orientation.M23 + vector->z * orientation.M33;

    // Account for object scaling
    boxScale *= scale;

    if (boxScale == 1.0f)
    {
        minB[0] = instance.BoxBack();
        minB[1] = instance.BoxLeft();
        minB[2] = instance.BoxTop();
        maxB[0] = instance.BoxFront();
        maxB[1] = instance.BoxRight();
        maxB[2] = instance.BoxBottom();
    }
    else
    {
        minB[0] = boxScale * instance.BoxBack();
        minB[1] = boxScale * instance.BoxLeft();
        minB[2] = boxScale * instance.BoxTop();
        maxB[0] = boxScale * instance.BoxFront();
        maxB[1] = boxScale * instance.BoxRight();
        maxB[2] = boxScale * instance.BoxBottom();
    }

    // find candiate planes
    orgp = (float *)&origin;
    minBp = (float *)&minB;
    maxBp = (float *)&maxB;

    for (i = 0; i < 3; i++, orgp++, minBp++, maxBp++)
    {
        if (*orgp < *minBp)
        {
            quadrant[i] = LEFT;
            candidatePlane[i] = *minBp;
            inside = FALSE;
        }
        else if (*orgp > *maxBp)
        {
            quadrant[i] = RIGHT;
            candidatePlane[i] = *maxBp;
            inside = FALSE;
        }
        else
        {
            quadrant[i] = MIDDLE;
        }
    }

    // origin is in box
    if (inside)
    {
        *collide = *from;
        return TRUE;
    }

    // calculate T distances to candidate planes and accumulate the largest
    if (quadrant[0] not_eq MIDDLE and vec.x not_eq 0.0f)
    {
        tMax = (candidatePlane[0] - origin.x) / vec.x;
        whichPlane = 0;
    }
    else
    {
        tMax = -1.0f;
    }

    if (quadrant[1] not_eq MIDDLE and vec.y not_eq 0.0f)
    {
        t = (candidatePlane[1] - origin.y) / vec.y;

        if (t > tMax)
        {
            tMax = t;
            whichPlane = 1;
        }
    }

    if (quadrant[2] not_eq MIDDLE and vec.z not_eq 0.0f)
    {
        t = (candidatePlane[2] - origin.z) / vec.z;

        if (t > tMax)
        {
            tMax = t;
            whichPlane = 2;
        }
    }

    // Check final candidate is within the segment of interest
    if (tMax < 0.0f or tMax > 1.0f)
    {
        return FALSE;
    }

    // Check final candidate is within the bounds of the side of the box
    orgp = (float *)&origin;
    vecp = (float *)&vec;
    collp = (float *)&pos;

    for (i = 0; i < 3; i++, vecp++, orgp++, collp++)
    {
        if (whichPlane not_eq i)
        {
            *collp = *orgp + tMax * (*vecp);

            if (*collp < minB[i] or  *collp > maxB[i])
            {
                // outside box
                return FALSE;
            }
        }
        else
        {
            *collp = candidatePlane[i];
        }
    }

    // We must transform the collision point from object space back into world space
    collide->x = pos.x * orientation.M11 + pos.y * orientation.M12 + pos.z * orientation.M13 + position.x;
    collide->y = pos.x * orientation.M21 + pos.y * orientation.M22 + pos.z * orientation.M23 + position.y;
    collide->z = pos.x * orientation.M31 + pos.y * orientation.M32 + pos.z * orientation.M33 + position.z;


    return TRUE;
};





// This function setup  visibility stuff for a BSP
// it returns false is the BSP results not visible for any reason
bool DrawableBSP::SetupVisibility(RenderOTW *renderer)
{
    float alpha, fog, z;

    // RED - Linear Fog - checvk if under visibility limit
    if (position.z > realWeather->VisibleLimit()) return false;


    //////////////////////////////////// FOG / HAZE ///////////////////////////////////////////////////

    z = renderer->ZDistanceFromCamera(&position);

    // RED - Linear Fog, if inside the layer, modulate with Hze, we can not use linear fog there
    if (realWeather->weatherCondition > FAIR and position.z > (realWeather->HiOvercast))
    {
        alpha = 1.0f - (-realWeather->HiOvercast + position.z) / (realWeather->stratusDepth / 2.0f);
        alpha *= alpha * alpha;
    }
    else
    {
        if (z > renderer->haze_start + renderer->haze_depth) alpha = 0.f;
        else if (z < renderer->PERSPECTIVE_RANGE) alpha = 1.f;
        else
        {
            if (renderer->GetHazeMode())
            {
                fog = min(renderer->GetValleyFog(z, position.z), .65f);

                if (z < renderer->haze_start) alpha = 1.f - fog;
                else
                {
                    alpha = renderer->GetRangeOnlyFog(z);

                    if (alpha < fog) alpha = fog;

                    alpha = 1.f - alpha;
                }
            }
            else
            {
                if (z < renderer->haze_start) alpha = 1.f;
                else
                {
                    alpha = renderer->GetRangeOnlyFog(z);
                    alpha = 1.f - alpha;
                }
            }
        }
    }

    // Set the Fog stuff...
    TheStateStack.SetFog(alpha, (Pcolor*)renderer->GetFogColor());


    // OBJECT TO DRAW
    return true;

}

/***************************************************************************\
 This is the call used by the out the window terrain rendering system.
\***************************************************************************/
//void DrawableBSP::Draw( RenderOTW *renderer, int LOD )
void DrawableBSP::Draw(RenderOTW *renderer, int)
{
    ThreeDVertex labelPoint;
    float x, y;

    ShiAssert(id >= 0);

    // check for inhibit
    if (inhibitDraw)
    {
        SetInhibitFlag(FALSE);
        return;
    }

    if ( not SetupVisibility(renderer)) return;

    // JB 010112
    float scalefactor = 1;

    if (g_bSmartScaling or PlayerOptions.ObjectDynScalingOn())
    {
        renderer->TransformPoint(&position, &labelPoint);

        if (radius <= 150 and (GetClass() == Guys or GetClass() == GroundVehicle or GetClass() == BSP))
            scalefactor = (labelPoint.csZ - 1200) / 6076 + 1;

        if (scalefactor < 1)
            scalefactor = 1;
        else if (scalefactor > 2)
            scalefactor = 2;
    }

    // JB 010112

    // Draw the object
    //JAM 09Dec03

    BOOL isShadow = FALSE;

    if (PlayerOptions.ShadowsOn() and realWeather->weatherCondition == FAIR)
    {
        Tpoint pv;
        Tcolor light;

        TheTimeOfDay.GetTextureLightingColor(&light);
        renderer->TransformPointToViewSwapped(&position, &pv);

        for (int row = 2; row < realWeather->numCells - 2; row++)
        {
            for (int col = 2; col < realWeather->numCells - 2; col++)
            {
                if (realWeather->weatherCellArray[row][col].onScreen)
                {
                    float dx = pv.x - realWeather->weatherCellArray[row][col].shadowPos.x;
                    float dy = pv.y - realWeather->weatherCellArray[row][col].shadowPos.y;
                    float dz = pv.z - realWeather->weatherCellArray[row][col].shadowPos.z;
                    float range = FabsF(SqrtF(dx * dx + dy * dy + dz * dz));

                    if (range < realWeather->cloudRadius)
                    {
                        isShadow = TRUE;

                        float interp = max(1.f - (realWeather->cloudRadius - range) / realWeather->cloudRadius, .5f);

                        float r = interp * light.r;
                        float g = interp * light.g;
                        float b = interp * light.b;

                        TheColorBank.SetLight(r, g, b);
                    }
                }
            }
        }
    }

    if (g_bSmartScaling or PlayerOptions.ObjectDynScalingOn())
        TheStateStack.DrawObject(&instance, &orientation, &position, scale * scalefactor);  // JB 010112 added scalefactor
    else
        TheStateStack.DrawObject(&instance, &orientation, &position, scale);

    if (isShadow)
    {
        Tcolor light;

        TheTimeOfDay.GetTextureLightingColor(&light);
        TheColorBank.SetLight(light.r, light.g, light.b);
    }


#ifdef DEBUG_LOD_ID

    // Now compute the starting location for our label text
    if (drawLabels and TheDXEngine.GetLodUsedLabel()[0])
    {
        renderer->TransformPoint(&position, &labelPoint);

        if (labelPoint.clipFlag == ON_SCREEN)
        {
            x = labelPoint.x - labelLen; // Centers text
            y = labelPoint.y - 12; // Place text above center of object
            renderer->SetColor(labelColor);
            renderer->SetFont(2);
            renderer->ScreenText(x, y, TheDXEngine.GetLodUsedLabel());
        }
    }

#else

    // Now compute the starting location for our label text
    if (drawLabels and labelLen)
    {
        if ( not g_bSmartScaling and not PlayerOptions.ObjectDynScalingOn())
            renderer->TransformPoint(&position, &labelPoint);   // JB 010112

        // JB 000807 Add near label limit and labels that get brighter as they get closer
        // if ( labelPoint.clipFlag == ON_SCREEN )//-
        // {//-
        // x = labelPoint.x - renderer->ScreenTextWidth(label) / 2; // Centers text//-
        // y = labelPoint.y - 12; // Place text above center of object//-
        // renderer->SetColor( labelColor );//-
        // renderer->ScreenText( x, y, label );//-
        // } //-

        // RV - RED - If ACMI force Label Limit to 150 nMiles
        long limit = (renderACMI ? 150 : g_nNearLabelLimit) * 6076 + 8, limitcheck;

        if ( not DrawablePoint::drawLabels)
            limitcheck = (renderACMI ? 150 : g_nNearLabelLimit) * 6076 + 8;
        else limitcheck = 300 * 6076 + 8; //

        //dpc LabelRadialDistanceFix
        //First check if Z distance is below "limitcheck" and only if it is then do additional
        //radial distance check (messes up .csZ value - but it shouldn't matter
        // since labelPoint is local and .csZ is not used afterwards)
        // Besides no need to calculate radial distance is Z distance is already greater
        if (g_bLabelRadialFix)
            if (labelPoint.clipFlag == ON_SCREEN and 
                labelPoint.csZ < limitcheck) //Same condition as below
            {
                float dx = position.x - renderer->X();
                float dy = position.y - renderer->Y();
                float dz = position.z - renderer->Z();
                labelPoint.csZ = (float)sqrt(dx * dx + dy * dy + dz * dz);
            }

        //end LabelRadialDistanceFix

        if (labelPoint.clipFlag == ON_SCREEN and 
            labelPoint.csZ < limitcheck)
        {
            int colorsub = int((labelPoint.csZ / (limit >> 3))) << 5;

            if (colorsub > 180) // let's not reduce brightness too much, keep a glimpse of the original color
                colorsub = 180;

            int red = (labelColor bitand 0x000000ff);
            red -= min(red, colorsub);
            int green = (labelColor bitand 0x0000ff00) >> 8;
            green -= min(green, colorsub + 30); // green would be too light -> +30
            int blue = (labelColor bitand 0x00ff0000) >> 16;
            blue -= min(blue, colorsub);

            long newlabelColor = blue << 16 bitor green << 8 bitor red;

            x = labelPoint.x - renderer->ScreenTextWidth(label) / 2; // Centers text
            y = labelPoint.y - 12; // Place text above center of object
            renderer->SetColor(newlabelColor);
            renderer->ScreenText(x, y, label);

            //dpc LabelRadialDistanceFix
            if (g_bLabelShowDistance)
            {
                char label2[32];
                sprintf(label2, "%4.1f nm", labelPoint.csZ / 6076); // convert from ft to nm
                float x2 = labelPoint.x - renderer->ScreenTextWidth(label2) / 2; // Centers text
                float y2 = labelPoint.y + 4; // Distance below center object
                renderer->ScreenText(x2, y2, label2);
            }

            //end LabelRadialDistanceFix
        }

        // JB 000807
    }

#endif

    if (g_bDrawBoundingBox) DrawBoundingBox(renderer);

#ifdef _DEBUG
    // TESTING CODE TO SHOW BOUNDING BOXES
    // DrawBoundingBox( renderer );
#endif
}



/***************************************************************************\
 This call is used for micellanious BSP object display.
\***************************************************************************/
void DrawableBSP::Draw(Render3D *renderer)
{
    ThreeDVertex labelPoint;
    float x, y;

    ShiAssert(id >= 0);

    if ( not renderer)
        return;

    // RED - NOPE - must be similar to any object
    // Make sure no left over fog affects this object...
    //TheStateStack.SetFog(1.f,NULL); //JAM 26Dec03

    TheStateStack.DrawObject(&instance, &orientation, &position, scale);

#ifdef DEBUG_LOD_ID

    // Now compute the starting location for our label text
    if (drawLabels and TheDXEngine.GetLodUsedLabel()[0])
    {
        renderer->TransformPoint(&position, &labelPoint);

        if (labelPoint.clipFlag == ON_SCREEN)
        {
            x = labelPoint.x - labelLen; // Centers text
            y = labelPoint.y - 12; // Place text above center of object
            renderer->SetFont(2);
            renderer->SetColor(labelColor);
            renderer->ScreenText(x, y, TheDXEngine.GetLodUsedLabel());
        }
    }

#else

    // Now compute the starting location for our label text
    if (drawLabels and labelLen)
    {
        renderer->TransformPoint(&position, &labelPoint);

        if (labelPoint.clipFlag == ON_SCREEN)
        {
            x = labelPoint.x - labelLen; // Centers text
            y = labelPoint.y - 12; // Place text above center of object
            renderer->SetColor(labelColor);
            renderer->ScreenText(x, y, label);
        }
    }

#endif
}



/***************************************************************************\
    Display the bounding box of the object.
\***************************************************************************/
void DrawableBSP::DrawBoundingBox(Render3D *renderer)
{
    Tpoint max, min;
    Tpoint p, p1, p2;
    Trotation M;

    ShiAssert(id >= 0);

    // TEMPORARY:  We're putting the data into the min/max structure in Erick's old
    // x Right, y Down, z Front ordering to avoid changes in the code below....
    max.x = instance.BoxRight();
    max.y = instance.BoxBottom();
    max.z = instance.BoxFront();
    min.x = instance.BoxLeft();
    min.y = instance.BoxTop();
    min.z = instance.BoxBack();

    // MatrixTranspose( &orientation, &M );
    M = orientation;

    renderer->SetColor(0xFF0000FF);

    // We need to transform the box into world space -- right now it is in object space
    p.x = max.z;
    p.y = max.x;
    p.z = max.y;
    MatrixMult(&M, &p, &p1);
    p1.x += position.x;
    p1.y += position.y;
    p1.z += position.z;

    p.x = max.z;
    p.y = max.x;
    p.z = min.y;
    MatrixMult(&M, &p, &p2);
    p2.x += position.x;
    p2.y += position.y;
    p2.z += position.z;

    renderer->Render3DLine(&p1, &p2);

    p.x = min.z;
    p.y = max.x;
    p.z = max.y;
    MatrixMult(&M, &p, &p1);
    p1.x += position.x;
    p1.y += position.y;
    p1.z += position.z;

    p.x = min.z;
    p.y = max.x;
    p.z = min.y;
    MatrixMult(&M, &p, &p2);
    p2.x += position.x;
    p2.y += position.y;
    p2.z += position.z;

    renderer->Render3DLine(&p1, &p2);

    p.x = max.z;
    p.y = min.x;
    p.z = max.y;
    MatrixMult(&M, &p, &p1);
    p1.x += position.x;
    p1.y += position.y;
    p1.z += position.z;

    p.x = max.z;
    p.y = min.x;
    p.z = min.y;
    MatrixMult(&M, &p, &p2);
    p2.x += position.x;
    p2.y += position.y;
    p2.z += position.z;

    renderer->Render3DLine(&p1, &p2);

    p.x = min.z;
    p.y = min.x;
    p.z = max.y;
    MatrixMult(&M, &p, &p1);
    p1.x += position.x;
    p1.y += position.y;
    p1.z += position.z;

    p.x = min.z;
    p.y = min.x;
    p.z = min.y;
    MatrixMult(&M, &p, &p2);
    p2.x += position.x;
    p2.y += position.y;
    p2.z += position.z;

    renderer->Render3DLine(&p1, &p2);

    p.x = max.z;
    p.y = max.x;
    p.z = max.y;
    MatrixMult(&M, &p, &p1);
    p1.x += position.x;
    p1.y += position.y;
    p1.z += position.z;

    p.x = max.z;
    p.y = min.x;
    p.z = max.y;
    MatrixMult(&M, &p, &p2);
    p2.x += position.x;
    p2.y += position.y;
    p2.z += position.z;

    renderer->Render3DLine(&p1, &p2);

    p.x = min.z;
    p.y = min.x;
    p.z = max.y;
    MatrixMult(&M, &p, &p1);
    p1.x += position.x;
    p1.y += position.y;
    p1.z += position.z;

    p.x = max.z;
    p.y = min.x;
    p.z = max.y;
    MatrixMult(&M, &p, &p2);
    p2.x += position.x;
    p2.y += position.y;
    p2.z += position.z;

    renderer->Render3DLine(&p1, &p2);

    p.x = min.z;
    p.y = min.x;
    p.z = max.y;
    MatrixMult(&M, &p, &p1);
    p1.x += position.x;
    p1.y += position.y;
    p1.z += position.z;

    p.x = min.z;
    p.y = max.x;
    p.z = max.y;
    MatrixMult(&M, &p, &p2);
    p2.x += position.x;
    p2.y += position.y;
    p2.z += position.z;

    renderer->Render3DLine(&p1, &p2);

    p.x = min.z;
    p.y = max.x;
    p.z = max.y;
    MatrixMult(&M, &p, &p1);
    p1.x += position.x;
    p1.y += position.y;
    p1.z += position.z;

    p.x = max.z;
    p.y = max.x;
    p.z = max.y;
    MatrixMult(&M, &p, &p2);
    p2.x += position.x;
    p2.y += position.y;
    p2.z += position.z;

    renderer->Render3DLine(&p1, &p2);

    p.x = max.z;
    p.y = max.x;
    p.z = min.y;
    MatrixMult(&M, &p, &p1);
    p1.x += position.x;
    p1.y += position.y;
    p1.z += position.z;

    p.x = max.z;
    p.y = min.x;
    p.z = min.y;
    MatrixMult(&M, &p, &p2);
    p2.x += position.x;
    p2.y += position.y;
    p2.z += position.z;

    renderer->Render3DLine(&p1, &p2);

    p.x = min.z;
    p.y = min.x;
    p.z = min.y;
    MatrixMult(&M, &p, &p1);
    p1.x += position.x;
    p1.y += position.y;
    p1.z += position.z;

    p.x = max.z;
    p.y = min.x;
    p.z = min.y;
    MatrixMult(&M, &p, &p2);
    p2.x += position.x;
    p2.y += position.y;
    p2.z += position.z;

    renderer->Render3DLine(&p1, &p2);

    p.x = min.z;
    p.y = min.x;
    p.z = min.y;
    MatrixMult(&M, &p, &p1);
    p1.x += position.x;
    p1.y += position.y;
    p1.z += position.z;

    p.x = min.z;
    p.y = max.x;
    p.z = min.y;
    MatrixMult(&M, &p, &p2);
    p2.x += position.x;
    p2.y += position.y;
    p2.z += position.z;

    renderer->Render3DLine(&p1, &p2);

    p.x = min.z;
    p.y = max.x;
    p.z = min.y;
    MatrixMult(&M, &p, &p1);
    p1.x += position.x;
    p1.y += position.y;
    p1.z += position.z;

    p.x = max.z;
    p.y = max.x;
    p.z = min.y;
    MatrixMult(&M, &p, &p2);
    p2.x += position.x;
    p2.y += position.y;
    p2.z += position.z;

    renderer->Render3DLine(&p1, &p2);
}



/***************************************************************************\
    This function is called from the miscellanious texture loader function.
 It must be hardwired into that function.
\***************************************************************************/
//void DrawableBSP::SetupTexturesOnDevice( DWORD rc )
void DrawableBSP::SetupTexturesOnDevice(DXContext *rc)
{
    // Initialize the lighting conditions and register for future time of day updates
    TimeUpdateCallback(NULL);
    TheTimeManager.RegisterTimeUpdateCB(TimeUpdateCallback, NULL);
}



/***************************************************************************\
    This function is called from the miscellanious texture cleanup function.
 It must be hardwired into that function.
\***************************************************************************/
//void DrawableBSP::ReleaseTexturesOnDevice( DWORD rc )
void DrawableBSP::ReleaseTexturesOnDevice(DXContext *rc)
{
    // Stop receiving time updates
    TheTimeManager.ReleaseTimeUpdateCB(TimeUpdateCallback, NULL);
}



/***************************************************************************\
    Update the light on the affected object texture palettes.
 NOTE:  Since the textures are static, this function can also
    be static, so the self parameter is ignored.
\***************************************************************************/
//void DrawableBSP::TimeUpdateCallback( void *self )
void DrawableBSP::TimeUpdateCallback(void *)
{
    Tcolor light;

    // Get the light level from the time of day manager
    TheTimeOfDay.GetTextureLightingColor(&light);

    // Update the staticly lit object colors
    TheColorBank.SetLight(light.r, light.g, light.b);

    /*JAM 05Jan04
     // Update all the textures which aren't dynamicly lit
     ThePaletteBank.LightReflectionPalette( 2, &light );
     ThePaletteBank.LightBuildingPalette( 3, &light );*/
}

/***************************************************************************\
** Return the min/max bounding box dimensions
\***************************************************************************/
void DrawableBSP::GetBoundingBox(Tpoint *minB, Tpoint *maxB)
{
    minB->x = instance.BoxBack();
    minB->y = instance.BoxLeft();
    minB->z = instance.BoxTop();
    maxB->x = instance.BoxFront();
    maxB->y = instance.BoxRight();
    maxB->z = instance.BoxBottom();
}
