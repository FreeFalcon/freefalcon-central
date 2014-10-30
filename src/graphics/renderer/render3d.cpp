/***************************************************************************\
    Render3D.cpp
    Scott Randolph
    December 29, 1995

    //JAM 08Jan04 - Begin Major Rewrite
\***************************************************************************/
#include <cISO646>
#include <math.h>
#include "falclib/include/debuggr.h"
#include "StateStack.h"
#include "ObjectInstance.h"
#include "Matrix.h"
#include "Render3D.h"

extern float g_fDefaultFOV; //Wombat778 10-31-2003
#include "Graphics/DXEngine/DXEngine.h"
#include "Graphics/DXEngine/DXTools.h"
extern bool g_bUse_DX_Engine;


Render3D::Render3D()
{
}


Render3D::~Render3D()
{
}


void Render3D::Cleanup()
{
    Render2D::Cleanup();
}


void Render3D::SetFar(float distance)
{
    far_clip = distance;
}


float Render3D::GetObjectDetail()
{
    return detailScaler;
}


float Render3D::GetFOV()
{
    return horizontal_half_angle * 2.0F;
}


float Render3D::GetVFOV()
{
    return vertical_half_angle * 2.0F;
}


float Render3D::GetDFOV()
{
    return diagonal_half_angle * 2.0F;
}


float Render3D::GetFar()
{
    return far_clip;
}


BOOL Render3D::GetObjectTextureState()
{
    return objTextureState;
}


float Render3D::X()
{
    return cameraPos.x;
}


float Render3D::Y()
{
    return cameraPos.y;
}


float Render3D::Z()
{
    return cameraPos.z;
}


float Render3D::Yaw()
{
    return yaw;
}


float Render3D::Pitch()
{
    return pitch;
}


float Render3D::Roll()
{
    return roll;
}


// Maybe better access methods for a class variable?
void Render3D::GetAt(Tpoint *v)
{
    v->x = cameraRot.M11;
    v->y = cameraRot.M12;
    v->z = cameraRot.M13;
}


void Render3D::GetLeft(Tpoint *v)
{
    v->x = cameraRot.M21;
    v->y = cameraRot.M22;
    v->z = cameraRot.M23;
}


void Render3D::GetUp(Tpoint *v)
{
    v->x = cameraRot.M31;
    v->y = cameraRot.M32;
    v->z = cameraRot.M33;
}


/***************************************************************************\
 Setup the rendering context for thiw view
\***************************************************************************/
void Render3D::Setup(ImageBuffer *imageBuffer)
{
    Tpoint pos;

    horizontal_half_angle = PI / 180.0f; // this is used in the 2d setup call
    detailScaler = 1.0f;
    SetFar(10000.0f);

    // Call our parents Setup code (win is available only after this call)
    Render2D::Setup(imageBuffer);
    SetFOV(60.0f * PI / 180.0f);

    // Initialize our camera parameters to something reasonable

    pos.x = pos.y = pos.z = 0.0f;
    SetCamera(&pos, &IMatrix);

    // Put a default light source into the 3D object drawing context

    //JAM 08Jan04
    lightAmbient = 0.4f;
    lightDiffuse = 0.6f;
    lightSpecular = 0.6f;
    //JAM

    Tpoint dir = { 0.0f, 0.0f, -1.0f };
    SetLightDirection(&dir);

    objTextureState = TRUE;
}



/***************************************************************************\
 Do start of frame housekeeping
\***************************************************************************/
void Render3D::StartDraw(void)
{
    // DX - YELLOW BUG FIX - RED
    Render2D::StartDraw();

    TheStateStack.SetContext(&context);
    TheStateStack.SetCameraProperties(oneOVERtanHFOV, oneOVERtanVFOV, scaleX, scaleY, shiftX, shiftY);
    TheStateStack.SetLight(lightAmbient, lightDiffuse, lightSpecular, &lightVector); //JAM 08Jan04
    TheStateStack.SetLODBias(resRelativeScaler);
    TheStateStack.SetTextureState(TRUE);  //objTextureState );
    TheColorBank.SetColorMode(ColorBankClass::NormalMode);
}

