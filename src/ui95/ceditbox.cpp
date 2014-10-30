#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CEB_NOTHING = 0,
    CEB_SETUP,
    CEB_SETMAXLEN,
    CEB_SETTEXT,
    CEB_SETINTEGER,
    CEB_SETMININTEGER,
    CEB_SETMAXINTEGER,
    CEB_SETFLOAT,
    CEB_SETMINFLOAT,
    CEB_SETMAXFLOAT,
    CEB_SETBGIMAGE,
    CEB_SETFGCOLOR,
    CEB_SETBGCOLOR,
    CEB_SETCURSORCOLOR,
    CEB_SETDECIMALPLACES,
    CEB_SETOUTLINECOLOR,
};

char *C_Eb_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[MAXLEN]",
    "[STRING]",
    "[INTEGER]",
    "[MININTEGER]",
    "[MAXINTEGER]",
    "[FLOAT]",
    "[MINFLOAT]",
    "[MAXFLOAT]",
    "[BGIMAGE]",
    "[FGCOLOR]",
    "[BGCOLOR]",
    "[CURSORCOLOR]",
    "[DECIMALPLACES]",
    "[OUTLINECOLOR]",
    0,
};

#endif

static _TCHAR _FilenameExclude_[] = ":\\./<>+*?;,\"|";

C_EditBox::C_EditBox() : C_Control()
{
    _SetCType_(_CNTL_EDITBOX_);
    Integer_ = 0;
    MinInteger_ = 0;
    MaxInteger_ = 0;
    Float_ = 0.0f;
    MinFloat_ = 0.0f;
    MaxFloat_ = 0.0f;
    MaxLen_ = 0;
    Start_ = 0;
    End_ = 0;
    Decimal_ = 2;
    Cursor_ = 0;
    Text_ = NULL;
    JustActivated_ = 0;
    SelStart_ = 0;
    SelEnd_ = 0;
    NoChanges_ = TRUE;
    CursorColor_ = 0xeeeeee;
    OutlineColor_ = 0x00ff00;
    UseCursor_ = 0;
    BgImage_ = NULL;
    OrigText_ = NULL;
    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE bitor C_BIT_SELECTABLE bitor C_BIT_MOUSEOVER;

    Font_ = NULL; // OW
    Text_ = NULL; // OW
}

C_EditBox::C_EditBox(char **stream) : C_Control(stream)
{
}

C_EditBox::C_EditBox(FILE *fp) : C_Control(fp)
{
}

C_EditBox::~C_EditBox()
{
}

long C_EditBox::Size()
{
    return(0);
}

void C_EditBox::Setup(long ID, short Type)
{
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();

    if (Text_ == NULL)
    {
        Text_ = new O_Output;
        Text_->SetOwner(this);
        Text_->SetFlags(GetFlags());
    }

    SetReady(1);
}

void C_EditBox::Cleanup(void)
{
    if (BgImage_)
    {
        BgImage_->Cleanup();
        delete BgImage_;
        BgImage_ = NULL;
    }

    if (Text_)
    {
        Text_->Cleanup();
        delete Text_;
        Text_ = NULL;
    }

    if (OrigText_)
    {
        delete [] OrigText_;
        OrigText_ = NULL;
    }
}

void C_EditBox::SetFont(long Font)
{
    Font_ = Font;

    if (Text_)
        Text_->SetFont(Font);
}

void C_EditBox::DeleteRange()
{
    int i, w; 
    _TCHAR *EditText_;

    NoChanges_ = 0;
    w = End_ - Start_;

    if (w > 0)
    {
        i = 0;
        EditText_ = Text_->GetText();

        for (i = Start_; i < MaxLen_; i++)
        {
            if ((i + w) < MaxLen_)
                EditText_[i] = EditText_[i + w];
            else
                EditText_[i] = 0;
        }

        End_ = Start_;
        Cursor_ = Start_;
        Text_->SetFlags(Text_->GetFlags() bitand compl C_BIT_OPAQUE);
        Refresh();
    }
}

