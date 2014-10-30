#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CBTN_NOTHING = 0,
    CBTN_SETUP,
    CBTN_SETHOTSPOT,
    CBTN_SETBACKIMAGE,
    CBTN_SETIMAGES,
    CBTN_SETSTATE,
    CBTN_SETLABEL,
    CBTN_SETALLLABEL,
    CBTN_SETUPCOLOR,
    CBTN_SETDOWNCOLOR,
    CBTN_SETSELCOLOR,
    CBTN_SETDISCOLOR,
    CBTN_SETBUTTONIMAGE,
    CBTN_SETBUTTONANIM,
    CBTN_SETTEXTOFFSET,
    CBTN_SETTEXTCOLOR,
    CBTN_SETBUTTONTEXT,
    CBTN_SETFILL,
    CBTN_SETPERCENT,
    CBTN_SETBUTTONCOLOR,
    CBTN_TEXTFLAGON,
    CBTN_TEXTFLAGOFF,
    CBTN_FIXED_HOTSPOT,
};

char *C_Btn_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[HOTSPOT]",
    "[BACKIMAGE]",
    "[IMAGES]",
    "[STATE]",
    "[LABEL]",
    "[LABELALL]",
    "[UPCOLOR]",
    "[DOWNCOLOR]",
    "[SELCOLOR]",
    "[DISCOLOR]",
    "[BUTTONIMAGE]",
    "[BUTTONANIM]",
    "[TEXTOFFSET]",
    "[TEXTCOLOR]",
    "[BUTTONTEXT]",
    "[BUTTONFILL]",
    "[PERCENT]",
    "[BUTTONCOLOR]",
    "[LABELFLAGBITON]",
    "[LABELFLAGBITOFF]",
    "[FIXEDHOTSPOT]",
    0,
};

#endif

static void ButtonCleanupCB(void *rec)
{
    BUTTONLIST *btn;

    if ( not rec)
        return;

    btn = (BUTTONLIST*)rec;

    if (btn->Image_)
    {
        btn->Image_->Cleanup();
        delete btn->Image_;
    }

    if (btn->Label_)
    {
        btn->Label_->Cleanup();
        delete btn->Label_;
    }

    delete btn;
}

C_Button::C_Button() : C_Control()
{
    _SetCType_(_CNTL_BUTTON_);
    SetReady(0);

    origx_ = 0;
    origy_ = 0;
    LabelFlags_ = C_BIT_HCENTER bitor C_BIT_VCENTER;
    HotSpot_.left = 0;
    HotSpot_.top = 0;
    HotSpot_.right = 0;
    HotSpot_.bottom = 0;
    Percent_ = 50;
    UseHotSpot_ = 0;
    state_ = 0;
    laststate_ = 0;
    HotKey_ = 0;
    Font_ = 0;
    FixedHotSpot_ = 0;
    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE bitor C_BIT_SELECTABLE bitor C_BIT_MOUSEOVER;

    Root_ = NULL;
    BgImage_ = NULL;
    Owner_ = NULL;
}

C_Button::C_Button(char **stream) : C_Control(stream)
{
}

C_Button::C_Button(FILE *fp) : C_Control(fp)
{
}

C_Button::~C_Button(void)
{
    Cleanup();
}

long C_Button::Size()
{
    return(0);
}

void C_Button::Setup(long ID, short Type, long x, long y)
{
    SetID(ID);
    SetType(Type);
    origx_ = x;
    origy_ = y;
    SetXY(x, y);
    Root_ = new C_Hash;
    Root_->Setup(1);
    Root_->SetFlags(C_BIT_REMOVE);
    Root_->SetCallback(ButtonCleanupCB);
    SetDefaultFlags();
}

void C_Button::SetXY(long x, long y)
{
    origx_ = x;
    origy_ = y;
    x_ = x;
    y_ = y;
}

void C_Button::Cleanup()
{
    if (BgImage_)
    {
        BgImage_->Cleanup();
        delete BgImage_;
        BgImage_ = NULL;
    }

    if (Root_)
    {
        Root_->Cleanup();
        delete Root_;
        Root_ = NULL;
    }
}

void C_Button::SetBackImage(long ImageID)
{
    if (BgImage_ == NULL)
    {
        BgImage_ = new O_Output;
        BgImage_->SetOwner(this);
        BgImage_->SetFlags(GetFlags());
    }

    SetFlagBitOn(C_BIT_USEBGIMAGE);
    BgImage_->SetImage(ImageID);
}

void C_Button::SetFlags(long flags)
{
    SetControlFlags(flags);

    if (BgImage_)
        BgImage_->SetFlags(flags);
}

void C_Button::SetLabelFlagBitsOn(long flags)
{
    LabelFlags_ or_eq flags;

    // Mutually exclusive flags...
    if (flags bitand C_BIT_TOP)
        LabelFlags_ and_eq compl (C_BIT_VCENTER bitor C_BIT_BOTTOM);

    if (flags bitand C_BIT_BOTTOM)
        LabelFlags_ and_eq compl (C_BIT_VCENTER bitor C_BIT_TOP);

    if (flags bitand C_BIT_VCENTER)
        LabelFlags_ and_eq compl (C_BIT_TOP bitor C_BIT_BOTTOM);

    if (flags bitand C_BIT_LEFT)
        LabelFlags_ and_eq compl (C_BIT_HCENTER bitor C_BIT_RIGHT);

    if (flags bitand C_BIT_RIGHT)
        LabelFlags_ and_eq compl (C_BIT_HCENTER bitor C_BIT_LEFT);

    if (flags bitand C_BIT_HCENTER)
        LabelFlags_ and_eq compl (C_BIT_LEFT bitor C_BIT_RIGHT);

    SetLabelInfo();
}

