#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>

#include "fontres.h"

C_Fontmgr::C_Fontmgr()
{
	ID_=0;
	first_=0;
	last_=0;

	bytesperline_=0;

	fNumChars_=0;
	fontTable_=NULL;

	kNumKerns_=0;
	kernList_=NULL;

	dSize_=0;
	fontData_=NULL;
}

C_Fontmgr::~C_Fontmgr()
{
	if(fontTable_ || fontData_ || kernList_)
		Cleanup();
}

void C_Fontmgr::Setup(long ID,char *fontfile)
{
	FILE *fp;

	ID_=ID;

	fp=fopen(fontfile,"rb");
	if(!fp)
	{
		printf("FONT error: %s not opened\n",fontfile);
		return;
	}
	fread(&name_,32,1,fp);
	fread(&pitch_,sizeof(long),1,fp);
	fread(&first_,sizeof(short),1,fp);
	fread(&last_, sizeof(short),1,fp);
	fread(&bytesperline_,sizeof(long),1,fp);
	fread(&fNumChars_,sizeof(long),1,fp);
	fread(&kNumKerns_,sizeof(long),1,fp);
	fread(&dSize_,sizeof(long),1,fp);
	if(fNumChars_)
	{
		fontTable_=new CharStr[fNumChars_];
		fread(fontTable_,sizeof(CharStr),fNumChars_,fp);
	}
	if(kNumKerns_)
	{
		kernList_=new KerningStr[kNumKerns_];
		fread(kernList_,sizeof(KerningStr),kNumKerns_,fp);
	}
	if(dSize_)
	{
		fontData_=new char [dSize_];
		fread(fontData_,dSize_,1,fp);
	}
	fclose(fp);
}

void C_Fontmgr::Cleanup()
{
	ID_=0;
	first_=0;
	last_=0;
	bytesperline_=0;

	fNumChars_=0;
	if(fontTable_)
	{
		delete fontTable_;
		fontTable_=NULL;
	}

	kNumKerns_=0;
	if(kernList_)
	{
		delete kernList_;
		kernList_=NULL;
	}

	dSize_=0;
	if(fontData_)
	{
		delete fontData_;
		fontData_=NULL;
	}
}

long C_Fontmgr::Width(_TCHAR c)
{
	long size;
	long thechar;

	size=0;
	thechar=c & 0xff;
	if(thechar >= first_ && thechar <= last_)
	{
		thechar-=first_;
		size+=fontTable_[thechar].lead + fontTable_[thechar].w;
	}

	return(size);
}

long C_Fontmgr::Width(_TCHAR *str)
{
	long i;
	long size;
	long thechar;
	long prevchar;

	size=0;
	i=0;
	prevchar=-1;
	while(str[i])
	{
		thechar=str[i] & 0xff;
		if(thechar >= first_ && thechar <= last_)
		{
			thechar-=first_;
			size+=fontTable_[thechar].lead + fontTable_[thechar].trail + fontTable_[thechar].w;
			if(prevchar >= 0)
				size+=fontTable_[prevchar].trail;
			prevchar=thechar;
		}
		i++;
	}
	return(size);
}

long C_Fontmgr::Width(_TCHAR *str,long len)
{
	long i;
	long size;
	long thechar;
	long prevchar;

	size=0;
	i=0;
	prevchar=-1;
	while(str[i] && i < len)
	{
		thechar=str[i] & 0xff;
		if(thechar >= first_ && thechar <= last_)
		{
			thechar-=first_;
			size+=fontTable_[thechar].lead + fontTable_[thechar].w;
			if(prevchar >= 0)
				size+=fontTable_[prevchar].trail;
			prevchar=thechar;
		}
		i++;
	}
	return(size);
}

long C_Fontmgr::Height()
{
	return(pitch_);
}

CharStr *C_Fontmgr::GetChar(short ID)
{
	if(fontTable_ && ID >= first_ && ID <= last_)
		return(&fontTable_[ID-first_]);
	return(NULL);
}


void C_Fontmgr::Draw(_TCHAR *str,WORD color,long x,long y,long dwidth,WORD *dest)
{
	long idx,i,j,k;
	long xoffset,yoffset;
	unsigned long thechar;
	unsigned char *sptr,seg;
	WORD *dstart,*dptr,*dendh,*dendv;

	if(!fontData_)
		return;

	idx=0;
	xoffset=x;
	yoffset=y;
	dendv=dest + 800 * 600; // Make sure we don't go past the end of the surface
	while(str[idx])
	{
		thechar=str[idx]&0xff;
		if(thechar >= first_ && thechar <= last_)
		{
			thechar-=first_;
			xoffset+=fontTable_[thechar].lead;

			sptr=(unsigned char *)(fontData_ + (thechar * bytesperline_ * pitch_));
			dstart=dest + (yoffset * dwidth) + xoffset;
			dendh=dest + (yoffset * dwidth) + dwidth;
			for(i=0;i<pitch_ && dstart < dendv;i++)
			{
				dptr=dstart;
				for(j=0;j<bytesperline_;j++)
				{
					seg=*sptr++;
					if(dptr < dendh)
						for(k=0;k<8;k++)
						{
							if(seg & 1)
								*dptr++=color;
							else
								dptr++;
							seg=seg >> 1;
						}
				}
				dstart+=dwidth;
				dendh+=dwidth;
			}
			// if fontTable_[thechar].flags & _FNT_CHECK_KERNING_)
			//{
			//}
			//else
				xoffset+=fontTable_[thechar].w + fontTable_[thechar].trail;
		}
		else // temporarily add 5 pixels when char isn't in charset
			xoffset+=5;

		idx++;
	}
}

void C_Fontmgr::Draw(_TCHAR *str,WORD color,long x,long y,RECT *cliprect,long dwidth,WORD *dest)
{
	long idx,i,j,k;
	long xoffset,yoffset;
	unsigned long thechar;
	unsigned char *sptr,seg;
	WORD *dstart,*dptr;
	WORD *dendh,*dendv;
	WORD *dclipx,*dclipy;

	if(!fontData_)
		return;

	idx=0;
	xoffset=x;
	yoffset=y;
	dclipy=dest + (cliprect->top * dwidth);
	dendv=dest + (cliprect->bottom * dwidth); // Make sure we don't go past the end of the surface
	while(str[idx])
	{
		thechar=str[idx]&0xff;
		if(thechar >= first_ && thechar <= last_)
		{
			thechar-=first_;
			xoffset+=fontTable_[thechar].lead;

			sptr=(unsigned char *)(fontData_ + (thechar * bytesperline_ * pitch_));
			dstart=dest + (yoffset * dwidth) + xoffset;
			dclipx=dclipy + cliprect->left;
			dendh=dclipx + (cliprect->right-cliprect->left);

			for(i=0;i<pitch_ && dstart < dendv;i++)
			{
				if(dstart >= dclipy)
				{
					dptr=dstart;
					for(j=0;j<bytesperline_;j++)
					{
						seg=*sptr++;
						for(k=0;k<8 && dptr < dendh;k++)
						{
							if(dptr >= dclipx)
							{
								if(seg & 1)
									*dptr++=color;
								else
									dptr++;
							}
							else
								dptr++;
							seg >>= 1;
						}
					}
				}
				dclipx+=dwidth;
				dstart+=dwidth;
				dendh+=dwidth;
			}
			// if fontTable_[thechar].flags & _FNT_CHECK_KERNING_)
			//{
			//}
			//else
				xoffset+=fontTable_[thechar].w + fontTable_[thechar].trail;
		}
		else // temporarily add 5 pixels when char isn't in charset
			xoffset+=5;

		idx++;
	}
}