/***************************************************************************\
 Set the dimensions and location of the viewport.
\***************************************************************************/
void Render3D::SetViewport(float l, float t, float r, float b)
{
    // First call the base class's version of this function
    Render2D::SetViewport(l, t, r, b);

    // Redo our FOV dependent setup
    SetFOV(GetFOV());
}


/***************************************************************************\
    Set the field of view of this camera.
\***************************************************************************/
void Render3D::SetFOV(float horizontal_fov, float NearZ)
{
    const float maxHalfFOV =  70 * PI / 180.0f;
    // JB 010120 Allow narrower field of views for Mavs and GBUs
    //const float minHalfFOV =   2 * PI/180.0f;
    //const float minHalfFOV = PI/180.0f;
    //MI need to be able to go higher for the TGP
    const float minHalfFOV = (PI / 180.0f) / 4;
    // JB 010120

    // Set the field of view
    horizontal_half_angle = horizontal_fov / 2.0f;

    // Keep the field of view values within a reasonable range
    if (horizontal_half_angle > maxHalfFOV)
    {
        horizontal_half_angle = maxHalfFOV;
    }
    else if (horizontal_half_angle < minHalfFOV)
    {
        horizontal_half_angle = minHalfFOV;
    }

    // Recompute the vertical and diangonal field of views to retain consistency
    // NOTE:  (Computation of vertical and diagonal ASSUMES SQUARE PIXELS)
    if (scaleX)
    {
        vertical_half_angle = (float)atan2(scaleY * tan(horizontal_half_angle), scaleX);
        diagonal_half_angle = (float)atan2(sqrt(scaleX * scaleX + scaleY * scaleY) * tan(horizontal_half_angle), scaleX);
    }
    else
    {
        vertical_half_angle = (float)atan2(3.0f * tan(horizontal_half_angle), 4.0f);
        diagonal_half_angle = (float)atan2(5.0f * tan(horizontal_half_angle), 4.0f);
    }

    oneOVERtanHFOV = 1.0f / (float)tan(horizontal_half_angle);
    oneOVERtanVFOV = 1.0f / (float)tan(vertical_half_angle);

    SetObjectDetail(detailScaler);

    // Send relevant stuff to the BSP object library
    TheStateStack.SetCameraProperties(oneOVERtanHFOV, oneOVERtanVFOV, scaleX, scaleY, shiftX, shiftY);

    // COBRA - DX - Setu up Projection here
    if (g_bUse_DX_Engine)
    {

        // Default FOV
        D3DXMATRIX matProj;
        D3DXMatrixPerspectiveFov(&matProj, PI / 2, (float)(xRes / yRes), NearZ, context.ZFAR);

        // Original FreeFalcon FOV transformation
        matProj.m10 *= oneOVERtanVFOV;
        matProj.m11 *= oneOVERtanVFOV;
        matProj.m12 *= oneOVERtanVFOV;

        matProj.m00 *= oneOVERtanHFOV;
        matProj.m01 *= oneOVERtanHFOV;
        matProj.m02 *= oneOVERtanHFOV;

        D3DXMATRIX Flip;
        ZeroMemory(&Flip, sizeof(Flip));
        Flip.m02 = -1.0f;
        Flip.m21 = -1.0f;
        Flip.m10 = 1.0f;
        Flip.m33 = 1.0f;
        D3DXMatrixMultiply(&matProj, &Flip, &matProj);


        TheDXEngine.SetProjection(&matProj);
    }
}