void C_Button::SetLabelFlagBitsOff(long flags)
{
    LabelFlags_ and_eq compl flags;
    SetLabelInfo();
}

void C_Button::SetLabel(long ID, _TCHAR *str)
{
    F4CSECTIONHANDLE* Leave;
    BUTTONLIST *btn;

    if ( not Root_ or not str)
        return;

    btn = (BUTTONLIST*)Root_->Find(ID);

    if (btn)
    {
        Leave = UI_Enter(Parent_);

        if ( not btn->Label_)
        {
            btn->Label_ = new O_Output;
            btn->Label_->SetOwner(this);
            btn->Label_->SetFont(Font_);
            btn->Label_->SetFlags(LabelFlags_);
            btn->Label_->SetFgColor(btn->FgColor_);
            btn->Label_->SetBgColor(btn->BgColor_);
        }

        if (btn->Label_)
        {
            btn->Label_->SetText(gStringMgr->GetText(gStringMgr->AddText(str)));
            btn->Label_->SetInfo();
        }

        UI_Leave(Leave);
    }
}

void C_Button::SetLabel(long ID, long txtID)
{
    F4CSECTIONHANDLE* Leave;
    BUTTONLIST *btn;

    if ( not Root_)
        return;

    btn = (BUTTONLIST*)Root_->Find(ID);

    if (btn)
    {
        Leave = UI_Enter(Parent_);

        if ( not btn->Label_)
        {
            btn->Label_ = new O_Output;
            btn->Label_->SetOwner(this);
            btn->Label_->SetFont(Font_);
            btn->Label_->SetFlags(LabelFlags_);
            btn->Label_->SetFgColor(btn->FgColor_);
            btn->Label_->SetBgColor(btn->BgColor_);
        }

        if (btn->Label_)
        {
            btn->Label_->SetText(gStringMgr->GetString(txtID));
            btn->Label_->SetInfo();
        }

        UI_Leave(Leave);
    }
}

_TCHAR *C_Button::GetText(short ID)
{
    BUTTONLIST *btn;

    if ( not Root_)
        return(NULL);

    btn = (BUTTONLIST*)Root_->Find(ID);

    if (btn)
    {
        if (btn->Image_)
            return(btn->Image_->GetText());
    }

    return(NULL);
}

_TCHAR *C_Button::GetLabel(short ID)
{
    BUTTONLIST *btn;

    if ( not Root_)
        return(NULL);

    btn = (BUTTONLIST*)Root_->Find(ID);

    if (btn)
    {
        if (btn->Label_)
            return(btn->Label_->GetText());
    }

    return(NULL);
}

void C_Button::SetAllLabel(_TCHAR *str)
{
    C_HASHNODE *cur;
    long curidx;
    BUTTONLIST *btn;

    if (Root_)
    {
        btn = (BUTTONLIST*)Root_->GetFirst(&cur, &curidx);

        while (btn)
        {
            SetLabel((short)cur->ID, str);
            btn = (BUTTONLIST*)Root_->GetNext(&cur, &curidx);
        }
    }
}

void C_Button::SetAllLabel(long txtID)
{
    C_HASHNODE *cur;
    long curidx;
    BUTTONLIST *btn;

    if (Root_)
    {
        btn = (BUTTONLIST*)Root_->GetFirst(&cur, &curidx);

        while (btn)
        {
            SetLabel(cur->ID, txtID);
            btn = (BUTTONLIST*)Root_->GetNext(&cur, &curidx);
        }
    }
}

void C_Button::SetFgColor(short ID, COLORREF color)
{
    BUTTONLIST *btn;

    if ( not Root_)
        return;

    btn = (BUTTONLIST*)Root_->Find(ID);

    if (btn)
    {
        btn->FgColor_ = color;

        if (btn->Image_)
            btn->Image_->SetFgColor(color);

        if (btn->Label_)
            btn->Label_->SetFgColor(color);
    }
}

O_Output *C_Button::GetImage(short ID)
{
    BUTTONLIST *btn;

    if ( not Root_)
        return(NULL);

    btn = (BUTTONLIST*)Root_->Find(ID);

    if (btn)
        return(btn->Image_);

    return(NULL);
}

void C_Button::SetBgColor(short ID, COLORREF color)
{
    BUTTONLIST *btn;

    if ( not Root_)
        return;

    btn = (BUTTONLIST*)Root_->Find(ID);

    if (btn)
    {
        btn->BgColor_ = color;

        if (btn->Image_)
            btn->Image_->SetBgColor(color);

        if (btn->Label_)
            btn->Label_->SetBgColor(color);
    }
}

