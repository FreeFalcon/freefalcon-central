#ifndef _DIK_TO_ASCII_H_
#define _DIK_TO_ASCII_H_

enum
{
	_IS_ASCII_			=0x01, // ANY printable
	_IS_ALPHA_			=0x02, // A-Z (upper or lower case)
	_IS_DIGIT_			=0x04, // 0-9
};

// [0] = NOT SHIFT, [1] = SHIFT
// Otherwise ALWAYS NOT ASCII
struct ASCII_TABLE
{
	char Ascii[8];
	unsigned char Flags[8];
};

// Ascii[0]= Lower case, Ascii[1]=Upper case, flags ... see above
extern struct ASCII_TABLE Key_Chart[256];

// Macros to access functions
#define AsciiChar(DIK_KEY,SHIFT_STATE)   (Key_Chart[(DIK_KEY) & 0xff].Ascii[(SHIFT_STATE)&7])
#define DIK_IsAscii(DIK_KEY,SHIFT_STATE) (Key_Chart[(DIK_KEY) & 0xff].Flags[(SHIFT_STATE)&7]&_IS_ASCII_)
#define DIK_IsAlpha(DIK_KEY,SHIFT_STATE) (Key_Chart[(DIK_KEY) & 0xff].Flags[(SHIFT_STATE)&7]&_IS_ALPHA_)
#define DIK_IsDigit(DIK_KEY,SHIFT_STATE) (Key_Chart[(DIK_KEY) & 0xff].Flags[(SHIFT_STATE)&7]&_IS_DIGIT_)

#endif