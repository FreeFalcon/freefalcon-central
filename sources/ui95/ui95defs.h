#ifndef _UI95_DEFS_H_
#define _UI95_DEFS_H_
// Bit Settings for the flags field
// build to suit :) BITWISE set the high byte of the long
enum // DONT use the highest bit (0x80000000) because this is a signed value
{
	// 1st 4 are Button states (AND MUST be 0,1,2,3)
	C_DONT_CARE=-2,
	C_STATE_0=0,
	C_STATE_1,
	C_STATE_2,
	C_STATE_3,
	C_STATE_4,
	C_STATE_5,
	C_STATE_6,
	C_STATE_7,
	C_STATE_8,
	C_STATE_9,
	C_STATE_10,
	C_STATE_11,
	C_STATE_12,
	C_STATE_13,
	C_STATE_14,
	C_STATE_15,
	C_STATE_16,
	C_STATE_17,
	C_STATE_18,
	C_STATE_19,
	C_STATE_20,
	C_STATE_DISABLED,
	C_STATE_SELECTED,
	C_STATE_MOUSE,
	C_TYPE_NOTHING,
	C_TYPE_NORMAL,
	C_TYPE_TOGGLE,
	C_TYPE_RADIO,
	C_TYPE_SELECT,
	C_TYPE_CUSTOM,
	C_TYPE_SIZEX,
	C_TYPE_SIZEY,
	C_TYPE_SIZEXY,
	C_TYPE_SIZEW,
	C_TYPE_SIZEH,
	C_TYPE_SIZEWH,
	C_TYPE_DRAGX,
	C_TYPE_DRAGY,
	C_TYPE_DRAGXY,
	C_TYPE_TEXT,
	C_TYPE_PASSWORD,
	C_TYPE_INTEGER,
	C_TYPE_FLOAT,
	C_TYPE_FILENAME,
	C_TYPE_LEFT,
	C_TYPE_CENTER,
	C_TYPE_RIGHT,
	C_TYPE_ROOT,
	C_TYPE_INFO,
	C_TYPE_MENU,
	C_TYPE_ITEM,
	C_TYPE_LMOUSEDOWN,
	C_TYPE_LMOUSEUP,
	C_TYPE_LMOUSEDBLCLK,
	C_TYPE_RMOUSEDOWN,
	C_TYPE_RMOUSEUP,
	C_TYPE_RMOUSEDBLCLK,
	C_TYPE_MOUSEOVER,
	C_TYPE_MOUSEREPEAT,
	C_TYPE_MOUSEWHEEL,
	C_TYPE_EXCLUSIVE,
	C_TYPE_MOUSEMOVE,
	C_TYPE_VERTICAL,
	C_TYPE_HORIZONTAL,
	C_TYPE_LOOP,
	C_TYPE_STOPATEND,
	C_TYPE_PINGPONG,
	C_TYPE_TIMER,
	C_TYPE_TRANSLUCENT,
	C_TYPE_REMOVE,
	C_TYPE_REPEAT,
	C_TYPE_WINDOW,
	C_TYPE_CONTROL,
	C_TYPE_LDROP,
	C_TYPE_IPADDRESS,
	_CNTL_ANIMATION_		= 0x4001,
	_CNTL_BITMAP_,
	_CNTL_BUTTON_,
	_CNTL_EDITBOX_,
	_CNTL_MAPICON_,
	_CNTL_LISTBOX_,
	_CNTL_LAYOUT_,
	_CNTL_POPUPLIST_,
	_CNTL_SCROLLBAR_,
	_CNTL_SLIDER_,
	_CNTL_PANNER_,
	_CNTL_TEXT_,
	_CNTL_TREELIST_,
	_CNTL_MARQUE_,
	_CNTL_LINE_,
	_CNTL_BOX_,
	_CNTL_CURSOR_,
	_CNTL_ACMI_,
	_CNTL_TIMERHOOK_,
	_CNTL_FLIGHT_,
	_CNTL_MAP_MOVER_,
	_CNTL_WAYPOINT_,
	_CNTL_SCALEBITMAP_,
	_CNTL_FILL_,
	_CNTL_PILOT_,
	_CNTL_MISSION_,
	_CNTL_BLIP_,
	_CNTL_SQUAD_,
	_CNTL_ENTITY_,
	_CNTL_CLOCK_,
	_CNTL_TILE_,
	_CNTL_DRAWLIST_,
	_CNTL_PACKAGE_,
	_CNTL_CUSTOM_,
	_CNTL_HELP_,
	_CNTL_DOG_FLIGHT_,
	_CNTL_BULLSEYE_,
	_CNTL_VICTORY_,
};

