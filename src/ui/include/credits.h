#ifndef _CREDITS_H_
#define _CREDITS_H_

struct PersonStr
{
	long FontID;
	long ColorID;
	char Name[100];
	char Job[100];
};

struct TitleStr
{
	long FontID;
	long ColorID;
	char Title[50];
};

struct LegalStr
{
	long FontID;
	long ColorID;
	char Legal[100];
};

enum
{
	_NOTHING_=0,
	_FONT_,
	_TITLE_,
	_NAME_,
	_LEGAL_,
	_COLOR_,
	_BLANK_,
};

#endif