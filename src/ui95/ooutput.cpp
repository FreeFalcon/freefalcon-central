#include <windows.h>
#include "chandler.h"

static _TCHAR _password_[100];

O_Output::O_Output(char **stream)
{
    memcpy(&flags_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&Font_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&ScaleSet_, *stream, sizeof(long));
    *stream += sizeof(long);

    memcpy(&FgColor_, *stream, sizeof(COLORREF));
    *stream += sizeof(COLORREF);
    memcpy(&BgColor_, *stream, sizeof(COLORREF));
    *stream += sizeof(COLORREF);
    memcpy(&Src_, *stream, sizeof(UI95_RECT));
    *stream += sizeof(UI95_RECT);
    memcpy(&Dest_, *stream, sizeof(UI95_RECT));
    *stream += sizeof(UI95_RECT);
    memcpy(&origx_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&origy_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&x_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&y_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&w_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&h_, *stream, sizeof(long));
    *stream += sizeof(long);

    memcpy(&_OType_, *stream, sizeof(short));
    *stream += sizeof(short);
    memcpy(&animtype_, *stream, sizeof(short));
    *stream += sizeof(short);
    memcpy(&frame_, *stream, sizeof(short));
    *stream += sizeof(short);
    memcpy(&direction_, *stream, sizeof(short));
    *stream += sizeof(short);
    memcpy(&ready_, *stream, sizeof(short));
    *stream += sizeof(short);
    memcpy(&fperc_, *stream, sizeof(short));
    *stream += sizeof(short);
    memcpy(&bperc_, *stream, sizeof(short));
    *stream += sizeof(short);
    memcpy(&LabelLen_, *stream, sizeof(short));
    *stream += sizeof(short);
}

O_Output::O_Output(FILE *fp)
{
    fread(&flags_, sizeof(long), 1, fp);
    fread(&Font_, sizeof(long), 1, fp);
    fread(&ScaleSet_, sizeof(long), 1, fp);

    fread(&FgColor_, sizeof(COLORREF), 1, fp);
    fread(&BgColor_, sizeof(COLORREF), 1, fp);
    fread(&Src_, sizeof(UI95_RECT), 1, fp);
    fread(&Dest_, sizeof(UI95_RECT), 1, fp);
    fread(&origx_, sizeof(long), 1, fp);
    fread(&origy_, sizeof(long), 1, fp);
    fread(&x_, sizeof(long), 1, fp);
    fread(&y_, sizeof(long), 1, fp);
    fread(&w_, sizeof(long), 1, fp);
    fread(&h_, sizeof(long), 1, fp);

    fread(&_OType_, sizeof(short), 1, fp);
    fread(&animtype_, sizeof(short), 1, fp);
    fread(&frame_, sizeof(short), 1, fp);
    fread(&direction_, sizeof(short), 1, fp);
    fread(&ready_, sizeof(short), 1, fp);
    fread(&fperc_, sizeof(short), 1, fp);
    fread(&bperc_, sizeof(short), 1, fp);
    fread(&LabelLen_, sizeof(short), 1, fp);
}

long O_Output::Size()
{
    long size = sizeof(long)
                + sizeof(long)
                + sizeof(long)

                + sizeof(COLORREF)
                + sizeof(COLORREF)
                + sizeof(UI95_RECT)
                + sizeof(UI95_RECT)
                + sizeof(long)
                + sizeof(long)
                + sizeof(long)
                + sizeof(long)
                + sizeof(long)
                + sizeof(long)
                + sizeof(short)
                + sizeof(short)
                + sizeof(short)
                + sizeof(short)
                + sizeof(short)
                + sizeof(short)
                + sizeof(short)
                + sizeof(short);
    return(size);
}

void O_Output::Save(char **stream)
{
    memcpy(*stream, &flags_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &Font_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &ScaleSet_, sizeof(long));
    *stream += sizeof(long);

    memcpy(*stream, &FgColor_, sizeof(COLORREF));
    *stream += sizeof(COLORREF);
    memcpy(*stream, &BgColor_, sizeof(COLORREF));
    *stream += sizeof(COLORREF);
    memcpy(*stream, &Src_, sizeof(UI95_RECT));
    *stream += sizeof(UI95_RECT);
    memcpy(*stream, &Dest_, sizeof(UI95_RECT));
    *stream += sizeof(UI95_RECT);
    memcpy(*stream, &origx_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &origy_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &x_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &y_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &w_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &h_, sizeof(long));
    *stream += sizeof(long);

    memcpy(*stream, &_OType_, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &animtype_, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &frame_, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &direction_, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &ready_, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &fperc_, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &bperc_, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &LabelLen_, sizeof(short));
    *stream += sizeof(short);
}

void O_Output::Save(FILE *fp)
{
    fwrite(&flags_, sizeof(long), 1, fp);
    fwrite(&Font_, sizeof(long), 1, fp);
    fwrite(&ScaleSet_, sizeof(long), 1, fp);

    fwrite(&FgColor_, sizeof(COLORREF), 1, fp);
    fwrite(&BgColor_, sizeof(COLORREF), 1, fp);
    fwrite(&Src_, sizeof(UI95_RECT), 1, fp);
    fwrite(&Dest_, sizeof(UI95_RECT), 1, fp);
    fwrite(&origx_, sizeof(long), 1, fp);
    fwrite(&origy_, sizeof(long), 1, fp);
    fwrite(&x_, sizeof(long), 1, fp);
    fwrite(&y_, sizeof(long), 1, fp);
    fwrite(&w_, sizeof(long), 1, fp);
    fwrite(&h_, sizeof(long), 1, fp);

    fwrite(&_OType_, sizeof(short), 1, fp);
    fwrite(&animtype_, sizeof(short), 1, fp);
    fwrite(&frame_, sizeof(short), 1, fp);
    fwrite(&direction_, sizeof(short), 1, fp);
    fwrite(&ready_, sizeof(short), 1, fp);
    fwrite(&fperc_, sizeof(short), 1, fp);
    fwrite(&bperc_, sizeof(short), 1, fp);
    fwrite(&LabelLen_, sizeof(short), 1, fp);
}

