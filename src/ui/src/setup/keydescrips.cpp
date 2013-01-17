#include <windows.h>
#include "chandler.h"
#include "sim\include\ascii.h"
#include "textids.h"
#include "sim\include\inpFunc.h"

struct kbstr
{
	unsigned char scancode;
	long def_ID;
};

struct kbstr default_desc[]=
{
	{ 0xDD,TXT_KBD_APPS_KEY },
	{ 0x91,TXT_KBD_PC98_AT },
	{ 0x96,TXT_KBD_JAPANAX },
	{ 0x0E,TXT_KBD_BACKSPACE },
	{ 0x3A,TXT_KBD_CAPSLOCK },
	{ 0x90,TXT_KBD_CIRCUMFLEX },
	{ 0x79,TXT_KBD_JPCONV },
	{ 0xD3,TXT_KBD_DELETE },
	{ 0xD0,TXT_KBD_DOWNARROW },
	{ 0xCF,TXT_KBD_END },
	{ 0x01,TXT_KBD_ESCAPE },
	{ 0x3B,TXT_KBD_F1 },
	{ 0x44,TXT_KBD_F10 },
	{ 0x57,TXT_KBD_F11 },
	{ 0x58,TXT_KBD_F12 },
	{ 0x64,TXT_KBD_F13 },
	{ 0x65,TXT_KBD_F14 },
	{ 0x66,TXT_KBD_F15 },
	{ 0x3C,TXT_KBD_F2 },
	{ 0x3D,TXT_KBD_F3 },
	{ 0x3E,TXT_KBD_F4 },
	{ 0x3F,TXT_KBD_F5 },
	{ 0x40,TXT_KBD_F6 },
	{ 0x41,TXT_KBD_F7 },
	{ 0x42,TXT_KBD_F8 },
	{ 0x43,TXT_KBD_F9 },
	{ 0x29,TXT_KBD_GRAVE },
	{ 0xC7,TXT_KBD_HOME },
	{ 0xD2,TXT_KBD_INSERT },
	{ 0x70,TXT_KBD_KANA },
	{ 0x94,TXT_KBD_KANJI },
	{ 0xCB,TXT_KBD_LEFTARROW },
	{ 0xDB,TXT_KBD_LEFTWIN },
	{ 0x7B,TXT_KBD_NOCONV },
	// 	// JB 011212 { 0x45,TXT_KBD_NUMLOCK },
	{ 0xC5,TXT_KBD_NUMLOCK },
	{ 0x52,TXT_KBD_NUM0 },
	{ 0x4F,TXT_KBD_NUM1 },
	{ 0x50,TXT_KBD_NUM2 },
	{ 0x51,TXT_KBD_NUM3 },
	{ 0x4B,TXT_KBD_NUM4 },
	{ 0x4C,TXT_KBD_NUM5 },
	{ 0x4D,TXT_KBD_NUM6 },
	{ 0x47,TXT_KBD_NUM7 },
	{ 0x48,TXT_KBD_NUM8 },
	{ 0x49,TXT_KBD_NUM9 },
	{ 0xB3,TXT_KBD_NUMCOMMA },
	{ 0x9C,TXT_KBD_NUMENTER },
	{ 0x8D,TXT_KBD_NUMEQUALS },
	{ 0x4A,TXT_KBD_NUMMINUS },
	{ 0x53,TXT_KBD_NUMPERIOD },
	{ 0x4E,TXT_KBD_NUMPLUS },
	{ 0xB5,TXT_KBD_NUMDIVIDE },
	{ 0x37,TXT_KBD_NUMTIMES },
	{ 0xD1,TXT_KBD_PAGEDOWN },
	{ 0xC9,TXT_KBD_PAGEUP },
	{ 0x1C,TXT_KBD_ENTER },
	{ 0xCD,TXT_KBD_RTARROW },
	{ 0xDC,TXT_KBD_RT_WIN },
	{ 0x46,TXT_KBD_SCROLLLOCK },
	{ 0x95,TXT_KBD_STOP },
	{ 0xB7,TXT_KBD_SYSREQ },
	{ 0x0F,TXT_KBD_TAB },
	{ 0x93,TXT_KBD_UNDERLINE },
	{ 0x97,TXT_KBD_UNLABELED },
	{ 0xC8,TXT_KBD_UPARROW },
	{ 0x7D,TXT_KBD_YEN },
	{ 0, 0 },
};

#define KEY_DESCRIP_LEN    20

char **KeyDescrips;

