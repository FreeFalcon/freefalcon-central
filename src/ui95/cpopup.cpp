#include <cISO646>
#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CPU_NOTHING = 0,
    CPU_SETUP,
    CPU_ADDITEM,
    CPU_SETITEMFLAGBITON,
    CPU_SETITEMFLAGBITOFF,
    CPU_SETGROUP,
    CPU_SETSTATE,
    CPU_SETNORMCOLOR,
    CPU_SETSELCOLOR,
    CPU_SETDISCOLOR,
    CPU_SETBARCOLOR,
    CPU_SETBGCOLOR,
    CPU_SETOPAQUE,
    CPU_SETBORDERCOLOR,
};

char *C_Pu_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[ADDITEM]",
    "[ITEMFLAGON]",
    "[ITEMFLAGOFF]",
    "[RADIOGROUP]",
    "[STATE]",
    "[NORMCOLOR]",
    "[SELCOLOR]",
    "[DISCOLOR]",
    "[BARCOLOR]",
    "[BGCOLOR]",
    "[OPAQUE]",
    "[BORDERCOLOR]",
    0,
};

#endif

C_PopupList::C_PopupList() : C_Control()
{
    _SetCType_(_CNTL_POPUPLIST_);
    Root_ = NULL;
    Selected_ = 0;
    Count_ = 0;
    WinType_ = 0;
    Direction_ = C_TYPE_RIGHT;
    MenuIconID_ = 0;
    CheckIconID_ = 0;
    NormalColor_ = 0;
    SelColor_ = 0xffffff;
    DisColor_ = 0x888888;
    BarColor_ = 0;
    BgColor_ = 0;
    Opaque_ = 0;
    BorderColor_ = 0;
    OpenCallback_ = NULL;
    Window_ = NULL;
    Handler_ = NULL;
    DefaultFlags_ = C_BIT_ENABLED;
}

C_PopupList::C_PopupList(char **stream) : C_Control(stream)
{
}

C_PopupList::C_PopupList(FILE *fp) : C_Control(fp)
{
}

C_PopupList::~C_PopupList()
{
    if (Root_)
        Cleanup();
}

long C_PopupList::Size()
{
    return(0);
}

void C_PopupList::Setup(long ID, short, C_Handler *Handler, long MenuIconID, long CheckIconID)
{
    SetID(ID);
    SetDefaultFlags();
    Handler_ = Handler;
    WinType_ = C_TYPE_NORMAL;
    MenuIconID_ = MenuIconID;
    CheckIconID_ = CheckIconID;
    SetReady(1);
}

void C_PopupList::Cleanup()
{
    RemoveAllItems();

    if (Window_)
    {
        Window_->Cleanup();
        delete Window_;
        Window_ = NULL;
    }
}

void C_PopupList::SetFont(long Font)
{
    POPUPLIST *cur;

    Font_ = Font;

    cur = Root_;

    while (cur)
    {
        if (cur->Label_)
            cur->Label_->SetFont(Font);

        if (cur->SubMenu_ and cur->Type_ == C_TYPE_MENU)
            cur->SubMenu_->SetFont(Font);

        cur = cur->Next;
    }
}

void C_PopupList::SetFlags(long flags)
{
    POPUPLIST *cur;

    SetControlFlags(flags);

    cur = Root_;

    while (cur)
    {
        if (cur->Label_)
            cur->Label_->SetFlags(flags);

        if (cur->SubMenu_ and cur->Type_ == C_TYPE_MENU)
            cur->SubMenu_->SetFlags(flags);

        cur = cur->Next;
    }
}

