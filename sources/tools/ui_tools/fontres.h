#ifndef _C_FONT_RES_H_
#define _C_FONT_RES_H_

enum
{
	_FNT_CHECK_KERNING_		=0x40,
};

struct KerningStr
{
	short first;
	short second;
	char  add;
};

struct CharStr
{
	unsigned char flags;
	unsigned char w;

	char lead;
	char trail;
};

class C_Fontmgr
{
	private:
		long		ID_;

		char		name_[32];
		long		pitch_;
		short		first_;
		short		last_;

		long		bytesperline_;

		long		fNumChars_;
		CharStr		*fontTable_;

		long		dSize_;
		char		*fontData_;

		long		kNumKerns_;
		KerningStr	*kernList_;

	public:

		C_Fontmgr();
		~C_Fontmgr();

		void Setup(long ID,char *fontfile);
		void Cleanup();

		short First() { return(first_); }
		short Last()  { return(last_); }
		long ByteWidth() { return(bytesperline_); }
		char *GetName() { return(name_); }

		long Width(_TCHAR *str);
		long Width(_TCHAR *str,long length);
		long Height();
		CharStr *GetChar(short ID);
		char *GetData() { return(fontData_); }

		// no cliping version (except for screen)
		void Draw(_TCHAR *str,WORD color,long x,long y,long dwidth,WORD *dest);
		// clipping version (use cliprect)
		void Draw(_TCHAR *str,WORD color,long x,long y,RECT *cliprect,long dwidth,WORD *dest);
};

#endif