void O_Output::SetFill()
{
    _SetOType_(_OUT_FILL_);

    SetInfo();
}

void O_Output::SetText(_TCHAR *txt)
{
    _SetOType_(_OUT_TEXT_);

    if (Label_ == NULL)
    {
        if (flags_ bitand C_BIT_FIXEDSIZE and LabelLen_ > 0)
#ifdef USE_SH_POOLS
            Label_ = (_TCHAR*)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(_TCHAR) * (LabelLen_), FALSE);

#else
            Label_ = new _TCHAR[LabelLen_];
#endif
    }

    if (txt)
    {
        if (flags_ bitand C_BIT_FIXEDSIZE)
        {
            _tcsncpy(Label_, txt, LabelLen_ - 1);
            Label_[LabelLen_ - 1] = 0;
        }
        else
            Label_ = txt;
    }
    else if (Label_ and (flags_ bitand C_BIT_FIXEDSIZE))
        memset(Label_, 0, sizeof(_TCHAR) * (LabelLen_));

    SetInfo();
}

int O_Output::FitString(int idx) // returns # characters to keep on this line
{
    // idx is the index into the start of the string
    int count, space;
    long w; 

    if ( not Label_[idx])
        return(0);

    w = WWWidth_;

    if ( not w)
        w = Owner_->GetW();

    if ( not w)
        return((short)_tcsclen(&Label_[idx]));    

    space = 0;
    count = 1;

    while (Label_[idx + count] and gFontList->StrWidth(Font_, &Label_[idx], (short)count) < w) 
    {
        if (Label_[idx + count] == ' ')
            space = count;

        // 2002-02-24 ADDED BY S.G. If we get a '\n', that's it for this line
        if (Label_[idx + count] == '\n')
        {
            space = count;
            break;
        }

        // END OF ADDED SECTION 2002-02-24
        count++;
    }

    if (gFontList->StrWidth(Font_, &Label_[idx], (short)count) < w) 
        return(count);

    if (space)
        return(space);

    if (count > 1)
        return((short)(count - 1)); 

    return(1);
}

void O_Output::WordWrap()
{
    int idx = 0, len = 0, fontheight = 0, linew = 0, maxw = 0;
    short count = 0, lenstr = 0;

    F4CSECTIONHANDLE *Leave = NULL;

    if (Owner_ and Owner_->Parent_)
        Leave = UI_Enter(Owner_->Parent_);

    if (Label_[0]) // pre-calc wordwrapping - 2 pass, 1st to figure out how many lines, 2nd to actually do it
    {
        idx = 0;
        count = 0;
        lenstr = (short)_tcsclen(Label_); 

        len = FitString(idx);

        while ((idx + len) < lenstr)
        {
            count++;
            idx += len;

            while (Label_[idx] == ' ' or Label_[idx] == '\n') // 2002-02-24 MODIFIED BY S.G. Skip '\n' as well since these will 'terminate' word wrapped line (like they should) and need to be skipped for the next one
                idx++;

            len = FitString(idx);
            linew = gFontList->StrWidth(Font_, &Label_[idx], len);

            if (linew > maxw)
                maxw = linew;
        }

        if (count)
        {
            count++;

            if (WWCount_ and WWCount_ not_eq count)
            {
                WWCount_ = 0;

                if (Wrap_)
                    delete Wrap_;

                Wrap_ = NULL;
            }

            WWCount_ = count;

            if ( not Wrap_)
                Wrap_ = new WORDWRAP[WWCount_];

            fontheight = gFontList->GetHeight(Font_);
            idx = 0;
            count = 0;
            len = FitString(idx);
            linew = gFontList->StrWidth(Font_, &Label_[idx], len);

            if (linew > maxw)
                maxw = linew;

            while ((idx + len) < lenstr)
            {
                Wrap_[count].Index = (short)idx; 
                Wrap_[count].Length = (short)len; 
                Wrap_[count].y = (short)(count * fontheight); 

                count++;
                idx += len;

                while (Label_[idx] == ' ')
                    idx++;

                len = FitString(idx);
                linew = gFontList->StrWidth(Font_, &Label_[idx], len);

                if (linew > maxw)
                    maxw = linew;
            }

            Wrap_[count].Index = (short)idx; 
            Wrap_[count].Length = (short)len; 
            Wrap_[count].y = (short)(count * fontheight); 
        }
        else
        {
            WWCount_ = 0;

            if (Wrap_)
                delete Wrap_;

            Wrap_ = NULL;
            count = 0;
            maxw = gFontList->StrWidth(Font_, Label_);
        }
    }

    SetWH(maxw, (count + 1)*gFontList->GetHeight(Font_));
    UI_Leave(Leave);
}