BOOL C_PopupList::AddItem(long ID, short Type, _TCHAR *Str, long ParentID)
{
    POPUPLIST *newitem, *cur;

    if (FindID(ID) and Type not_eq C_TYPE_NOTHING)
        return(FALSE);

    if ( not Str and Type not_eq C_TYPE_NOTHING)
        return(FALSE);

    if (ParentID)
    {
        cur = FindID(ParentID);

        if (cur)
        {
            if (cur->Type_ == C_TYPE_MENU)
            {
                if (cur->SubMenu_ == NULL)
                {
                    cur->SubMenu_ = new C_PopupList;
                    cur->SubMenu_->Setup(cur->ID_, C_TYPE_NORMAL, Handler_, MenuIconID_, CheckIconID_);
                    cur->SubMenu_->SetFlags(GetFlags());
                    cur->SubMenu_->SetFont(GetFont());
                    cur->SubMenu_->SetNormColor(NormalColor_);
                    cur->SubMenu_->SetSelColor(SelColor_);
                    cur->SubMenu_->SetDisColor(DisColor_);
                    cur->SubMenu_->SetBarColor(BarColor_);
                    cur->SubMenu_->SetBgColor(BgColor_);
                    cur->SubMenu_->SetOpaque(Opaque_);
                    cur->SubMenu_->SetBorderColor(BorderColor_);
                }

                return(cur->SubMenu_->AddItem(ID, Type, Str, 0));
            }
        }
    }

    newitem = new POPUPLIST;
    newitem->ID_ = ID;
    newitem->Type_ = Type;
    newitem->flags_ = C_BIT_ENABLED;
    newitem->Label_ = NULL;
    newitem->MenuIcon_ = NULL;
    newitem->CheckIcon_ = NULL;
    newitem->State_ = 0;

    if (Str and Type not_eq C_TYPE_NOTHING)
    {
        newitem->Label_ = new O_Output;
        newitem->Label_->SetOwner(this);
        newitem->Label_->SetFlags(GetFlags());
        newitem->Label_->SetFont(GetFont());
        newitem->Label_->SetText(gStringMgr->GetText(gStringMgr->AddText(Str)));
        newitem->Label_->SetX(20);

        if (Type == C_TYPE_MENU and MenuIconID_)
        {
            newitem->MenuIcon_ = new O_Output;
            newitem->MenuIcon_->SetOwner(this);
            newitem->MenuIcon_->SetFlags(GetFlags());
            newitem->MenuIcon_->SetImage(MenuIconID_);
        }
        else if ((Type == C_TYPE_RADIO or Type == C_TYPE_TOGGLE) and CheckIconID_)
        {
            newitem->CheckIcon_ = new O_Output;
            newitem->CheckIcon_->SetOwner(this);
            newitem->CheckIcon_->SetFlags(GetFlags());
            newitem->CheckIcon_->SetImage(CheckIconID_);
            newitem->CheckIcon_->SetX(4);
        }
    }

    newitem->SubMenu_ = NULL;
    newitem->Callback_ = NULL;
    newitem->Next = NULL;

    if (Root_ == NULL)
    {
        Root_ = newitem;
    }
    else
    {
        cur = Root_;

        while (cur->Next)
            cur = cur->Next;

        cur->Next = newitem;
    }

    return(TRUE);
}

BOOL C_PopupList::AddItem(long ID, short Type, long txtID, long ParentID)
{
    return(AddItem(ID, Type, gStringMgr->GetString(txtID), ParentID));
}

void C_PopupList::RemoveList(POPUPLIST *list)
{
    POPUPLIST *cur, *last;

    cur = list;

    while (cur)
    {
        last = cur;
        cur = cur->Next;

        if (last->Label_)
        {
            last->Label_->Cleanup();
            delete last->Label_;
        }

        if (last->MenuIcon_)
        {
            last->MenuIcon_->Cleanup();
            delete last->MenuIcon_;
        }

        if (last->CheckIcon_)
        {
            last->CheckIcon_->Cleanup();
            delete last->CheckIcon_;
        }

        if (last->SubMenu_)
        {
            last->SubMenu_->Cleanup();
            delete last->SubMenu_;
        }

        delete last;
    }
}

void C_PopupList::RemoveAllItems()
{
    if (Root_)
    {
        RemoveList(Root_);
        Root_ = NULL;
    }
}