void C_Button::SetFill(short ID, short w, short h)
{
    BUTTONLIST *btn;

    if ( not Root_)
        return;

    btn = (BUTTONLIST*)Root_->Find(ID);

    if (btn)
    {
        if (btn->Image_)
        {
            if (btn->Image_->_GetOType_() not_eq O_Output::_OUT_BITMAP_)
            {
                btn->Image_->Cleanup();
                delete btn->Image_;
                btn->Image_ = NULL;
            }
        }

        if ( not btn->Image_)
        {
            btn->Image_ = new O_Output;
            btn->Image_->SetOwner(this);
        }

        btn->Image_->SetWH(w, h);
        btn->Image_->SetFlags(GetFlags());
        btn->Image_->SetFill();

        if ( not ID)
        {
            SetReady(btn->Image_->Ready());

            if (Ready())
                SetWH(w, h);
        }
    }
}

void C_Button::SetImage(short ID, long ImageID)
{
    BUTTONLIST *btn;

    if ( not Root_)
        return;

    btn = (BUTTONLIST*)Root_->Find(ID);

    if ( not btn)
    {
        btn = new BUTTONLIST;
        btn->FgColor_ = 0xcccccc;
        btn->BgColor_ = 0;
        btn->Image_ = NULL;
        btn->Label_ = NULL;
        btn->x_ = 0;
        btn->y_ = 0;
        Root_->Add(ID, btn);
    }

    if (btn)
    {
        if (btn->Image_)
        {
            if (btn->Image_->_GetOType_() not_eq O_Output::_OUT_BITMAP_)
            {
                btn->Image_->Cleanup();
                delete btn->Image_;
                btn->Image_ = NULL;
            }
        }

        if ( not btn->Image_)
        {
            btn->Image_ = new O_Output;
            btn->Image_->SetOwner(this);
        }

        btn->Image_->SetFlags(GetFlags());
        btn->Image_->SetImage(ImageID);

        if ( not ID)
        {
            SetReady(btn->Image_->Ready());

            if (Ready())
            {
                SetWH(btn->Image_->GetW(), btn->Image_->GetH());
            }
        }
    }
}

void C_Button::ClearImage(short ID, long ImageID)
{
    BUTTONLIST *btn;

    if ( not Root_)
        return;

    btn = (BUTTONLIST*)Root_->Find(ID);

    if (btn)
    {
        if (btn->Image_)
        {
            btn->Image_->Cleanup();
            delete btn->Image_;
            btn->Image_ = NULL;
        }
    }
}

void C_Button::SetImage(short ID, IMAGE_RSC *image)
{
    BUTTONLIST *btn;

    if ( not Root_)
        return;

    btn = (BUTTONLIST*)Root_->Find(ID);

    if ( not btn)
    {
        btn = new BUTTONLIST;
        btn->FgColor_ = 0xcccccc;
        btn->BgColor_ = 0;
        btn->Image_ = NULL;
        btn->Label_ = NULL;
        btn->x_ = 0;
        btn->y_ = 0;
        Root_->Add(ID, btn);
    }

    if (btn)
    {
        if (btn->Image_)
        {
            if (btn->Image_->_GetOType_() not_eq O_Output::_OUT_BITMAP_)
            {
                btn->Image_->Cleanup();
                delete btn->Image_;
                btn->Image_ = NULL;
            }
        }

        if ( not btn->Image_)
        {
            btn->Image_ = new O_Output;
            btn->Image_->SetOwner(this);
        }

        btn->Image_->SetFlags(GetFlags());
        btn->Image_->SetImage(image);

        if ( not ID)
        {
            SetReady(btn->Image_->Ready());

            if (Ready())
            {
                SetWH(btn->Image_->GetW(), btn->Image_->GetH());
            }
        }
    }
}

void C_Button::SetAnim(short ID, long AnimID, short animtype, short dir)
{
    BUTTONLIST *btn;

    if ( not Root_)
        return;

    btn = (BUTTONLIST*)Root_->Find(ID);

    if ( not btn)
    {
        btn = new BUTTONLIST;
        btn->FgColor_ = 0xcccccc;
        btn->BgColor_ = 0;
        btn->Image_ = NULL;
        btn->Label_ = NULL;
        btn->x_ = 0;
        btn->y_ = 0;
        Root_->Add(ID, btn);
    }

    if (btn)
    {
        if (btn->Image_)
        {
            if (btn->Image_->_GetOType_() not_eq O_Output::_OUT_ANIM_)
            {
                btn->Image_->Cleanup();
                delete btn->Image_;
                btn->Image_ = NULL;
            }
        }

        if (btn->Image_ == NULL)
        {
            btn->Image_ = new O_Output;
            btn->Image_->SetOwner(this);
        }

        btn->Image_->SetFlags(GetFlags());
        btn->Image_->SetAnim(AnimID);
        btn->Image_->SetAnimType(animtype);
        btn->Image_->SetDirection(dir);

        if ( not ID)
        {
            SetReady(btn->Image_->Ready());

            if (Ready())
            {
                SetWH(btn->Image_->GetW(), btn->Image_->GetH());
            }
        }

        SetFlags((GetFlags() bitor C_BIT_TIMER));
    }
}