void O_Output::SetTextWidth(long w)
{
    _SetOType_(_OUT_TEXT_);

    if (Label_)
    {
        if (flags_ bitand C_BIT_FIXEDSIZE)
        {
            if (w == LabelLen_)
                return;

#ifdef USE_SH_POOLS
            MemFreePtr(Label_);
#else
            delete Label_;
#endif
        }
        else
        {
            MonoPrint("ERROR:Calling SetFixedWidth() when a string has already been assigned\n");
        }
    }

    LabelLen_ = static_cast<short>(w);
#ifdef USE_SH_POOLS
    Label_ = (_TCHAR*)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(_TCHAR) * (LabelLen_ + 1), FALSE);
#else
    Label_ = new _TCHAR[LabelLen_ + 1];
#endif
    memset(Label_, 0, sizeof(_TCHAR) * (LabelLen_));
    SetFlags(flags_ bitor C_BIT_FIXEDSIZE);
    SetInfo();
}

void O_Output::Cleanup()
{
    if (Label_)
    {
        if (flags_ bitand C_BIT_FIXEDSIZE)
#ifdef USE_SH_POOLS
            MemFreePtr(Label_);

#else
            delete Label_;
#endif
        Label_ = NULL;
        SetReady(0);
    }

    if (Image_)
        Image_ = NULL;

    if (Rows_)
    {
#ifdef USE_SH_POOLS
        MemFreePtr(Rows_);
#else
        delete Rows_;
#endif
        Rows_ = NULL;
    }

    if (Cols_)
    {
#ifdef USE_SH_POOLS
        MemFreePtr(Cols_);
#else
        delete Cols_;
#endif
        Cols_ = NULL;
    }

    WWCount_ = 0;

    if (Wrap_)
    {
        delete Wrap_;
        Wrap_ = NULL;
    }
}

void O_Output::SetInfo()
{
    switch (_GetOType_())
    {
        case _OUT_FILL_:
            SetReady(1);

            if (flags_ bitand C_BIT_HCENTER)
                x_ = (origx_ - (w_ >> 1));
            else if (flags_ bitand C_BIT_RIGHT)
                x_ = (origx_ - w_);
            else
                x_ = (origx_);

            if (flags_ bitand C_BIT_VCENTER)
                y_ = (origy_ - (h_ >> 1));
            else
                y_ = (origy_);

            break;

        case _OUT_TEXT_:
            if (Label_)
            {
                SetReady(1);

                if (GetFlags() bitand C_BIT_PASSWORD)
                    SetWH(gFontList->StrWidth(Font_, "*")*_tcsclen(Label_), gFontList->GetHeight(Font_));
                else
                    SetWH(gFontList->StrWidth(Font_, Label_), gFontList->GetHeight(Font_));

                if (Label_[0] and (flags_ bitand C_BIT_WORDWRAP) and Owner_ and GetW() > 50)
                    WordWrap(); // Sets WH internally

                if (flags_ bitand C_BIT_HCENTER)
                    x_ = (origx_ - (w_ >> 1));
                else if (flags_ bitand C_BIT_RIGHT)
                    x_ = (origx_ - w_);
                else
                    x_ = (origx_);

                if (flags_ bitand C_BIT_VCENTER)
                    y_ = (origy_ - (h_ >> 1));
                else
                    y_ = (origy_);
            }
            else
                SetReady(0);

            break;

        case _OUT_BITMAP_:
            if (Image_)
            {
                SetReady(1);

                if (flags_ bitand C_BIT_HCENTER)
                    x_ = (origx_ - Image_->Header->centerx);
                else if (flags_ bitand C_BIT_RIGHT)
                    x_ = (origx_ - w_);
                else
                    x_ = (origx_);

                if (flags_ bitand C_BIT_VCENTER)
                    y_ = (origy_ - Image_->Header->centery);
                else
                    y_ = (origy_);
            }
            else
                SetReady(0);

            break;

        case _OUT_SCALEBITMAP_:
            if (Image_ and ScaleSet_)
                SetReady(1);
            else
                SetReady(0);

            return; // centering Doesn't apply here
            break;

        case _OUT_ANIM_:
            if (Anim_)
            {
                SetReady(1);

                if (flags_ bitand C_BIT_HCENTER)
                    x_ = (origx_ - (w_ >> 1));
                else if (flags_ bitand C_BIT_RIGHT)
                    x_ = (origx_ - w_);
                else
                    x_ = (origx_);

                if (flags_ bitand C_BIT_VCENTER)
                    y_ = (origy_ - (h_ >> 1));
                else
                    y_ = (origy_);
            }
            else
                SetReady(0);

            break;
    }
}

void O_Output::Refresh()
{
    long x, y, w, h;

    if ( not Ready()) return;

    if (_GetOType_() == _OUT_SCALEBITMAP_)
    {
        Owner_->Parent_->update_ or_eq C_DRAW_REFRESH;
        Owner_->Parent_->SetUpdateRect(Owner_->Parent_->ClientArea_[Owner_->GetClient()].left,
                                       Owner_->Parent_->ClientArea_[Owner_->GetClient()].top,
                                       Owner_->Parent_->ClientArea_[Owner_->GetClient()].right,
                                       Owner_->Parent_->ClientArea_[Owner_->GetClient()].bottom,
                                       C_BIT_ABSOLUTE, Owner_->GetClient());
    }
    else
    {
        x = Owner_->GetX() + GetX();
        y = Owner_->GetY() + GetY();
        w = x + GetW();
        h = y + GetH();

        if (x < lastx_)
            lastx_ = x;

        if (y < lasty_)
            lasty_ = y;

        if (w > lastw_)
            lastw_ = w;

        if (h > lasth_)
            lasth_ = h;

        Owner_->Parent_->SetUpdateRect(lastx_, lasty_, lastw_, lasth_, Owner_->GetFlags(), Owner_->GetClient());
        lastx_ = x;
        lasty_ = y;
        lastw_ = w;
        lasty_ = h;
    }
}