/***************************************************************************\
    Set the position and orientation of the camera in world space.
 (Note:  we're storing the inverse of the camera rotation)
\***************************************************************************/
void Render3D::SetCamera(const Tpoint* pos, const Trotation* rot)
{
    float sinRoll,  cosRoll;
    float sinPitch, cosPitch;


    // Store the provided position and orientation
    if (rot)
    {
        memcpy(&cameraPos, pos, sizeof(cameraPos));
        MatrixTranspose(rot, &cameraRot);
    }

    // Back compute the roll, pitch, and yaw of the viewer
    pitch = (float) - asin(cameraRot.M13);
    roll = (float)atan2(cameraRot.M23, cameraRot.M33);
    yaw = (float)atan2(cameraRot.M12, cameraRot.M11);

    if (yaw < 0.0f)
    {
        yaw += 2.0f * PI; // Convert from +/-180 to 0-360 degrees
    }


    // The result of multiplying a point by this matrix will be permuted into
    // screen space by renaming the components of the output vector
    // (after this, plus X will be to the right and plus Y will be down the screen)
    // Output x axis corresponds to row 2
    // Output y axis corresponds to row 3
    // Output z axis corresponds to row 1
    // In this matrix, we also scale output x and y to account for the field of view.
    // Now |X| > Z or |Y| > Z are out
    T.M11 = cameraRot.M11;
    T.M12 = cameraRot.M12;
    T.M13 = cameraRot.M13;

    T.M21 = cameraRot.M21 * oneOVERtanHFOV;
    T.M22 = cameraRot.M22 * oneOVERtanHFOV;
    T.M23 = cameraRot.M23 * oneOVERtanHFOV;

    T.M31 = cameraRot.M31 * oneOVERtanVFOV;
    T.M32 = cameraRot.M32 * oneOVERtanVFOV;
    T.M33 = cameraRot.M33 * oneOVERtanVFOV;

    // Compute the vector from the camera to the origin rotated into camera space
    move.x = - cameraPos.x * T.M11 - cameraPos.y * T.M12 - cameraPos.z * T.M13;
    move.y = - cameraPos.x * T.M21 - cameraPos.y * T.M22 - cameraPos.z * T.M23;
    move.z = - cameraPos.x * T.M31 - cameraPos.y * T.M32 - cameraPos.z * T.M33;

    // Build the billboard matrix (appropriate for Erick's row vector object library)
    sinRoll = (float)sin(roll);
    cosRoll = (float)cos(roll);
    Tbb.M11 = 1.0f;
    Tbb.M21 = 0.0f;
    Tbb.M31 =  0.0f;
    Tbb.M12 = 0.0f;
    Tbb.M22 = cosRoll * oneOVERtanHFOV;
    Tbb.M32 = -sinRoll * oneOVERtanVFOV;
    Tbb.M13 = 0.0f;
    Tbb.M23 = sinRoll * oneOVERtanHFOV;
    Tbb.M33 =  cosRoll * oneOVERtanVFOV;

    // Build the tree matrix
    sinPitch = (float)sin(pitch);
    cosPitch = (float)cos(pitch);
    Tt.M11 =  cosPitch;
    Tt.M21 = sinPitch * Tbb.M23;
    Tt.M31 = sinPitch * Tbb.M33;
    Tt.M12 =  0.0f;
    Tt.M22 = Tbb.M22;
    Tt.M32 = Tbb.M32;
    Tt.M13 = -sinPitch;
    Tt.M23 = cosPitch * Tbb.M23;
    Tt.M33 = cosPitch * Tbb.M33;

    // Send relevant stuff to the BSP object library
    if (g_bUse_DX_Engine)
    {
        D3DXMATRIX a, c;
        Pmatrix b;
        D3DXMatrixRotationY(&a, pitch);
        D3DXMatrixRotationZ(&c, yaw);
        D3DXMatrixMultiply(&a, &a, &c);
        AssignD3DXMATRIXToPmatrix(&b, &a);
        TheStateStack.SetCamera(pos, (Pmatrix*)&cameraRot, (Pmatrix*)&b, &Tt);
    }
    else
    {
        TheStateStack.SetCamera(pos, &T, &Tbb, &Tt);
    }

    //JAM 02Jan04
    TheStateStack.SetView(pos, &cameraRot);
    TheStateStack.SetProjection(horizontal_half_angle * 2.f, (float)(xRes / yRes));
}


/***************************************************************************\
    Set the detail adjustment scaler for objects
\***************************************************************************/
void Render3D::SetObjectDetail(float scaler)
{
    // This constant is set based on the design resolution for the visual objects
    // art work.  Specificly, 60 degree field of view
    // on a display 640 pixels across.  This is our reference display
    // precision, or LOD scale 1.0.
    static const float RadiansPerPixel = (60.0f * PI / 180.0f) / 640;

    // Ignor invalid parameters
    if (scaler <= 0.0f)
    {
        return;
    }

    detailScaler = scaler;
    resRelativeScaler = detailScaler * RadiansPerPixel * scaleX * oneOVERtanHFOV;

    TheStateStack.SetLODBias(resRelativeScaler);
}



