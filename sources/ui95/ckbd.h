#ifndef _C_KEYBOARD_VALUES_H_
#define _C_KEYBOARD_VALUES_H_

#define _MAX_KEYS_		256
#define _MAX_STATES_	2

#define _SHIFT_DOWN_	(0x01)
#define _CTRL_DOWN_		(0x02)
#define _ALT_DOWN_		(0x04)

// Ascii conversion table from Direct Input Keys (DIK/Scan Code) equivalents
// SHIFT,CTRL,ALT are treated as "states" and NOT individual keys
// CTRL/ALT are treated as "unprintable".... therefore not in table
//
unsigned char _Keys_[_MAX_KEYS_][_MAX_STATES_]=
{
	{ 0,0 }, //                     0x00
	{ 0,0 }, // DIK_ESCAPE          0x01
	{ '1','!' }, // DIK_1           0x02
	{ '2','@' }, // DIK_2           0x03
	{ '3','#' }, // DIK_3           0x04
	{ '4','$' }, // DIK_4           0x05
	{ '5','%' }, // DIK_5           0x06
	{ '6','^' }, // DIK_6           0x07
	{ '7','&' }, // DIK_7           0x08
	{ '8','*' }, // DIK_8           0x09
	{ '9','(' }, // DIK_9           0x0A
	{ '0',')' }, // DIK_0           0x0B
	{ '-','_' }, // DIK_MINUS       0x0C    /* - on main keyboard */
	{ '=','+' }, // DIK_EQUALS      0x0D
	{ 0,0 }, // DIK_BACK            0x0E    /* backspace */
	{ 0,0 }, // DIK_TAB             0x0F
	{ 'q','Q' }, // DIK_Q           0x10
	{ 'w','W' }, // DIK_W           0x11
	{ 'e','E' }, // DIK_E           0x12
	{ 'r','R' }, // DIK_R           0x13
	{ 't','T' }, // DIK_T           0x14
	{ 'y','Y' }, // DIK_Y           0x15
	{ 'u','U' }, // DIK_U           0x16
	{ 'i','I' }, // DIK_I           0x17
	{ 'o','O' }, // DIK_O           0x18
	{ 'p','P' }, // DIK_P           0x19
	{ '[','{' }, // DIK_LBRACKET    0x1A
	{ ']','}' }, // DIK_RBRACKET    0x1B
	{ 0,0 }, // DIK_RETURN          0x1C    /* Enter on main keyboard */
	{ 0,0 }, // DIK_LCONTROL        0x1D
	{ 'a','A' }, // DIK_A           0x1E
	{ 's','S' }, // DIK_S           0x1F
	{ 'd','D' }, // DIK_D           0x20
	{ 'f','F' }, // DIK_F           0x21
	{ 'g','G' }, // DIK_G           0x22
	{ 'h','H' }, // DIK_H           0x23
	{ 'j','J' }, // DIK_J           0x24
	{ 'k','K' }, // DIK_K           0x25
	{ 'l','L' }, // DIK_L           0x26
	{ ';',':' }, // DIK_SEMICOLON   0x27
	{ '\'','"' }, // DIK_APOSTROPHE 0x28
	{ '`','~' }, // DIK_GRAVE       0x29    /* accent grave */
	{ 0,0 }, // DIK_LSHIFT          0x2A
	{ '\\','|' }, // DIK_BACKSLASH  0x2B
	{ 'z','Z' }, // DIK_Z           0x2C
	{ 'x','X' }, // DIK_X           0x2D
	{ 'c','C' }, // DIK_C           0x2E
	{ 'v','V' }, // DIK_V           0x2F
	{ 'b','B' }, // DIK_B           0x30
	{ 'n','N' }, // DIK_N           0x31
	{ 'm','M' }, // DIK_M           0x32
	{ ',','<' }, // DIK_COMMA       0x33
	{ '.','>' }, // DIK_PERIOD      0x34    /* . on main keyboard */
	{ '/','?' }, // DIK_SLASH       0x35    /* / on main keyboard */
	{ 0,0 }, // DIK_RSHIFT          0x36
	{ '*',0 }, // DIK_MULTIPLY      0x37    /* * on numeric keypad */
	{ 0,0 }, // DIK_LMENU           0x38    /* left Alt */
	{ ' ',0 }, // DIK_SPACE         0x39
	{ 0,0 }, // DIK_CAPITAL         0x3A
	{ 0,0 }, // DIK_F1              0x3B
	{ 0,0 }, // DIK_F2              0x3C
	{ 0,0 }, // DIK_F3              0x3D
	{ 0,0 }, // DIK_F4              0x3E
	{ 0,0 }, // DIK_F5              0x3F
	{ 0,0 }, // DIK_F6              0x40
	{ 0,0 }, // DIK_F7              0x41
	{ 0,0 }, // DIK_F8              0x42
	{ 0,0 }, // DIK_F9              0x43
	{ 0,0 }, // DIK_F10             0x44
	{ 0,0 }, // DIK_NUMLOCK         0x45
	{ 0,0 }, // DIK_SCROLL          0x46    /* Scroll Lock */
	{ 0,'7' }, // DIK_NUMPAD7       0x47
	{ 0,'8' }, // DIK_NUMPAD8       0x48
	{ 0,'9' }, // DIK_NUMPAD9       0x49
	{ '-',0 }, // DIK_SUBTRACT      0x4A    /* - on numeric keypad */
	{ 0,'4' }, // DIK_NUMPAD4       0x4B
	{ 0,'5' }, // DIK_NUMPAD5       0x4C
	{ 0,'6' }, // DIK_NUMPAD6       0x4D
	{ '+',0 }, // DIK_ADD           0x4E    /* + on numeric keypad */
	{ 0,'1' }, // DIK_NUMPAD1       0x4F
	{ 0,'2' }, // DIK_NUMPAD2       0x50
	{ 0,'3' }, // DIK_NUMPAD3       0x51
	{ 0,'0' }, // DIK_NUMPAD0       0x52
	{ 0,'.' }, // DIK_DECIMAL       0x53    /* . on numeric keypad */
	{ 0,0 }, //                     0x54
	{ 0,0 }, //                     0x55
	{ 0,0 }, //                     0x56
	{ 0,0 }, // DIK_F11             0x57
	{ 0,0 }, // DIK_F12             0x58
	{ 0,0 }, //                     0x59
	{ 0,0 }, //                     0x5a
	{ 0,0 }, //                     0x5b
	{ 0,0 }, //                     0x5c
	{ 0,0 }, //                     0x5d
	{ 0,0 }, //                     0x5e
	{ 0,0 }, //                     0x5f
	{ 0,0 }, //                     0x60
	{ 0,0 }, //                     0x61
	{ 0,0 }, //                     0x62
	{ 0,0 }, //                     0x63
	{ 0,0 }, // DIK_F13             0x64    /*                     (NEC PC98) */
	{ 0,0 }, // DIK_F14             0x65    /*                     (NEC PC98) */
	{ 0,0 }, // DIK_F15             0x66    /*                     (NEC PC98) */
	{ 0,0 }, //                     0x67
	{ 0,0 }, //                     0x68
	{ 0,0 }, //                     0x69
	{ 0,0 }, //                     0x6a
	{ 0,0 }, //                     0x6b
	{ 0,0 }, //                     0x6c
	{ 0,0 }, //                     0x6d
	{ 0,0 }, //                     0x6e
	{ 0,0 }, //                     0x6f
	{ 0,0 }, // DIK_KANA            0x70    /* (Japanese keyboard)            */
	{ 0,0 }, //                     0x71
	{ 0,0 }, //                     0x72
	{ 0,0 }, //                     0x73
	{ 0,0 }, //                     0x74
	{ 0,0 }, //                     0x75
	{ 0,0 }, //                     0x76
	{ 0,0 }, //                     0x77
	{ 0,0 }, //                     0x78
	{ 0,0 }, // DIK_CONVERT         0x79    /* (Japanese keyboard)            */
	{ 0,0 }, //                     0x7a
	{ 0,0 }, // DIK_NOCONVERT       0x7B    /* (Japanese keyboard)            */
	{ 0,0 }, //                     0x7c
	{ 0,0 }, // DIK_YEN             0x7D    /* (Japanese keyboard)            */
	{ 0,0 }, //                     0x7e
	{ 0,0 }, //                     0x7f
	{ 0,0 }, //                     0x80
	{ 0,0 }, //                     0x81
	{ 0,0 }, //                     0x82
	{ 0,0 }, //                     0x83
	{ 0,0 }, //                     0x84
	{ 0,0 }, //                     0x85
	{ 0,0 }, //                     0x86
	{ 0,0 }, //                     0x87
	{ 0,0 }, //                     0x88
	{ 0,0 }, //                     0x89
	{ 0,0 }, //                     0x8a
	{ 0,0 }, //                     0x8b
	{ 0,0 }, //                     0x8c
	{ '=',0 }, // DIK_NUMPADEQUALS0x8D    /* = on numeric keypad (NEC PC98) */
	{ 0,0 }, //                     0x8e
	{ 0,0 }, //                     0x8f
	{ 0,0 }, // DIK_CIRCUMFLEX      0x90    /* (Japanese keyboard)            */
	{ 0,0 }, // DIK_AT              0x91    /*                     (NEC PC98) */
	{ 0,0 }, // DIK_COLON           0x92    /*                     (NEC PC98) */
	{ 0,0 }, // DIK_UNDERLINE       0x93    /*                     (NEC PC98) */
	{ 0,0 }, // DIK_KANJI           0x94    /* (Japanese keyboard)            */
	{ 0,0 }, // DIK_STOP            0x95    /*                     (NEC PC98) */
	{ 0,0 }, // DIK_AX              0x96    /*                     (Japan AX) */
	{ 0,0 }, // DIK_UNLABELED       0x97    /*                        (J3100) */
	{ 0,0 }, //                     0x98
	{ 0,0 }, //                     0x99
	{ 0,0 }, //                     0x9a
	{ 0,0 }, //                     0x9b
	{ 0,0 }, // DIK_NUMPADENTER     0x9C    /* Enter on numeric keypad */
	{ 0,0 }, // DIK_RCONTROL        0x9D
	{ 0,0 }, //                     0x9e
	{ 0,0 }, //                     0x9f
	{ 0,0 }, //                     0xa0
	{ 0,0 }, //                     0xa1
	{ 0,0 }, //                     0xa2
	{ 0,0 }, //                     0xa3
	{ 0,0 }, //                     0xa4
	{ 0,0 }, //                     0xa5
	{ 0,0 }, //                     0xa6
	{ 0,0 }, //                     0xa7
	{ 0,0 }, //                     0xa8
	{ 0,0 }, //                     0xa9
	{ 0,0 }, //                     0xaa
	{ 0,0 }, //                     0xab
	{ 0,0 }, //                     0xac
	{ 0,0 }, //                     0xad
	{ 0,0 }, //                     0xae
	{ 0,0 }, //                     0xaf
	{ 0,0 }, //                     0xb0
	{ 0,0 }, //                     0xb1
	{ 0,0 }, //                     0xb2
	{ ',',',' }, // DIK_NUMPADCOMMA 0xB3    /* , on numeric keypad (NEC PC98) */
	{ 0,0 }, //                     0xb4
	{ '/','/' }, // DIK_DIVIDE      0xB5    /* / on numeric keypad */
	{ 0,0 }, //                     0xb6
	{ 0,0 }, // DIK_SYSRQ           0xB7
	{ 0,0 }, // DIK_RMENU           0xB8    /* right Alt */
	{ 0,0 }, //                     0xb9
	{ 0,0 }, //                     0xba
	{ 0,0 }, //                     0xbb
	{ 0,0 }, //                     0xbc
	{ 0,0 }, //                     0xbd
	{ 0,0 }, //                     0xbe
	{ 0,0 }, //                     0xbf
	{ 0,0 }, //                     0xc0
	{ 0,0 }, //                     0xc1
	{ 0,0 }, //                     0xc2
	{ 0,0 }, //                     0xc3
	{ 0,0 }, //                     0xc4
	{ 0,0 }, //                     0xc5
	{ 0,0 }, //                     0xc6
	{ 0,0 }, // DIK_HOME            0xC7    /* Home on arrow keypad */
	{ 0,0 }, // DIK_UP              0xC8    /* UpArrow on arrow keypad */
	{ 0,0 }, // DIK_PRIOR           0xC9    /* PgUp on arrow keypad */
	{ 0,0 }, //                     0xca
	{ 0,0 }, // DIK_LEFT            0xCB    /* LeftArrow on arrow keypad */
	{ 0,0 }, //                     0xcc
	{ 0,0 }, // DIK_RIGHT           0xCD    /* RightArrow on arrow keypad */
	{ 0,0 }, //                     0xce
	{ 0,0 }, // DIK_END             0xCF    /* End on arrow keypad */
	{ 0,0 }, // DIK_DOWN            0xD0    /* DownArrow on arrow keypad */
	{ 0,0 }, // DIK_NEXT            0xD1    /* PgDn on arrow keypad */
	{ 0,0 }, // DIK_INSERT          0xD2    /* Insert on arrow keypad */
	{ 0,0 }, // DIK_DELETE          0xD3    /* Delete on arrow keypad */
	{ 0,0 }, //                     0xd4
	{ 0,0 }, //                     0xd5
	{ 0,0 }, //                     0xd6
	{ 0,0 }, //                     0xd7
	{ 0,0 }, //                     0xd8
	{ 0,0 }, //                     0xd9
	{ 0,0 }, //                     0xda
	{ 0,0 }, // DIK_LWIN            0xDB    /* Left Windows key */
	{ 0,0 }, // DIK_RWIN            0xDC    /* Right Windows key */
	{ 0,0 }, // DIK_APPS            0xDD    /* AppMenu key */
	{ 0,0 }, //                     0xde
	{ 0,0 }, //                     0xdf
	{ 0,0 }, //                     0xe0
	{ 0,0 }, //                     0xe1
	{ 0,0 }, //                     0xe2
	{ 0,0 }, //                     0xe3
	{ 0,0 }, //                     0xe4
	{ 0,0 }, //                     0xe5
	{ 0,0 }, //                     0xe6
	{ 0,0 }, //                     0xe7
	{ 0,0 }, //                     0xe8
	{ 0,0 }, //                     0xe9
	{ 0,0 }, //                     0xea
	{ 0,0 }, //                     0xeb
	{ 0,0 }, //                     0xec
	{ 0,0 }, //                     0xed
	{ 0,0 }, //                     0xee
	{ 0,0 }, //                     0xef
	{ 0,0 }, //                     0xf0
	{ 0,0 }, //                     0xf1
	{ 0,0 }, //                     0xf2
	{ 0,0 }, //                     0xf3
	{ 0,0 }, //                     0xf4
	{ 0,0 }, //                     0xf5
	{ 0,0 }, //                     0xf6
	{ 0,0 }, //                     0xf7
	{ 0,0 }, //                     0xf8
	{ 0,0 }, //                     0xf9
	{ 0,0 }, //                     0xfa
	{ 0,0 }, //                     0xfb
	{ 0,0 }, //                     0xfc
	{ 0,0 }, //                     0xfd
	{ 0,0 }, //                     0xfe
	{ 0,0 }, //                     0xff
};

#endif // _C_KEYBOARD_VALUES_H_
	 
	 
	 
	 
	 
	 
	 
	 
	 
	 
	 
	 
	 
	 
	 
	 
	 
	 
	 
	 
	 
	 
	 
	 
	 
	 