long O_Output::GetCursorPos(long relx, long rely) // Based on mouse location
{
    C_Fontmgr *cur;
    unsigned long i, j; 
    long x, y, w;

    if (_GetOType_() not_eq _OUT_TEXT_)
        return(0);

    cur = gFontList->Find(Font_);

    if ( not cur)
        return(0);

    if (WWCount_ and (flags_ bitand C_BIT_WORDWRAP))
    {
        if (rely < 0)
            return(0);

        if (rely > (Wrap_[WWCount_ - 1].y + cur->Height()))
            return((short)_tcsclen(Label_));

        i = 0;

        while (rely >= (Wrap_[i].y + cur->Height()) and i < (unsigned long)WWCount_) 
            i++;

        j = 0;
        w = (cur->Width(&Label_[Wrap_[i].Index + j], 1) - 1) / 2;

        while (relx >= (cur->Width(&Label_[Wrap_[i].Index], j) - 1 + w) and j < (unsigned long)Wrap_[i].Length) 
        {
            j++;

            if (j < (unsigned long)Wrap_[i].Length)
                w = (cur->Width(&Label_[Wrap_[i].Index + j], 1) - 1) / 2;
        }

        return(short(Wrap_[i].Index + j));
    }
    else
    {
        x = 0;
        y = 0;

        if (rely < y)
            return(0);

        if (rely > y + cur->Height())
            return((short)_tcsclen(Label_));

        j = 0;
        w = (cur->Width(Label_, 1) - 1) / 2;

        while (relx >= (cur->Width(Label_, j) - 1 + x + w) and j < _tcsclen(Label_))
        {
            j++;

            if (j < _tcsclen(Label_))
                w = (cur->Width(&Label_[j], 1) - 1) / 2;
        }

        return(j);
    }

    return(0);
}

void O_Output::GetCharXY(short idx, long *cx, long *cy) // Based on cursor location
{
    C_Fontmgr *cur;
    short i;

    if (_GetOType_() not_eq _OUT_TEXT_)
        return;

    cur = gFontList->Find(Font_);

    if ( not cur)
        return;

    *cx = GetX();
    *cy = GetY();

    if (WWCount_ and (flags_ bitand C_BIT_WORDWRAP))
    {
        for (i = 0; i < WWCount_; i++)
        {
            if (idx <= Wrap_[i].Index + Wrap_[i].Length)
            {
                *cx = cur->Width(&Label_[Wrap_[i].Index], idx - Wrap_[i].Index);
                *cy = Wrap_[i].y;
                return;
            }
        }
    }
    else
    {
        if (GetFlags() bitand C_BIT_PASSWORD)
        {
            *cx += cur->Width("*") * idx;
        }
        else
            *cx += cur->Width(Label_, idx);
    }
}