void C_Button::SetFont(long FontID)
{
    C_HASHNODE *cur;
    long curidx;
    BUTTONLIST *btn;

    Font_ = FontID;

    if (BgImage_)
    {
        BgImage_->SetFont(FontID);
        BgImage_->SetInfo();
    }

    if (Root_)
    {
        btn = (BUTTONLIST*)Root_->GetFirst(&cur, &curidx);

        while (btn)
        {
            if (btn->Image_)
            {
                btn->Image_->SetFont(Font_);
                btn->Image_->SetInfo();
            }

            if (btn->Label_)
            {
                btn->Label_->SetFont(Font_);
                btn->Label_->SetInfo();
            }

            btn = (BUTTONLIST*)Root_->GetNext(&cur, &curidx);
        }
    }
}

void C_Button::SetText(short ID, const _TCHAR *str)
{
    F4CSECTIONHANDLE* Leave;
    BUTTONLIST *btn;

    if ( not Root_)
        return;

    btn = (BUTTONLIST*)Root_->Find(ID);

    if ( not btn)
    {
        btn = new BUTTONLIST;
        btn->FgColor_ = 0xcccccc;
        btn->BgColor_ = 0;
        btn->Image_ = NULL;
        btn->Label_ = NULL;
        btn->x_ = 0;
        btn->y_ = 0;
        Root_->Add(ID, btn);
    }

    if (btn)
    {
        Leave = UI_Enter(Parent_);

        if (btn->Image_)
        {
            if (btn->Image_->_GetOType_() not_eq O_Output::_OUT_TEXT_)
            {
                btn->Image_->Cleanup();
                delete btn->Image_;
                btn->Image_ = NULL;
            }
        }

        if (btn->Image_ == NULL)
        {
            btn->Image_ = new O_Output;
            btn->Image_->SetOwner(this);
            btn->Image_->SetFlags(GetFlags());
            btn->Image_->SetFont(Font_);
        }

        btn->Image_->SetText(gStringMgr->GetText(gStringMgr->AddText(str)));

        if ( not ID)
        {
            SetReady(1);
        }

        UI_Leave(Leave);
    }
}

void C_Button::SetText(short ID, long txtID)
{
    F4CSECTIONHANDLE* Leave;
    BUTTONLIST *btn;

    if ( not Root_)
        return;

    btn = (BUTTONLIST*)Root_->Find(ID);

    if ( not btn)
    {
        btn = new BUTTONLIST;
        btn->FgColor_ = 0xcccccc;
        btn->BgColor_ = 0;
        btn->Image_ = NULL;
        btn->Label_ = NULL;
        btn->x_ = 0;
        btn->y_ = 0;
        Root_->Add(ID, btn);
    }

    if (btn)
    {
        Leave = UI_Enter(Parent_);

        if (btn->Image_)
        {
            if (btn->Image_->_GetOType_() not_eq O_Output::_OUT_TEXT_)
            {
                btn->Image_->Cleanup();
                delete btn->Image_;
                btn->Image_ = NULL;
            }
        }

        if (btn->Image_ == NULL)
        {
            btn->Image_ = new O_Output;
            btn->Image_->SetOwner(this);
            btn->Image_->SetFlags(GetFlags());
            btn->Image_->SetFont(Font_);
        }

        btn->Image_->SetText(gStringMgr->GetString(txtID));

        if ( not ID)
        {
            SetReady(1);
        }

        UI_Leave(Leave);
    }
}

void C_Button::SetColor(short ID, COLORREF color)
{
    F4CSECTIONHANDLE* Leave;
    BUTTONLIST *btn;

    if ( not Root_)
        return;

    btn = (BUTTONLIST*)Root_->Find(ID);

    if (btn)
    {
        Leave = UI_Enter(Parent_);

        if (btn->Image_)
            btn->Image_->SetFgColor(color);

        UI_Leave(Leave);
    }
}

void C_Button::SetLabelOffset(short ID, long x, long y)
{
    F4CSECTIONHANDLE* Leave;
    BUTTONLIST *btn;

    if ( not Root_)
        return;

    btn = (BUTTONLIST*)Root_->Find(ID);

    if (btn)
    {
        Leave = UI_Enter(Parent_);
        btn->x_ = x;
        btn->y_ = y;
        UI_Leave(Leave);
    }
}

void C_Button::SetLabelColor(short ID, COLORREF color)
{
    F4CSECTIONHANDLE* Leave;
    BUTTONLIST *btn;

    if ( not Root_)
        return;

    btn = (BUTTONLIST*)Root_->Find(ID);

    if (btn)
    {
        Leave = UI_Enter(Parent_);

        if (btn->Label_)
            btn->Label_->SetFgColor(color);

        UI_Leave(Leave);
    }
}