POPUPLIST *C_PopupList::FindID(long pID)
{
    POPUPLIST *Pop, *ret;

    Pop = Root_;

    while (Pop)
    {
        if (Pop->ID_ == pID)
            return(Pop);
        else if (Pop->Type_ == C_TYPE_MENU and Pop->SubMenu_)
        {
            ret = Pop->SubMenu_->FindID(pID);

            if (ret)
                return(ret);
        }

        Pop = Pop->Next;
    }

    return(NULL);
}

_TCHAR *C_PopupList::GetItemLabel(long ID)
{
    POPUPLIST *cur;

    cur = FindID(ID);

    if (cur)
        if (cur->Label_)
            return(cur->Label_->GetText());

    return(NULL);
}

void C_PopupList::SetItemLabel(long ID, _TCHAR *txt)
{
    POPUPLIST *cur;

    cur = FindID(ID);

    if (cur)
        cur->Label_->SetText(gStringMgr->GetText(gStringMgr->AddText(txt)));
}

void C_PopupList::SetItemLabel(long ID, long textID)
{
    POPUPLIST *cur;

    cur = FindID(ID);

    if (cur)
        cur->Label_->SetText(gStringMgr->GetString(textID));
}

void C_PopupList::SetItemFlagBitOn(long ID, long flags)
{
    POPUPLIST *cur;

    cur = FindID(ID);

    if (cur)
        cur->flags_ or_eq flags;
}

void C_PopupList::SetItemFlagBitOff(long ID, long flags)
{
    POPUPLIST *cur;

    cur = FindID(ID);

    if (cur)
        cur->flags_ and_eq compl flags;
}

void C_PopupList::SetItemState(long ID, short val)
{
    POPUPLIST *cur;

    cur = FindID(ID);

    if (cur)
    {
        if (cur->Type_ == C_TYPE_TOGGLE)
            cur->State_ = static_cast<short>(val bitand 1); 
        else if (cur->Type_ == C_TYPE_RADIO and cur->Group_)
        {
            ClearRadioGroup(cur->Group_);
            cur->State_ = static_cast<short>(val bitand 1); 
        }
    }
}

short C_PopupList::GetItemState(long ID)
{
    POPUPLIST *cur;

    cur = FindID(ID);

    if (cur)
        return(cur->State_);

    return(0);
}

void C_PopupList::SetItemGroup(long ID, long Group)
{
    POPUPLIST *cur;

    cur = FindID(ID);

    if (cur)
        cur->Group_ = Group;
}

long C_PopupList::GetItemGroup(long ID)
{
    POPUPLIST *cur;

    cur = FindID(ID);

    if (cur)
        return(cur->Group_);

    return(0);
}

void C_PopupList::ClearRadioGroup(long GroupID)
{
    POPUPLIST *Pop;

    Pop = Root_;

    while (Pop)
    {
        if (Pop->Type_ == C_TYPE_RADIO and Pop->Group_ == GroupID)
            Pop->State_ = 0;
        else if (Pop->Type_ == C_TYPE_MENU and Pop->SubMenu_)
            Pop->SubMenu_->ClearRadioGroup(GroupID);

        Pop = Pop->Next;
    }
}

void C_PopupList::GetSize(short *width, short *height)
{
    short w, len;
    POPUPLIST *cur;

    w = 0;

    *height = static_cast<short>(gFontList->GetHeight(Font_) * Count_); 
    cur = Root_;

    while (cur)
    {
        if ( not (cur->flags_ bitand C_BIT_INVISIBLE))
        {
            if (cur->Type_ not_eq C_TYPE_NOTHING)
            {
                if (cur->Label_)
                    len = static_cast<short>(gFontList->StrWidth(Font_, cur->Label_->GetText())); 
                else
                    len = 0;

                if (len > w)
                    w = len;
            }
        }

        cur = cur->Next;
    }

    *width = w;
}

void C_PopupList::SetCallback(long ID, void (*routine)(long ID, short ht, C_Base *))
{
    POPUPLIST *cur;

    cur = FindID(ID);

    if (cur)
        cur->Callback_ = routine;
}

