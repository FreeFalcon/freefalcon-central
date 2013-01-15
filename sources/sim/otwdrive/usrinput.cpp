#include "Graphics\Include\RenderOW.h"
#include "Graphics\Include\constant.h"
#include "stdhdr.h"
#include "otwdrive.h"

// OW FIXME: this had to be added after installing the DX8 Beta 1 SDK
//Retro_dead 15Jan2004	#define DIRECTINPUT_VERSION 0x0700
//Retro_dead 15Jan2004	#include "dinput.h"

extern int CommandsKeyCombo;
extern int CommandsKeyComboMod;
extern unsigned int chatterCount;
extern char chatterStr[256];

void OTWDriverClass::GetUserPosition (void)
{
static int state = 0;
char promptStr[256];
static float xPos, yPos, zPos;
  
   renderer->SetViewport(-0.25F, 0.7F, 0.25F, 0.5F);
   renderer->SetColor (0xff000000);
   renderer->ClearDraw();
   renderer->SetColor (0xff00ff00);
   memset (promptStr, ' ', chatterCount);
   if (insertMode)
      promptStr[chatterCount] = '+';
   else
      promptStr[chatterCount] = ')';
   promptStr[chatterCount+1] = 0;

   switch (state)
   {
      case 0:
         sprintf (chatterStr, "%.2f", flyingEye->XPos() / FEET_PER_KM);
         state ++;
      case 1:
         renderer->TextCenter (0.0F, 0.8F, "Enter X Coord");

         if (CommandsKeyCombo == 0)
         {
            xPos = (float)atof (chatterStr) * FEET_PER_KM;
            sprintf (chatterStr, "%.2f", flyingEye->YPos() / FEET_PER_KM);
            chatterCount = 0;
            CommandsKeyCombo = -1;
            CommandsKeyComboMod = -1;
            state ++;
         }
      break;

      case 2:
         renderer->TextCenter (0.0F, 0.8F, "Enter Y Coord");

         if (CommandsKeyCombo == 0)
         {
            yPos = (float)atof (chatterStr) * FEET_PER_KM;
            sprintf (chatterStr, "%.2f", flyingEye->ZPos());
            chatterCount = 0;
            CommandsKeyCombo = -1;
            CommandsKeyComboMod = -1;
            state ++;
         }
      break;

      case 3:
         renderer->TextCenter (0.0F, 0.8F, "Enter Z Coord");

         if (CommandsKeyCombo == 0)
         {
            zPos = (float)atof (chatterStr);
            memset (chatterStr, 0, 256);
            chatterCount = 0;
            getNewCameraPos = FALSE;
            state = 0;
            flyingEye->SetPosition (xPos, yPos, zPos);
         }
      break;
   }
   renderer->TextLeft (-0.5F, -0.5F, chatterStr);
   renderer->TextLeft (-0.5F, -0.5F, promptStr);
}