void C_Button::SetLabelInfo()
{
    C_HASHNODE *cur;
    long curidx;
    BUTTONLIST *btn;
    long x, y;

    if (Root_)
    {
        btn = (BUTTONLIST*)Root_->GetFirst(&cur, &curidx);

        while (btn)
        {
            if (btn->Label_)
            {
                btn->Label_->SetFlags(LabelFlags_);
                btn->Label_->SetFont(Font_);

                x = 0;
                y = 0;

                if (GetFlags() bitand C_BIT_HCENTER)
                    x = btn->Image_->GetX() - (GetW() >> 1);
                else if (GetFlags() bitand C_BIT_RIGHT)
                    x = btn->Image_->GetX() + GetW();

                if (GetFlags() bitand C_BIT_VCENTER)
                    x = btn->Image_->GetX() - (GetH() >> 1);

                if (LabelFlags_ bitand C_BIT_RIGHT)
                    btn->Label_->SetX(GetW() + btn->x_ + x);
                else if (LabelFlags_ bitand C_BIT_LEFT)
                    btn->Label_->SetX(btn->x_ + x);
                else if (LabelFlags_ bitand C_BIT_HCENTER)
                    btn->Label_->SetX((GetW() >> 1) + btn->x_ + x);
                else
                    btn->Label_->SetX(btn->x_ + x);

                if (LabelFlags_ bitand C_BIT_BOTTOM)
                    btn->Label_->SetY(GetH() + btn->y_ + y);
                else if (LabelFlags_ bitand C_BIT_TOP)
                    btn->Label_->SetY(btn->y_ + y);
                else if (LabelFlags_ bitand C_BIT_VCENTER)
                    btn->Label_->SetY((GetH() >> 1) + btn->y_ + y);
                else
                    btn->Label_->SetY(btn->y_ + y);

                btn->Label_->SetInfo();
            }

            btn = (BUTTONLIST*)Root_->GetNext(&cur, &curidx);
        }
    }
}

void C_Button::SetSubParents(C_Window *)
{
    C_HASHNODE *cur;
    long curidx;
    BUTTONLIST *btn;

    if (BgImage_)
        BgImage_->SetInfo();

    if (Root_)
    {
        btn = (BUTTONLIST*)Root_->GetFirst(&cur, &curidx);

        while (btn)
        {
            if (btn->Image_)
            {
                btn->Image_->SetFrontPerc(Percent_);
                btn->Image_->SetBackPerc((short)(100 - Percent_));
                btn->Image_->SetFont(Font_);
                btn->Image_->SetFlags(GetFlags());
                btn->Image_->SetInfo();
            }

            if ( not cur->ID and btn->Image_)
                SetWH(btn->Image_->GetW(), btn->Image_->GetH());

            btn = (BUTTONLIST*)Root_->GetNext(&cur, &curidx);
        }

        SetLabelInfo();
    }
    else
        SetReady(0);
}

BOOL C_Button::TimerUpdate()
{
    short i;
    BUTTONLIST *btn;

    if ( not Root_)
        return(FALSE);

    i = state_;

    if ( not i and Parent_->GetHandler()->Over() == this)
        i = C_STATE_MOUSE;

    btn = (BUTTONLIST*)Root_->Find(i);

    if (btn)
    {
        if (btn->Image_->_GetOType_() == O_Output::_OUT_ANIM_)
        {
            laststate_ = 1;

            switch (btn->Image_->GetAnimType())
            {
                case C_TYPE_LOOP:
                    btn->Image_->SetFrame(btn->Image_->GetFrame() + btn->Image_->GetDirection());

                    if (btn->Image_->GetFrame() < 0)
                        btn->Image_->SetFrame(btn->Image_->GetAnim()->Anim->Frames - 1);

                    if (btn->Image_->GetFrame() >= btn->Image_->GetAnim()->Anim->Frames)
                        btn->Image_->SetFrame(0);

                    return(TRUE);
                    break;

                case C_TYPE_STOPATEND:
                    btn->Image_->SetFrame(btn->Image_->GetFrame() + btn->Image_->GetDirection());

                    if (btn->Image_->GetFrame() < 0)
                    {
                        btn->Image_->SetFrame(0);
                        return(FALSE);
                    }

                    if (btn->Image_->GetFrame() >= btn->Image_->GetAnim()->Anim->Frames)
                    {
                        btn->Image_->SetFrame(btn->Image_->GetAnim()->Anim->Frames - 1);
                        return(FALSE);
                    }

                    return(TRUE);
                    break;

                case C_TYPE_PINGPONG:
                    btn->Image_->SetFrame(btn->Image_->GetFrame() + btn->Image_->GetDirection());

                    if ((btn->Image_->GetFrame() < 0) or (btn->Image_->GetFrame() >= btn->Image_->GetAnim()->Anim->Frames))
                    {
                        btn->Image_->SetFrame(btn->Image_->GetFrame() - btn->Image_->GetDirection());
                        btn->Image_->SetDirection(-btn->Image_->GetDirection());
                    }

                    return(TRUE);
                    break;
            }
        }
        else if (laststate_)
        {
            laststate_ = 0;
            return(TRUE);
        }
    }
    else if (laststate_)
    {
        laststate_ = 0;
        return(TRUE);
    }

    return(FALSE);
}

