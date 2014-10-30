#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CLBP_NOTHING = 0,
    CLBP_SETUP,
    CLBP_ADDITEM,
    CLBP_ADDSCROLLBAR,
    CLBP_SETITEMFLAGS,
    CLBP_SETBGIMAGE,
    CLBP_SETBGFILL,
    CLBP_SETBGCOLOR,
    CLBP_SETNORMALCOLOR,
    CLBP_SETSELCOLOR,
    CLBP_SETBARCOLOR,
    CLBP_SETVALUE,
    CLBP_ITEMGROUP,
    CLBP_ITEMCLUSTER,
    CLBP_ITEMUSERDATA,
    CLBP_SETDROPDOWN,
    CLBP_SETLABELCOLOR,
};

char *C_Lbp_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[ADDITEM]",
    "[ADDSCROLLBAR]",
    "[ITEMFLAGS]",
    "[BGIMAGE]",
    "[BGFILL]",
    "[BGCOLOR]",
    "[NORMCOLOR]",
    "[SELCOLOR]",
    "[BARCOLOR]",
    "[VALUE]",
    "[ITEMGROUP]",
    "[ITEMCLUSTER]",
    "[ITEMUSERDATA]",
    "[DROPDOWN]",
    "[LABELCOLOR]",
    0,
};

#endif

C_ListBox::C_ListBox() : C_Control()
{
    _SetCType_(_CNTL_LISTBOX_);
    Root_ = NULL;
    Selected_ = 0;
    Count_ = 0;
    ScrollCount_ = 7;
    WinType_ = 0;
    Label_ = NULL;
    LabelVal_ = 0;
    ScrollBar_ = NULL;
    BgImage_ = NULL;
    DropDown_ = NULL;
    NormalColor_ = 0;
    SelColor_ = 0;
    BarColor_ = 0;
    BgColor_ = 0;
    LabelColor_ = 0;
    memset(&BgRect_, 0, sizeof(UI95_RECT));

    OpenCallback_ = NULL;
    Window_ = NULL;
    Handler_ = NULL;
    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE bitor C_BIT_MOUSEOVER;
    Font_ = 0; // JPO initialise
}

C_ListBox::C_ListBox(char **stream) : C_Control(stream)
{
    ScrollCount_ = 7;
}

C_ListBox::C_ListBox(FILE *fp) : C_Control(fp)
{
    ScrollCount_ = 7;
}

C_ListBox::~C_ListBox()
{
    if (Root_)
    {
        Cleanup();
    }
}

long C_ListBox::Size()
{
    return(0);
}

void C_ListBox::Setup(long ID, short WinType, C_Handler *Handler)
{
    SetID(ID); // used for adding/removing control from window (Not to be associated with item ID)
    Handler_ = Handler;
    WinType_ = WinType;
    Label_ = new O_Output;
    Label_->SetOwner(this);
    Label_->SetX(5);
    SetDefaultFlags();
}

void C_ListBox::Cleanup()
{
    if ( not (GetFlags() bitand C_BIT_NOCLEANUP))
        RemoveAllItems();

    if (BgImage_)
    {
        BgImage_->Cleanup();
        delete BgImage_;
        BgImage_ = NULL;
    }

    if (Label_)
    {
        Label_->Cleanup();
        delete Label_;
        Label_ = NULL;
    }

    if (ScrollBar_)
    {
        ScrollBar_->Cleanup();
        delete ScrollBar_;
        ScrollBar_ = NULL;
    }

    if (DropDown_)
    {
        DropDown_->Cleanup();
        delete DropDown_;
        DropDown_ = NULL;
    }

    if (Window_)
    {
        Window_->Cleanup();
        delete Window_;
        Window_ = NULL;
    }
}

void C_ListBox::SetBgImage(long ImageID)
{
    if (ImageID)
    {
        if (BgImage_ == NULL)
        {
            BgImage_ = new O_Output;
            BgImage_->SetOwner(this);
        }

        BgImage_->SetImage(ImageID);
        BgImage_->SetFlags(GetFlags());

        if (BgImage_->Ready())
        {
            SetFlagBitOn(C_BIT_USEBGIMAGE);
        }
        else
        {
            SetFlagBitOff(C_BIT_USEBGIMAGE);
        }
    }
}