// This routine will grab the REAL keyboard keys... not what the DIK table says they should be
// This supports foreign keyboards too
//
void InitKeyDescrips(void)
{
	char *temp;
	short i;

	KeyDescrips = new char*[256];

	memset(KeyDescrips,0,sizeof(char *) * 256);

	temp = new char[KEY_DESCRIP_LEN];
	strcpy(temp, gStringMgr->GetString(TXT_NONE));
	KeyDescrips[0] = temp;

	// Scan Ascii table for displayable keys
	for(i=1;i<256;i++)
		if(AsciiChar(i,0) && AsciiChar(i,0) != ' ')
		{
			temp=new char[KEY_DESCRIP_LEN];
			temp[0]=AsciiChar(i,0);
			temp[1]=0;
			KeyDescrips[i]=temp;
		}

	temp=new char[KEY_DESCRIP_LEN];
	strcpy(temp,gStringMgr->GetString(TXT_KBD_SPACE));
	KeyDescrips[0x39]=temp;

	// Go through list of default values and set the ones not
	// yet defined
	i=0;
	while(default_desc[i].scancode)
	{
		if(!KeyDescrips[default_desc[i].scancode])
		{
			temp=new char[KEY_DESCRIP_LEN];
			strncpy(temp,gStringMgr->GetString(default_desc[i].def_ID), KEY_DESCRIP_LEN);
			ShiAssert (temp[KEY_DESCRIP_LEN-1] == 0);
			temp[KEY_DESCRIP_LEN-1] = 0;
			KeyDescrips[default_desc[i].scancode]=temp;
		}
		i++;
	}
}

#if 0