void C_PopupList::CloseSubMenus()
{
    POPUPLIST *cur;

    Handler_->EnterCritical();
    cur = Root_;

    while (cur)
    {
        if (cur->Type_ == C_TYPE_MENU)
        {
            if (cur->SubMenu_)
            {
                if (cur->SubMenu_->Window_)
                {
                    cur->SubMenu_->CloseSubMenus();
                    cur->SubMenu_->CloseWindow();
                }
            }
        }

        cur = cur->Next;
    }

    Handler_->LeaveCritical();
}

long C_PopupList::CheckHotSpots(long relX, long relY)
{
    POPUPLIST *cur;
    short i;

    if (GetFlags() bitand C_BIT_INVISIBLE or not (GetFlags() bitand C_BIT_ENABLED))
        return(0);

    if (relX < Parent_->ClientArea_[GetClient()].left or relX > Parent_->ClientArea_[GetClient()].right or
        relY < Parent_->ClientArea_[GetClient()].top or relY > Parent_->ClientArea_[GetClient()].bottom)
        return(0);

    Selected_ = static_cast<short>((relY - 5) / gFontList->GetHeight(GetFont())); 
    cur = Root_;

    i = 0;

    while (cur and i < Selected_)
    {
        if ( not (cur->flags_ bitand C_BIT_INVISIBLE))
            i++;

        cur = cur->Next;
    }

    while (cur and (cur->flags_ bitand C_BIT_INVISIBLE))
        cur = cur->Next;

    if (cur == NULL)
        return(0);

    if ( not (cur->flags_ bitand C_BIT_ENABLED))
        cur = NULL;

    if (cur and cur->Type_ not_eq C_TYPE_NOTHING)
    {
        SetRelXY(relX - GetX(), relY - GetY());
        return(cur->ID_);
    }

    return(0);
}

BOOL C_PopupList::Process(long ID, short HitType)
{
    POPUPLIST *cur;

    gSoundMgr->PlaySound(GetSound(HitType));

    if (HitType not_eq C_TYPE_LMOUSEUP and HitType not_eq C_TYPE_RMOUSEUP)
        return(FALSE);

    cur = FindID(ID);

    if (cur)
    {
        switch (cur->Type_)
        {
            case C_TYPE_RADIO:
                ClearRadioGroup(cur->Group_);
                cur->State_ = C_STATE_1;
                Refresh();
                break;

            case C_TYPE_TOGGLE:
                cur->State_ xor_eq 1;
                Refresh();
                break;

            case C_TYPE_ITEM:
                break;
        }

        if (cur->Callback_)
            (*cur->Callback_)(ID, HitType, this);

        return(TRUE);
    }

    return(FALSE);
}