long C_EditBox::CheckHotSpots(long relX, long relY)
{
    if (GetFlags() bitand C_BIT_INVISIBLE or not (GetFlags() bitand C_BIT_ENABLED))
        return(0);

    if (relX >= GetX() and relX < (GetX() + GetW()) and relY >= GetY() and relY < (GetY() + GetH()))
    {
        SetRelXY(relX - GetX(), relY - GetY());
        return(GetID());
    }

    return(0);
}

BOOL C_EditBox::CheckKeyboard(unsigned char DKScanCode, unsigned char Ascii, unsigned char ShiftStates, long)
{
    if (Ascii)
        return(CheckChar(Ascii));

    return(CheckKeyDown(DKScanCode, ShiftStates));
}

BOOL C_EditBox::CheckKeyDown(unsigned char key, unsigned char)
{
    int i; 
    _TCHAR *EditText_;

    if (Text_ == NULL)
        return(FALSE);

    EditText_ = Text_->GetText();

    switch (key)
    {
        case DIK_NUMPAD6:
        case DIK_RIGHT:
            NoChanges_ = 0;
            Start_ = Cursor_;
            End_ = Cursor_;
            Text_->SetFlags(Text_->GetFlags() bitand compl C_BIT_OPAQUE);

            if (Cursor_ < MaxLen_ and EditText_[Cursor_])
            {
                Cursor_++;
            }

            Refresh();
            return(TRUE);
            break;

        case DIK_NUMPAD4:
        case DIK_LEFT:
            NoChanges_ = 0;
            Start_ = Cursor_;
            End_ = Cursor_;
            Text_->SetFlags(Text_->GetFlags() bitand compl C_BIT_OPAQUE);

            if (Cursor_ > 0)
            {
                Cursor_--;
            }

            Refresh();
            return(TRUE);
            break;

        case DIK_NUMPAD0:
        case DIK_INSERT:
            Refresh();
            return(TRUE);
            break;

        case DIK_NUMPAD7:
        case DIK_HOME:
            NoChanges_ = 0;
            Start_ = Cursor_;
            End_ = Cursor_;
            Text_->SetFlags(Text_->GetFlags() bitand compl C_BIT_OPAQUE);
            Cursor_ = 0;
            Refresh();
            return(TRUE);
            break;

        case DIK_NUMPAD1:
        case DIK_END:
            NoChanges_ = 0;
            Start_ = Cursor_;
            End_ = Cursor_;
            Text_->SetFlags(Text_->GetFlags() bitand compl C_BIT_OPAQUE);

            while (EditText_[Cursor_] and Cursor_ < (MaxLen_))
                Cursor_++;

            Refresh();
            return(TRUE);
            break;

        case DIK_DECIMAL:
        case DIK_DELETE:
            NoChanges_ = 0;

            if (End_ > Start_)
            {
                DeleteRange();
                return(TRUE);
            }

            if (Cursor_ >= (MaxLen_))
                break;

            if (EditText_[Cursor_])
            {
                i = Cursor_ + 1;

                if (i <= MaxLen_)
                {
                    while (EditText_[i] and i < MaxLen_)
                    {
                        EditText_[i - 1] = EditText_[i];
                        i++;
                    }

                    EditText_[i - 1] = 0;
                    Text_->SetInfo();
                    Refresh();
                    return(TRUE);
                }
            }

            break;

        case DIK_BACK:
            NoChanges_ = 0;

            if (End_ > Start_)
            {
                DeleteRange();
                return(TRUE);
            }

            if (Cursor_ > 0)
            {
                Cursor_--;

                for (i = Cursor_; i < MaxLen_; i++)
                    EditText_[i] = EditText_[i + 1];

                EditText_[MaxLen_] = 0;
                Text_->SetInfo();
                Refresh();
                return(TRUE);
            }

            break;

        case DIK_NUMPADENTER:
        case DIK_RETURN:
            Cursor_ = 0;
            CopyFromText();
            Text_->SetFlags(Text_->GetFlags() bitand compl C_BIT_OPAQUE);

            if (Callback_)
            {
                // TODO create C_TYPE_RETURN_HIT
                Callback_(GetID(), key, this);
            }

            if ( not NoChanges_)
            {
                Activate();
                return(TRUE);
            }

            return(FALSE);
            break;

        case DIK_ESCAPE:
            Cursor_ = 0;
            CopyToText();
            Refresh();
            Text_->SetFlags(Text_->GetFlags() bitand compl C_BIT_OPAQUE);

            if (Callback_)
            {
                // TODO create C_TYPE_ESCAPE_HIT
                Callback_(GetID(), key, this);
            }

            if ( not NoChanges_)
            {
                Activate();
                return(TRUE);
            }

            return(FALSE);
            break;
    }

    return(FALSE);
}