/***************************************************************************\
    Set the lighting direction
 (based on a FreeFalcon X north, Y east, Z down coordinate system)
\***************************************************************************/
void Render3D::SetLightDirection(const Tpoint* dir)
{
    lightVector = *dir;
    TheStateStack.SetLight(lightAmbient, lightDiffuse, lightSpecular, &lightVector); //JAM 08Jan04
}



/***************************************************************************\
    Get the lighting direction
 (based on a FreeFalcon X north, Y east, Z down coordinate system)
\***************************************************************************/
void Render3D::GetLightDirection(Tpoint* dir)
{
    *dir = lightVector;
}



/***************************************************************************\
 Select the amount of textureing employed
\***************************************************************************/
void Render3D::SetObjectTextureState(BOOL state)
{
    if (state not_eq objTextureState)
    {
        // Record the choice
        objTextureState = state;

        // Update the object library
        // NOTE:  If any other renderer is inside a StartFrame, this will clobber their settings
        TheStateStack.SetTextureState(objTextureState);
    }
}



/***************************************************************************\
    Transform the given point (from World space to Screen space)
\***************************************************************************/
void Render3D::TransformPoint(Tpoint* p, ThreeDVertex* result)
{
    register float scratch_x;
    register float scratch_y;
    register float scratch_z;
    register DWORD clipFlag;


    // This part does rotation, translation, and scaling
    // Note, we're swapping the x and z axes here to get from z up/down to z far/near
    // then we're swapping the x and y axes to get into conventional screen pixel coordinates
    scratch_z = T.M11 * p->x + T.M12 * p->y + T.M13 * p->z + move.x;
    scratch_x = T.M21 * p->x + T.M22 * p->y + T.M23 * p->z + move.y;
    scratch_y = T.M31 * p->x + T.M32 * p->y + T.M33 * p->z + move.z;


    // Now determine if the point is out behind us or to the sides
    clipFlag  = GetRangeClipFlags(scratch_z, far_clip);
    clipFlag or_eq GetHorizontalClipFlags(scratch_x, scratch_z);
    clipFlag or_eq GetVerticalClipFlags(scratch_y, scratch_z);

    // Finally, do the perspective divide and scale and shift into screen space
    register float OneOverZ = 1.0f / scratch_z;
    result->csX = scratch_x;
    result->csY = scratch_y;
    result->csZ = scratch_z;
    result->x = viewportXtoPixel(scratch_x * OneOverZ);
    result->y = viewportYtoPixel(scratch_y * OneOverZ);
    result->clipFlag = clipFlag;
}

/***************************************************************************\
    Transform the given point (from World space to Screen space)
 The point should be camera-centric: ie has the camera XYZ subtracted
 out of its world space XYZ.  Why have this function?  Because of
 floating point roundoff error.  We may want a high-precision transform
 of a world space point that would otherwise get rounded off if the
 world space point had high-range XYZ values.
\***************************************************************************/
void Render3D::TransformCameraCentricPoint(Tpoint* p, ThreeDVertex* result)
{
    register float scratch_x;
    register float scratch_y;
    register float scratch_z;
    register DWORD clipFlag;


    // This part does rotation, translation, and scaling
    // Note, we're swapping the x and z axes here to get from z up/down to z far/near
    // then we're swapping the x and y axes to get into conventional screen pixel coordinates
    scratch_z = T.M11 * p->x + T.M12 * p->y + T.M13 * p->z;
    scratch_x = T.M21 * p->x + T.M22 * p->y + T.M23 * p->z;
    scratch_y = T.M31 * p->x + T.M32 * p->y + T.M33 * p->z;


    // Now determine if the point is out behind us or to the sides
    clipFlag  = GetRangeClipFlags(scratch_z, far_clip);
    clipFlag or_eq GetHorizontalClipFlags(scratch_x, scratch_z);
    clipFlag or_eq GetVerticalClipFlags(scratch_y, scratch_z);

    // Finally, do the perspective divide and scale and shift into screen space
    register float OneOverZ = 1.0f / scratch_z;
    result->csX = scratch_x;
    result->csY = scratch_y;
    result->csZ = scratch_z;
    result->x = viewportXtoPixel(scratch_x * OneOverZ);
    result->y = viewportYtoPixel(scratch_y * OneOverZ);
    result->clipFlag = clipFlag;
}



