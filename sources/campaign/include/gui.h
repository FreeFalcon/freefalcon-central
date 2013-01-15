#ifndef GUI
#define GUI

// =====================
// GUI Display Functions
// =====================

   #define Black         0
   #define Blue          1
   #define Green         2
   #define Cyan          3
   #define Red           4
   #define Magenta       5
   #define Brown         6
   #define Gray          7
   #define DarkGray      8
   #define LightBlue     9
   #define LightGreen   10
   #define LightCyan    11
   #define LightRed     12
   #define LightMagenta 13
   #define Yellow       14
   #define White        15

// --------------------------------------
// Window data structure
// --------------------------------------

   struct WindowStructure {
                     char*             Buffer;
                     int               Size;
                     int               ULX,ULY;       // Upper left coordinates
                     int               LRX,LRY;       // Lower right coordinates
                     int               FColor;        // Foreground color
                     int               BColor;
                     };
   typedef struct WindowStructure WindowStruct;
   typedef WindowStruct* Window;

   extern Window Screen;

// -----------------
// User Descriptions
// -----------------

   extern void LoadNewPalette (char * FileName);

   extern void PutData(short ULX, short ULY, int data, int clr);

   extern int GetData(short ULX, short ULY, int clr);

   extern char* GetText(char* buf, short ULX, short ULY, int clr);

   extern Window GUI_CreateWindow (int ULX, int ULY, int LRX, int LRY, int FColor, int BColor);

   extern void DisposeWindow (Window w);

   extern void _wmoveto (Window w, int x, int y);

   extern void _wprintf (Window w, char *string, ... );

   extern void _wgprintf (Window w, int x, int y, char *string, ... );

   extern void _wgprintfbox (Window w, int x, int y, int color, char *string, ... );

   extern void _wgprintfcolor (Window w, int x, int y, int FColor, int BColor, char *string, ... );

   extern void _wputchar (Window w, int x, int y, int Fcol, int Bcol, char c);

   extern char* _wggetstr (Window w, int x, int y, char *buf, int mlen);


// ====================================
// Mouse Data Structure
// ====================================

   struct callback_data
      {
      int right_button;
      int mouse_event;
      unsigned short mouse_code;
      unsigned short mouse_button;
      unsigned short mouse_x;
      unsigned short mouse_y;
      signed short mouse_si;
      signed short mouse_di;
      };

   extern struct callback_data Mouse_Data;

// ------------------------------------
// Functions
// ------------------------------------

   extern int lock_region (void *address, unsigned length);

   extern int InitMouse (void);

   extern void ShowMouse();

   extern void HideMouse();
   
   extern void PositionMouse(int x, int y);
   
   extern void UnInitMouse();


// ================
// Keyboard Routine
// ================

// -----------------
// User Descriptions
// -----------------

// ***** PENDING *****

// ---------------------------------------
// Type and External Function Declarations
// ---------------------------------------

   #define KEYESCAPE            0x1
   #define KEY1                 0x2
   #define KEY2                 0x3
   #define KEY3                 0x4
   #define KEY4                 0x5
   #define KEY5                 0x6
   #define KEY6                 0x7
   #define KEY7                 0x8
   #define KEY8                 0x9
   #define KEY9                 0x0a
   #define KEY0                 0x0b
   #define KEYMINUS             0x0c
   #define KEYPLUS              0x0d
   #define KEYBACK              0x0e
   #define KEYTAB               0x0f
   #define KEYQ                 0x10
   #define KEYW                 0x11
   #define KEYE                 0x12
   #define KEYR                 0x13
   #define KEYT                 0x14
   #define KEYY                 0x15
   #define KEYU                 0x16
   #define KEYI                 0x17
   #define KEYO                 0x18
   #define KEYP                 0x19
   #define KEYLEFTBRACKET       0x1a
   #define KEYRIGHTBRACKET      0x1b
   #define KEYENTER             0x1c
   #define KEYCONTROL           0x1d
   #define KEYA                 0x1e
   #define KEYS                 0x1f
   #define KEYD                 0x20
   #define KEYF                 0x21
   #define KEYG                 0x22
   #define KEYH                 0x23
   #define KEYJ                 0x24
   #define KEYK                 0x25
   #define KEYL                 0x26
   #define KEYCOLON             0x27
   #define KEYQUOTE             0x28
   #define KEYTILDA             0x29
   #define KEYLEFTSHIFT         0x2a
   #define KEYBACKSLASH         0x2b
   #define KEYZ                 0x2c
   #define KEYX                 0x2d
   #define KEYC                 0x2e
   #define KEYV                 0x2f
   #define KEYB                 0x30
   #define KEYN                 0x31
   #define KEYM                 0x32
   #define KEYCOMMA             0x33
   #define KEYDOT               0x34
   #define KEYSLASH             0x35
   #define KEYRIGHTSHIFT        0x36
   #define KEYSTAR              0x37
   #define KEYALT               0x38
   #define KEYSPACE             0x39
   #define KEYCAPSLOCK          0x3a
   #define KEYF1                0x3b
   #define KEYF2                0x3c
   #define KEYF3                0x3d
   #define KEYF4                0x3e
   #define KEYF5                0x3f
   #define KEYF6                0x40
   #define KEYF7                0x41
   #define KEYF8                0x42
   #define KEYF9                0x43
   #define KEYF10               0x44
   #define KEYNUMLOCK           0x45
   #define KEYSCROLLLOCK        0x46
   #define KEYHOME              0x47
   #define KEYUP                0x48
   #define KEYPAGEUP            0x49
   #define KEYGREYMINUS         0x4a
   #define KEYLEFT              0x4b
   #define KEYCENTER            0x4c
   #define KEYRIGHT             0x4d
   #define KEYGREYPLUS          0x4e
   #define KEYEND               0x4f
   #define KEYDOWN              0x50
   #define KEYPAGEDOWN          0x51
   #define KEYINSERT            0x52
   #define KEYDELETE            0x53
   #define STATUSKEY_RIGHTSHIFT 0x01
   #define STATUSKEY_LEFTSHIFT  0x02
   #define STATUSKEY_CTRL       0x04
   #define STATUSKEY_ALT        0x08
   #define STATUSKEY_SCROLLLOCK 0x10
   #define STATUSKEY_NUMLOCK    0x20
   #define STATUSKEY_CAPSLOCK   0x40
   #define STATUSKEY_INSERT     0x80

   extern signed short CheckKeyPressed (void);

#endif
































   
