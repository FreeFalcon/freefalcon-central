#include "Graphics\Include\Render2d.h"
#include "stdhdr.h"
#include "drawable.h"
#include "otwdrive.h"
#include "cppanel.h"
#include "tod.h"

extern bool g_bEnableColorMfd, g_bMFDHighContrast;

BOOL DrawableClass::greenMode = FALSE;
//float DrawableClass::lighting[3] = { 1.0f, 1.0f, 1.0f }; //sfr: added for night lighting

void DrawableClass::DisplayExit (void)
{
	if (privateDisplay){
		privateDisplay->Cleanup();
		delete privateDisplay;
		privateDisplay = NULL;
	}
}

// JPO draw a border around the MFD.
void DrawableClass::DrawBorder ()
{
    static const float bd = 0.99f;
    static const float st = 0.90f;
    display->Line (bd, st, bd, -st);
    display->Line (st, -bd, -st, -bd);
    display->Line (-bd, -st, -bd, st);
    display->Line (-st, bd, st, bd);
}

unsigned int DrawableClass::MFDColors[] = { // JPO MLU MFD Colors
	0xFF00FF00, // green (default)
	0xFFFFFFFF, // white
	0xFF0000ff, // red
	0xFF00ffff, // yellow
	0xFFffff00, // cyan
	0xFFff00ff, // magenta
	0xFFff0000, // blue
	0xFF7b7b7b, // grey
	0xFF66ff00, // bright green
	0xFFafafaf, // "whity gray"
};

unsigned int DrawableClass::AltMFDColors[] = { // JPO Alternative High contrast color table (for color blind use)
	0xFF00CC00, // green (default)
	0xFFFFFFFF, // white
	0xFF6666ff, // pink
	0xFF00ffff, // yellow
	0xFFffcc33, // cyan
	0xFFff00ff, // magenta
	0xFFff0000, // blue
	0xFF7b7b7b, // grey
	0xFF66ff00, // bright green
	0xFFafafaf, // "whity gray"
};

// JPO - pick color with intensity and backwards compat
unsigned int DrawableClass::GetMfdColor (MfdColor type) 
{
    ShiAssert(GetIntensity() != 0 &&
		GetIntensity() != 0xCCCCCCCC); // we shouldn't ever switch off completely.
    if (!g_bEnableColorMfd || greenMode)
		type = MFD_DEFAULT;
    if (g_bMFDHighContrast)
		return AltMFDColors[type] & GetIntensity();
    else
		return MFDColors[type] & GetIntensity();
}

// JPO - pick color with intensity and backwards compat
unsigned int DrawableClass::GetAgedMfdColor (MfdColor type, int age) 
{
    unsigned int color = GetMfdColor (type);
    if (age == 0) return color;
    color =  (((color & 0xff0000) >> age) & 0xff0000) | // RED (or BLUE)
	(((color & 0xff00) >> age) & 0xff00) | //  GREEN
	(((color & 0xff) >> age) & 0xff); // BLUE (or RED)
    return color;
}


// NOTE - Sms buttons are labeled starting at the top left and
//        proceeding clockwise around the MFD.
static const float textLoc[20][2] = {
   -0.6F,  0.95F,   // 0
   -0.3F,  0.95F,
    0.0F,  0.95F,
    0.3F,  0.95F,
    0.6F,  0.95F,   // 4
    0.95F,  0.6F,   // 5
    0.95F,  0.3F,
    0.95F,  0.0F,
    0.95F, -0.3F,
    0.95F, -0.6F,   // 8
    0.6F, -0.85F,   // 9
    0.3F, -0.85F,
    0.0F, -0.85F,
   -0.3F, -0.85F,
   -0.6F, -0.85F,   // 14
   -0.95F, -0.6F,   // 15
   -0.95F, -0.3F,
   -0.95F,  0.0F,
   -0.95F,  0.3F,
   -0.95F,  0.6F    // 19
};

// JPO - work out where a button would be.
void DrawableClass::GetButtonPos(int bno, float *xp, float *yp)
{
    CPPanel* curPanel = OTWDriver.pCockpitManager->GetActivePanel();
    
    if (curPanel)
    {
		//Wombat778 4-12-04 rewrote to allow independant MFD 3 and 4 OSBS	
		//Wombat778 4-15-04 Made safer, because it looks like MFDOn might not get initiliazed
		switch (MFDOn)
		{
		default:
			*xp = curPanel->osbLocation[0][bno][0];
			*yp = curPanel->osbLocation[0][bno][1];	
			break;
		case 1:
			*xp = curPanel->osbLocation[1][bno][0];
			*yp = curPanel->osbLocation[1][bno][1];	
			break;
		case 2:
			*xp = curPanel->osbLocation[2][bno][0];
			*yp = curPanel->osbLocation[2][bno][1];	
			break;
		case 3:
			*xp = curPanel->osbLocation[3][bno][0];
			*yp = curPanel->osbLocation[3][bno][1];	
			break;
		}


    }
    else
    {
	*xp = textLoc[bno][0];
	*yp = textLoc[bno][1];
    }
}

void DrawableClass::LabelButton (int idx, char* str1, char* str2, int inverse)
{

    float multiLineOffset;
    float xPos, yPos;
    GetButtonPos(idx, &xPos, &yPos); // JPO use new routine
    
    multiLineOffset = display->TextHeight();
    
    if (inverse)
    {
	inverse = 2;
    }

    if (str2 == NULL || *str2 == '\0') // JPO - ignore 2nd string null
    {
	if (idx > 4 && idx < 10)
	{
	    display->TextRight (xPos, yPos, str1, inverse);
	}
	else if (idx > 14)
	{
	    display->TextLeft (xPos, yPos, str1, inverse);
	}
	else
	    display->TextCenter (xPos, yPos, str1, inverse);
    }
    else
    {
	if (idx <= 4)
	{
	    display->TextCenter (xPos, yPos, str1, inverse);
	    display->TextCenter (xPos, yPos - multiLineOffset, str2, inverse);
	}
	else if (idx > 4 && idx < 10)
	{
	    display->TextRight (xPos, yPos + multiLineOffset*0.5F, str1, inverse);
	    display->TextRight (xPos, yPos - multiLineOffset*0.5F, str2, inverse);
	}
	else if (idx > 14)
	{
	    display->TextLeft (xPos, yPos + multiLineOffset*0.5F, str1, inverse);
	    display->TextLeft (xPos, yPos - multiLineOffset*0.5F, str2, inverse);
	}
	else
	{
	    display->TextCenter (xPos, yPos, str1, inverse);
	    display->TextCenter (xPos, yPos + multiLineOffset, str2, inverse);
	}
    }
}

void DrawableClass::SetGreenMode(BOOL state)
{
	greenMode = state;
	//now we use green light
	/*lighting[0] = 0.1f;
	lighting[1] = 0.4f;
	lighting[2] = 0.1f;*/
}

//sfr: added for night lighting
void DrawableClass::SetLighting(float red, float green, float blue){
	/*lighting[0] = red;
	lighting[1] = green;
	lighting[2] = blue;*/
}