BOOL C_EditBox::CheckChar(unsigned char key)
{
    int i;
    _TCHAR *EditText_;

    if (Text_ == NULL)
        return(FALSE);

    EditText_ = Text_->GetText();

    if (key < 0x20)
        return(FALSE);

    switch (GetType())
    {
        case C_TYPE_IPADDRESS:
            if ( not isdigit(key))
            {
                if (key == '.' and Parent_)
                    Parent_->SetNextControl();

                return FALSE;
            }

            break;

        case C_TYPE_INTEGER:
            if ( not isdigit(key) and key not_eq _T('-'))
                return(FALSE);

            break;

        case C_TYPE_FILENAME:
            for (i = 0; i < (short)_tcsclen(_FilenameExclude_); i++) 
                if (key == _FilenameExclude_[i])
                    return(FALSE);

            break;
    }

    NoChanges_ = 0;

    if (Cursor_ < (MaxLen_))
    {
        if (End_ > Start_)
            DeleteRange();

        if (Parent_->KeyboardMode())
        {
            // move rest of chars to the right
            for (i = MaxLen_ - 2; i >= Cursor_; i--)
                EditText_[i + 1] = EditText_[i];

            EditText_[MaxLen_] = 0;
        }

        if ( not EditText_[Cursor_])
            EditText_[Cursor_ + 1] = 0;

        EditText_[Cursor_] = (_TCHAR)key;
        Cursor_++;
        CopyFromText();
    }

    Text_->SetInfo();
    Refresh();
    return(TRUE);
}

BOOL C_EditBox::Process(long, short HitType)
{
    gSoundMgr->PlaySound(GetSound(HitType));

    switch (HitType)
    {
        case C_TYPE_LMOUSEDOWN:
            SelStart_ = (short)Text_->GetCursorPos(GetRelX() - Text_->GetX(), GetRelY() - Text_->GetY()); 
            SelEnd_ = SelStart_;
            break;

        case C_TYPE_LMOUSEUP:
            if (JustActivated_)
            {
                JustActivated_ = FALSE;
                break;
            }

            NoChanges_ = 0;
            Cursor_ = (short)Text_->GetCursorPos(GetRelX() - Text_->GetX(), GetRelY() - Text_->GetY()); 
            Start_ = Cursor_;
            End_ = Cursor_;
            Text_->SetFlags(Text_->GetFlags() bitand compl C_BIT_OPAQUE);
            Text_->SetOpaqueRange(Start_, End_);
            Refresh();
            break;

        case C_TYPE_LDROP:
            break;
    }

    return(TRUE);
}

void C_EditBox::Refresh()
{
    if ( not Ready() or (Flags_ bitand C_BIT_INVISIBLE) or Parent_ == NULL)
        return;

    Parent_->SetUpdateRect(GetX(), GetY(), GetX() + GetW(), GetY() + GetH(), GetFlags(), GetClient());
}