/***************************************************************************\
    Transform the given point (from World space to View space)
\***************************************************************************/
void Render3D::TransformPointToView(Tpoint * p, Tpoint * result)
{
    // This part does rotation, translation, and scaling
    // no swapping...
    result->x = T.M11 * p->x + T.M12 * p->y + T.M13 * p->z + move.x;
    result->y = T.M21 * p->x + T.M22 * p->y + T.M23 * p->z + move.y;
    result->z = T.M31 * p->x + T.M32 * p->y + T.M33 * p->z + move.z;
}

//JAM 03Dec03
void Render3D::TransformPointToViewSwapped(Tpoint *p, Tpoint *result)
{
    result->z = T.M11 * p->x + T.M12 * p->y + T.M13 * p->z + move.x;
    result->x = T.M21 * p->x + T.M22 * p->y + T.M23 * p->z + move.y;
    result->y = T.M31 * p->x + T.M32 * p->y + T.M33 * p->z + move.z;
}



/***************************************************************************\
    Transform the given point (from World space to Screen space)
 Uses billboard matrix.  Most of this is just duplicatcion of
 TransformPoint and perhaps should be condensed into 1 function
\***************************************************************************/
void Render3D::TransformBillboardPoint(Tpoint* p, Tpoint *viewOffset, ThreeDVertex* result)
{
    register float scratch_x;
    register float scratch_y;
    register float scratch_z;
    register DWORD clipFlag;

    // This part does rotation, translation, and scaling
    // Note, we're swapping the x and z axes here to get from z up/down to z far/near
    // then we're swapping the x and y axes to get into conventional screen pixel coordinates
    // Note: since this is a billboard we don't have to do the full:
    //  scratch_z = T.M11 * p->x + T.M12 * p->y + T.M13 * p->z + move.x;
    //  scratch_x = T.M21 * p->x + T.M22 * p->y + T.M23 * p->z + move.y;
    //  scratch_y = T.M31 * p->x + T.M32 * p->y + T.M33 * p->z + move.z;
    // since we know where some 1's and 0's are

    scratch_z =  p->x + viewOffset->x;
    scratch_x =  Tbb.M22 * p->y + Tbb.M23 * p->z + viewOffset->y;
    scratch_y =  Tbb.M32 * p->y + Tbb.M33 * p->z + viewOffset->z;


    // Now determine if the point is out behind us or to the sides
    clipFlag  = GetRangeClipFlags(scratch_z, far_clip);
    clipFlag or_eq GetHorizontalClipFlags(scratch_x, scratch_z);
    clipFlag or_eq GetVerticalClipFlags(scratch_y, scratch_z);

    // Finally, do the perspective divide and scale and shift into screen space
    register float OneOverZ = 1.0f / scratch_z;
    result->csX = scratch_x;
    result->csY = scratch_y;
    result->csZ = scratch_z;
    result->x = viewportXtoPixel(scratch_x * OneOverZ);
    result->y = viewportYtoPixel(scratch_y * OneOverZ);
    result->clipFlag = clipFlag;
}


/***************************************************************************\
    Transform the given point (from World space to Screen space)
 Uses tree matrix.  Most of this is just duplicatcion of
 TransformPoint and perhaps should be condensed into 1 function
\***************************************************************************/
void Render3D::TransformTreePoint(Tpoint* p, Tpoint *viewOffset, ThreeDVertex* result)
{
    register float scratch_x;
    register float scratch_y;
    register float scratch_z;
    register DWORD clipFlag;


    // This part does rotation, translation, and scaling
    // Note, we're swapping the x and z axes here to get from z up/down to z far/near
    // then we're swapping the x and y axes to get into conventional screen pixel coordinates
    scratch_z = Tt.M11 * p->x + Tt.M12 * p->y + Tt.M13 * p->z + viewOffset->x;
    scratch_x = Tt.M21 * p->x + Tt.M22 * p->y + Tt.M23 * p->z + viewOffset->y;
    scratch_y = Tt.M31 * p->x + Tt.M32 * p->y + Tt.M33 * p->z + viewOffset->z;


    // Now determine if the point is out behind us or to the sides
    clipFlag  = GetRangeClipFlags(scratch_z, far_clip);
    clipFlag or_eq GetHorizontalClipFlags(scratch_x, scratch_z);
    clipFlag or_eq GetVerticalClipFlags(scratch_y, scratch_z);

    // Finally, do the perspective divide and scale and shift into screen space
    register float OneOverZ = 1.0f / scratch_z;
    result->csX = scratch_x;
    result->csY = scratch_y;
    result->csZ = scratch_z;
    result->x = viewportXtoPixel(scratch_x * OneOverZ);
    result->y = viewportYtoPixel(scratch_y * OneOverZ);
    result->clipFlag = clipFlag;
}