// MUST return TRUE
BOOL C_PopupList::MouseOver(long relX, long relY, C_Base *)
{
    POPUPLIST *cur;
    short i, item, w, h;

    if (relX < Parent_->ClientArea_[GetClient()].left or relX > Parent_->ClientArea_[GetClient()].right or
        relY < Parent_->ClientArea_[GetClient()].top or relY > Parent_->ClientArea_[GetClient()].bottom)
    {
        /* CloseSubMenus();
         CloseWindow();
        */
        return(TRUE);
    }

    F4CSECTIONHANDLE* Leave = UI_Enter(Parent_);

    item = static_cast<short>((relY - 5) / gFontList->GetHeight(GetFont()));

    if (item not_eq Selected_)
    {
        Refresh();

        i = 0;
        cur = Root_;

        while (cur)
        {
            if (cur->flags_ bitand C_BIT_ENABLED)
            {
                if (cur->Type_ == C_TYPE_MENU and cur->SubMenu_)
                {
                    if (cur->SubMenu_->Window_)
                    {
                        cur->SubMenu_->CloseSubMenus();
                        cur->SubMenu_->CloseWindow();
                    }
                }
            }

            cur = cur->Next;
        }

        Selected_ = item;

        cur = Root_;

        while (cur and i < item)
        {
            if ( not (cur->flags_ bitand C_BIT_INVISIBLE))
                i++;

            cur = cur->Next;
        }

        while (cur and (cur->flags_ bitand C_BIT_INVISIBLE))
            cur = cur->Next;

        if (cur == NULL)
        {
            UI_Leave(Leave);
            return(TRUE);
        }

        if (cur->flags_ bitand C_BIT_ENABLED and not (cur->flags_ bitand C_BIT_INVISIBLE))
        {
            if (cur->Type_ == C_TYPE_MENU)
            {
                if (cur->SubMenu_)
                {
                    cur->SubMenu_->GetSize(&w, &h);

                    if (Direction_ == C_TYPE_RIGHT)
                    {
                        if ((Parent_->GetX() + Parent_->GetW() + w + 48 - 15) < Handler_->GetW())
                            cur->SubMenu_->OpenWindow(static_cast<short>(Parent_->GetX() + Parent_->GetW() - 15),
                                                      static_cast<short>(Selected_ * gFontList->GetHeight(GetFont()) + Parent_->GetY()),
                                                      C_TYPE_RIGHT); 
                        else
                            cur->SubMenu_->OpenWindow(static_cast<short>(Parent_->GetX() - w - 48 + 15),
                                                      static_cast<short>(Selected_ * gFontList->GetHeight(GetFont()) + Parent_->GetY()),
                                                      C_TYPE_LEFT); 
                    }
                    else
                    {
                        if ((Parent_->GetX() - w - 48 + 15) > Handler_->GetX())
                            cur->SubMenu_->OpenWindow(static_cast<short>(Parent_->GetX() - w - 48 + 15),
                                                      static_cast<short>(Selected_ * gFontList->GetHeight(GetFont()) + Parent_->GetY()),
                                                      C_TYPE_LEFT);  
                        else
                            cur->SubMenu_->OpenWindow(static_cast<short>(Parent_->GetX() + Parent_->GetW() - 15),
                                                      static_cast<short>(Selected_ * gFontList->GetHeight(GetFont()) + Parent_->GetY()),
                                                      C_TYPE_RIGHT); 
                    }
                }
            }
        }
    }

    UI_Leave(Leave);
    return(TRUE);
}