void C_ListBox::SetBgFill(int x, int y, int w, int h)
{
    x += GetX();
    y += GetY();
    w += GetW();
    h += GetH();

    BgRect_.left = x;
    BgRect_.top = y;
    BgRect_.right = BgRect_.left + w;
    BgRect_.bottom = BgRect_.top + h;

    if (w > 0 and h > 0)
    {
        SetFlagBitOn(C_BIT_USEBGFILL);
    }
    else
    {
        SetFlagBitOff(C_BIT_USEBGFILL);
    }
}

void C_ListBox::SetDropDown(long ImageID)
{
    if (ImageID)
    {
        if (DropDown_ == NULL)
        {
            DropDown_ = new O_Output;
            DropDown_->SetOwner(this);
        }

        DropDown_->SetImage(ImageID);
        DropDown_->SetXY(GetW() - DropDown_->GetW() - 2, GetH() / 2 - DropDown_->GetH() / 2);
        DropDown_->SetFlags(GetFlags());
    }
}

void C_ListBox::SetFlags(long flags)
{
    SetControlFlags(flags);

    if (Label_)
    {
        Label_->SetFlags(flags bitand compl (C_BIT_USEBGFILL));
    }
}

void C_ListBox::SetFont(long font)
{
    LISTBOX *cur;

    Font_ = font;

    Label_->SetFont(font);
    Label_->SetInfo();

    cur = Root_;

    while (cur)
    {
        cur->Label_->SetFont(Font_);
        cur = cur->Next;
    }
}

// sfr: added number of items of scrollbar
void C_ListBox::AddScrollBar(long MinusUp, long MinusDown, long PlusUp, long PlusDown, long Slider, long nItems)
{
    ScrollCount_ = (nItems <= 0) or (nItems > 1000) ? 7 : static_cast<short>(nItems);

    if (ScrollBar_)
    {
        ScrollBar_->Cleanup();
    }
    else
    {
        ScrollBar_ = new C_ScrollBar;
    }

    ScrollBar_->Setup(GetID(), C_TYPE_VERTICAL);
    ScrollBar_->SetSliderImage(Slider);
    ScrollBar_->SetButtonImages(MinusUp, MinusDown, PlusUp, PlusDown);
    ScrollBar_->SetFlags(GetFlags());
    ScrollBar_->SetLineColor(11370561);
    ScrollBar_->SetFlagBitOn(C_BIT_USELINE);
    ScrollBar_->SetFlagBitOff(C_BIT_REMOVE);
}

C_ListBox *C_ListBox::AddItem(long ID, short, _TCHAR *Str)
{
    LISTBOX *newitem, *cur;

    if ( not ID) return(NULL);

    if (Str == NULL) return(NULL);

    if ( not (GetFlags() bitand C_BIT_REMOVE)) // Using premade list ... don't add to it
        return(NULL);

    if (FindID(ID))
        return(NULL);

    newitem = new LISTBOX;
    newitem->Label_ = new C_Button;
    newitem->Label_->Setup(ID, C_TYPE_RADIO, 0, 0);
    newitem->Label_->SetText(C_STATE_0, Str);
    newitem->Label_->SetText(C_STATE_1, Str);
    newitem->Label_->SetColor(C_STATE_0, NormalColor_);
    newitem->Label_->SetColor(C_STATE_1, SelColor_);
    newitem->Label_->SetGroup(5551212);
    newitem->Label_->SetFont(Font_);
    newitem->Label_->SetFlags((GetFlags() bitand compl (C_BIT_INVISIBLE bitor C_BIT_ABSOLUTE bitor C_BIT_USEBGFILL bitor C_BIT_REMOVE)));
    newitem->Label_->SetOwner(this);
    newitem->Label_->SetParent(Parent_);
    newitem->Next = NULL;

    if (Root_ == NULL)
    {
        Root_ = newitem;
        Count_ = 1;
        Root_->Label_->SetState(C_STATE_1);
        Label_->SetText(Root_->Label_->GetText(C_STATE_0));
        LabelVal_ = Root_->Label_->GetID();
    }
    else
    {
        cur = Root_;

        while (cur->Next)
        {
            cur = cur->Next;
        }

        cur->Next = newitem;
        Count_++;
    }

    newitem->Label_->SetXY(5, (Count_ - 1)*gFontList->GetHeight(Font_));

    if (Root_ and Root_->Label_ and Root_->Label_->Ready())
    {
        SetReady(1);
    }

    return(this);
}