void O_Output::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    if ( not Ready()) return;

    switch (_GetOType_())
    {
        case _OUT_FILL_:
            UI95_RECT src, dest;

            dest.left = Owner_->GetX() + GetX();
            dest.top = Owner_->GetY() + GetY();
            dest.right = dest.left + GetW();
            dest.bottom = dest.top + GetH();

            if ( not (Owner_->GetFlags() bitand C_BIT_ABSOLUTE))
            {
                dest.left += Owner_->Parent_->VX_[Owner_->GetClient()];
                dest.top += Owner_->Parent_->VY_[Owner_->GetClient()];
                dest.right += Owner_->Parent_->VX_[Owner_->GetClient()];
                dest.bottom += Owner_->Parent_->VY_[Owner_->GetClient()];
            }

            if ( not Owner_->Parent_->ClipToArea(&src, &dest, cliprect))
                return;

            if ( not (Owner_->GetFlags() bitand C_BIT_ABSOLUTE))
                if ( not Owner_->Parent_->ClipToArea(&src, &dest, &Owner_->Parent_->ClientArea_[Owner_->GetClient()]))
                    break;

            Owner_->Parent_->BlitFill(surface, FgColor_, &dest, C_BIT_ABSOLUTE, 0);
            break;

        case _OUT_TEXT_:
        {
            long x, y, origx, origy, i;
            long idx, len, lenout, nx;
            UI95_RECT rect, dummy;
            C_Fontmgr *cur;

            x = GetX() + Owner_->GetX();
            y = GetY() + Owner_->GetY();

            if (WWCount_ and (flags_ bitand C_BIT_WORDWRAP))
            {
                if ( not (Owner_->GetFlags() bitand C_BIT_ABSOLUTE))
                {
                    x += Owner_->Parent_->VX_[Owner_->GetClient()];
                    y += Owner_->Parent_->VY_[Owner_->GetClient()];
                }

                rect.left = x;
                rect.top = y;
                rect.right = rect.left + GetW();
                rect.bottom = rect.top + GetH();

                if ( not Owner_->Parent_->ClipToArea(&dummy, &rect, cliprect))
                    return;

                if ( not (Owner_->GetFlags() bitand C_BIT_ABSOLUTE))
                    if ( not Owner_->Parent_->ClipToArea(&dummy, &rect, &Owner_->Parent_->ClientArea_[Owner_->GetClient()]))
                        return;

                x += Owner_->Parent_->GetX();
                y += Owner_->Parent_->GetY();
                rect.left += Owner_->Parent_->GetX();
                rect.top += Owner_->Parent_->GetY();
                rect.right += Owner_->Parent_->GetX();
                rect.bottom += Owner_->Parent_->GetY();

                cur = gFontList->Find(Font_);

                if (cur)
                {
                    for (i = 0; i < WWCount_; i++)
                    {
                        if (GetFlags() bitand C_BIT_OPAQUE)
                        {
                            idx = Wrap_[i].Index;
                            len = Wrap_[i].Length;
                            nx = 0;
                            lenout = 0;

                            if (len and idx < OpStart_)
                            {
                                len = min(len, OpStart_ - idx);
                                cur->Draw(surface, &Label_[idx], len, UI95_RGB24Bit(FgColor_), x, y + Wrap_[i].y, &rect);
                                idx += len;
                                lenout += len;
                                len = Wrap_[i].Length - lenout;
                                nx = cur->Width(&Label_[Wrap_[i].Index], lenout) - 1;
                            }

                            if (len and idx < OpEnd_)
                            {
                                len = min(len, OpEnd_ - idx);
                                cur->DrawSolid(surface, &Label_[idx], len, UI95_RGB24Bit(FgColor_), UI95_RGB24Bit(BgColor_), x + nx, y + Wrap_[i].y, &rect);
                                idx += len;
                                lenout += len;
                                len = Wrap_[i].Length - lenout;
                                nx = cur->Width(&Label_[Wrap_[i].Index], lenout) - 1;
                            }

                            if (len)
                            {
                                cur->Draw(surface, &Label_[idx], len, UI95_RGB24Bit(FgColor_), x + nx, y + Wrap_[i].y, &rect);
                            }
                        }
                        else
                            cur->Draw(surface, &Label_[Wrap_[i].Index], Wrap_[i].Length, UI95_RGB24Bit(FgColor_), x, y + Wrap_[i].y, &rect);
                    }

                    if (flags_ bitand C_BIT_USELINE)
                    {
                        for (i = 0; i < WWCount_; i++)
                            Owner_->Parent_->DrawHLine(surface, FgColor_, GetX() + Owner_->GetX(), GetY() + Owner_->GetY() + Wrap_[i].y + cur->Height() - 1, Wrap_[i].Length, Owner_->GetFlags(), Owner_->GetClient(), cliprect);
                    }
                }
            }
            else
            {
                origx = x;
                origy = y;

                if ( not (Owner_->GetFlags() bitand C_BIT_ABSOLUTE))
                {
                    x += Owner_->Parent_->VX_[Owner_->GetClient()];
                    y += Owner_->Parent_->VY_[Owner_->GetClient()];
                }

                rect.left = x;
                rect.top = y;
                rect.right = rect.left + GetW();
                rect.bottom = rect.top + GetH();

                if ( not Owner_->Parent_->ClipToArea(&dummy, &rect, cliprect))
                    return;

                if ( not (Owner_->GetFlags() bitand C_BIT_ABSOLUTE))
                    if ( not Owner_->Parent_->ClipToArea(&dummy, &rect, &Owner_->Parent_->ClientArea_[Owner_->GetClient()]))
                        return;

                x += Owner_->Parent_->GetX();
                y += Owner_->Parent_->GetY();
                rect.left += Owner_->Parent_->GetX();
                rect.top += Owner_->Parent_->GetY();
                rect.right += Owner_->Parent_->GetX();
                rect.bottom += Owner_->Parent_->GetY();

                cur = gFontList->Find(Font_);

                if (cur)
                {
                    if (GetFlags() bitand C_BIT_PASSWORD) // Password... draw asterixs
                    {
                        memset(_password_, _T('*'), _tcsclen(Label_));

                        if (GetFlags() bitand C_BIT_OPAQUE)
                        {
                            idx = 0;
                            len = _tcsclen(_password_);
                            nx = 0;
                            lenout = 0;

                            if (len and idx < OpStart_)
                            {
                                len = min(len, OpStart_ - idx);
                                cur->Draw(surface, _password_, len, UI95_RGB24Bit(FgColor_), x, y, &rect);
                                idx += len;
                                lenout += len;
                                len = _tcsclen(_password_) - lenout;
                                nx = cur->Width(_password_, lenout) - 1;
                            }

                            if (len and idx < OpEnd_)
                            {
                                len = min(len, OpEnd_ - idx);
                                cur->DrawSolid(surface, &_password_[idx], len, UI95_RGB24Bit(FgColor_), UI95_RGB24Bit(BgColor_), x + nx, y, &rect);
                                idx += len;
                                lenout += len;
                                len = _tcsclen(_password_) - lenout;
                                nx = cur->Width(_password_, lenout) - 1;
                            }

                            if (len)
                            {
                                cur->Draw(surface, &_password_[idx], len, UI95_RGB24Bit(FgColor_), x + nx, y, &rect);
                            }
                        }
                        else
                            cur->Draw(surface, _password_, UI95_RGB24Bit(FgColor_), x, y, &rect);
                    }
                    else
                    {
                        if (GetFlags() bitand C_BIT_OPAQUE)
                        {
                            idx = 0;
                            len = _tcsclen(Label_);
                            nx = 0;
                            lenout = 0;

                            if (len and idx < OpStart_)
                            {
                                len = min(len, OpStart_ - idx);
                                cur->Draw(surface, Label_, len, UI95_RGB24Bit(FgColor_), x, y, &rect);
                                idx += len;
                                lenout += len;
                                len = _tcsclen(Label_) - lenout;
                                nx = cur->Width(Label_, lenout) - 1;
                            }

                            if (len and idx < OpEnd_)
                            {
                                len = min(len, OpEnd_ - idx);
                                cur->DrawSolid(surface, &Label_[idx], len, UI95_RGB24Bit(FgColor_), UI95_RGB24Bit(BgColor_), x + nx, y, &rect);
                                idx += len;
                                lenout += len;
                                len = _tcsclen(Label_) - lenout;
                                nx = cur->Width(Label_, lenout) - 1;
                            }

                            if (len)
                            {
                                cur->Draw(surface, &Label_[idx], len, UI95_RGB24Bit(FgColor_), x + nx, y, &rect);
                            }
                        }
                        else
                            cur->Draw(surface, Label_, UI95_RGB24Bit(FgColor_), x, y, &rect);
                    }

                    if (flags_ bitand C_BIT_USELINE)
                        Owner_->Parent_->DrawHLine(surface, FgColor_, origx, origy + cur->Height() - 1, cur->Width(Label_), Owner_->GetFlags(), Owner_->GetClient(), cliprect);
                }
            }
        }
        break;

        case _OUT_BITMAP_:
        {
            UI95_RECT src, dest;
            src.left = 0;
            src.top = 0;
            src.right = Image_->Header->w;
            src.bottom = Image_->Header->h;
            dest.left = Owner_->GetX() + GetX();
            dest.top = Owner_->GetY() + GetY();
            dest.right = dest.left + GetW();
            dest.bottom = dest.top + GetH();

            if ( not (Owner_->GetFlags() bitand C_BIT_ABSOLUTE))
            {
                dest.left += Owner_->Parent_->VX_[Owner_->GetClient()];
                dest.top += Owner_->Parent_->VY_[Owner_->GetClient()];
                dest.right += Owner_->Parent_->VX_[Owner_->GetClient()];
                dest.bottom += Owner_->Parent_->VY_[Owner_->GetClient()];
            }

            if ( not Owner_->Parent_->ClipToArea(&src, &dest, cliprect))
                return;

            if ( not (Owner_->GetFlags() bitand C_BIT_ABSOLUTE))
                if ( not Owner_->Parent_->ClipToArea(&src, &dest, &Owner_->Parent_->ClientArea_[Owner_->GetClient()]))
                    break;

            dest.left += Owner_->Parent_->GetX();
            dest.top += Owner_->Parent_->GetY();

            if (flags_ bitand C_BIT_TRANSLUCENT and fperc_ < 100)
                Image_->Blend(surface, src.left, src.top, src.right - src.left, src.bottom - src.top, dest.left, dest.top, fperc_, 100 - fperc_);
            else
                Image_->Blit(surface, src.left, src.top, src.right - src.left, src.bottom - src.top, dest.left, dest.top);
        }
        break;

        case _OUT_SCALEBITMAP_:
        {
            UI95_RECT dummy, clip;
            clip = *cliprect;

            if ( not (Owner_->GetFlags() bitand C_BIT_ABSOLUTE))
                if ( not Owner_->Parent_->ClipToArea(&dummy, &clip, &Owner_->Parent_->ClientArea_[Owner_->GetClient()]))
                    break;

            if (ScaleSet_ > 500)
                Image_->ScaleDown8(surface, Rows_, Cols_, clip.left + Owner_->Parent_->GetX(), clip.top + Owner_->Parent_->GetY(), (clip.right - clip.left), (clip.bottom - clip.top), Src_.left * 1000 / ScaleSet_ + (clip.left - Dest_.left), Src_.top * 1000 / ScaleSet_ + (clip.top - Dest_.top));
            else
                Image_->ScaleUp8(surface, &Rows_[clip.top - Dest_.top], &Cols_[clip.left - Dest_.left], clip.left + Owner_->Parent_->GetX(), clip.top + Owner_->Parent_->GetY(), (clip.right - clip.left), (clip.bottom - clip.top));
        }
        break;

        case _OUT_ANIM_:
        {
            UI95_RECT dest, src;
            long dx, dy;

            dest.left = Owner_->GetX() + GetX();
            dest.top = Owner_->GetY() + GetY();

            dest.right = dest.left + GetW();
            dest.bottom = dest.top + GetH();

            src.left = 0;
            src.top = 0;
            src.right = GetW();
            src.bottom = GetH();

            if ( not (Owner_->GetFlags() bitand C_BIT_ABSOLUTE))
            {
                dest.left += Owner_->Parent_->VX_[Owner_->GetClient()];
                dest.top += Owner_->Parent_->VY_[Owner_->GetClient()];
                dest.right += Owner_->Parent_->VX_[Owner_->GetClient()];
                dest.bottom += Owner_->Parent_->VY_[Owner_->GetClient()];
            }

            dx = dest.left + Owner_->Parent_->GetX();
            dy = dest.top + Owner_->Parent_->GetY();

            if ( not Owner_->Parent_->ClipToArea(&src, &dest, cliprect))
                return;

            if ( not (Owner_->GetFlags() bitand C_BIT_ABSOLUTE))
                if ( not Owner_->Parent_->ClipToArea(&src, &dest, &Owner_->Parent_->ClientArea_[Owner_->GetClient()]))
                    return;

            dest.left += Owner_->Parent_->GetX();
            dest.right += Owner_->Parent_->GetX();
            dest.top += Owner_->Parent_->GetY();
            dest.bottom += Owner_->Parent_->GetY();

            ExtractAnim(surface, frame_, dx, dy, &src, &dest);
        }
        break;
    }
}

