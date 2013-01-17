#include <windows.h>
#include <stdio.h>
#include "..\..\sim\include\ascii.h"


// [0] = NOT SHIFT, [1] = SHIFT
// Otherwise ALWAYS NOT ASCII
struct OLD_ASCII_TABLE
{
	char Ascii[2];
	unsigned char Flags[2];
};

struct OLD_ASCII_TABLE Old_Key_Chart[256]=
{
	{ 0, 0, 0, 0},// 0 (Space holder)    0x00
	{ 0, 0, 0, 0},// DIK_ESCAPE          0x01
	{ '1', '!', _IS_ASCII_|_IS_DIGIT_,_IS_ASCII_},// DIK_1               0x02
	{ '2', '@', _IS_ASCII_|_IS_DIGIT_,_IS_ASCII_},// DIK_2               0x03
	{ '3', '#', _IS_ASCII_|_IS_DIGIT_,_IS_ASCII_},// DIK_3               0x04
	{ '4', '$', _IS_ASCII_|_IS_DIGIT_,_IS_ASCII_},// DIK_4               0x05
	{ '5', '%', _IS_ASCII_|_IS_DIGIT_,_IS_ASCII_},// DIK_5               0x06
	{ '6', '^', _IS_ASCII_|_IS_DIGIT_,_IS_ASCII_},// DIK_6               0x07
	{ '7', '&', _IS_ASCII_|_IS_DIGIT_,_IS_ASCII_},// DIK_7               0x08
	{ '8', '*', _IS_ASCII_|_IS_DIGIT_,_IS_ASCII_},// DIK_8               0x09
	{ '9', '(', _IS_ASCII_|_IS_DIGIT_,_IS_ASCII_},// DIK_9               0x0A
	{ '0', ')', _IS_ASCII_|_IS_DIGIT_,_IS_ASCII_},// DIK_0               0x0B
	{ '-', '_', _IS_ASCII_|_IS_DIGIT_,_IS_ASCII_},// DIK_MINUS           0x0C    /* - on main keyboard */
	{ '=', '+', _IS_ASCII_|_IS_DIGIT_,_IS_ASCII_},// DIK_EQUALS          0x0D
	{ 0, 0, 0, 0},// DIK_BACK            0x0E    /* backspace */
	{ 0, 0, 0, 0},// DIK_TAB             0x0F
	{ 'q', 'Q', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_Q               0x10
	{ 'w', 'W', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_W               0x11
	{ 'e', 'E', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_E               0x12
	{ 'r', 'R', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_R               0x13
	{ 't', 'T', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_T               0x14
	{ 'y', 'Y', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_Y               0x15
	{ 'u', 'U', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_U               0x16
	{ 'i', 'I', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_I               0x17
	{ 'o', 'O', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_O               0x18
	{ 'p', 'P', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_P               0x19
	{ '[', '{', _IS_ASCII_,_IS_ASCII_},// DIK_LBRACKET        0x1A
	{ ']', '}', _IS_ASCII_,_IS_ASCII_},// DIK_RBRACKET        0x1B
	{ 0, 0, 0, 0},// DIK_RETURN          0x1C    /* Enter on main keyboard */
	{ 0, 0, 0, 0},// DIK_LCONTROL        0x1D
	{ 'a', 'A', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_A               0x1E
	{ 's', 'S', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_S               0x1F
	{ 'd', 'D', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_D               0x20
	{ 'f', 'F', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_F               0x21
	{ 'g', 'G', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_G               0x22
	{ 'h', 'H', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_H               0x23
	{ 'j', 'J', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_J               0x24
	{ 'k', 'K', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_K               0x25
	{ 'l', 'L', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_L               0x26
	{ ';', ':', _IS_ASCII_,_IS_ASCII_},// DIK_SEMICOLON       0x27
	{ '\'', '"', _IS_ASCII_,_IS_ASCII_},// DIK_APOSTROPHE      0x28
	{ '`', '~', _IS_ASCII_,_IS_ASCII_},// DIK_GRAVE           0x29    /* accent grave */
	{ 0, 0, 0, 0},// DIK_LSHIFT          0x2A
	{ '\\', '|', _IS_ASCII_,_IS_ASCII_},// DIK_BACKSLASH       0x2B
	{ 'z', 'Z', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_Z               0x2C
	{ 'x', 'X', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_X               0x2D
	{ 'c', 'C', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_C               0x2E
	{ 'v', 'V', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_V               0x2F
	{ 'b', 'B', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_B               0x30
	{ 'n', 'N', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_N               0x31
	{ 'm', 'M', _IS_ASCII_|_IS_ALPHA_,_IS_ASCII_|_IS_ALPHA_},// DIK_M               0x32
	{ ',', '<', _IS_ASCII_,_IS_ASCII_},// DIK_COMMA           0x33
	{ '.', '>', _IS_ASCII_,_IS_ASCII_},// DIK_PERIOD          0x34    /* . on main keyboard */
	{ '/', '?', _IS_ASCII_,_IS_ASCII_},// DIK_SLASH           0x35    /* / on main keyboard */
	{ 0, 0, 0, 0},// DIK_RSHIFT          0x36
	{ '*', 0, _IS_ASCII_,_IS_ASCII_},// DIK_MULTIPLY        0x37    /* * on numeric keypad */
	{ 0, 0, 0, 0},// DIK_LMENU           0x38    /* left Alt */
	{ ' ', ' ', _IS_ASCII_,_IS_ASCII_},// DIK_SPACE           0x39
	{ 0, 0, 0, 0},// DIK_CAPITAL         0x3A
	{ 0, 0, 0, 0},// DIK_F1              0x3B
	{ 0, 0, 0, 0},// DIK_F2              0x3C
	{ 0, 0, 0, 0},// DIK_F3              0x3D
	{ 0, 0, 0, 0},// DIK_F4              0x3E
	{ 0, 0, 0, 0},// DIK_F5              0x3F
	{ 0, 0, 0, 0},// DIK_F6              0x40
	{ 0, 0, 0, 0},// DIK_F7              0x41
	{ 0, 0, 0, 0},// DIK_F8              0x42
	{ 0, 0, 0, 0},// DIK_F9              0x43
	{ 0, 0, 0, 0},// DIK_F10             0x44
	{ 0, 0, 0, 0},// DIK_NUMLOCK         0x45
	{ 0, 0, 0, 0},// DIK_SCROLL          0x46    /* Scroll Lock */
	{ 0, '7', 0,_IS_ASCII_|_IS_DIGIT_},// DIK_NUMPAD7         0x47
	{ 0, '8', 0,_IS_ASCII_|_IS_DIGIT_},// DIK_NUMPAD8         0x48
	{ 0, '9', 0,_IS_ASCII_|_IS_DIGIT_},// DIK_NUMPAD9         0x49
	{ 0, '-', 0},// DIK_SUBTRACT        0x4A    /* - on numeric keypad */
	{ 0, '4', 0,_IS_ASCII_|_IS_DIGIT_},// DIK_NUMPAD4         0x4B
	{ 0, '5', 0,_IS_ASCII_|_IS_DIGIT_},// DIK_NUMPAD5         0x4C
	{ 0, '6', 0,_IS_ASCII_|_IS_DIGIT_},// DIK_NUMPAD6         0x4D
	{ 0, '+', 0},// DIK_ADD             0x4E    /* + on numeric keypad */
	{ 0, '1', 0,_IS_ASCII_|_IS_DIGIT_},// DIK_NUMPAD1         0x4F
	{ 0, '2', 0,_IS_ASCII_|_IS_DIGIT_},// DIK_NUMPAD2         0x50
	{ 0, '3', 0,_IS_ASCII_|_IS_DIGIT_},// DIK_NUMPAD3         0x51
	{ 0, '0', 0,_IS_ASCII_|_IS_DIGIT_},// DIK_NUMPAD0         0x52
	{ 0, '.', 0},// DIK_DECIMAL         0x53    /* . on numeric keypad */
	{ 0, 0, 0, 0},// 0 (Space holder)    0x54
	{ 0, 0, 0, 0},// 0 (Space holder)    0x55
	{ 0, 0, 0, 0},// 0 (Space holder)    0x56
	{ 0, 0, 0, 0},// DIK_F11             0x57
	{ 0, 0, 0, 0},// DIK_F12             0x58
	{ 0, 0, 0, 0},// 0 (Space holder)    0x59
	{ 0, 0, 0, 0},// 0 (Space holder)    0x5A
	{ 0, 0, 0, 0},// 0 (Space holder)    0x5B
	{ 0, 0, 0, 0},// 0 (Space holder)    0x5C
	{ 0, 0, 0, 0},// 0 (Space holder)    0x5D
	{ 0, 0, 0, 0},// 0 (Space holder)    0x5E
	{ 0, 0, 0, 0},// 0 (Space holder)    0x5F
	{ 0, 0, 0, 0},// 0 (Space holder)    0x60
	{ 0, 0, 0, 0},// 0 (Space holder)    0x61
	{ 0, 0, 0, 0},// 0 (Space holder)    0x62
	{ 0, 0, 0, 0},// 0 (Space holder)    0x63
	{ 0, 0, 0, 0},// DIK_F13             0x64    /*                     (NEC PC98) */
	{ 0, 0, 0, 0},// DIK_F14             0x65    /*                     (NEC PC98) */
	{ 0, 0, 0, 0},// DIK_F15             0x66    /*                     (NEC PC98) */
	{ 0, 0, 0, 0},// 0 (Space holder)    0x67
	{ 0, 0, 0, 0},// 0 (Space holder)    0x68
	{ 0, 0, 0, 0},// 0 (Space holder)    0x69
	{ 0, 0, 0, 0},// 0 (Space holder)    0x6A
	{ 0, 0, 0, 0},// 0 (Space holder)    0x6B
	{ 0, 0, 0, 0},// 0 (Space holder)    0x6C
	{ 0, 0, 0, 0},// 0 (Space holder)    0x6D
	{ 0, 0, 0, 0},// 0 (Space holder)    0x6E
	{ 0, 0, 0, 0},// 0 (Space holder)    0x6F
	{ 0, 0, 0, 0},// DIK_KANA            0x70    /* (Japanese keyboard)            */
	{ 0, 0, 0, 0},// 0 (Space holder)    0x71
	{ 0, 0, 0, 0},// 0 (Space holder)    0x72
	{ 0, 0, 0, 0},// 0 (Space holder)    0x73
	{ 0, 0, 0, 0},// 0 (Space holder)    0x74
	{ 0, 0, 0, 0},// 0 (Space holder)    0x75
	{ 0, 0, 0, 0},// 0 (Space holder)    0x76
	{ 0, 0, 0, 0},// 0 (Space holder)    0x77
	{ 0, 0, 0, 0},// 0 (Space holder)    0x78
	{ 0, 0, 0, 0},// DIK_CONVERT         0x79    /* (Japanese keyboard)            */
	{ 0, 0, 0, 0},// 0 (Space holder)    0x7A
	{ 0, 0, 0, 0},// DIK_NOCONVERT       0x7B    /* (Japanese keyboard)            */
	{ 0, 0, 0, 0},// 0 (Space holder)    0x7C
	{ 0, 0, 0, 0},// DIK_YEN             0x7D    /* (Japanese keyboard)            */
	{ 0, 0, 0, 0},// 0 (Space holder)    0x7E
	{ 0, 0, 0, 0},// 0 (Space holder)    0x7F
	{ 0, 0, 0, 0},// 0 (Space holder)    0x80
	{ 0, 0, 0, 0},// 0 (Space holder)    0x81
	{ 0, 0, 0, 0},// 0 (Space holder)    0x82
	{ 0, 0, 0, 0},// 0 (Space holder)    0x83
	{ 0, 0, 0, 0},// 0 (Space holder)    0x84
	{ 0, 0, 0, 0},// 0 (Space holder)    0x85
	{ 0, 0, 0, 0},// 0 (Space holder)    0x86
	{ 0, 0, 0, 0},// 0 (Space holder)    0x87
	{ 0, 0, 0, 0},// 0 (Space holder)    0x88
	{ 0, 0, 0, 0},// 0 (Space holder)    0x89
	{ 0, 0, 0, 0},// 0 (Space holder)    0x8A
	{ 0, 0, 0, 0},// 0 (Space holder)    0x8B
	{ 0, 0, 0, 0},// 0 (Space holder)    0x8C
	{ 0, '=', 0, _IS_ASCII_},// DIK_NUMPADEQUALS    0x8D    /* = on numeric keypad (NEC PC98) */
	{ 0, 0, 0, 0},// 0 (Space holder)    0x8E
	{ 0, 0, 0, 0},// 0 (Space holder)    0x8F
	{ 0, 0, 0, 0},// DIK_CIRCUMFLEX      0x90    /* (Japanese keyboard)            */
	{ 0, '@', 0, _IS_ASCII_},// DIK_AT              0x91    /*                     (NEC PC98) */
	{ 0, ':', 0, _IS_ASCII_},// DIK_COLON           0x92    /*                     (NEC PC98) */
	{ 0, '_', 0, _IS_ASCII_},// DIK_UNDERLINE       0x93    /*                     (NEC PC98) */
	{ 0, 0, 0, 0},// DIK_KANJI           0x94    /* (Japanese keyboard)            */
	{ 0, 0, 0, 0},// DIK_STOP            0x95    /*                     (NEC PC98) */
	{ 0, 0, 0, 0},// DIK_AX              0x96    /*                     (Japan AX) */
	{ 0, 0, 0, 0},// DIK_UNLABELED       0x97    /*                        (J3100) */
	{ 0, 0, 0, 0},// 0 (Space holder)    0x98
	{ 0, 0, 0, 0},// 0 (Space holder)    0x99
	{ 0, 0, 0, 0},// 0 (Space holder)    0x9A
	{ 0, 0, 0, 0},// 0 (Space holder)    0x9B
	{ 0, 0, 0, 0},// DIK_NUMPADENTER     0x9C    /* Enter on numeric keypad */
	{ 0, 0, 0, 0},// DIK_RCONTROL        0x9D
	{ 0, 0, 0, 0},// 0 (Space holder)    0x9E
	{ 0, 0, 0, 0},// 0 (Space holder)    0x9F
	{ 0, 0, 0, 0},// 0 (Space holder)    0xA0
	{ 0, 0, 0, 0},// 0 (Space holder)    0xA1
	{ 0, 0, 0, 0},// 0 (Space holder)    0xA2
	{ 0, 0, 0, 0},// 0 (Space holder)    0xA3
	{ 0, 0, 0, 0},// 0 (Space holder)    0xA4
	{ 0, 0, 0, 0},// 0 (Space holder)    0xA5
	{ 0, 0, 0, 0},// 0 (Space holder)    0xA6
	{ 0, 0, 0, 0},// 0 (Space holder)    0xA7
	{ 0, 0, 0, 0},// 0 (Space holder)    0xA8
	{ 0, 0, 0, 0},// 0 (Space holder)    0xA9
	{ 0, 0, 0, 0},// 0 (Space holder)    0xAA
	{ 0, 0, 0, 0},// 0 (Space holder)    0xAB
	{ 0, 0, 0, 0},// 0 (Space holder)    0xAC
	{ 0, 0, 0, 0},// 0 (Space holder)    0xAD
	{ 0, 0, 0, 0},// 0 (Space holder)    0xAE
	{ 0, 0, 0, 0},// 0 (Space holder)    0xAF
	{ 0, 0, 0, 0},// 0 (Space holder)    0xB0
	{ 0, 0, 0, 0},// 0 (Space holder)    0xB1
	{ 0, 0, 0, 0},// 0 (Space holder)    0xB2
	{ 0, ',', 0, _IS_ASCII_},// DIK_NUMPADCOMMA     0xB3    /* , on numeric keypad (NEC PC98) */
	{ 0, 0, 0, 0},// 0 (Space holder)    0xB4
	{ 0, '/', 0, _IS_ASCII_},// DIK_DIVIDE          0xB5    /* / on numeric keypad */
	{ 0, 0, 0, 0},// 0 (Space holder)    0xB6
	{ 0, 0, 0, 0},// DIK_SYSRQ           0xB7
	{ 0, 0, 0, 0},// DIK_RMENU           0xB8    /* right Alt */
	{ 0, 0, 0, 0},// 0 (Space holder)    0xB9
	{ 0, 0, 0, 0},// 0 (Space holder)    0xBA
	{ 0, 0, 0, 0},// 0 (Space holder)    0xBB
	{ 0, 0, 0, 0},// 0 (Space holder)    0xBC
	{ 0, 0, 0, 0},// 0 (Space holder)    0xBD
	{ 0, 0, 0, 0},// 0 (Space holder)    0xBE
	{ 0, 0, 0, 0},// 0 (Space holder)    0xBF
	{ 0, 0, 0, 0},// 0 (Space holder)    0xC0
	{ 0, 0, 0, 0},// 0 (Space holder)    0xC1
	{ 0, 0, 0, 0},// 0 (Space holder)    0xC2
	{ 0, 0, 0, 0},// 0 (Space holder)    0xC3
	{ 0, 0, 0, 0},// 0 (Space holder)    0xC4
	{ 0, 0, 0, 0},// 0 (Space holder)    0xC5
	{ 0, 0, 0, 0},// 0 (Space holder)    0xC6
	{ 0, 0, 0, 0},// DIK_HOME            0xC7    /* Home on arrow keypad */
	{ 0, 0, 0, 0},// DIK_UP              0xC8    /* UpArrow on arrow keypad */
	{ 0, 0, 0, 0},// DIK_PRIOR           0xC9    /* PgUp on arrow keypad */
	{ 0, 0, 0, 0},// 0 (Space holder)    0xCA
	{ 0, 0, 0, 0},// DIK_LEFT            0xCB    /* LeftArrow on arrow keypad */
	{ 0, 0, 0, 0},// 0 (Space holder)    0xCC
	{ 0, 0, 0, 0},// DIK_RIGHT           0xCD    /* RightArrow on arrow keypad */
	{ 0, 0, 0, 0},// 0 (Space holder)    0xCE
	{ 0, 0, 0, 0},// DIK_END             0xCF    /* End on arrow keypad */
	{ 0, 0, 0, 0},// DIK_DOWN            0xD0    /* DownArrow on arrow keypad */
	{ 0, 0, 0, 0},// DIK_NEXT            0xD1    /* PgDn on arrow keypad */
	{ 0, 0, 0, 0},// DIK_INSERT          0xD2    /* Insert on arrow keypad */
	{ 0, 0, 0, 0},// DIK_DELETE          0xD3    /* Delete on arrow keypad */
	{ 0, 0, 0, 0},// 0 (Space holder)    0xD4
	{ 0, 0, 0, 0},// 0 (Space holder)    0xD5
	{ 0, 0, 0, 0},// 0 (Space holder)    0xD6
	{ 0, 0, 0, 0},// 0 (Space holder)    0xD7
	{ 0, 0, 0, 0},// 0 (Space holder)    0xD8
	{ 0, 0, 0, 0},// 0 (Space holder)    0xD9
	{ 0, 0, 0, 0},// 0 (Space holder)    0xDA
	{ 0, 0, 0, 0},// DIK_LWIN            0xDB    /* Left Windows key */
	{ 0, 0, 0, 0},// DIK_RWIN            0xDC    /* Right Windows key */
	{ 0, 0, 0, 0},// DIK_APPS            0xDD    /* AppMenu key */
	{ 0, 0, 0, 0},// 0 (Space holder)    0xDE
	{ 0, 0, 0, 0},// 0 (Space holder)    0xDF
	{ 0, 0, 0, 0},// 0 (Space holder)    0xE0
	{ 0, 0, 0, 0},// 0 (Space holder)    0xE1
	{ 0, 0, 0, 0},// 0 (Space holder)    0xE2
	{ 0, 0, 0, 0},// 0 (Space holder)    0xE3
	{ 0, 0, 0, 0},// 0 (Space holder)    0xE4
	{ 0, 0, 0, 0},// 0 (Space holder)    0xE5
	{ 0, 0, 0, 0},// 0 (Space holder)    0xE6
	{ 0, 0, 0, 0},// 0 (Space holder)    0xE7
	{ 0, 0, 0, 0},// 0 (Space holder)    0xE8
	{ 0, 0, 0, 0},// 0 (Space holder)    0xE9
	{ 0, 0, 0, 0},// 0 (Space holder)    0xEA
	{ 0, 0, 0, 0},// 0 (Space holder)    0xEB
	{ 0, 0, 0, 0},// 0 (Space holder)    0xEC
	{ 0, 0, 0, 0},// 0 (Space holder)    0xED
	{ 0, 0, 0, 0},// 0 (Space holder)    0xEE
	{ 0, 0, 0, 0},// 0 (Space holder)    0xEF
	{ 0, 0, 0, 0},// 0 (Space holder)    0xF0
	{ 0, 0, 0, 0},// 0 (Space holder)    0xF1
	{ 0, 0, 0, 0},// 0 (Space holder)    0xF2
	{ 0, 0, 0, 0},// 0 (Space holder)    0xF3
	{ 0, 0, 0, 0},// 0 (Space holder)    0xF4
	{ 0, 0, 0, 0},// 0 (Space holder)    0xF5
	{ 0, 0, 0, 0},// 0 (Space holder)    0xF6
	{ 0, 0, 0, 0},// 0 (Space holder)    0xF7
	{ 0, 0, 0, 0},// 0 (Space holder)    0xF8
	{ 0, 0, 0, 0},// 0 (Space holder)    0xF9
	{ 0, 0, 0, 0},// 0 (Space holder)    0xFA
	{ 0, 0, 0, 0},// 0 (Space holder)    0xFB
	{ 0, 0, 0, 0},// 0 (Space holder)    0xFC
	{ 0, 0, 0, 0},// 0 (Space holder)    0xFD
	{ 0, 0, 0, 0},// 0 (Space holder)    0xFE
	{ 0, 0, 0, 0},// 0 (Space holder)    0xFF
};

struct TableLookup
{
	char *Name;
	long Value;
};

struct TableLookup States_Table[]=
{
	{ "SHIFT",            1 },
	{ "CTRL",             2 },
	{ "ALT",              4 },
	{ NULL, 0 },
};

struct TableLookup Flags_Table[]=
{
	{ "ALPHA",             _IS_ALPHA_ },
	{ "DIGIT",             _IS_DIGIT_ },
	{ NULL, 0 },
};

struct TableLookup DIK_Table[]=
{
	{ "DIK_ESCAPE",       1 },
	{ "DIK_1",            2 },
	{ "DIK_2",            3 },
	{ "DIK_3",            4 },
	{ "DIK_4",            5 },
	{ "DIK_5",            6 },
	{ "DIK_6",            7 },
	{ "DIK_7",            8 },
	{ "DIK_8",            9 },
	{ "DIK_9",            10 },
	{ "DIK_0",            11 },
	{ "DIK_MINUS",        12 },
	{ "DIK_EQUALS",       13 },
	{ "DIK_BACK",         14 },
	{ "DIK_TAB",          15 },
	{ "DIK_Q",            16 },
	{ "DIK_W",            17 },
	{ "DIK_E",            18 },
	{ "DIK_R",            19 },
	{ "DIK_T",            20 },
	{ "DIK_Y",            21 },
	{ "DIK_U",            22 },
	{ "DIK_I",            23 },
	{ "DIK_O",            24 },
	{ "DIK_P",            25 },
	{ "DIK_LBRACKET",     26 },
	{ "DIK_RBRACKET",     27 },
	{ "DIK_RETURN",       28 },
	{ "DIK_LCONTROL",     29 },
	{ "DIK_A",            30 },
	{ "DIK_S",            31 },
	{ "DIK_D",            32 },
	{ "DIK_F",            33 },
	{ "DIK_G",            34 },
	{ "DIK_H",            35 },
	{ "DIK_J",            36 },
	{ "DIK_K",            37 },
	{ "DIK_L",            38 },
	{ "DIK_SEMICOLON",    39 },
	{ "DIK_APOSTROPHE",   40 },
	{ "DIK_GRAVE",        41 },
	{ "DIK_LSHIFT",       42 },
	{ "DIK_BACKSLASH",    43 },
	{ "DIK_Z",            44 },
	{ "DIK_X",            45 },
	{ "DIK_C",            46 },
	{ "DIK_V",            47 },
	{ "DIK_B",            48 },
	{ "DIK_N",            49 },
	{ "DIK_M",            50 },
	{ "DIK_COMMA",        51 },
	{ "DIK_PERIOD",       52 },
	{ "DIK_SLASH",        53 },
	{ "DIK_RSHIFT",       54 },
	{ "DIK_MULTIPLY",     55 },
	{ "DIK_LMENU",        56 },
	{ "DIK_SPACE",        57 },
	{ "DIK_CAPITAL",      58 },
	{ "DIK_F1",           59 },
	{ "DIK_F2",           60 },
	{ "DIK_F3",           61 },
	{ "DIK_F4",           62 },
	{ "DIK_F5",           63 },
	{ "DIK_F6",           64 },
	{ "DIK_F7",           65 },
	{ "DIK_F8",           66 },
	{ "DIK_F9",           67 },
	{ "DIK_F10",          68 },
	{ "DIK_NUMLOCK",      69 },
	{ "DIK_SCROLL",       70 },
	{ "DIK_NUMPAD7",      71 },
	{ "DIK_NUMPAD8",      72 },
	{ "DIK_NUMPAD9",      73 },
	{ "DIK_SUBTRACT",     74 },
	{ "DIK_NUMPAD4",      75 },
	{ "DIK_NUMPAD5",      76 },
	{ "DIK_NUMPAD6",      77 },
	{ "DIK_ADD",          78 },
	{ "DIK_NUMPAD1",      79 },
	{ "DIK_NUMPAD2",      80 },
	{ "DIK_NUMPAD3",      81 },
	{ "DIK_NUMPAD0",      82 },
	{ "DIK_DECIMAL",      83 },
	{ "DIK_F11",          87 },
	{ "DIK_F12",          88 },
	{ "DIK_F13",          100 },
	{ "DIK_F14",          101 },
	{ "DIK_F15",          102 },
	{ "DIK_KANA",         112 },
	{ "DIK_CONVERT",      121 },
	{ "DIK_NOCONVERT",    123 },
	{ "DIK_YEN",          125 },
	{ "DIK_NUMPADEQUALS", 141 },
	{ "DIK_CIRCUMFLEX",   144 },
	{ "DIK_AT",           145 },
	{ "DIK_COLON",        146 },
	{ "DIK_UNDERLINE",    147 },
	{ "DIK_KANJI",        148 },
	{ "DIK_STOP",         149 },
	{ "DIK_AX",           150 },
	{ "DIK_UNLABELED",    151 },
	{ "DIK_NUMPADENTER",  156 },
	{ "DIK_RCONTROL",     157 },
	{ "DIK_NUMPADCOMMA",  179 },
	{ "DIK_DIVIDE",       181 },
	{ "DIK_SYSRQ",        183 },
	{ "DIK_RMENU",        184 },
	{ "DIK_HOME",         199 },
	{ "DIK_UP",           200 },
	{ "DIK_PRIOR",        201 },
	{ "DIK_LEFT",         203 },
	{ "DIK_RIGHT",        205 },
	{ "DIK_END",          207 },
	{ "DIK_DOWN",         208 },
	{ "DIK_NEXT",         209 },
	{ "DIK_INSERT",       210 },
	{ "DIK_DELETE",       211 },
	{ "DIK_LWIN",         219 },
	{ "DIK_RWIN",         220 },
	{ "DIK_APPS",         221 },
	{ NULL,0 },
};

struct ASCII_TABLE Key_Chart[256];

char *FlagTable[]=
{
	"0", // 0
	"_IS_ASCII_", // 1
	"_IS_ALPHA_", // 2
	"_IS_ASCII_|_IS_ALPHA_", // 3
	"_IS_DIGIT_", // 4
	"_IS_ASCII_|_IS_DIGIT_", // 5
	"_IS_ALPHA_|_IS_DIGIT_", // 6
	"_IS_ASCII_|_IS_ALPHA_", // 7
};


long TableSearch(struct TableLookup *list,char *str)
{
	short i;

	i=0;
	while(list[i].Name)
	{
		if(!stricmp(list[i].Name,str))
			return(list[i].Value);
		i++;
	}
	return(0);
}

void main(int argc,char **argv)
{
	char buffer[200];
	char *token;
	short i,j;
	long Index, ascii, shiftstates, AsciiFlags;
	long tokval;
	FILE *fp;

	printf("Ascii Table generator v1.0 - Peter Ward\n");
	if(argc < 3)
	{
		printf("Usage: ascii <input file> <output file with no extension>\n");
		printf("    <input file> - a list of DIK_KEY codes to override default english ascii set\n");
		printf("      record format:  DIK_KEY_VALUE    ASCII_VALUE      SHIFT_STATES      ASCII_FLAGS\n");
		printf("      where: DIK_KEY_VALUE is a number from 1 to 255\n");
		printf("             ASCII_VALUE is the index in the font for the character you need printed\n");
		printf("             SHIFT_STATES are any combination of SHIFT, ALT, CTRL\n");
		printf("                          each separated ba at least a space\n");
		printf("             ASCII_FLAGS  if the Ascii character is a number put DIGIT on the line\n");
		printf("                          if the ascii character is part of the alphabet put ALPHA on the line\n");
		printf("Examples:\n");
		printf("40                196     SHIFT   ALPHA    <- this would be the line for the 'a' with the '\"' over it\n");
		printf("DIK_APOSTROPHE    196     SHIFT   ALPHA    <- this would be the line for the 'a' with the '\"' over it\n");
		printf("16      64      CTRL ALT    <- this would be the line for the '@' on the german keyboard\n");
		printf("DIK_Q   64      CTRL ALT    <- this would be the line for the '@' on the german keyboard\n\n");
		printf("    <output files> - This program will spit out 2 files...\n");
		printf("             1st file   <output>.h   - a text file you can look at to see if everything is ok\n");
		printf("             2nd file   <output>.bin - data file loaded into the game\n\n");
		printf("NOTE: if you wish to just generate the 'english' version\n");
		printf("      put a bogus filename for the input file\n\n");
		exit(0);
	}
	memset(Key_Chart,0,sizeof(Key_Chart));

	for(i=0;i<256;i++) // Copy in default english set
	{
		for(j=0;j<2;j++)
		{
			Key_Chart[i].Ascii[j]=Old_Key_Chart[i].Ascii[j];
			Key_Chart[i].Flags[j]=Old_Key_Chart[i].Flags[j];
		}
	}

	fp=fopen(argv[1],"r");
	if(fp) // process ascii default changes
	{
		while(fgets(buffer,200,fp))
		{
			token=strtok(buffer," ,\t\n");
			if(token)
			{
				if(isdigit(token[0]))
					Index=atol(token);
				else
					Index=TableSearch(DIK_Table,token);
				if(Index)
				{
					token=strtok(NULL," ,\t\n");
					ascii=atol(token);
					if(ascii)
					{
						shiftstates=0;
						AsciiFlags=0;
						token=strtok(NULL," ,\t\n");
						while(token)
						{
							tokval=TableSearch(States_Table,token);
							if(tokval)
								shiftstates |= tokval;
							tokval=TableSearch(Flags_Table,token);
							if(tokval)
								AsciiFlags |= tokval;
							token=strtok(NULL," ,\t\n");
						}
						Key_Chart[Index].Ascii[shiftstates]=ascii;
						Key_Chart[Index].Flags[shiftstates]=_IS_ASCII_ | AsciiFlags;
					}
				}
			}
		}
		fclose(fp);
	}
	sprintf(buffer,"%s.h",argv[2]);
	fp=fopen(buffer,"w");
	if(fp)
	{
		fprintf(fp,"struct ASCII_TABLE Key_Chart[256]=\n{\n");
		for(i=0;i<256;i++)
		{
			fprintf(fp,"\t{ ");
			for(j=0;j<8;j++)
				fprintf(fp,"%3ld,",Key_Chart[i].Ascii[j]);

			fprintf(fp,"      ");
			for(j=0;j<8;j++)
				fprintf(fp,"%s,",FlagTable[Key_Chart[i].Flags[j]]);
			fprintf(fp," }, // ");
			for(j=0;j<8;j++)
				fprintf(fp,"%c ",(Key_Chart[i].Flags[j] & _IS_ASCII_ ? Key_Chart[i].Ascii[j] : ' '));
			fprintf(fp,"\n");
		}
		fprintf(fp,"};\n");
		fclose(fp);
	}
	sprintf(buffer,"%s.bin",argv[2]);
	fp=fopen(buffer,"wb");
	if(fp)
	{
		fwrite(Key_Chart,sizeof(Key_Chart),1,fp);
		fclose(fp);
	}
	exit(0);
}