C_ListBox *C_ListBox::AddItem(long ID, short Type, long txtID)
{
    return(AddItem(ID, Type, gStringMgr->GetString(txtID)));
}

void C_ListBox::SetRoot(LISTBOX *NewRoot)
{
    LISTBOX *cur;
    short i;

    Root_ = NewRoot;

    if (NewRoot == NULL)
    {
        SetFlagBitOn(C_BIT_REMOVE);
        Count_ = 0;
        return;
    }

    cur = Root_;
    Count_ = 0;
    i = 0;

    while (cur)
    {
        cur->Label_->SetSubParents(Parent_);

        if (LabelVal_ == cur->Label_->GetID())
        {
            Label_->SetText(cur->Label_->GetText(C_STATE_0));
            i = 1;
        }

        Count_++;
        cur = cur->Next;
    }

    if ( not i and Root_)
    {
        LabelVal_ = Root_->Label_->GetID();
        Label_->SetText(Root_->Label_->GetText(C_STATE_0));
    }

    SetFlagBitOff(C_BIT_REMOVE);
    SetReady(1);
}

void C_ListBox::RemoveAllItems()
{
    LISTBOX *cur, *last;

    if ( not (GetFlags() bitand C_BIT_REMOVE))
    {
        return;
    }

    cur = Root_;

    while (cur)
    {
        last = cur;
        cur = cur->Next;

        if (last->Label_)
        {
            last->Label_->Cleanup();
            delete last->Label_;
        }

        delete last;
    }

    Root_ = NULL;
}

LISTBOX *C_ListBox::FindID(long pID)
{
    LISTBOX *Pop;

    Pop = Root_;

    while (Pop)
    {
        if (Pop->Label_->GetID() == pID)
        {
            return(Pop);
        }

        Pop = Pop->Next;
    }

    return(NULL);
}

C_Button *C_ListBox::GetItem(long ID)
{
    LISTBOX *item;

    item = FindID(ID);

    if (item)
    {
        return(item->Label_);
    }

    return(NULL);
}

void C_ListBox::SetValue(long ID)
{
    LISTBOX *cur;
    BOOL found = FALSE;

    cur = Root_;

    while (cur)
    {
        if (cur->Label_->GetID() == ID)
        {
            Label_->SetText(cur->Label_->GetText(C_STATE_0));
            LabelVal_ = cur->Label_->GetID();
            cur->Label_->SetState(C_STATE_1);
            found = TRUE;
        }
        else
        {
            cur->Label_->SetState(C_STATE_0);
        }

        cur = cur->Next;
    }

    if ( not found and Root_)
    {
        Label_->SetText(Root_->Label_->GetText(C_STATE_0));
        LabelVal_ = Root_->Label_->GetID();
        Root_->Label_->SetState(C_STATE_1);
    }
}

// JB 011124
void C_ListBox::SetValueText(long inText)
{
    LISTBOX *cur;
    BOOL found = FALSE;

    cur = Root_;

    int width;

    while (cur and not found)
    {
        width = atoi(cur->Label_->GetText(C_STATE_0));

        if (width == inText)
        {
            Label_->SetText(cur->Label_->GetText(C_STATE_0));
            LabelVal_ = cur->Label_->GetID();
            cur->Label_->SetState(C_STATE_1);
            found = TRUE;
        }
        else
        {
            cur->Label_->SetState(C_STATE_0);
        }

        cur = cur->Next;
    }

    if ( not found and Root_)
    {
        Label_->SetText(Root_->Label_->GetText(C_STATE_0));
        LabelVal_ = Root_->Label_->GetID();
        Root_->Label_->SetState(C_STATE_1);
    }
}

void C_ListBox::SetItemFlags(long ID, long flags)
{
    LISTBOX *cur;

    cur = FindID(ID);

    if (cur)
    {
        cur->Label_->SetFlags(flags bitand compl (C_BIT_USEBGFILL));
    }
}