void C_Button::Refresh()
{
    BUTTONLIST *btn;
    short i;

    if ( not Root_ or not Ready() or (GetFlags() bitand C_BIT_INVISIBLE) or not Parent_)
        return;

    if (UseHotSpot_)
    {
        if (FixedHotSpot_)
            Parent_->SetUpdateRect(GetX() + HotSpot_.left, GetY() + HotSpot_.top, GetX() + HotSpot_.right, GetY() + HotSpot_.bottom, GetFlags(), GetClient());
        else
        {
            i = state_;
            btn = (BUTTONLIST*)Root_->Find(i);

            if ( not btn and i)
                btn = (BUTTONLIST*)Root_->Find(0);

            if (btn)
            {
                UI95_RECT clip;
                clip.left = GetX() + btn->Image_->GetX() + HotSpot_.left;
                clip.top = GetY() + btn->Image_->GetY() + HotSpot_.top;
                clip.right = GetX() + btn->Image_->GetX() + btn->Image_->GetW() + HotSpot_.right;
                clip.bottom = GetY() + btn->Image_->GetY() + btn->Image_->GetH() + HotSpot_.bottom;
                Parent_->SetUpdateRect(clip.left, clip.top, clip.right, clip.bottom, GetFlags(), GetClient());
            }
        }
    }

    if ((GetFlags() bitand C_BIT_USEBGIMAGE) and BgImage_)
        BgImage_->Refresh();

    i = state_;
    btn = (BUTTONLIST*)Root_->Find(i);

    if ( not btn and i)
        btn = (BUTTONLIST*)Root_->Find(0);

    if (btn)
    {
        if (btn->Image_)
            btn->Image_->Refresh();

        if ( not (GetFlags() bitand C_BIT_NOLABEL) and btn->Label_)
            btn->Label_->Refresh();
    }
}

void C_Button::HighLite(SCREEN *surface, UI95_RECT *cliprect)
{
    UI95_RECT clip, tmp;
    BUTTONLIST *btn;

    memset(&clip, 0, sizeof(clip)); // OW BC
    memset(&tmp, 0, sizeof(tmp)); // OW BC

    if (UseHotSpot_)
    {
        if (FixedHotSpot_)
        {
            clip.left = GetX() + HotSpot_.left;
            clip.top = GetY() + HotSpot_.top;
            clip.right = GetX() + HotSpot_.right;
            clip.bottom = GetY() + HotSpot_.bottom;
        }
        else
        {
            btn = (BUTTONLIST*)Root_->Find(GetState());

            if ( not btn)
                btn = (BUTTONLIST*)Root_->Find(0);

            if (btn and btn->Image_)
            {
                clip.left = GetX() + btn->Image_->GetX() + HotSpot_.left;
                clip.top = GetY() + btn->Image_->GetY() + HotSpot_.top;
                clip.right = GetX() + btn->Image_->GetX() + btn->Image_->GetW() + HotSpot_.right;
                clip.bottom = GetY() + btn->Image_->GetY() + btn->Image_->GetH() + HotSpot_.bottom;
            }
            else
                return;
        }
    }
    else
    {
        btn = (BUTTONLIST*)Root_->Find(GetState());

        if ( not btn)
            btn = (BUTTONLIST*)Root_->Find(0);

        if (btn and btn->Image_)
        {
            clip.left = GetX() + btn->Image_->GetX();
            clip.top = GetY() + btn->Image_->GetY();
            clip.right = clip.left + btn->Image_->GetW();
            clip.bottom = clip.top + btn->Image_->GetH();
        }
        else
            return;
    }

    if ( not (Flags_ bitand C_BIT_ABSOLUTE))
    {
        clip.left += Parent_->VX_[Client_];
        clip.top += Parent_->VY_[Client_];
        clip.right += Parent_->VX_[Client_];
        clip.bottom += Parent_->VY_[Client_];
    }

    if ( not Parent_->ClipToArea(&tmp, &clip, cliprect))
        return;

    if ( not (Flags_ bitand C_BIT_ABSOLUTE))
        if ( not Parent_->ClipToArea(&tmp, &clip, &Parent_->ClientArea_[Client_]))
            return;

    if (UseHotSpot_)
    {
        Parent_->CustomBlitTranslucent(surface, MouseOverColor_, MouseOverPercent_, &clip, C_BIT_ABSOLUTE, 0);
    }
    else
    {
        Parent_->BlitTranslucent(surface, MouseOverColor_, MouseOverPercent_, &clip, C_BIT_ABSOLUTE, 0);
    }
}

void C_Button::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    BUTTONLIST *btn;
    short i;

    if ( not Root_ or not Ready() or (GetFlags() bitand C_BIT_INVISIBLE) or not Parent_)
        return;

    if ( not (GetFlags() bitand C_BIT_ENABLED))
        i = C_STATE_DISABLED;
    else
        i = state_;

    if ( not state_ and Parent_ and Parent_->GetHandler()->Over() == this)
        i = C_STATE_MOUSE;

    btn = (BUTTONLIST*)Root_->Find(i);

    if ( not btn and i)
        btn = (BUTTONLIST*)Root_->Find(0);

    if (BgImage_)
        BgImage_->Draw(surface, cliprect);

    if (btn and btn->Image_)
        btn->Image_->Draw(surface, cliprect);

    if ( not (GetFlags() bitand C_BIT_NOLABEL) and btn and btn->Label_)
        btn->Label_->Draw(surface, cliprect);

    if (MouseOver_ or (GetFlags() bitand C_BIT_FORCEMOUSEOVER))
        HighLite(surface, cliprect);
}