/***************************************************************************\
    Reverse transform the given point (from screen space to world space vector)
\***************************************************************************/
void Render3D::UnTransformPoint(Tpoint* p, Tpoint* result)
{
    float scratch_x, scratch_y, scratch_z;
    float x, y, z;
    float mag;

    // Undo the viewport computations to get normalized screen coordinates
    scratch_x = (p->x - shiftX) / scaleX;
    scratch_y = (p->y - shiftY) / scaleY;
    scratch_z = 1.0f;

    // Assume the distance from the viewer is 1.0 -- we'll normalize later
    // This means we don't have to undo the perspective divide

    // Now undo the field of view scale applied in the transformation
    scratch_x /= oneOVERtanHFOV;
    scratch_y /= oneOVERtanVFOV;

    // Undo the camera rotation (apply the inverse rotation)
    x = cameraRot.M11 * scratch_z + cameraRot.M21 * scratch_x + cameraRot.M31 * scratch_y;
    y = cameraRot.M12 * scratch_z + cameraRot.M22 * scratch_x + cameraRot.M32 * scratch_y;
    z = cameraRot.M13 * scratch_z + cameraRot.M23 * scratch_x + cameraRot.M33 * scratch_y;

    // Don't need to undo the camera translation, because we want a vector
    // Lets normalize just to be kind
    mag = x * x + y * y + z * z;
    mag = 1.0f / (float)sqrt(mag);
    result->x = x * mag;
    result->y = y * mag;
    result->z = z * mag;
}



/***************************************************************************\
    Return the distance of the world space point from the camera plane.
 A negative result indicates the point is behind the camera.
\***************************************************************************/
float Render3D::ZDistanceFromCamera(Tpoint* p)
{
    // This part does rotation, translation, and scaling
    // Note, we're swapping the x and z axes here to get from z up/down to z far/near
    // then we're swapping the x and y axes to get into conventional screen pixel coordinates.
    // Here we only care about the Z axis since that is the distance from the viewer.
    return ((T.M11 * p->x) + (T.M12 * p->y) + (T.M13 * p->z) + move.x);
}



/*******************************************************************************************\
 Immediatly draw the specified object.
 Warning:  Going this route causes the specfied object to be loaded/unloaded each
 time it is drawn.  Besides being wasteful, this may result in too simple an LOD, no
 textures, or nothing being drawn because the data will not have arrived from disk
 by the time the Draw call happens.  A prior LockAndLoad() call could avoid
 these problems.
\*******************************************************************************************/
void Render3D::Render3DObject(int id, Tpoint* pos, const Trotation* orientation)
{
    ShiAssert(IsReady());

    ObjectInstance instance(id);

    TheStateStack.DrawObject(&instance, orientation, pos);
}



/***************************************************************************\
 Draw a colored pixel in worldspace using the current camera.
 For now, we don't try to draw primitives with any verticies off screen.
\***************************************************************************/
void Render3D::Render3DPoint(Tpoint* p1)
{
    ThreeDVertex ps1;

    // Transform the point from world space to window space
    TransformPoint(p1, &ps1);

    if (ps1.clipFlag not_eq ON_SCREEN)  return;

    // Draw the point
    Render2DPoint((UInt16)ps1.x, (UInt16)ps1.y);
}