void C_ListBox::SetItemGroup(long ID, long group)
{
    LISTBOX *cur;

    cur = FindID(ID);

    if (cur)
    {
        cur->Label_->SetGroup(group);
    }
}

void C_ListBox::SetItemCluster(long ID, long cluster)
{
    LISTBOX *cur;

    cur = FindID(ID);

    if (cur)
    {
        cur->Label_->SetCluster(cluster);
    }
}

void C_ListBox::SetItemUserData(long ID, short idx, long value)
{
    LISTBOX *cur;

    cur = FindID(ID);

    if (cur)
    {
        cur->Label_->SetUserNumber(idx, value);
    }
}

short C_ListBox::GetListHeight()
{
    short sResult = (short)(gFontList->GetHeight(Font_) * Count_ + 8);    

    return sResult;
}

long C_ListBox::CheckHotSpots(long relX, long relY)
{
    if (GetFlags() bitand C_BIT_INVISIBLE or not (GetFlags() bitand C_BIT_ENABLED))
        return(0);

    if (relX < GetX() or relX > (GetX() + GetW()) or relY < GetY() or relY > (GetY() + GetH()))
        return(0);

    SetRelXY(relX - GetX(), relY - GetY());
    return(GetID());
}

BOOL C_ListBox::Process(long ID, short HitType)
{
    gSoundMgr->PlaySound(GetSound(HitType));

    switch (HitType)
    {
        case C_TYPE_LMOUSEUP:
            if (Callback_)
                (*Callback_)(ID, HitType, this);

            if (GetFlags() bitand C_BIT_ABSOLUTE)
                OpenWindow((short)(Parent_->GetX() + GetX()),
                           (short)(Parent_->GetY() + GetY() + GetH() + 1),
                           (short)GetW(), (short)GetListHeight());
            else
                OpenWindow((short)(Parent_->GetX() + Parent_->VX_[GetClient()] + GetX()),
                           (short)(Parent_->GetY() + Parent_->VY_[GetClient()] + GetY() + GetH() + 1),
                           (short)GetW(), (short)GetListHeight()); 

            return(TRUE);
            break;
    }

    return(FALSE);
}