void InitKeyDescrips(void)
{
char *temp;

KeyDescrips = new char*[256];

memset(KeyDescrips,0,sizeof(char *) * 256);

temp = new char[20];
strcpy(temp, "0");
KeyDescrips[0x0B] = temp;

temp = new char[20];
strcpy(temp, "1");
KeyDescrips[0x02] = temp;

temp = new char[20];
strcpy(temp, "2");
KeyDescrips[0x03] = temp;

temp = new char[20];
strcpy(temp, "3");
KeyDescrips[0x04] = temp;

temp = new char[20];
strcpy(temp, "4");
KeyDescrips[0x05] = temp;

temp = new char[20];
strcpy(temp, "5");
KeyDescrips[0x06] = temp;

temp = new char[20];
strcpy(temp, "6");
KeyDescrips[0x07] = temp;

temp = new char[20];
strcpy(temp, "7");
KeyDescrips[0x08] = temp;

temp = new char[20];
strcpy(temp, "8");
KeyDescrips[0x09] = temp;

temp = new char[20];
strcpy(temp, "9");
KeyDescrips[0x0A] = temp;

temp = new char[20];
strcpy(temp, "a");
KeyDescrips[0x1E] = temp;

temp = new char[20];
strcpy(temp, "'");
KeyDescrips[0x28] = temp;

temp = new char[20];
strcpy(temp, "Apps key");
KeyDescrips[0xDD] = temp;

temp = new char[20];
strcpy(temp, "PC98 AT");
KeyDescrips[0x91] = temp;

temp = new char[20];
strcpy(temp, "Japan AX");
KeyDescrips[0x96] = temp;

temp = new char[20];
strcpy(temp, "b");
KeyDescrips[0x30] = temp;

temp = new char[20];
strcpy(temp, "\\");
KeyDescrips[0x2B] = temp;

temp = new char[20];
strcpy(temp, "Bkspace");
KeyDescrips[0x0E] = temp;

temp = new char[20];
strcpy(temp, "c");
KeyDescrips[0x2E] = temp;

temp = new char[20];
strcpy(temp, "Caps Lk");
KeyDescrips[0x3A] = temp;

temp = new char[20];
strcpy(temp, "Circumflex");
KeyDescrips[0x90] = temp;

temp = new char[20];
strcpy(temp, ":");
KeyDescrips[0x92] = temp;

temp = new char[20];
strcpy(temp, ",");
KeyDescrips[0x33] = temp;

temp = new char[20];
strcpy(temp, "Jp Conv");
KeyDescrips[0x79] = temp;

temp = new char[20];
strcpy(temp, "d");
KeyDescrips[0x20] = temp;

temp = new char[20];
strcpy(temp, "Delete");
KeyDescrips[0xD3] = temp;

temp = new char[20];
strcpy(temp, "Dn Arrow");
KeyDescrips[0xD0] = temp;

temp = new char[20];
strcpy(temp, "e");
KeyDescrips[0x12] = temp;

temp = new char[20];
strcpy(temp, "End");
KeyDescrips[0xCF] = temp;

temp = new char[20];
strcpy(temp, "=");
KeyDescrips[0x0D] = temp;

temp = new char[20];
strcpy(temp, "Escape");
KeyDescrips[0x01] = temp;

temp = new char[20];
strcpy(temp, "f");
KeyDescrips[0x21] = temp;

temp = new char[20];
strcpy(temp, "F1");
KeyDescrips[0x3B] = temp;

temp = new char[20];
strcpy(temp, "F10");
KeyDescrips[0x44] = temp;

temp = new char[20];
strcpy(temp, "F11");
KeyDescrips[0x57] = temp;

temp = new char[20];
strcpy(temp, "F12");
KeyDescrips[0x58] = temp;

temp = new char[20];
strcpy(temp, "F13");
KeyDescrips[0x64] = temp;

temp = new char[20];
strcpy(temp, "F14");
KeyDescrips[0x65] = temp;

temp = new char[20];
strcpy(temp, "F15");
KeyDescrips[0x66] = temp;

temp = new char[20];
strcpy(temp, "F2");
KeyDescrips[0x3C] = temp;

temp = new char[20];
strcpy(temp, "F3");
KeyDescrips[0x3D] = temp;

temp = new char[20];
strcpy(temp, "F4");
KeyDescrips[0x3E] = temp;

temp = new char[20];
strcpy(temp, "F5");
KeyDescrips[0x3F] = temp;

temp = new char[20];
strcpy(temp, "F6");
KeyDescrips[0x40] = temp;

temp = new char[20];
strcpy(temp, "F7");
KeyDescrips[0x41] = temp;

temp = new char[20];
strcpy(temp, "F8");
KeyDescrips[0x42] = temp;

temp = new char[20];
strcpy(temp, "F9");
KeyDescrips[0x43] = temp;

temp = new char[20];
strcpy(temp, "g");
KeyDescrips[0x22] = temp;

temp = new char[20];
strcpy(temp, "Grave");
KeyDescrips[0x29] = temp;

temp = new char[20];
strcpy(temp, "h");
KeyDescrips[0x23] = temp;

temp = new char[20];
strcpy(temp, "Home");
KeyDescrips[0xC7] = temp;

temp = new char[20];
strcpy(temp, "I");
KeyDescrips[0x17] = temp;

temp = new char[20];
strcpy(temp, "Insert");
KeyDescrips[0xD2] = temp;

temp = new char[20];
strcpy(temp, "j");
KeyDescrips[0x24] = temp;

temp = new char[20];
strcpy(temp, "k");
KeyDescrips[0x25] = temp;

temp = new char[20];
strcpy(temp, "Kana");
KeyDescrips[0x70] = temp;

temp = new char[20];
strcpy(temp, "Kanji");
KeyDescrips[0x94] = temp;

temp = new char[20];
strcpy(temp, "l");
KeyDescrips[0x26] = temp;

temp = new char[20];
strcpy(temp, "");
KeyDescrips[0x38] = temp;

temp = new char[20];
strcpy(temp, "[");
KeyDescrips[0x1A] = temp;

temp = new char[20];
strcpy(temp, "");
KeyDescrips[0x1D] = temp;

temp = new char[20];
strcpy(temp, "Lt Arrow");
KeyDescrips[0xCB] = temp;

temp = new char[20];
strcpy(temp, "");
KeyDescrips[0x2A] = temp;

temp = new char[20];
strcpy(temp, "Lt Win");
KeyDescrips[0xDB] = temp;

temp = new char[20];
strcpy(temp, "m");
KeyDescrips[0x32] = temp;

temp = new char[20];
strcpy(temp, "-");
KeyDescrips[0x0C] = temp;

temp = new char[20];
strcpy(temp, "n");
KeyDescrips[0x31] = temp;

temp = new char[20];
strcpy(temp, "Jp No Convert");
KeyDescrips[0x7B] = temp;

temp = new char[20];
strcpy(temp, "Numlock");
KeyDescrips[0x45] = temp;

temp = new char[20];
strcpy(temp, "Num 0");
KeyDescrips[0x52] = temp;

temp = new char[20];
strcpy(temp, "Num 1");
KeyDescrips[0x4F] = temp;

temp = new char[20];
strcpy(temp, "Num 2");
KeyDescrips[0x50] = temp;

temp = new char[20];
strcpy(temp, "Num 3");
KeyDescrips[0x51] = temp;

temp = new char[20];
strcpy(temp, "Num 4");
KeyDescrips[0x4B] = temp;

temp = new char[20];
strcpy(temp, "Num 5");
KeyDescrips[0x4C] = temp;

temp = new char[20];
strcpy(temp, "Num 6");
KeyDescrips[0x4D] = temp;

temp = new char[20];
strcpy(temp, "Num 7");
KeyDescrips[0x47] = temp;

temp = new char[20];
strcpy(temp, "Num 8");
KeyDescrips[0x48] = temp;

temp = new char[20];
strcpy(temp, "Num 9");
KeyDescrips[0x49] = temp;

temp = new char[20];
strcpy(temp, "Num ,");
KeyDescrips[0xB3] = temp;

temp = new char[20];
strcpy(temp, "Num Enter");
KeyDescrips[0x9C] = temp;

temp = new char[20];
strcpy(temp, "Num =");
KeyDescrips[0x8D] = temp;

temp = new char[20];
strcpy(temp, "Num -");
KeyDescrips[0x4A] = temp;

temp = new char[20];
strcpy(temp, "Num .");
KeyDescrips[0x53] = temp;

temp = new char[20];
strcpy(temp, "Num +");
KeyDescrips[0x4E] = temp;

temp = new char[20];
strcpy(temp, "Num /");
KeyDescrips[0xB5] = temp;

temp = new char[20];
strcpy(temp, "Num *");
KeyDescrips[0x37] = temp;

temp = new char[20];
strcpy(temp, "o");
KeyDescrips[0x18] = temp;

temp = new char[20];
strcpy(temp, "p");
KeyDescrips[0x19] = temp;

temp = new char[20];
strcpy(temp, ".");
KeyDescrips[0x34] = temp;

temp = new char[20];
strcpy(temp, "PgDn");
KeyDescrips[0xD1] = temp;

temp = new char[20];
strcpy(temp, "PgUp");
KeyDescrips[0xC9] = temp;

temp = new char[20];
strcpy(temp, "q");
KeyDescrips[0x10] = temp;

temp = new char[20];
strcpy(temp, "r");
KeyDescrips[0x13] = temp;

temp = new char[20];
strcpy(temp, "");
KeyDescrips[0xB8] = temp;

temp = new char[20];
strcpy(temp, "]");
KeyDescrips[0x1B] = temp;

temp = new char[20];
strcpy(temp, "");
KeyDescrips[0x9D] = temp;

temp = new char[20];
strcpy(temp, "Enter");
KeyDescrips[0x1C] = temp;

temp = new char[20];
strcpy(temp, "Rt Arrow");
KeyDescrips[0xCD] = temp;

temp = new char[20];
strcpy(temp, "");
KeyDescrips[0x36] = temp;

temp = new char[20];
strcpy(temp, "Rt Win");
KeyDescrips[0xDC] = temp;

temp = new char[20];
strcpy(temp, "s");
KeyDescrips[0x1F] = temp;

temp = new char[20];
strcpy(temp, "Scroll Lk");
KeyDescrips[0x46] = temp;

temp = new char[20];
strcpy(temp, ";");
KeyDescrips[0x27] = temp;

temp = new char[20];
strcpy(temp, "/");
KeyDescrips[0x35] = temp;

temp = new char[20];
strcpy(temp, "Space");
KeyDescrips[0x39] = temp;

temp = new char[20];
strcpy(temp, "Stop");
KeyDescrips[0x95] = temp;

temp = new char[20];
strcpy(temp, "Sys Req");
KeyDescrips[0xB7] = temp;

temp = new char[20];
strcpy(temp, "t");
KeyDescrips[0x14] = temp;

temp = new char[20];
strcpy(temp, "Tab");
KeyDescrips[0x0F] = temp;

temp = new char[20];
strcpy(temp, "u");
KeyDescrips[0x16] = temp;

temp = new char[20];
strcpy(temp, "Underline");
KeyDescrips[0x93] = temp;

temp = new char[20];
strcpy(temp, "Unlabeled");
KeyDescrips[0x97] = temp;

temp = new char[20];
strcpy(temp, "Up Arrow");
KeyDescrips[0xC8] = temp;

temp = new char[20];
strcpy(temp, "v");
KeyDescrips[0x2F] = temp;

temp = new char[20];
strcpy(temp, "w");
KeyDescrips[0x11] = temp;

temp = new char[20];
strcpy(temp, "x");
KeyDescrips[0x2D] = temp;

temp = new char[20];
strcpy(temp, "y");
KeyDescrips[0x15] = temp;

temp = new char[20];
strcpy(temp, "Yen");
KeyDescrips[0x7D] = temp;

temp = new char[20];
strcpy(temp, "z");
KeyDescrips[0x2C] = temp;

temp = new char[20];
strcpy(temp, "None");
KeyDescrips[0] = temp;

}
#endif

void CleanupKeys(void)
{
    for(int i =0;i<256;i++)
    {
        if(KeyDescrips[i])
        {
            delete KeyDescrips[i];
            KeyDescrips[i] = NULL;
        }
    }
    delete [] KeyDescrips;
    KeyDescrips = NULL;
}