/***************************************************************************\
 Draw a colored one pixel line in worldspace using the current camera.
\***************************************************************************/
void Render3D::Render3DLine(Tpoint* p1, Tpoint* p2)
{
    ThreeDVertex ps1, ps2;

    // Transform the points from world space to window space
    TransformPoint(p1, &ps1);
    TransformPoint(p2, &ps2);

    // Quit now if both ends are clipped by the same edge
    if (ps1.clipFlag bitand ps2.clipFlag)  return;

    // Clip the line as necessary
    if (ps1.clipFlag bitand CLIP_NEAR)
    {
        IntersectNear(&ps1, &ps2, &ps1);
    }
    else if (ps2.clipFlag bitand CLIP_NEAR)
    {
        IntersectNear(&ps1, &ps2, &ps2);
    }

    if (ps1.clipFlag bitand ps2.clipFlag)  return;

    if (ps1.clipFlag bitand CLIP_BOTTOM)
    {
        IntersectBottom(&ps1, &ps2, &ps1);
    }
    else if (ps2.clipFlag bitand CLIP_BOTTOM)
    {
        IntersectBottom(&ps1, &ps2, &ps2);
    }

    if (ps1.clipFlag bitand CLIP_TOP)
    {
        IntersectTop(&ps1, &ps2, &ps1);
    }
    else if (ps2.clipFlag bitand CLIP_TOP)
    {
        IntersectTop(&ps1, &ps2, &ps2);
    }

    if (ps1.clipFlag bitand ps2.clipFlag)  return;

    if (ps1.clipFlag bitand CLIP_RIGHT)
    {
        IntersectRight(&ps1, &ps2, &ps1);
    }
    else if (ps2.clipFlag bitand CLIP_RIGHT)
    {
        IntersectRight(&ps1, &ps2, &ps2);
    }

    if (ps1.clipFlag bitand CLIP_LEFT)
    {
        IntersectLeft(&ps1, &ps2, &ps1);
    }
    else if (ps2.clipFlag bitand CLIP_LEFT)
    {
        IntersectLeft(&ps1, &ps2, &ps2);
    }

    // Draw the line
    Render2DLine((UInt16)ps1.x, (UInt16)ps1.y, (UInt16)ps2.x, (UInt16)ps2.y);
}



/***************************************************************************\
 Draw a flat shaded triangle in worldspace using the current camera.
 For now, we don't try to draw primitives with any verticies inside the
 near plane.
\***************************************************************************/
void Render3D::Render3DFlatTri(Tpoint* p1, Tpoint* p2, Tpoint* p3)
{
    ThreeDVertex ps1, ps2, ps3;

    // Transform the points from world space to window space
    TransformPoint(p1, &ps1);
    TransformPoint(p2, &ps2);
    TransformPoint(p3, &ps3);

    // Skip the tri if clipped anywhere
    // I'm not sure this function is called anywhere anyway.  With the current
    // state of things, just checking near clip could cause bad problems since
    // MPR no longer does other edge clipping.  We'd have to do that here.
    if (ps1.clipFlag or ps2.clipFlag or ps3.clipFlag)  return;

    // Don't draw the triangle if it is backfacing (counter-clockwise in screen space)
    // edg: always draw irregardless of backfacing
    /*
    if ( (ps2.y - ps1.y)*(ps3.x - ps1.x) >
      (ps2.x - ps1.x)*(ps3.y - ps1.y)   ) {
     return;
    }
    */

    // Draw the triangle
    Render2DTri((UInt16)ps1.x, (UInt16)ps1.y, (UInt16)ps2.x, (UInt16)ps2.y, (UInt16)ps3.x, (UInt16)ps3.y);
}