void C_ListBox::Refresh()
{
    if ( not Ready() or GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    Parent_->SetUpdateRect(GetX(), GetY(), GetX() + GetW() + 1, GetY() + GetH() + 1, GetFlags(), GetClient());
}

void C_ListBox::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    if ( not Ready() or GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    if (GetFlags() bitand C_BIT_INVISIBLE)
        return;

    if (GetFlags() bitand C_BIT_USEBGIMAGE)
        BgImage_->Draw(surface, cliprect);

    if (GetFlags() bitand C_BIT_USEBGFILL)
    {
        Parent_->BlitFill(surface, BgColor_, GetX() + BgRect_.left, GetY() + BgRect_.top, BgRect_.right + 1, BgRect_.bottom + 1, GetFlags(), GetClient(), cliprect);
    }

    if (Label_)
    {
        Label_->SetFgColor(LabelColor_);
        Label_->Draw(surface, cliprect);
    }

    if (DropDown_ and (GetFlags() bitand C_BIT_ENABLED))
        DropDown_->Draw(surface, cliprect);

    if (MouseOver_ or (GetFlags() bitand C_BIT_FORCEMOUSEOVER))
        HighLite(surface, cliprect);
}

BOOL C_ListBox::OpenWindow(short x, short y, short w, short h)
{
    LISTBOX *cur = NULL;
    C_Line *line = NULL;
    short UseScrollBar = 0;
    short sb_w = 0, virtualh = 0;
    int draw_y = 0; 
    int fh = 0, i = 0; 

    if (Handler_ == NULL)
    {
        return(FALSE);
    }

    if ( not Ready())
    {
        return(FALSE);
    }

    if (OpenCallback_)
    {
        (*OpenCallback_)(this);
    }

    fh = gFontList->GetHeight(Font_);

    i = 0;
    cur = Root_;

    while (cur)
    {
        if ( not (cur->Label_->GetFlags() bitand C_BIT_INVISIBLE))
        {
            i++;
        }

        cur = cur->Next;
    }

    Count_ = (short)i; 

    h = (short)(i * fh + 8); 

    // if bigger than 7 elements and scrollable
    if ((h > (fh * 7 + 8)) and (ScrollBar_))
    {
        virtualh = h;
        UseScrollBar = 1;
        h = (short)(fh * ScrollCount_ /*7*/ + 8); 
    }

    Window_ = new C_Window;
    Window_->Setup(1, C_TYPE_EXCLUSIVE, w, h);
    Window_->SetFlagBitOn(C_BIT_NOCLEANUP);
    Window_->SetClientArea(2, 2, w - 4, h - 4, 0);

    if ((x + w) > Handler_->GetW()) x = (short)(Handler_->GetW() - w - 1); 

    if ((y + h) > Handler_->GetH()) y = (short)(Handler_->GetH() - h - 1); 

    if (x < 0) x = 0;

    if (y < 0) y = 0;

    Window_->SetRanges(0, 0, (short)Handler_->GetW(), (short)Handler_->GetH(), w, h); 
    Window_->SetXY(x, y);
    line = new C_Line;
    line->Setup(C_DONT_CARE, C_TYPE_VERTICAL);
    line->SetXYWH(0, 0, Window_->GetW(), Window_->GetH());
    line->SetFlagBitOn(C_BIT_ABSOLUTE);
    // KCK added - Since listbox background color was always black
    line->SetColor(BgColor_);
    Window_->AddControl(line);
    // KCK added - Since I thought it looked nice and won't effect
    // any current items which don't set barrier color. Kill me if you want
    C_Box *box = new C_Box;
    box->Setup(C_DONT_CARE, C_TYPE_VERTICAL);
    box->SetXYWH(0, 0, Window_->GetW() - 1, Window_->GetH() - 1);
    box->SetFlagBitOn(C_BIT_ABSOLUTE);
    box->SetColor(BarColor_);
    Window_->AddControl(box);
    // END KCK added shit

    if (ScrollBar_ and UseScrollBar)
    {
        sb_w = (short)ScrollBar_->GetW(); 
        Window_->ClientArea_[0].right -= sb_w;
        ScrollBar_->SetClient(0);
        ScrollBar_->SetXY(Window_->ClientArea_[0].right, Window_->ClientArea_[0].top);
        ScrollBar_->SetWH(sb_w, Window_->ClientArea_[0].bottom);
        ScrollBar_->ClearVH();
        ScrollBar_->SetFlagBitOn(C_BIT_ABSOLUTE);

        if (ScrollBar_->Minus_)
        {
            ScrollBar_->Minus_->SetFlagBitOn(C_BIT_ABSOLUTE);
            ScrollBar_->Minus_->SetClient(0);
        }

        if (ScrollBar_->Plus_)
        {
            ScrollBar_->Plus_->SetFlagBitOn(C_BIT_ABSOLUTE);
            ScrollBar_->Plus_->SetClient(0);
        }

        ScrollBar_->SetVirtualH(virtualh);
        ScrollBar_->SetDistance(fh);
        Window_->AddScrollBar(ScrollBar_);
    }

    draw_y = 0;
    cur = Root_;

    while (cur)
    {
        if ( not (cur->Label_->GetFlags() bitand C_BIT_INVISIBLE))
        {
            if (cur->Label_->GetID() == LabelVal_)
            {
                cur->Label_->SetState(1);
            }
            else
            {
                cur->Label_->SetState(0);
            }

            if (cur->Label_->GetState())
            {
                if (ScrollBar_ and UseScrollBar)
                {
                    Window_->SetVirtualY(draw_y - 2, 0);
                }
            }

            cur->Label_->SetFgColor(C_STATE_0, NormalColor_);
            cur->Label_->SetFgColor(C_STATE_1, SelColor_);
            cur->Label_->SetXY(0, draw_y);
            cur->Label_->SetClient(0);
            cur->Label_->SetFont(Font_);
            cur->Label_->SetOwner(this);
            cur->Label_->SetFlagBitOn(C_BIT_CLOSEWINDOW);
            cur->Label_->SetFlagBitOff(C_BIT_ABSOLUTE);
            cur->Label_->SetCursorID(GetCursorID());
            cur->Label_->SetDragCursorID(GetDragCursorID());
            Window_->AddControl(cur->Label_);
            cur->Label_->SetW(w);
            draw_y += fh;
        }
        else
        {
            cur->Label_->SetState(0);
        }

        cur = cur->Next;
    }

    Selected_ = -1;
    Window_->SetOwner(this);
    Handler_->AddWindow(Window_, C_BIT_ENABLED);
    Window_->ScanClientArea(0);
    Window_->AdjustScrollbar(0);
    Window_->update_ or_eq C_DRAW_REFRESHALL;
    Window_->RefreshWindow();
    Handler_->WindowToFront(Window_);
    return(TRUE);
}

BOOL C_ListBox::CloseWindow()
{
    LISTBOX *cur;
    F4CSECTIONHANDLE *Leave;

    if (Window_ == NULL)
        return(FALSE);

    Leave = UI_Enter(Window_);
    cur = Root_;

    while (cur)
    {
        if (cur->Label_->GetState())
        {
            Label_->SetText(cur->Label_->GetText(C_STATE_0));
            LabelVal_ = cur->Label_->GetID();
            cur = NULL;
        }
        else
            cur = cur->Next;
    }

    SetSubParents(NULL);
    Handler_->RemoveWindow(Window_);
    Window_->Cleanup();
    delete Window_;
    Window_ = NULL;

    if (ScrollBar_)
    {
        ScrollBar_->SetParent(NULL);
        ScrollBar_->SetSubParents(NULL);
    }

    this->Refresh();

    if (Callback_)
    {
        (*Callback_)(GetID(), C_TYPE_SELECT, this);
    }

    UI_Leave(Leave);


    return(TRUE);
}

void C_ListBox::SetSubParents(C_Window *Parent)
{
    LISTBOX *cur;

    if ( not LabelColor_)
        LabelColor_ = NormalColor_;

    if (BgImage_)
        BgImage_->SetInfo();

    if (DropDown_)
    {
        DropDown_->SetXY(GetW() - DropDown_->GetW() - 2, GetH() / 2 - DropDown_->GetH() / 2);
        DropDown_->SetInfo();
        DropDown_->SetFlags(GetFlags());
    }

    cur = Root_;

    while (cur)
    {
        cur->Label_->SetParent(Parent);
        cur->Label_->SetSubParents(Parent);
        cur->Label_->SetOwner(this);
        cur = cur->Next;
    }
}

#ifdef _UI95_PARSER_
short C_ListBox::LocalFind(char *token)
{
    short i = 0;

    while (C_Lbp_Tokens[i])
    {
        if (strnicmp(token, C_Lbp_Tokens[i], strlen(C_Lbp_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_ListBox::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *Hndlr)
{
    switch (ID)
    {
        case CLBP_SETUP:
            Setup(P[0], (short)P[1], Hndlr);
            break;

        case CLBP_ADDITEM:
            AddItem(P[0], (short)P[1], P[2]);
            break;

        case CLBP_ADDSCROLLBAR:
            AddScrollBar(P[0], P[1], P[2], P[3], P[4], P[5]);
            break;

        case CLBP_SETITEMFLAGS:
            SetItemFlags(P[0], P[1]);
            break;

        case CLBP_SETBGIMAGE:
            SetBgImage(P[0]);
            break;

        case CLBP_SETBGFILL:
            SetBgFill((short)P[0], (short)P[1], (short)P[2], (short)P[3]);
            break;

        case CLBP_SETBGCOLOR:
            SetBgColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CLBP_SETNORMALCOLOR:
            SetNormColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CLBP_SETSELCOLOR:
            SetSelColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CLBP_SETBARCOLOR:
            SetBarColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CLBP_SETLABELCOLOR:
            SetLabelColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CLBP_SETVALUE:
            SetValue(P[0]);
            break;

        case CLBP_ITEMGROUP:
            SetItemGroup(P[0], P[1]);
            break;

        case CLBP_ITEMCLUSTER:
            SetItemCluster(P[0], P[1]);
            break;

        case CLBP_ITEMUSERDATA:
            SetItemUserData(P[0], (short)P[1], P[2]);
            break;

        case CLBP_SETDROPDOWN:
            SetDropDown(P[0]);
            break;
    }
}

extern char ParseSave[];
extern char ParseCRLF[];

#endif // PARSER