void C_EditBox::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    long x, y, h;

    if (GetFlags() bitand C_BIT_INVISIBLE)
        return;

    if (GetFlags() bitand C_BIT_USEBGIMAGE)
        BgImage_->Draw(surface, cliprect);


    if (Text_)
    {
        if (GetType() == C_TYPE_PASSWORD)
            Text_->SetFlags((Text_->GetFlags() bitor C_BIT_PASSWORD) bitand compl C_BIT_OPAQUE);

        Text_->Draw(surface, cliprect);

        if (UseCursor_ and (End_ <= Start_))
        {
            h = gFontList->GetHeight(Font_);
            Text_->GetCharXY(Cursor_, &x, &y);
            x += GetX();
            y += GetY();

            Parent_->BlitFill(surface, CursorColor_, x - 2, y + 2, 2, h - 3, GetFlags(), GetClient(), cliprect);
        }

        if (GetFlags() bitand C_BIT_USEOUTLINE) // Kludge for outline
        {
            Parent_->DrawHLine(surface, OutlineColor_, GetX(), GetY(), GetW(), GetFlags(), GetClient(), cliprect);
            Parent_->DrawHLine(surface, OutlineColor_, GetX(), GetY() + GetH() - 1, GetW(), GetFlags(), GetClient(), cliprect);
            Parent_->DrawVLine(surface, OutlineColor_, GetX(), GetY(), GetH() - 1, GetFlags(), GetClient(), cliprect);
            Parent_->DrawVLine(surface, OutlineColor_, GetX() + GetW() - 1, GetY(), GetH() - 1, GetFlags(), GetClient(), cliprect);
        }
    }

    if (MouseOver_ or (GetFlags() bitand C_BIT_FORCEMOUSEOVER))
        HighLite(surface, cliprect);
}

void C_EditBox::HighLite(SCREEN *surface, UI95_RECT *cliprect)
{
    UI95_RECT clip, tmp;

    clip.left = GetX();
    clip.top = GetY();

    if ( not (Flags_ bitand C_BIT_ABSOLUTE))
    {
        clip.left += Parent_->VX_[Client_];
        clip.top += Parent_->VY_[Client_];
    }

    clip.right = clip.left + GetW();
    clip.bottom = clip.top + GetH();
    tmp = clip; // JPO - just so its initialised.

    if ( not Parent_->ClipToArea(&tmp, &clip, cliprect))
        return;

    if ( not (Flags_ bitand C_BIT_ABSOLUTE))
        if ( not Parent_->ClipToArea(&tmp, &clip, &Parent_->ClientArea_[Client_]))
            return;

    Parent_->BlitTranslucent(surface, MouseOverColor_, MouseOverPercent_, &clip, C_BIT_ABSOLUTE, 0);
}

void C_EditBox::SetBGImage(long ImageID)
{
    if (gImageMgr == NULL)
        return;

    if (BgImage_ == NULL)
    {
        BgImage_ = new O_Output;
        BgImage_->SetOwner(this);
    }

    BgImage_->SetFlags(GetFlags());
    BgImage_->SetImage(ImageID);
    SetWH(BgImage_->GetW(), BgImage_->GetH());
    SetFlagBitOn(C_BIT_USEBGIMAGE);
}

void C_EditBox::SetSubParents(C_Window *)
{
    if (BgImage_)
        BgImage_->SetInfo();

    if (Text_)
    {
        if (GetType() == C_TYPE_INTEGER or GetType() == C_TYPE_FLOAT or GetType() == C_TYPE_IPADDRESS)
            CopyToText();

        Text_->SetFlags(GetFlags()& compl C_BIT_OPAQUE);

        if (GetFlags() bitand C_BIT_VCENTER)
            Text_->SetY(GetH() / 2);

        if (GetFlags() bitand C_BIT_RIGHT)
            Text_->SetX(GetW() - 2);
        else if (GetFlags() bitand C_BIT_HCENTER)
            Text_->SetX(GetW() / 2);
        else
            Text_->SetX(1);

        Text_->SetInfo();
    }
}

void C_EditBox::SetMaxLen(short len)
{
    if (len > 0)
    {
        if (Text_ == NULL)
        {
            Text_ = new O_Output;
            Text_->SetOwner(this);
        }

        if (GetType() == C_TYPE_TEXT)
        {
            if (OrigText_)
                delete [] OrigText_;
        }

        SetFlags(GetFlags() bitor C_BIT_FIXEDSIZE);
        MaxLen_ = len;
        Text_->SetFlags(GetFlags()& compl C_BIT_OPAQUE);
        Text_->SetFont(Font_);
        Text_->SetTextWidth((short)(MaxLen_ + 1)); 

        if (GetType() == C_TYPE_TEXT)
        {
            OrigText_ = new _TCHAR[MaxLen_ + 1];
            memset(OrigText_, 0, sizeof(TCHAR) * (MaxLen_ + 1));
        }
    }
}