/***************************************************************************\
    Draw a quad as a fan.  Uses current state.  Not required to be planar.
\***************************************************************************/
void Render3D::DrawSquare(ThreeDVertex* v0, ThreeDVertex* v1, ThreeDVertex* v2, ThreeDVertex* v3, int CullFlag, bool gifPicture, bool terrain) //JAM 14Sep03
{
    unsigned short count;
    BOOL useFirst = TRUE;
    BOOL useLast = TRUE;

    // Check the clipping flags on the verteces which bound this region
    if (v0->clipFlag bitor v1->clipFlag bitor v2->clipFlag bitor v3->clipFlag)
    {
        // If all verticies are clipped by the same edge, skip this square
        if (v0->clipFlag bitand v1->clipFlag bitand v2->clipFlag bitand v3->clipFlag)
            return;

        ThreeDVertex *vertPointers[4];
        vertPointers[2] = v2;

        // If any verteces are clipped, do separate triangles since the quad isn't necessarily planar
        if (v0->clipFlag bitor v1->clipFlag bitor v2->clipFlag)
        {
            vertPointers[0] = v0;
            vertPointers[1] = v1;
            ClipAndDraw3DFan(&vertPointers[0], 3, CullFlag, gifPicture, terrain); //JAM 14Sep03
            useFirst = FALSE;
        }

        if (v0->clipFlag bitor v2->clipFlag bitor v3->clipFlag)
        {
            vertPointers[1] = v0;
            vertPointers[3] = v3;
            ClipAndDraw3DFan(&vertPointers[1], 3, CullFlag, gifPicture, terrain); //JAM 14Sep03

            if (useFirst)
                useLast = FALSE;
            else
                return;
        }
    }

    if (CullFlag)
    {
        if (CullFlag == CULL_ALLOW_CW)
        {
            // Decide if either of the two triangles are back facing
            if (useFirst)
            {
                if (((v2->y - v1->y)) * ((v0->x - v1->x)) > ((v2->x - v1->x)) * ((v0->y - v1->y)))
                    useFirst = FALSE;
            }

            if (useLast)
            {
                if (((v0->y - v3->y)) * ((v2->x - v3->x)) > ((v0->x - v3->x)) * ((v2->y - v3->y)))
                    useLast = FALSE;
            }
        }

        else
        {
            // Decide if either of the two triangles are back facing
            if (useFirst)
            {
                if (((v2->y - v1->y)) * ((v0->x - v1->x)) < ((v2->x - v1->x)) * ((v0->y - v1->y)))
                    useFirst = FALSE;
            }

            if (useLast)
            {
                if (((v0->y - v3->y)) * ((v2->x - v3->x)) < ((v0->x - v3->x)) * ((v2->y - v3->y)))
                    useLast = FALSE;
            }
        }
    }

    // If culling or clipping took care of both triangles, quit now
    if (useFirst and useLast)
        count = 4;
    else
    {
        if (useFirst or useLast)
            count = 3;
        else
            return;
    }

    if (useFirst)
    {
        MPRVtxTexClr_t *arr[] = { v0, v1, v2, v3 };
        context.DrawPrimitive(MPR_PRM_TRIFAN, MPR_VI_COLOR bitor MPR_VI_TEXTURE, count, arr, terrain); //JAM 14Sep03
    }
    else
    {
        MPRVtxTexClr_t *arr[] = { v0, v2, v3 };
        context.DrawPrimitive(MPR_PRM_TRIFAN, MPR_VI_COLOR bitor MPR_VI_TEXTURE, count, arr, terrain); //JAM 14Sep03
    }

}

/***************************************************************************\
    Draw a triangle using current state
\***************************************************************************/
void Render3D::DrawTriangle(ThreeDVertex* v0, ThreeDVertex* v1, ThreeDVertex* v2, int CullFlag, bool gifPicture, bool terrain)  //JAM 14Sep03
{
    // Check the clipping flags on the verteces which bound this region
    if (v0->clipFlag bitor v1->clipFlag bitor v2->clipFlag)
    {
        // If all verticies are clipped by the same edge, skip this triangle
        if (v0->clipFlag bitand v1->clipFlag bitand v2->clipFlag)
            return;

        // If any verteces are clipped, do them as a special case
        ThreeDVertex *vertPointers[3];
        vertPointers[0] = v0;
        vertPointers[1] = v1;
        vertPointers[2] = v2;
        ClipAndDraw3DFan(vertPointers, 3, CullFlag, gifPicture, terrain); //JAM 14Sep03
        return;
    }

    if (CullFlag)
    {
        if (CullFlag == CULL_ALLOW_CW)
        {
            // Decide if back facing CW
            if (((v2->y - v1->y)) * ((v0->x - v1->x)) > ((v2->x - v1->x)) * ((v0->y - v1->y)))
                return;
        }

        else
        {
            // Decide if back facing CCW
            if (((v2->y - v1->y)) * ((v0->x - v1->x)) < ((v2->x - v1->x)) * ((v0->y - v1->y)))
                return;
        }
    }

    // Draw the tri
    MPRVtxTexClr_t *arr[] = { v0, v1, v2 };
    context.DrawPrimitive(MPR_PRM_TRIFAN, MPR_VI_COLOR bitor MPR_VI_TEXTURE, 3, arr, terrain); //JAM 14Sep03
}