void O_Output::Blend4Bit(SCREEN *surface, BYTE *overlay, WORD *Palette[], UI95_RECT *cliprect)
{
    UI95_RECT dummy, clip;
    clip = *cliprect;

    if ( not (Owner_->GetFlags() bitand C_BIT_ABSOLUTE))
        if ( not Owner_->Parent_->ClipToArea(&dummy, &clip, &Owner_->Parent_->ClientArea_[Owner_->GetClient()]))
            return;

    if (ScaleSet_ > 500)
        Image_->ScaleDown8Overlay(surface, Rows_, Cols_, clip.left + Owner_->Parent_->GetX(), clip.top + Owner_->Parent_->GetY(), (clip.right - clip.left), (clip.bottom - clip.top), Src_.left * 1000 / ScaleSet_ + (clip.left - Dest_.left), Src_.top * 1000 / ScaleSet_ + (clip.top - Dest_.top), overlay, Palette);
    else
        Image_->ScaleUp8Overlay(surface, &Rows_[clip.top - Dest_.top], &Cols_[clip.left - Dest_.left], clip.left + Owner_->Parent_->GetX(), clip.top + Owner_->Parent_->GetY(), (clip.right - clip.left), (clip.bottom - clip.top), overlay, Palette);
}

void O_Output::SetImage(long ID)
{
    IMAGE_RSC *image;

    image = gImageMgr->GetImage(ID);

    if (image == NULL)
    {
        if (ID > 0)
            MonoPrint("Image [%1ld] Not found in O_Output::SetImage(ID) Control=(%1ld)\n", ID, Owner_->GetID());

        SetReady(0);
        return;
    }

    SetImage(image);
}

