#include <windows.h>
#include <stdio.h>
#include "uihash.h"

LRESULT CALLBACK WinProc (HWND, UINT,WPARAM,LPARAM);

char *DikTable[256]=
{
	"0",
	"DIK_ESCAPE",
	"DIK_1",
	"DIK_2",
	"DIK_3",
	"DIK_4",
	"DIK_5",
	"DIK_6",
	"DIK_7",
	"DIK_8",
	"DIK_9",
	"DIK_0",
	"DIK_MINUS",
	"DIK_EQUALS",
	"DIK_BACK",
	"DIK_TAB",
	"DIK_Q",
	"DIK_W",
	"DIK_E",
	"DIK_R",
	"DIK_T",
	"DIK_Y",
	"DIK_U",
	"DIK_I",
	"DIK_O",
	"DIK_P",
	"DIK_LBRACKET",
	"DIK_RBRACKET",
	"DIK_RETURN",
	"DIK_LCONTROL",
	"DIK_A",
	"DIK_S",
	"DIK_D",
	"DIK_F",
	"DIK_G",
	"DIK_H",
	"DIK_J",
	"DIK_K",
	"DIK_L",
	"DIK_SEMICOLON",
	"DIK_APOSTROPHE",
	"DIK_GRAVE",
	"DIK_LSHIFT",
	"DIK_BACKSLASH",
	"DIK_Z",
	"DIK_X",
	"DIK_C",
	"DIK_V",
	"DIK_B",
	"DIK_N",
	"DIK_M",
	"DIK_COMMA",
	"DIK_PERIOD",
	"DIK_SLASH",
	"DIK_RSHIFT",
	"DIK_MULTIPLY",
	"DIK_LMENU",
	"DIK_SPACE",
	"DIK_CAPITAL",
	"DIK_F1",
	"DIK_F2",
	"DIK_F3",
	"DIK_F4",
	"DIK_F5",
	"DIK_F6",
	"DIK_F7",
	"DIK_F8",
	"DIK_F9",
	"DIK_F10",
	"DIK_NUMLOCK",
	"DIK_SCROLL",
	"DIK_NUMPAD7",
	"DIK_NUMPAD8",
	"DIK_NUMPAD9",
	"DIK_SUBTRACT",
	"DIK_NUMPAD4",
	"DIK_NUMPAD5",
	"DIK_NUMPAD6",
	"DIK_ADD",
	"DIK_NUMPAD1",
	"DIK_NUMPAD2",
	"DIK_NUMPAD3",
	"DIK_NUMPAD0",
	"DIK_DECIMAL",
	"0",
	"0",
	"0",
	"DIK_F11",
	"DIK_F12",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"DIK_F13",
	"DIK_F14",
	"DIK_F15",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"DIK_KANA",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"DIK_CONVERT",
	"0",
	"DIK_NOCONVERT",
	"0",
	"DIK_YEN",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"DIK_NUMPADEQUALS",
	"0",
	"0",
	"DIK_CIRCUMFLEX",
	"DIK_AT",
	"DIK_COLON",
	"DIK_UNDERLINE",
	"DIK_KANJI",
	"DIK_STOP",
	"DIK_AX",
	"DIK_UNLABELED",
	"0",
	"0",
	"0",
	"0",
	"DIK_NUMPADENTER",
	"DIK_RCONTROL",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"DIK_NUMPADCOMMA",
	"0",
	"DIK_DIVIDE",
	"0",
	"DIK_SYSRQ",
	"DIK_RMENU",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"DIK_HOME",
	"DIK_UP",
	"DIK_PRIOR",
	"0",
	"DIK_LEFT",
	"0",
	"DIK_RIGHT",
	"0",
	"DIK_END",
	"DIK_DOWN",
	"DIK_NEXT",
	"DIK_INSERT",
	"DIK_DELETE",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"DIK_LWIN",
	"DIK_RWIN",
	"DIK_APPS",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
};

char *ShiftTable[]=
{
	"",//0
	"SHIFT",//1
	"CTRL",//2
	"SHIFT CTRL",//3
	"ALT",//4
	"SHIFT ALT",//5
	"CTRL ALT",//6
	"SHIFT CTRL ALT",//7
};

char*FlagTable[]=
{
	"",//0
	"ASCII",//1
	"ALPHA",//2
	"ASCII ALPHA",//3
	"DIGIT",//4
	"ASCII DIGIT",//5
	"ALPHA DIGIT",//6
	"ASCII ALPHA DIGIT",//7
};

enum
{
	_SHIFT_=1,
	_CTRL_=2,
	_ALT_=4,
	_ASCII_=1,
	_ALPHA_=2,
	_DIGIT_=4,
};