long C_Button::CheckHotSpots(long relx, long rely)
{
    BUTTONLIST *btn;
    long x, y, w, h;

    if ( not Root_ or (GetFlags() bitand C_BIT_INVISIBLE) or not (GetFlags() bitand C_BIT_ENABLED) or ( not Ready() and not UseHotSpot_))
        return(0);


    if (UseHotSpot_)
    {
        if (FixedHotSpot_)
        {
            if (relx >= (GetX() + HotSpot_.left) and rely >= (GetY() + HotSpot_.top) and relx <= (GetX() + HotSpot_.right) and rely <= (GetY() + HotSpot_.bottom))
            {
                SetRelXY(relx - GetX() - HotSpot_.left, rely - GetY() - HotSpot_.top);
                return(GetID());
            }
        }
        else
        {
            btn = (BUTTONLIST*)Root_->Find(GetState());

            if ( not btn)
                btn = (BUTTONLIST*)Root_->Find(0);

            if (btn and btn->Image_)
            {
                x = GetX() + btn->Image_->GetX() + HotSpot_.left;
                y = GetY() + btn->Image_->GetY() + HotSpot_.top;
                w = GetX() + btn->Image_->GetX() + btn->Image_->GetW() + HotSpot_.right;
                h = GetY() + btn->Image_->GetX() + btn->Image_->GetH() + HotSpot_.bottom;

                if (relx >= (x) and relx < (w) and 
                    rely >= (y) and rely < (h))
                {
                    SetRelXY(relx - GetX() - HotSpot_.left, rely - GetY() - HotSpot_.top);
                    return(GetID());
                }
            }
        }
    }
    else
    {
        btn = (BUTTONLIST*)Root_->Find(0);

        if (btn and btn->Image_)
        {
            x = GetX() + btn->Image_->GetX();
            y = GetY() + btn->Image_->GetY();
            w = btn->Image_->GetW();
            h = btn->Image_->GetH();

            if (relx >= (x) and relx < (x + w) and 
                rely >= (y) and rely < (y + h))
            {
                SetRelXY(relx - x, rely - y);
                return(GetID());
            }
        }
    }

    return(0);
};

BOOL C_Button::Process(long ID, short HitType)
{
    BUTTONLIST *btn;
    short startstate;
    //if (state_<0)
    // state_=1;

    startstate = state_;

    switch (HitType)
    {
        case C_TYPE_LMOUSEDOWN:
        case C_TYPE_REPEAT:
            if (GetType() == C_TYPE_NORMAL)
                state_ = 1;

            break;

        case C_TYPE_LMOUSEUP:
            if (GetType() == C_TYPE_NORMAL)
                state_ = 0;
            else if ((GetType() == C_TYPE_RADIO) and state_ not_eq 1)
            {
                Parent_->SetGroupState(GetGroup(), 0);
                state_ = 1;
            }
            else if (GetType() == C_TYPE_SELECT)
            {
                do
                {
                    state_++;
                    btn = (BUTTONLIST*)Root_->Find(state_);

                    if ( not btn and state_)
                        state_ = 0;
                }
                while ( not btn and state_ not_eq startstate and ( not btn and state_));
            }
            else if (GetType() == C_TYPE_TOGGLE)
                state_ = (short)((1 - state_) bitand 1); 

            if (GetFlags() bitand C_BIT_CLOSEWINDOW)
            {
                if (Owner_ and Owner_->_GetCType_() == _CNTL_LISTBOX_)
                {
                    ((C_ListBox *)Owner_)->CloseWindow();
                }
            }

            break;

    }

    //if(state_ < 0)
    // return(TRUE);

    if (startstate not_eq state_)
    {
        Refresh();
        gSoundMgr->PlaySound(GetSound(HitType));
        gSoundMgr->PlaySound(GetSound(state_));
    }

    // JB 000812 // Callbacks were called twice (mousedown and mouseup)
    //if(Callback_)
    if (Callback_ and HitType not_eq C_TYPE_LMOUSEDOWN)
        // JB 000812
        (*Callback_)(ID, HitType, this);

    return(TRUE);
}

BOOL C_Button::MouseOver(long relx, long rely, C_Base *me)
{
    BUTTONLIST *btn;
    long x, y, w, h;

    if (GetFlags() bitand C_BIT_INVISIBLE or not (GetFlags() bitand C_BIT_ENABLED) or not Ready())
        return(FALSE);

    if (UseHotSpot_)
    {
        if (FixedHotSpot_)
        {
            if (relx >= (GetX() + HotSpot_.left) and rely >= (GetY() + HotSpot_.top) and relx <= (GetX() + HotSpot_.right) and rely <= (GetY() + HotSpot_.bottom))
            {
                // Set cursor...
                if (this not_eq (C_Button *)me)
                {
                    gSoundMgr->PlaySound(GetSound(C_TYPE_MOUSEOVER));
                }

                return(TRUE);
            }
        }
        else
        {
            btn = (BUTTONLIST*)Root_->Find(GetState());

            if ( not btn)
                btn = (BUTTONLIST*)Root_->Find(0);

            if (btn and btn->Image_)
            {
                x = GetX() + btn->Image_->GetX() + HotSpot_.left;
                y = GetY() + btn->Image_->GetY() + HotSpot_.top;
                w = GetX() + btn->Image_->GetX() + btn->Image_->GetW() + HotSpot_.right;
                h = GetY() + btn->Image_->GetX() + btn->Image_->GetH() + HotSpot_.bottom;

                if (relx >= (x) and relx < (w) and 
                    rely >= (y) and rely < (h))
                {
                    // Set cursor...
                    if (this not_eq (C_Button *)me)
                    {
                        gSoundMgr->PlaySound(GetSound(C_TYPE_MOUSEOVER));
                    }

                    return(TRUE);
                }
            }
        }
    }
    else
    {
        btn = (BUTTONLIST*)Root_->Find(GetState());

        if ( not btn)
            btn = (BUTTONLIST*)Root_->Find(0);

        if (btn and btn->Image_)
        {
            x = GetX() + btn->Image_->GetX();
            y = GetY() + btn->Image_->GetY();
            w = btn->Image_->GetW();
            h = btn->Image_->GetH();

            if (relx >= (x) and relx < (x + w) and 
                rely >= (y) and rely < (y + h))
            {
                // Set cursor...
                if (this not_eq (C_Button *)me)
                {
                    gSoundMgr->PlaySound(GetSound(C_TYPE_MOUSEOVER));
                }

                return(TRUE);
            }
        }
    }

    return(FALSE);
}