void C_PopupList::Refresh()
{
    if ( not Ready() or GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    Parent_->update_ or_eq C_DRAW_REFRESHALL;
    Parent_->RefreshWindow();
}

void C_PopupList::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    POPUPLIST *cur;
    short i;
    short fh;
    UI95_RECT frect, drect;

    if ( not Ready())
        return;

    if (GetFlags() bitand C_BIT_INVISIBLE)
        return;

    // Draw border
    if (BorderColor_ not_eq BgColor_)
    {
        Parent_->DrawHLine(surface, BorderColor_, 0, 0, Parent_->GetW(), C_BIT_ABSOLUTE, GetClient(), cliprect);
        Parent_->DrawHLine(surface, BorderColor_, 0, Parent_->GetH() - 1, Parent_->GetW(), C_BIT_ABSOLUTE, GetClient(), cliprect);
        Parent_->DrawVLine(surface, BorderColor_, 0, 0, Parent_->GetH(), C_BIT_ABSOLUTE, GetClient(), cliprect);
        Parent_->DrawVLine(surface, BorderColor_, Parent_->GetW() - 1, 0, Parent_->GetH(), C_BIT_ABSOLUTE, GetClient(), cliprect);
    }

    drect = *cliprect;

    fh = static_cast<short>(gFontList->GetHeight(GetFont())); 

    cur = Root_;
    i = 0;

    while (cur and i < Selected_ and i < Count_)
    {
        if ( not (cur->flags_ bitand C_BIT_INVISIBLE))
            i++;

        cur = cur->Next;
    }

    while (cur and (cur->flags_ bitand C_BIT_INVISIBLE))
        cur = cur->Next;

    if (cur)
    {
        if (cur->Type_ == C_TYPE_NOTHING or not (cur->flags_ bitand C_BIT_ENABLED))
            cur = NULL;
    }

    if (Selected_ >= 0 and Selected_ < Count_ and cur)
    {
        drect.left = 2;
        drect.top = 5 + (Selected_ * fh);
        drect.right = drect.left + Parent_->GetW() - 2;
        drect.bottom = drect.top + fh;

        if (Parent_->ClipToArea(&frect, &drect, cliprect)) // frect not used... don't care what's in it
            Parent_->BlitFill(surface, BarColor_, &drect, C_BIT_ABSOLUTE, 0);
    }

    cur = Root_;
    i = 0;

    while (cur)
    {
        if ( not (cur->flags_ bitand C_BIT_INVISIBLE))
        {
            if (cur->Type_ == C_TYPE_NOTHING)
            {
                Parent_->DrawHLine(surface, DisColor_, 5, 5 + (i * fh) + (fh >> 1) - 1, Parent_->GetW() - 10, GetFlags(), GetClient(), cliprect);
                Parent_->DrawHLine(surface, SelColor_, 5, 5 + (i * fh) + (fh >> 1), Parent_->GetW() - 10, GetFlags(), GetClient(), cliprect);
            }
            else
            {
                if (cur->Label_)
                {
                    if ( not (cur->flags_ bitand C_BIT_ENABLED))
                        cur->Label_->SetFgColor(DisColor_);
                    else if (Selected_ == i)
                        cur->Label_->SetFgColor(SelColor_);
                    else
                        cur->Label_->SetFgColor(NormalColor_);

                    cur->Label_->Draw(surface, cliprect);
                }

                if (cur->flags_ bitand C_BIT_ENABLED and cur->Type_ == C_TYPE_MENU and cur->SubMenu_ and cur->MenuIcon_)
                {
                    cur->MenuIcon_->Draw(surface, cliprect);
                }
                else if (cur->flags_ bitand C_BIT_ENABLED and (cur->Type_ == C_TYPE_TOGGLE or cur->Type_ == C_TYPE_RADIO) and cur->State_ and cur->CheckIcon_)
                {
                    cur->CheckIcon_->Draw(surface, cliprect);
                }
            }

            i++;
        }

        cur = cur->Next;
    }
}

void C_PopupList::GetWindowSize(short *w, short *h)
{
    GetSize(w, h);
    (*w) += 48;
    (*h) += 10;

    if ((*w) > Handler_->GetW())
        (*w) = static_cast<short>(Handler_->GetW()); 

    if ((*h) > Handler_->GetH())
        (*h) = static_cast<short>(Handler_->GetH()); 
}