void C_EditBox::SetText(_TCHAR *str)
{
    if (Text_)
        Text_->SetText(str);

    Text_->SetFont(Font_);
    Text_->SetFlags(GetFlags()& compl C_BIT_OPAQUE);

    if (GetType() == C_TYPE_TEXT)
    {
        if ( not OrigText_)
            OrigText_ = new _TCHAR[MaxLen_ + 1];

        _tcsncpy(OrigText_, Text_->GetText(), MaxLen_);
    }

    if (GetFlags() bitand C_BIT_VCENTER)
        Text_->SetY(GetH() / 2);
}

void C_EditBox::SetText(long txtID)
{
    SetText(gStringMgr->GetString(txtID));
}

_TCHAR *C_EditBox::GetText()
{
    if (Text_)
        return(Text_->GetText());

    return(NULL);
}

void C_EditBox::SetInteger(long value)
{
    if (MinInteger_ not_eq MaxInteger_)
    {
        if (value < MinInteger_)
            value = MinInteger_;

        if (value > MaxInteger_)
            value = MaxInteger_;
    }

    if (Integer_ not_eq value)
    {
        Integer_ = value;
        CopyToText();
    }
}

void C_EditBox::SetFloat(double value)
{
    if (MinFloat_ not_eq MaxFloat_)
    {
        if (value < MinFloat_)
            value = MinFloat_;

        if (value > MaxFloat_)
            value = MaxFloat_;
    }

    if (Float_ not_eq value)
    {
        Float_ = value;
        CopyToText();
    }
}

void C_EditBox::SetFgColor(COLORREF color)
{
    if (Text_)
        Text_->SetFgColor(color);
}

void C_EditBox::SetBgColor(COLORREF color)
{
    if (Text_)
        Text_->SetBgColor(color);
}

void C_EditBox::CopyToText()
{
    _TCHAR buffer[40];

    switch (GetType())
    {
        case C_TYPE_INTEGER:
        case C_TYPE_IPADDRESS:
            Refresh();

            if (GetFlags() bitand C_BIT_LEADINGZEROS)
                _stprintf(buffer, "%0*ld", MaxLen_, Integer_);
            else
                _stprintf(buffer, "%1ld", Integer_);

            Text_->SetText(buffer);
            Refresh();
            break;

        case C_TYPE_FLOAT:
            Refresh();

            if (GetFlags() bitand C_BIT_LEADINGZEROS)
                _stprintf(buffer, "%0*.*lf", MaxLen_, Decimal_, Float_);
            else
                _stprintf(buffer, "%.*lf", Decimal_, Float_);

            Text_->SetText(buffer);
            Refresh();
            break;

        case C_TYPE_TEXT:
            if (OrigText_)
                Text_->SetText(OrigText_);

            break;
    }
}

void C_EditBox::CopyFromText()
{
    switch (GetType())
    {
        case C_TYPE_INTEGER:
        case C_TYPE_IPADDRESS:
            Integer_ = atol(Text_->GetText());

            if (MinInteger_ or MaxInteger_)
            {
                if (Integer_ < MinInteger_)
                {
                    Integer_ = MinInteger_;
                    CopyToText();
                }
                else if (Integer_ > MaxInteger_)
                {
                    Integer_ = MaxInteger_;
                    CopyToText();
                }
            }

            break;

        case C_TYPE_FLOAT:
            Float_ = atof(Text_->GetText());

            if (MinFloat_ or MaxFloat_)
            {
                if (Float_ < MinFloat_)
                {
                    Float_ = MinFloat_;
                    CopyToText();
                }
                else if (Float_ > MaxFloat_)
                {
                    Float_ = MaxFloat_;
                    CopyToText();
                }
            }

            break;

        case C_TYPE_TEXT:
            if (OrigText_)
                _tcsncpy(OrigText_, Text_->GetText(), MaxLen_);

            break;
    }
}