enum // Bit table
{
	C_BIT_NOTHING           = 0x00000000, // Place holder... don't do jack
	C_BIT_FIXEDSIZE			= 0x00000001, // Used EXCLUSIVELY for marking that the text has a fixed with buffer
	C_BIT_LEADINGZEROS		= 0x00000002, // Used in cclock & ceditbox to put leading zeros in from of numeric values
	C_BIT_VERTICAL			= 0x00000004, // Used in ctile for tiling vertically
	C_BIT_HORIZONTAL		= 0x00000008, // Used in ctile for tiling horizontally
	C_BIT_USEOUTLINE		= 0x00000010, // Used to draw an outline on the control's x,y,w,h corners
	C_BIT_LEFT				= 0x00000020, // Used in cbutton to put labels at specific locations relative to the button
	C_BIT_RIGHT				= 0x00000040, // Used in cbutton to put labels at specific locations relative to the button
	C_BIT_TOP				= 0x00000080, // Used in cbutton to put labels at specific locations relative to the button
	C_BIT_BOTTOM			= 0x00000100, // Used in cbutton to put labels at specific locations relative to the button
	C_BIT_HCENTER			= 0x00000200, // used for horizontal centering
	C_BIT_VCENTER			= 0x00000400, // used for vertical centering
	C_BIT_ENABLED			= 0x00000800, // used for enabling/disabling controls
	C_BIT_DRAGABLE			= 0x00001000, // used to set dragging capabilities
	C_BIT_INVISIBLE			= 0x00002000, // used to make a control visible/invisible
	C_BIT_FORCEMOUSEOVER	= 0x00004000, // used to force control to draw as if the mouse was over the control
	C_BIT_USEBGIMAGE		= 0x00008000, // Used to draw a background image 1st before drawing rest of control
	C_BIT_TIMER				= 0x00010000, // flag used to designate a timer control
	C_BIT_CLOSEWINDOW		= 0x00020000, // used in clistbox to Close the Parent Window when done
	C_BIT_ABSOLUTE			= 0x00040000, // flag used to indicate NOT to use a client area
	C_BIT_SELECTABLE		= 0x00080000, // flag used for TAB/SHIFT TAB to jump between controls with keyboard
	C_BIT_OPAQUE			= 0x00100000, // flag used to make text draw opaque... uses BgColor for filling behind char (can't set this for editboxes from script, since it is used to mark selected text)
	C_BIT_CANTMOVE			= 0x00200000, // flag used by chandler to determine if a window can have it's depth changed
	C_BIT_USELINE			= 0x00400000, // flag used for text... when set, draws an underline under the text
	C_BIT_PASSWORD			= 0x00800000, // flag used to draw '*' instead of actual text
	C_BIT_NOLABEL			= 0x01000000, // flag used to tell button NOT to draw it's label
	C_BIT_WORDWRAP			= 0x02000000, // flag used in text to Word wrap
	C_BIT_REMOVE			= 0x04000000, // flag used to tell cleanup code to cleanup (or NOT cleanup) a control
	C_BIT_NOCLEANUP			= 0x08000000, // flag used by cwindow to tell it NOT to cleanup ANY of it's controls
	C_BIT_MOUSEOVER			= 0x10000000, // flag used to disable MOUSEOVER hi-liting
	C_BIT_TRANSLUCENT		= 0x20000000, // flag used to draw translucent bitmaps & fills, or mark a window as translucent so it refreshes windows behind itself also
	C_BIT_USEBGFILL			= 0x40000000, // Similar to USEBGIMAGE, fills the x,y,w,h with the bgcolor
	C_BIT_USEGRADIENT		= 0x80000000, // used to do a gradient fill in a window... not currently used in scripts
};

enum
{
	C_DRAW_NOTHING			= 0x00000000, // do nothing
	C_DRAW_REFRESH			= 0x00000001, // Draw this item
	C_DRAW_REFRESHALL		= 0x00000002, // Draw ALL
	C_DRAW_COPYWINDOW		= 0x10000000, // Copy Window contents to front (OFFSCREEN) surface
	C_DRAW_COPYTOFRONT		= 0x20000000, // Copy Front (OFFSCREEN) to Primary (SCREEN)
	C_DRAW_UPDATE			= 0x40000000, // Flag used when the PostUpdate function is called
};

enum // WM_USER Messages for the UI
{
	C_WM_DRAWWINDOW			=WM_USER+5000,
	C_WM_UPDATE,
	C_WM_COPYTOFRONT,
	C_WM_TIMER,
};

#define _UI95_VU_ID_SLOT_       (6)
#define _UI95_DELGROUP_SLOT_    (7)
#define _UI95_DELGROUP_ID_      (5551212)
#define _UI95_TIMER_DELAY_      (4)
#define _UI95_TIMER_COUNTER_    (5)
#define _UI95_TICKS_PER_SECOND_ (10)

#define FT_PER_NMILE NM_TO_FT
#define FT_PER_MILE  (5280) // THIS is for you guys :-)

#ifndef uchar
typedef unsigned char uchar;
#endif

typedef struct
{
	long left,top,right,bottom;
} UI95_RECT;

enum
{
	UI_GENERAL_POOL=0,
	UI_CONTROL_POOL,
	UI_ART_POOL,
	UI_SOUND_POOL,
	UI_MAX_POOLS,
};

#endif