void O_Output::SetImage(IMAGE_RSC *newimage)
{
    _SetOType_(_OUT_BITMAP_);

    Image_ = newimage;

    if (Image_ == NULL)
    {
        MonoPrint("Image Pointer is NULL in O_Output::SetImage(image*) Control=(%1ld)\n", Owner_->GetID());
        SetReady(0);
        return;
    }

    SetWH(Image_->Header->w, Image_->Header->h);
    SetInfo();
}

void O_Output::SetImagePtr(IMAGE_RSC *newimage)
{
    _SetOType_(_OUT_BITMAP_);

    Image_ = newimage;

    if (Image_ == NULL)
    {
        MonoPrint("Image Pointer is NULL in O_Output::SetImage(image*) Control=(%1ld)\n", Owner_->GetID());
        SetReady(0);
        return;
    }

    SetInfo();
}

void O_Output::SetScaleImage(long ID)
{
    IMAGE_RSC *image;

    image = gImageMgr->GetImage(ID);

    if (image == NULL)
    {
        if (ID > 0)
            MonoPrint("Image [%1ld] Not found in O_Output::SetImage(ID) Control=(%1ld)\n", ID, Owner_->GetID());

        SetReady(0);
        return;
    }

    SetScaleImage(image);
}

void O_Output::SetScaleImage(IMAGE_RSC *newimage)
{
    _SetOType_(_OUT_SCALEBITMAP_);

    Image_ = newimage;

    if (Image_ == NULL)
    {
        MonoPrint("Image Pointer is NULL in O_Output::SetImage(image*) Control=(%1ld)\n", Owner_->GetID());
        SetReady(0);
        return;
    }

    SetXY(0, 0);

    if (Rows_ == NULL)
    {
#ifdef USE_SH_POOLS
        Rows_ = (long*)MemAllocPtr(UI_Pools[UI_ART_POOL], sizeof(long) * (Image_->Header->h * 2), FALSE);
#else
        Rows_ = new long[Image_->Header->h * 2];
#endif
        memset(Rows_, 0, sizeof(long)*Image_->Header->h * 2);
    }

    if (Cols_ == NULL)
    {
#ifdef USE_SH_POOLS
        Cols_ = (long*)MemAllocPtr(UI_Pools[UI_ART_POOL], sizeof(long) * (Image_->Header->w * 2), FALSE);
#else
        Cols_ = new long[Image_->Header->w * 2];
#endif
        memset(Cols_, 0, sizeof(long)*Image_->Header->w * 2);
    }

    ScaleSet_ = 0;
    SetWH(Image_->Header->w, Image_->Header->h);
    SetInfo();
}

void O_Output::SetScaleInfo(long scale)
{
    long i, k, dd;
    long st, sb, sl, sr;

    if (scale > 500) // Shrink
    {
        if (scale == ScaleSet_)
            return;

        st = 0;
        sb = Image_->Header->h;
        sl = 0;
        sr = Image_->Header->w;
    }
    else // Grow
    {
        st = Src_.top;
        sb = Src_.bottom;
        sl = Src_.left;
        sr = Src_.right;
    }

    ScaleSet_ = scale;

    k = 0;
    dd = 0;

    for (i = st; i <= sb; i++)
    {
        dd -= 1000;

        while (dd <= 0)
        {
            if ( not F4IsBadWritePtr(&(Rows_[k + 1]), sizeof(short))) // JB 010304 CTD
                Rows_[k++] = i;

            dd += scale;
        }
    }

    k = 0;
    dd = 0;

    for (i = sl; i <= sr; i++)
    {
        dd -= 1000;

        while (dd <= 0)
        {
            if ( not F4IsBadWritePtr(&(Cols_[k + 1]), sizeof(short))) // JB 010304 CTD
                Cols_[k++] = i;

            dd += scale;
        }
    }

    if (Image_)
        SetReady(1);
    else
        SetReady(0);
}