void C_EditBox::Activate()
{
    UseCursor_ = 1;
    CopyToText();
    NoChanges_ = 1;
    Start_ = 0;
    End_ = (short)_tcsclen(Text_->GetText()); 
    Text_->SetOpaqueRange(Start_, End_);
    Text_->SetFlags(Text_->GetFlags() bitor C_BIT_OPAQUE);
    Cursor_ = 0;
    JustActivated_ = TRUE;
    Refresh();

    if (Callback_)
    {
        // use callback 1
        // TODO create an C_TYPE_ACTIVATE
        Callback_(GetID(), 1, this);
    }
}

void C_EditBox::Deactivate()
{
    UseCursor_ = 0;
    Cursor_ = 0;
    CopyFromText();
    Text_->SetFlags(Text_->GetFlags() bitand compl C_BIT_OPAQUE);
    Text_->SetInfo();
    Refresh();

    if (Callback_)
    {
        // use callback 0
        // TODO create a C_TYPE_DEACTIVATE
        Callback_(GetID(), 0, this);
    }
}

BOOL C_EditBox::Drag(GRABBER *, WORD MouseX, WORD MouseY, C_Window *)
{
    long relx, rely;

    if (JustActivated_)
    {
        JustActivated_ = FALSE;
        return(FALSE);
    }

    relx = MouseX - Parent_->GetX() - GetX() - Text_->GetX();
    rely = MouseY - Parent_->GetY() - GetY() - Text_->GetY();

    SelEnd_ = (short)Text_->GetCursorPos(relx, rely); 

    if (SelStart_ not_eq SelEnd_)
    {
        Text_->SetFlags(Text_->GetFlags() bitor C_BIT_OPAQUE);

        if (SelStart_ < SelEnd_)
            Text_->SetOpaqueRange(SelStart_, SelEnd_);
        else
            Text_->SetOpaqueRange(SelEnd_, SelStart_);
    }
    else
        Text_->SetFlags(Text_->GetFlags() bitand compl C_BIT_OPAQUE);

    NoChanges_ = 0;
    Refresh();
    return(TRUE);
}

BOOL C_EditBox::Drop(GRABBER *, WORD , WORD , C_Window *)
{
    if (SelStart_ < SelEnd_)
    {
        Start_ = SelStart_;
        End_ = SelEnd_;
    }
    else
    {
        Start_ = SelEnd_;
        End_ = SelStart_;
    }

    if (Start_ < End_)
    {
        Text_->SetOpaqueRange(Start_, End_);
        Text_->SetFlags(Text_->GetFlags() bitor C_BIT_OPAQUE);
    }
    else
        Text_->SetFlags(Text_->GetFlags() bitand compl C_BIT_OPAQUE);

    NoChanges_ = 0;
    Refresh();
    return(TRUE);
}

#ifdef _UI95_PARSER_

short C_EditBox::LocalFind(char *token)
{
    short i = 0;

    while (C_Eb_Tokens[i])
    {
        if (strnicmp(token, C_Eb_Tokens[i], strlen(C_Eb_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_EditBox::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CEB_SETUP:
            Setup(P[0], (short)P[1]);
            break;

        case CEB_SETMAXLEN:
            SetMaxLen((short)P[0]);
            break;

        case CEB_SETTEXT:
            SetText(P[0]);
            break;

        case CEB_SETINTEGER:
            SetInteger(P[0]);
            break;

        case CEB_SETMININTEGER:
            SetMinInteger(P[0]);
            break;

        case CEB_SETMAXINTEGER:
            SetMaxInteger(P[0]);
            break;

        case CEB_SETFLOAT:
            SetFloat((double)P[0]);
            break;

        case CEB_SETMINFLOAT:
            SetMinFloat((double)P[0]);
            break;

        case CEB_SETMAXFLOAT:
            SetMaxFloat((double)P[0]);
            break;

        case CEB_SETBGIMAGE:
            SetBGImage(P[0]);
            break;

        case CEB_SETFGCOLOR:
            SetFgColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CEB_SETBGCOLOR:
            SetBgColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CEB_SETCURSORCOLOR:
            SetCursorColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CEB_SETDECIMALPLACES:
            SetDecimalPlaces((short)P[0]);
            break;

        case CEB_SETOUTLINECOLOR:
            SetOutlineColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;
    }
}

extern char ParseSave[];
extern char ParseCRLF[];

#endif // PARSER