BOOL C_PopupList::OpenWindow(short x, short y, short Dir)
{
    POPUPLIST *cur;
    C_Fill *fill;
    short w, h, i, fh;

    if (Handler_ == NULL)
        return(FALSE);

    fh = static_cast<short>(gFontList->GetHeight(GetFont())); 

    i = 0;
    cur = Root_;

    while (cur)
    {
        if ( not (cur->flags_ bitand C_BIT_INVISIBLE))
        {
            if (cur->MenuIcon_)
            {
                cur->MenuIcon_->SetY(fh / 2 - cur->MenuIcon_->GetH() / 2 + 5 + (i)*fh);
                cur->MenuIcon_->SetInfo();
            }

            if (cur->Label_)
            {
                cur->Label_->SetY(5 + (i)*fh);
                cur->Label_->SetInfo();
            }

            if (cur->CheckIcon_)
            {
                cur->CheckIcon_->SetY(fh / 2 - cur->CheckIcon_->GetH() / 2 + 5 + (i)*fh);
                cur->CheckIcon_->SetInfo();
            }

            i++;
        }

        cur = cur->Next;
    }

    Count_ = i;

    GetWindowSize(&w, &h);

    Direction_ = Dir;
    Window_ = new C_Window;
    Window_->Setup(GetID(), WinType_, w, h);
    Window_->SetFlagBitOn(C_BIT_NOCLEANUP);
    Window_->SetOwner(this);
    Window_->SetMenuFlags(1);

    if (GetFlags() bitand C_BIT_TRANSLUCENT)
        Window_->SetFlagBitOn(C_BIT_TRANSLUCENT);

    if ((x + w) > Handler_->GetW()) x = static_cast<short>(Handler_->GetW() - w - 1); 

    if ((y + h) > Handler_->GetH()) y = static_cast<short>(Handler_->GetH() - h - 1); 

    if (x < 0) x = 0;

    if (y < 0) y = 0;

    Window_->SetRanges(0, 0, static_cast<short>(Handler_->GetW()), static_cast<short>(Handler_->GetH()), w, h); 
    Window_->SetXY(x, y);

    cur = Root_;

    while (cur)
    {
        if ( not (cur->flags_ bitand C_BIT_INVISIBLE))
        {
            if (cur->MenuIcon_)
            {
                cur->MenuIcon_->SetX(w - cur->MenuIcon_->GetW() - 2);
                cur->MenuIcon_->SetInfo();
            }
        }

        cur = cur->Next;
    }

    // Setup BG
    fill = new C_Fill;
    fill->Setup(C_DONT_CARE, C_TYPE_VERTICAL);
    fill->SetXYWH(0, 0, w, h);
    fill->SetFlagBitOn(C_BIT_ABSOLUTE);

    if (GetFlags() bitand C_BIT_TRANSLUCENT)
        fill->SetFlagBitOn(C_BIT_TRANSLUCENT);

    fill->SetColor(BgColor_);
    fill->SetGradient(Opaque_, Opaque_);
    Window_->AddControl(fill);

    // Add Popup menu to window
    Window_->AddControl(this);

    Selected_ = -1;
    Window_->update_ or_eq C_DRAW_REFRESHALL;
    Window_->RefreshWindow();
    Window_->SetDepth(10000);
    Window_->SetCritical(Handler_->GetCritical());
    Handler_->AddWindow(Window_, C_BIT_ENABLED bitor C_BIT_CANTMOVE);

    if (WinType_ == C_TYPE_EXCLUSIVE)
        MouseOver(x - Window_->GetX(), y - Window_->GetY(), this);

    return(TRUE);
}

BOOL C_PopupList::CloseWindow()
{
    Handler_->EnterCritical();
    CloseSubMenus();
    // CloseWindow();
    Handler_->RemoveWindow(Window_);
    Window_->Cleanup();
    delete Window_;
    Window_ = NULL;
    this->SetParent(NULL);
    Handler_->LeaveCritical();
    return(TRUE);
}

#ifdef _UI95_PARSER_
short C_PopupList::LocalFind(char *token)
{
    short i = 0;

    while (C_Pu_Tokens[i])
    {
        if (strnicmp(token, C_Pu_Tokens[i], strlen(C_Pu_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_PopupList::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *Hndlr)
{
    switch (ID)
    {
        case CPU_SETUP:
            Setup(P[0], (short)P[1], Hndlr, P[2], P[3]);
            break;

        case CPU_ADDITEM:
            AddItem(P[0], (short)P[1], P[2], P[3]);
            break;

        case CPU_SETITEMFLAGBITON:
            SetItemFlagBitOn(P[0], P[1]);
            break;

        case CPU_SETITEMFLAGBITOFF:
            SetItemFlagBitOff(P[0], P[1]);
            break;

        case CPU_SETGROUP:
            SetItemGroup(P[0], P[1]);
            break;

        case CPU_SETSTATE:
            SetItemState(P[0], (short)P[1]);
            break;

        case CPU_SETNORMCOLOR:
            SetNormColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CPU_SETSELCOLOR:
            SetSelColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CPU_SETDISCOLOR:
            SetDisColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CPU_SETBARCOLOR:
            SetBarColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CPU_SETBGCOLOR:
            SetBgColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CPU_SETOPAQUE:
            SetOpaque(P[0]);
            break;

        case CPU_SETBORDERCOLOR:
            SetBorderColor(P[0] bitor ((P[1] bitand 0xff) << 8) bitor ((P[2] bitand 0xff) << 16));
            break;
    }
}

#endif // PARSER
