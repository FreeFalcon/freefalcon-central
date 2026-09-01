#include "stdhdr.h"
#include "misldisp.h"
#include "graphics/include/render2d.h"
#include "graphics/include/mono2d.h"

MissileDisplayClass::MissileDisplayClass(SimMoverClass* newPlatform)
    : DrawableClass()
{

    platform = newPlatform;
    displayType = NoDisplay;
    display = NULL;
    flags = 0;
}

void MissileDisplayClass::DisplayInit(ImageBuffer* image)
{
    privateDisplay = new Render2D;
    ((Render2D*)privateDisplay)->Setup(image);
    privateDisplay->SetColor(0xff00ff00);
}

void MissileDisplayClass::Display(VirtualDisplay*)
{
}