BOOL C_Button::Drag(GRABBER *, WORD MouseX, WORD MouseY, C_Window *)
{
    long x, y;
    F4CSECTIONHANDLE* Leave;

    if (GetFlags() bitand C_BIT_INVISIBLE or not (GetFlags() bitand C_BIT_ENABLED) or not (GetFlags() bitand C_BIT_DRAGABLE))
        return(FALSE);

    Leave = UI_Enter(Parent_);

    x = MouseX - Parent_->GetX() - GetX();
    y = MouseY - Parent_->GetY() - GetY();

    if ( not (GetFlags() bitand C_BIT_ABSOLUTE))
    {
        x -= Parent_->ClientArea_[GetClient()].left;
        y -= Parent_->ClientArea_[GetClient()].top;
    }

    SetRelXY(x, y);

    if (Callback_)
        (*Callback_)(GetID(), C_TYPE_DRAGXY, this);

    UI_Leave(Leave);
    return(TRUE);
}

#ifdef _UI95_PARSER_

short C_Button::LocalFind(char *token)
{
    short i = 0;

    while (C_Btn_Tokens[i])
    {
        if (strnicmp(token, C_Btn_Tokens[i], strlen(C_Btn_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Button::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CBTN_SETUP:
            Setup(P[0], (short)P[1], (short)P[2], (short)P[3]);
            break;

        case CBTN_SETHOTSPOT:
            SetFixedHotSpot(0);
            SetHotSpot((short)P[0], (short)P[1], (short)P[2], (short)P[3]);
            break;

        case CBTN_SETBACKIMAGE:
            SetBackImage(P[0]);
            break;

        case CBTN_SETIMAGES:
            SetImage(C_STATE_0, P[0]);
            SetImage(C_STATE_1, P[1]);
            SetImage(C_STATE_SELECTED, P[2]);
            SetImage(C_STATE_DISABLED, P[3]);
            break;

        case CBTN_SETSTATE:
            SetState((short)P[0]);
            break;

        case CBTN_SETLABEL:
            SetLabel((short)P[0], P[1]);
            break;

        case CBTN_SETALLLABEL:
            SetAllLabel(P[0]);
            break;

        case CBTN_SETUPCOLOR:
            SetColor(C_STATE_0, P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CBTN_SETDOWNCOLOR:
            SetColor(C_STATE_1, P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CBTN_SETDISCOLOR:
            SetColor(C_STATE_DISABLED, P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CBTN_SETSELCOLOR:
            SetColor(C_STATE_SELECTED, P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CBTN_SETBUTTONIMAGE:
            SetImage((short)P[0], P[1]);
            break;

        case CBTN_SETBUTTONTEXT:
            SetText((short)P[0], P[1]);
            break;

        case CBTN_SETBUTTONANIM:
            SetAnim((short)P[0], P[1], (short)P[2], (short)P[3]);
            break;

        case CBTN_SETTEXTOFFSET:
            SetLabelOffset((short)P[0], P[1], P[2]);
            break;

        case CBTN_SETTEXTCOLOR:
            SetLabelColor((short)P[0], P[1] bitor (P[2] << 8) bitor (P[3] << 16));
            break;

        case CBTN_SETFILL:
            SetFill((short)P[0], (short)P[1], (short)P[2]);
            break;

        case CBTN_SETPERCENT:
            SetPercent((short)P[0]);
            break;

        case CBTN_SETBUTTONCOLOR:
            SetColor((short)P[0], (P[1] bitor (P[2] << 8) bitor (P[3] << 16))); 
            break;

        case CBTN_TEXTFLAGON:
            SetLabelFlagBitsOn(P[0]);
            break;

        case CBTN_TEXTFLAGOFF:
            SetLabelFlagBitsOff(P[0]);
            break;

        case CBTN_FIXED_HOTSPOT:
            SetFixedHotSpot(1);
            SetHotSpot((short)P[0], (short)P[1], (short)P[2], (short)P[3]);
            break;
    }
}

extern char ParseSave[];
extern char ParseCRLF[];

#endif // PARSER