long ShiftStates=0;
long AsciiFlags=0;
long LastScanCode=0;
long LastAscii=0;
RECT myrect;

UI_Hash *NewKeys=NULL;

int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hPrev, PSTR szCmdLine, int iCmdShow)
{
	static char szAppName[] = "genascii";
	HWND hwnd;
	MSG msg;
	WNDCLASSEX wndclass;
	UI_HASHNODE *current;
	long curidx;
	char *rec;
	FILE *fp;

	wndclass.cbSize = sizeof(wndclass);
	wndclass.style  = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc=WinProc;
	wndclass.cbClsExtra=0;
	wndclass.cbWndExtra=0;
	wndclass.hInstance=hInst;
	wndclass.hIcon=LoadIcon (NULL,IDI_APPLICATION);
	wndclass.hCursor=LoadCursor(NULL,IDC_ARROW);
	wndclass.hbrBackground=(HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName=NULL;
	wndclass.lpszClassName=szAppName;
	wndclass.hIconSm=LoadIcon(NULL,IDI_APPLICATION);

	RegisterClassEx(&wndclass);

	hwnd=CreateWindow(szAppName,
					  "Ascii Conversion Program",
					  WS_OVERLAPPEDWINDOW,
					  CW_USEDEFAULT,
					  CW_USEDEFAULT,
					  CW_USEDEFAULT,
					  CW_USEDEFAULT,
					  NULL,
					  NULL,
					  hInst,
					  NULL);

	ShowWindow(hwnd,iCmdShow);
	UpdateWindow(hwnd);

	NewKeys=new UI_Hash;
	NewKeys->Setup(10);

	while(GetMessage(&msg,NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	rec=(char*)NewKeys->GetFirst(&current,&curidx);
	if(rec)
	{
		fp=fopen("keys.asc","w");
		if(fp)
		{
			while(rec)
			{
				fprintf(fp,"%s\n",rec);
				rec=(char*)NewKeys->GetNext(&current,&curidx);
			}
			fclose(fp);
		}
	}

	NewKeys->Cleanup();
	delete NewKeys;

	return(msg.lParam);
}

LRESULT CALLBACK WinProc (HWND hwnd, UINT message,WPARAM wParam,LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	LRESULT retval=1;
	long ID;
	char buffer[200];
	char *rec;

	switch(message)
	{
		case WM_CREATE:
			return(0);
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			LastScanCode=((lParam >> 16) & 0xff) | ((lParam >> 17) & 0x80);
			return(0);
		case WM_CHAR:
			ShiftStates=0;
			if(GetKeyState(VK_SHIFT) & 0x80)
				ShiftStates |= _SHIFT_;
			if(GetKeyState(VK_MENU) & 0x80)
				ShiftStates |= _ALT_;
			if(GetKeyState(VK_CONTROL) & 0x80)
				ShiftStates |= _CTRL_;

			LastAscii=wParam;
			AsciiFlags=0;
			if(isdigit(LastAscii))
				AsciiFlags |= _DIGIT_;
			if(isalpha(LastAscii))
				AsciiFlags |= _ALPHA_;

			if(LastAscii > 31)
			{
				AsciiFlags |= _ASCII_;
				sprintf(buffer,"%1ld   %1ld     %s       %s",LastScanCode,LastAscii,ShiftTable[ShiftStates],FlagTable[AsciiFlags]);
				ID=(LastScanCode << 8) | ShiftStates;
				if(!NewKeys->Find(ID))
				{
					rec=new char[strlen(buffer)+1];
					strcpy(rec,buffer);
					NewKeys->Add(ID,rec);
				}
			
				InvalidateRect(hwnd,&myrect,TRUE);
			}
			return(0);

		case WM_PAINT:
			hdc=BeginPaint(hwnd,&ps);
			GetClientRect(hwnd,&rect);

			myrect=rect;

			DrawText(hdc, "Press All the keys for the language which need to be remapped", -1, &myrect, DT_SINGLELINE);
			myrect.top += 40;
			if(LastAscii)
			{
				sprintf(buffer,"%s       %1c (%1ld)         %s       %s",DikTable[LastScanCode],LastAscii,LastAscii,ShiftTable[ShiftStates],FlagTable[AsciiFlags]);
				DrawText(hdc, buffer, -1, &myrect, DT_SINGLELINE);
			}
  
			EndPaint(hwnd,&ps);
			return(0);

		case WM_DESTROY:
			PostQuitMessage(0);
			return(0);
	}
	return(DefWindowProc(hwnd,message,wParam,lParam));
}