void O_Output::SetAnim(long ID)
{
    ANIM_RES *anim;

    anim = gAnimMgr->GetAnim(ID);

    if (anim == NULL)
    {
        MonoPrint("Anim [%1ld] Not found in O_Output::SetAnim(ID) Control=(%1ld)\n", ID, Owner_->GetID());
        SetReady(0);
        return;
    }

    SetAnim(anim);
}

void O_Output::SetAnim(ANIM_RES *newanim)
{
    _SetOType_(_OUT_ANIM_);

    Anim_ = newanim;

    if (Anim_ == NULL)
    {
        MonoPrint("Anim Pointer is NULL in O_Output::SetAnim(image*) Control=(%1ld)\n", Owner_->GetID());
        SetReady(0);
        return;
    }

    SetWH(Anim_->Anim->Width, Anim_->Anim->Height);
    SetInfo();
}

void O_Output::ExtractAnim(SCREEN *surface, long FrameNo, long x, long y, UI95_RECT *src, UI95_RECT *dest)
{
    switch (Anim_->Anim->BytesPerPixel)
    {
        case 2:
            switch (Anim_->Anim->Compression)
            {
                case 0:
                    Extract16Bit(surface, FrameNo, x, y, src, dest);
                    break;

                case 1:
                case 2:
                case 3:
                case 4:
                    Extract16BitRLE(surface, FrameNo, x, y, src, dest);
                    break;
            }
    }
}

void O_Output::Extract16BitRLE(SCREEN *surface, long FrameNo, long destx, long desty, UI95_RECT *src, UI95_RECT *clip)
{
    src; //Unused

    ShiAssert(surface->bpp not_eq 32);  //XX

    long i, dx, dy, done;
    WORD Key, count;
    ANIM_FRAME *FramePtr;
    WORD *sptr;
    WORD *dptr, *lptr;

    i = 0;
    FramePtr = (ANIM_FRAME *)&Anim_->Anim->Start[0];

    while (i < FrameNo and i < Anim_->Anim->Frames)
    {
        FramePtr = (ANIM_FRAME *)&FramePtr->Data[FramePtr->Size];
        i++;
    }

    sptr = (WORD *)&FramePtr->Data[0];
    lptr = (WORD *)surface->mem;
    lptr += (desty * surface->width + destx);
    dptr = lptr;
    dx = destx;
    dy = desty;
    done = 0;

    while ( not done)
    {
        Key   = (WORD)(*sptr bitand RLE_KEYMASK);
        count = (WORD)(*sptr bitand RLE_COUNTMASK);
        sptr++;

        if (Key bitand RLE_END)
            done = 1;
        else if (dy < clip->top)
        {
            // go through compressed stuff, bitand don't do anything for output
            if ( not (Key bitand RLE_KEYMASK))
            {
                sptr += count;
            }
            else if (Key bitand RLE_REPEAT)
            {
                sptr++;
            }
            else if (Key bitand RLE_SKIPCOL)
            {
            }
            else if (Key bitand RLE_SKIPROW)
            {
                lptr += (count * surface->width);
                dptr = lptr;
                dx = destx;
                dy += count;
            }
        }
        else
        {
            if ( not (Key bitand RLE_KEYMASK))
            {
                while (count > 0)
                {
                    if (dx >= clip->left and dx < clip->right)
                    {
                        *dptr = *sptr;
                        dptr++;
                    }
                    else
                        dptr++;

                    dx++;
                    sptr++;
                    count--;
                }
            }
            else if (Key bitand RLE_REPEAT)
            {
                while (count > 0)
                {
                    if (dx >= clip->left and dx < clip->right)
                    {
                        *dptr = *sptr;
                        dptr++;
                    }
                    else
                        dptr++;

                    dx++;
                    count--;
                }

                sptr++;
            }
            else if (Key bitand RLE_SKIPCOL)
            {
                dptr += count;
                dx += count;
            }
            else if (Key bitand RLE_SKIPROW)
            {
                lptr += (count * surface->width);
                dptr = lptr;
                dx = destx;
                dy += count;
            }

            if (dy >= clip->bottom or dy >= surface->height)
                done = 1;
        }
    }
}

void O_Output::Extract16Bit(SCREEN *, long , long , long , UI95_RECT *, UI95_RECT *)
{
#if 0
    ANIM_FRAME *Frame;
    WORD *sptr;
    WORD *dptr;
    long i;

    i = 0;
    Frame = (ANIM_FRAME *)&Anim_->Anim->Start[0];
    dptr = (WORD *)Mem;

    while (i < FrameNo and i < Anim_->Anim->Frames)
    {
        Frame = (ANIM_FRAME *)&Frame->Data[Frame->Size];
        i++;
    }

    sptr = (WORD *)&Frame->Data[0];
    dptr += (y + dy * w) + x + dx;
    sptr += (dy * Anim_->Anim->Width) + dx;

    for (i = 0; i < dh; i++)
    {
        memmove(dptr, sptr, dw * Anim_->Anim->BytesPerPixel);
        dptr += w;
        sptr += Anim_->Anim->Width;
    }

#endif
}
