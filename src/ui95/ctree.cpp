#include <windows.h>
#include "chandler.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CTL_NOTHING = 0,
    CTL_SETUP,
    CTL_SETXOFFSET,
    CTL_SETYOFFSET,
    CTL_SETIMAGES,
    CTL_SETSORT,
    CTL_ADDTEXTITEM,
    CTL_ADDBITMAPITEM,
    CTL_ADDHELPITEM,
    CTL_HELPITEMIMAGE,
    CTL_HELPITEMTEXT,
    CTL_HELPITEMFONT,
    CTL_HELPITEMFLAGON,
    CTL_HELPITEMFLAGOFF,
    CTL_ADDWORDWRAPITEM,
};

char *C_Tl_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[XOFFSET]",
    "[YOFFSET]",
    "[IMAGES]",
    "[SORTBY]",
    "[ADDTEXTITEM]",
    "[ADDBITMAPITEM]",
    "[ADDHELPITEM]",
    "[HELPITEMIMAGE]",
    "[HELPITEMTEXT]",
    "[HELPITEMFONT]",
    "[HELPITEMFLAGON]",
    "[HELPITEMFLAGOFF]",
    "[ADDWORDWRAPITEM]",
    0,
};

#endif

C_TreeList::C_TreeList() : C_Control()
{
    _SetCType_(_CNTL_TREELIST_);
    Root_ = NULL;
    Hash_ = NULL;
    ChildImage_[0] = NULL;
    ChildImage_[1] = NULL;
    ChildImage_[2] = NULL;
    xoffset_ = 0;
    yoffset_ = 0;
    treew_ = 0;
    treeh_ = 0;
    CheckFlag_ = 0;
    LastActive_ = NULL;
    LastFound_ = NULL;
    MouseFound_ = NULL;
    DelCallback_ = NULL;
    SearchCB_ = NULL;
    SortCB_ = NULL;
    SortType_ = TREE_SORT_BY_ID;
    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE bitor C_BIT_SELECTABLE bitor C_BIT_MOUSEOVER;
}

C_TreeList::C_TreeList(char **stream) : C_Control(stream)
{
}

C_TreeList::C_TreeList(FILE *fp) : C_Control(fp)
{
}

C_TreeList::~C_TreeList()
{
    if (Root_ or Hash_)
        Cleanup();
}

long C_TreeList::Size()
{
    return(0);
}

void C_TreeList::Setup(long ID, short Type)
{
    SetReady(0);
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();
    Hash_ = new C_Hash;
    Hash_->Setup(512);
}

void C_TreeList::SetImages(long Closed, long Open, long Disd)
{
    ChildImage_[0] = gImageMgr->GetImage(Closed);
    ChildImage_[1] = gImageMgr->GetImage(Open);
    ChildImage_[2] = gImageMgr->GetImage(Disd);
}

void C_TreeList::DeleteBranch(TREELIST *top)
{
    TREELIST *last;

    if (top == NULL)
        return;

    F4CSECTIONHANDLE* Leave = UI_Enter(Parent_);

    if (top == Root_)
        Root_ = NULL;

    if (top->Parent)
        top->Parent->Child = NULL;

    while (top)
    {
        if (top->Child)
        {
            DeleteBranch(top->Child);
            top->Child = NULL;
        }

        last = top;
        top = top->Next;

        if (last == LastActive_)
            LastActive_ = NULL;

        if (last == LastFound_)
            LastFound_ = NULL;

        if (last == MouseFound_)
            MouseFound_ = NULL;

        if (DelCallback_)
            (*DelCallback_)(last);

        if (last->Item_)
        {
            if (last->Item_->GetFlags() bitand C_BIT_REMOVE)
            {
                if (Parent_)
                    Parent_->RemovingControl(last->Item_);

                last->Item_->Cleanup();
                delete last->Item_;
                last->Item_ = NULL;
            }
            else
            {
                OutputDebugString("Hidi Ho");
            }
        }

        Hash_->Remove(last->ID_);
        delete last;
    }

    UI_Leave(Leave);
}

void C_TreeList::Cleanup()
{
    if (Root_)
    {
        DeleteBranch(Root_);
        Root_ = NULL;
    }

    if (Hash_)
    {
        Hash_->Cleanup();
        delete Hash_;
        Hash_ = NULL;
    }
}

void C_TreeList::DeleteItem(long ID)
{
    TREELIST *cur;

    cur = Find(ID);

    if (cur == NULL)
        return;

    DeleteItem(cur);
}

void C_TreeList::DeleteItem(TREELIST *item)
{
    if ( not item)
    {
        return;
    }

    F4CSECTIONHANDLE *Leave;
    Leave = UI_Enter(Parent_);

    if (LastFound_ == item)
    {
        LastFound_ = NULL;
    }

    if (Root_ == item)
    {
        Root_ = item->Next;

        if (Root_)
        {
            Root_->Prev = NULL;
        }
    }

    if (Parent_)
    {
        Parent_->RemovingControl(item->Item_);
    }

    if (item == LastActive_)
    {
        LastActive_ = NULL;
    }

    if (item == LastFound_)
    {
        LastFound_ = NULL;
    }

    if (item == MouseFound_)
    {
        MouseFound_ = NULL;
    }

    // sfr: remove JB hack and fix the child parent relations
    // FRB - removed
    //#define CTREE_FIX 1
#if CTREE_FIX

    if ((item->Parent) and (item->Parent->Child == item))
    {
        item->Parent->Child = item->Next;
    }

    // sfr: shouldnt we also fix children stuff?
    if ((item->Child) and (item->Child->Parent == item))
    {
        item->Child->Parent = item->Parent;
    }

#else

    if ( not F4IsBadReadPtr(item->Parent, sizeof(TREELIST)) and // JB 010317 CTD
 not F4IsBadReadPtr(item->Parent->Child, sizeof(TREELIST)) and // M.N. 011209 CTD
        (item->Parent) and (item->Parent->Child == item))
    {
        item->Parent->Child = item->Next;
    }

#endif

    if (item->Prev)
    {
        item->Prev->Next = item->Next;
    }

    if (item->Next)
    {
        item->Next->Prev = item->Prev;
    }

    if (DelCallback_)
    {
        (*DelCallback_)(item);
    }

    if (item->Item_)
    {
        if (item->Item_->GetFlags() bitand C_BIT_REMOVE)
        {
            item->Item_->Cleanup();
            delete item->Item_;
        }
    }

    Hash_->Remove(item->ID_); // Note: This deletes item also
    delete item;

    UI_Leave(Leave);
}

TREELIST *C_TreeList::CreateItem(long NewID, long ItemType, C_Base *Item)
{
    TREELIST *NewItem;

    if (Item == NULL)
        return(NULL);

    NewItem = new TREELIST;

    if (NewItem == NULL)
        return(NULL);

    NewItem->ID_ = NewID;
    NewItem->Type_ = ItemType;
    NewItem->state_ = 0; // Menu Open/Closed (Max Value is 1)
    NewItem->Item_ = Item;
    NewItem->x_ = NewItem->y_ = 0; // JPO set
    Item->SetClient(GetClient());
    Item->SetParent(Parent_);
    Item->SetSubParents(Parent_);

    NewItem->Next = NULL;
    NewItem->Prev = NULL;
    NewItem->Parent = NULL;
    NewItem->Child = NULL;
    return(NewItem);
}

void C_TreeList::Add(TREELIST *current, TREELIST *NewItem)
{
    if (Root_ == NULL)
    {
        Root_ = NewItem;
    }
    else
    {
        while (current->Next and current->Type_ < NewItem->Type_)
            current = current->Next;

        if (SortType_ == TREE_SORT_BY_ID)
        {
            while (current->Next and current->ID_ <= NewItem->ID_)
                current = current->Next;

            if (NewItem->Type_ < current->Type_)
            {
                if (Root_ == current)
                    Root_ = NewItem;

                NewItem->Parent = current->Parent;
                NewItem->Prev = current->Prev;

                if (NewItem->Prev)
                    NewItem->Prev->Next = NewItem;

                NewItem->Next = current;

                if (current->Parent and not current->Prev)
                    current->Parent->Child = NewItem;

                current->Prev = NewItem;
            }
            else if (NewItem->Type_ == current->Type_ and NewItem->ID_ < current->ID_)
            {
                // Insert before
                if (current->Parent and current->Parent->Child == current)
                    current->Parent->Child = NewItem;

                if (Root_ == current)
                    Root_ = NewItem;

                NewItem->Parent = current->Parent;
                NewItem->Prev = current->Prev;

                if (NewItem->Prev)
                    NewItem->Prev->Next = NewItem;

                NewItem->Next = current;

                if (current->Parent and not current->Prev)
                    current->Parent->Child = NewItem;

                current->Prev = NewItem;
            }
            else
            {
                // Insert After
                NewItem->Parent = current->Parent;
                NewItem->Prev = current;
                NewItem->Next = current->Next;
                current->Next = NewItem;

                if (NewItem->Next)
                    NewItem->Next->Prev = NewItem;
            }
        }
        else if (SortType_ == TREE_SORT_BY_ITEM_ID)
        {
            while (current->Next and current->Item_->GetID() <= NewItem->Item_->GetID())
                current = current->Next;

            if (NewItem->Type_ < current->Type_)
            {
                if (Root_ == current)
                    Root_ = NewItem;

                NewItem->Parent = current->Parent;
                NewItem->Prev = current->Prev;

                if (NewItem->Prev)
                    NewItem->Prev->Next = NewItem;

                NewItem->Next = current;

                if (current->Parent and not current->Prev)
                    current->Parent->Child = NewItem;

                current->Prev = NewItem;
            }
            else if (NewItem->Type_ == current->Type_ and NewItem->Item_->GetID() < current->Item_->GetID())
            {
                // Insert before
                if (current->Parent and current->Parent->Child == current)
                    current->Parent->Child = NewItem;

                if (Root_ == current)
                    Root_ = NewItem;

                NewItem->Parent = current->Parent;
                NewItem->Prev = current->Prev;

                if (NewItem->Prev)
                    NewItem->Prev->Next = NewItem;

                NewItem->Next = current;

                if (current->Parent and not current->Prev)
                    current->Parent->Child = NewItem;

                current->Prev = NewItem;
            }
            else
            {
                // Insert After
                NewItem->Parent = current->Parent;
                NewItem->Prev = current;
                NewItem->Next = current->Next;
                current->Next = NewItem;

                if (NewItem->Next)
                    NewItem->Next->Prev = NewItem;
            }
        }
        else if (SortType_ == TREE_SORT_CALLBACK and SortCB_)
        {
            while (current->Next and not (*SortCB_)(current, NewItem))
                current = current->Next;

            if ((NewItem->Type_ < current->Type_) or (NewItem->Type_ == current->Type_ and (*SortCB_)(current, NewItem)))
            {
                // Insert before
                if (current->Parent and current->Parent->Child == current)
                    current->Parent->Child = NewItem;

                if (Root_ == current)
                    Root_ = NewItem;

                NewItem->Parent = current->Parent;
                NewItem->Prev = current->Prev;

                if (NewItem->Prev)
                    NewItem->Prev->Next = NewItem;

                NewItem->Next = current;

                if (current->Parent and not current->Prev)
                    current->Parent->Child = NewItem;

                current->Prev = NewItem;
            }
            else
            {
                // Insert After
                NewItem->Parent = current->Parent;
                NewItem->Prev = current;
                NewItem->Next = current->Next;
                current->Next = NewItem;

                if (NewItem->Next)
                    NewItem->Next->Prev = NewItem;
            }
        }
        else
        {
            while (current->Next)
                current = current->Next;

            current->Next = NewItem;
            NewItem->Prev = current;
            NewItem->Parent = current->Parent;
        }
    }
}

void C_TreeList::AddChild(TREELIST *par, TREELIST *NewItem)
{
    if (par->Child == NULL)
    {
        par->Child = NewItem;
        NewItem->Parent = par;
    }
    else
        Add(par->Child, NewItem);
}

BOOL C_TreeList::AddChildItem(TREELIST *par, TREELIST *NewItem)
{
    if (par == NULL or Root_ == NULL or NewItem == NULL)
        return(FALSE);

    if (Hash_->Find(NewItem->ID_))
        return(FALSE);

    if (par->Type_ ==  C_TYPE_INFO) // No SUB items can be attached to this type
        return(FALSE);

    if ( not NewItem->Item_->GetCursorID())
        NewItem->Item_->SetCursorID(GetCursorID());

    if ( not NewItem->Item_->GetDragCursorID())
        NewItem->Item_->SetDragCursorID(GetDragCursorID());

    F4CSECTIONHANDLE* Leave = UI_Enter(Parent_);

    if (NewItem->Type_ not_eq C_TYPE_INFO)
        Hash_->Add(NewItem->ID_, NewItem);

    AddChild(par, NewItem);
    UI_Leave(Leave);
    return(TRUE);
}

BOOL C_TreeList::AddItem(TREELIST *current, TREELIST *NewItem)
{
    if (( not current and Root_) or not NewItem or not NewItem->Item_)
        return(FALSE);

    if (Hash_->Find(NewItem->ID_))
        return(FALSE);

    if ( not NewItem->Item_->GetCursorID())
        NewItem->Item_->SetCursorID(GetCursorID());

    if ( not NewItem->Item_->GetDragCursorID())
        NewItem->Item_->SetDragCursorID(GetDragCursorID());

    F4CSECTIONHANDLE* Leave = UI_Enter(Parent_);
    Add(current, NewItem);

    if (NewItem->Type_ not_eq C_TYPE_INFO)
        Hash_->Add(NewItem->ID_, NewItem);

    UI_Leave(Leave);
    return(TRUE);
}

void C_TreeList::MoveChildItem(TREELIST *Parent, TREELIST *item)
{
    F4CSECTIONHANDLE *Leave;

    if (item and Parent and item not_eq Parent)
    {
        Leave = UI_Enter(Parent_);

        if (item == Root_)
            Root_ = item->Next;

        if (item->Next)
            item->Next->Prev = item->Prev;

        if (item->Prev)
            item->Prev->Next = item->Next;

        if (item->Parent and item->Parent->Child == item)
            item->Parent->Child = item->Next;

        if (LastFound_ == item)
            LastFound_ = NULL;

        item->Next = NULL;
        item->Prev = NULL;
        item->Parent = NULL;

        AddChild(Parent, item);
        UI_Leave(Leave);
    }
}

BOOL C_TreeList::ChangeItemID(TREELIST *item, long NewID)
{
    if ( not item or Find(NewID))
        return(FALSE);

    if (item->Type_ not_eq C_TYPE_INFO)
    {
        Hash_->Remove(item->ID_);
        item->ID_ = NewID;
        Hash_->Add(NewID, item);
    }
    else
        item->ID_ = NewID;

    return(TRUE);
}

TREELIST *C_TreeList::Find(long cID)
{
    return((TREELIST *)Hash_->Find(cID));
}

TREELIST *C_TreeList::FindOpen(long cID)
{
    TREELIST *item, *cur;

    item = Find(cID);

    if (item)
    {
        cur = item;

        while (cur->Parent)
        {
            if ( not cur->Parent->state_)
                return(NULL);

            cur = cur->Parent;
        }

        return(item);
    }

    return(NULL);
}

TREELIST *C_TreeList::FindItemWithCB(TREELIST *me)
{
    TREELIST *tmp;

    if (me == NULL)
        me = Root_;

    if (me == NULL)
        return(NULL);

    if ((*SearchCB_)(me))
        return(me);

    if (me->Child)
    {
        tmp = FindItemWithCB(me->Child);

        if (tmp)
            return(tmp);
    }

    if (me->Next)
        return(FindItemWithCB(me->Next));

    return(NULL);
}


TREELIST *C_TreeList::GetNextBranch(TREELIST *me)
{
    if (me)
        return(me->Next);

    return(NULL);
}

TREELIST *C_TreeList::GetChild(TREELIST *me)
{
    if (me)
        return(me->Child);

    return(NULL);
}

void C_TreeList::SetItemState(long ItemID, short newstate)
{
    TREELIST *item;

    item = Find(ItemID);

    if (item)
        item->state_ = newstate bitand 1;
}

void C_TreeList::ToggleItemState(long ItemID)
{
    TREELIST *item;

    item = Find(ItemID);

    ToggleItemState(item);
}

void C_TreeList::SetAllBranches(long Mask, short newstate, TREELIST *me)
{
    TREELIST *cur;

    cur = me;

    while (cur)
    {
        if (cur->Type_ == Mask)
            cur->state_ = newstate bitand 1;

        if (cur->Child)
            SetAllBranches(Mask, newstate, cur->Child);

        cur = cur->Next;
    }
}

void C_TreeList::SetAllControlStates(short newstate, TREELIST *me)
{
    TREELIST *cur;

    cur = me;

    while (cur)
    {
        if (cur->Type_ not_eq C_TYPE_INFO)
            cur->Item_->SetState(newstate);

        if (cur->Child)
            SetAllControlStates(newstate, cur->Child);

        cur = cur->Next;
    }
}

long C_TreeList::GetMenu()
{
    if (LastFound_ and LastFound_->Item_ and LastFound_->Item_->GetMenu())
        return(LastFound_->Item_->GetMenu());

    return(C_Control::GetMenu());
}

void C_TreeList::ClearAllStates(long Mask)
{
    SetAllBranches(Mask, 0, Root_);
}

void C_TreeList::RemoveOldBranch(long UserSlot, long Age, TREELIST *me)
{
    TREELIST *cur;

    cur = me;

    while (cur)
    {
        me = cur;
        cur = cur->Next;

        if (me->Type_ not_eq C_TYPE_INFO)
        {
            if (me->Item_)
            {
                if (me->Item_->GetUserNumber(UserSlot) and me->Item_->GetUserNumber(UserSlot) not_eq Age)
                {
                    DeleteItem(me);
                }
                else
                {
                    if (me->Child)
                        RemoveOldBranch(UserSlot, Age, me->Child);
                }
            }
        }
        else if (me->Type_ == C_TYPE_INFO)
        {
            if (me->Item_)
            {
                if ((me->Item_->GetUserNumber(UserSlot)) and (me->Item_->GetUserNumber(UserSlot) < Age))
                {
                    DeleteItem(me);
                }
            }
        }
    }
}

void C_TreeList::RemoveOld(long UserSlot, long Age)
{
    F4CSECTIONHANDLE *Leave;

    Leave = UI_Enter(Parent_);
    RemoveOldBranch(UserSlot, Age, Root_);
    UI_Leave(Leave);
}

BOOL C_TreeList::FindVisible(TREELIST *top)
{
    TREELIST *item;

    item = top;

    while (item)
    {
        if ( not (item->Item_->GetFlags() bitand C_BIT_INVISIBLE))
            return(TRUE);

        item = item->Next;
    }

    return(FALSE);
}

long C_TreeList::CalculateTreePositions(TREELIST *top, long offx, long offy)
{
    TREELIST *current;
    long width;

    current = top;

    while (current)
    {
        if (current->Item_ and not (current->Item_->GetFlags() bitand C_BIT_INVISIBLE))
        {
            current->x_ = offx;
            current->y_ = offy;

            current->Item_->SetXY(current->x_ + xoffset_, current->y_);

            if (current->Item_->GetH() < yoffset_)
                offy += yoffset_;
            else
                offy += current->Item_->GetH();

            width = current->Item_->GetX() + current->Item_->GetW();

            if (width > treew_)
                treew_ = width;

            if (current->state_ and current->Child)
                offy = CalculateTreePositions(current->Child, offx + xoffset_, offy);
        }

        current = current->Next;
    }

    return(offy);
}

TREELIST *C_TreeList::CheckBranch(TREELIST *me, long mx, long my)
{
    TREELIST *cur, *tmp;

    cur = me;

    while (cur)
    {
        if (cur->Item_ and not (cur->Item_->GetFlags() bitand C_BIT_INVISIBLE))
        {
            if (cur->Child and ChildImage_[0] and FindVisible(cur->Child))
            {
                if (mx >= (cur->x_ + 2) and mx <= (cur->x_ + (ChildImage_[0]->Header->w)) and 
                    my >= (cur->y_ + 2) and my <= (cur->y_ + ChildImage_[0]->Header->h))
                {
                    CheckFlag_ = C_TYPE_MENU;
                    return(cur);
                }
            }

            if (cur->Item_->CheckHotSpots(mx, my))
            {
                CheckFlag_ = C_TYPE_ITEM;
                return(cur);
            }

            if (cur->state_ and cur->Child)
            {
                tmp = CheckBranch(cur->Child, mx, my);

                if (tmp)
                    return(tmp);
            }
        }

        cur = cur->Next;
    }

    return(NULL);
}

void C_TreeList::RecalcSize()
{
    F4CSECTIONHANDLE *Leave;

    if ( not Ready()) return;

    if (Parent_)
    {
        Leave = UI_Enter(Parent_);
        treew_ = 0;
        treeh_ = CalculateTreePositions(Root_, GetX(), GetY());
        treeh_ += yoffset_;

        SetWH(treew_, treeh_);

        Parent_->ScanClientArea(GetClient());

        UI_Leave(Leave);
    }
}

long C_TreeList::CheckHotSpots(long relX, long relY)
{
    TREELIST *cur;

    if (GetFlags() bitand C_BIT_INVISIBLE or not (GetFlags() bitand C_BIT_ENABLED))
        return(0);

    CheckFlag_ = C_BIT_NOTHING; // (0)
    cur = CheckBranch(Root_, relX, relY);

    if (cur == NULL)
    {
        LastFound_ = NULL;
        return(0);
    }

    if (LastFound_ not_eq cur)
    {
        Parent_->DeactivateControl();
    }

    LastFound_ = cur;
    SetRelXY(relX - GetX(), relY - GetY());
    return(GetID());
}

void C_TreeList::Activate()
{
    if (LastActive_ == LastFound_)
        return;

    if (LastActive_ and LastActive_->Item_)
        LastActive_->Item_->Deactivate();

    if (LastFound_ and LastFound_->Item_)
    {
        LastActive_ = LastFound_;
        LastFound_->Item_->Activate();
    }
}

void C_TreeList::Deactivate()
{
    if (LastActive_ and LastActive_->Item_)
        LastActive_->Item_->Deactivate();

    LastActive_ = NULL;
}

C_Base *C_TreeList::GetMe()
{
    if (MouseFound_)
    {
        if (CheckFlag_ == C_TYPE_MENU)
            return(this);

        return(MouseFound_->Item_);
    }

    return(NULL);
}

BOOL C_TreeList::CheckKeyboard(unsigned char DKScanCode, unsigned char Ascii, unsigned char ShiftStates, long RepeatCount)
{
    if (LastFound_ and LastFound_->Item_)
        return(LastFound_->Item_->CheckKeyboard(DKScanCode, Ascii, ShiftStates, RepeatCount));

    return(FALSE);
}

BOOL C_TreeList::Process(long cID, short HitType)
{
    if (GetFlags() bitand C_BIT_INVISIBLE or not (GetFlags() bitand C_BIT_ENABLED))
        return(0);

    if (CheckFlag_ == C_BIT_NOTHING) return(FALSE); // CheckFlag is the segment of the button pressed (0=Nothing)

    gSoundMgr->PlaySound(GetSound(HitType));

    switch (HitType)
    {
        case C_TYPE_LMOUSEDOWN:
            switch (CheckFlag_)
            {
                case C_TYPE_MENU:
                    if (LastFound_)
                    {
                        Refresh();
                        ToggleItemState(LastFound_);

                        if (Callback_)
                            (*Callback_)(GetID(), HitType, this);

                        RecalcSize();

                        Refresh();
                    }

                    break;

                case C_TYPE_ITEM:
                    if (LastFound_)
                    {
                        LastFound_->Item_->Process(cID, HitType);

                        if (Callback_)
                            (*Callback_)(cID, HitType, this);
                    }

                    break;
            }

            break;

        case C_TYPE_LMOUSEUP:
        case C_TYPE_LMOUSEDBLCLK:
            switch (CheckFlag_)
            {
                case C_TYPE_ITEM:
                    if (LastFound_)
                    {
                        LastFound_->Item_->Process(cID, HitType);

                        if (Callback_)
                            (*Callback_)(cID, HitType, this);
                    }

                    break;
            }

            break;
    }

    return(TRUE);
}

void C_TreeList::Refresh()
{
    if ( not Ready() or GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    Parent_->SetUpdateRect(GetX(), GetY(), GetX() + GetW() + 1, GetY() + GetH() + 1, GetFlags(), GetClient());
}

void C_TreeList::DrawBranch(SCREEN *surface, TREELIST *branch, UI95_RECT *cliprect)
{
    TREELIST *current;
    UI95_RECT src, dest;
    BOOL doit;

    current = branch;

    while (current)
    {
        if (current->Item_ and not (current->Item_->GetFlags() bitand C_BIT_INVISIBLE))
        {
            if (Parent_->InsideClientHeight(current->y_ - current->Item_->GetH(), current->y_ + current->Item_->GetH(), GetClient()))
            {
                if (current->Type_ not_eq C_TYPE_INFO)
                {
                    if (current->Child and FindVisible(current->Child))
                    {
                        if (ChildImage_[current->state_])
                        {
                            src.left = 0;
                            src.top = 0;
                            src.right = ChildImage_[current->state_]->Header->w;
                            src.bottom = ChildImage_[current->state_]->Header->h;

                            dest.left = current->x_ + 2;

                            if (Flags_ bitand C_BIT_VCENTER)
                                dest.top = current->y_ + current->Item_->GetH() / 2 - (src.bottom - src.top) / 2;
                            else
                                dest.top = current->y_ + 2;

                            dest.right = dest.left + src.right - src.left;
                            dest.bottom = dest.top + src.bottom - src.top;

                            doit = TRUE;

                            if (GetFlags() bitand C_BIT_ABSOLUTE)
                            {
                                if ( not Parent_->ClipToArea(&src, &dest, &Parent_->Area_))
                                    doit = FALSE;
                            }
                            else
                            {
                                dest.left += Parent_->VX_[GetClient()];
                                dest.top += Parent_->VY_[GetClient()];
                                dest.right += Parent_->VX_[GetClient()];
                                dest.bottom += Parent_->VY_[GetClient()];

                                if ( not Parent_->ClipToArea(&src, &dest, &Parent_->ClientArea_[GetClient()]))
                                    doit = FALSE;
                            }

                            if ( not Parent_->ClipToArea(&src, &dest, cliprect))
                                doit = FALSE;

                            if (doit)
                            {
                                dest.left += Parent_->GetX();
                                dest.top += Parent_->GetY();
                                dest.right += Parent_->GetX();
                                dest.bottom += Parent_->GetY();

                                ChildImage_[current->state_]->Blit(surface, src.left, src.top, src.right - src.left, src.bottom - src.top,
                                                                   dest.left, dest.top);
                            }
                        }
                    }

                    current->Item_->Draw(surface, cliprect);
                }
                else
                    current->Item_->Draw(surface, cliprect);
            }

            if (current->state_ and current->Child)
                DrawBranch(surface, current->Child, cliprect);
        }

        current = current->Next;
    }

    if (MouseOver_ or (GetFlags() bitand C_BIT_FORCEMOUSEOVER))
        HighLite(surface, cliprect);
}

void C_TreeList::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    if ( not Ready())
        return;

    if (GetFlags() bitand C_BIT_INVISIBLE)
        return;

    DrawBranch(surface, Root_, cliprect);
}

void C_TreeList::HighLite(SCREEN *surface, UI95_RECT *cliprect)
{
    UI95_RECT clip, tmp;

    if (MouseFound_ and CheckFlag_ == C_TYPE_MENU)
    {
        clip.left = MouseFound_->x_ + 2;
        clip.top = MouseFound_->y_ + 2;

        if ( not (Flags_ bitand C_BIT_ABSOLUTE))
        {
            clip.left += Parent_->VX_[Client_];
            clip.top += Parent_->VY_[Client_];
        }

        clip.right = clip.left + ChildImage_[0]->Header->w;
        clip.bottom = clip.top + ChildImage_[0]->Header->h;

        if ( not Parent_->ClipToArea(&tmp, &clip, cliprect))
            return;

        if ( not (Flags_ bitand C_BIT_ABSOLUTE))
            if ( not Parent_->ClipToArea(&tmp, &clip, &Parent_->ClientArea_[Client_]))
                return;

        Parent_->BlitTranslucent(surface, MouseOverColor_, MouseOverPercent_, &clip, C_BIT_ABSOLUTE, 0);
    }
}

BOOL C_TreeList::MouseOver(long relx, long rely, C_Base *me)
{
    if (GetFlags() bitand C_BIT_INVISIBLE or not (GetFlags() bitand C_BIT_ENABLED))
        return(FALSE);

    CheckFlag_ = C_BIT_NOTHING; // (0)
    MouseFound_ = CheckBranch(Root_, relx, rely);

    if (MouseFound_)
    {
        if (CheckFlag_ == C_TYPE_MENU)
            return(TRUE);
        else if (MouseFound_->Item_)
            return(MouseFound_->Item_->MouseOver(relx, rely, me));
    }

    MouseFound_ = NULL;
    return(FALSE);
}

void C_TreeList::SetControlParents(TREELIST *me)
{
    while (me)
    {
        if (me->Item_)
        {
            me->Item_->SetParent(Parent_);
            me->Item_->SetClient(GetClient());
            me->Item_->SetSubParents(Parent_);
        }

        if (me->Child)
            SetControlParents(me->Child);

        me = me->Next;
    }
}

void C_TreeList::SetSubParents(C_Window *)
{
    SetReady(0);

    if (Parent_ == NULL)
        return;

    SetControlParents(Root_);
    SetReady(1);
    RecalcSize();
}

void C_TreeList::GetItemXY(long ID, long *x, long *y)
{
    TREELIST *cur;

    cur = FindOpen(ID);

    if (cur == NULL)
        return;

    *x = cur->x_;
    *y = cur->y_;
}

// This ASSUMES you have changed the Sorting criteria somehow
// Otherwise, you are just wasting time
// THIS MUST BE DONE IN A CRITICAL SECTION
void C_TreeList::ReorderBranch(TREELIST *branch)
{
    TREELIST *cur, *top, *limb;

    if ( not branch)
        return;

    cur = branch;

    while (cur)
    {
        if (cur->Child)
            ReorderBranch(cur->Child);

        cur = cur->Next;
    }

    // Sort here
    top = branch;
    limb = top->Next;

    if (limb)
    {
        top->Next = NULL;
        cur = limb;

        while (cur)
        {
            limb = cur;
            cur = cur->Next;
            Add(top, limb);

            while (top->Prev)
                top = top->Prev;
        }

        if (top->Parent)
            top->Parent->Child = top;
    }
}

void C_TreeList::AddTextItem(long ID, long Type, long ParentID, long TextID, long color)
{
    C_Text *txt;
    TREELIST *item;

    txt = new C_Text;
    txt->Setup(ID, C_TYPE_NORMAL);
    txt->SetText(TextID);
    txt->SetFGColor(color);
    txt->SetFont(Font_);

    item = CreateItem(ID, Type, txt);

    if (item)
    {
        if (ParentID)
        {
            if ( not AddChildItem(Find(ParentID), item))
            {
                txt->Cleanup();
                delete txt;
                delete item;
            }
        }
        else
        {
            if ( not AddItem(Root_, item))
            {
                txt->Cleanup();
                delete txt;
                delete item;
            }
        }
    }
    else
    {
        txt->Cleanup();
        delete txt;
    }
}

void C_TreeList::AddTextItem(long ID, long Type, long ParentID, char *Text, long color)
{
    C_Text *txt;
    TREELIST *item;

    txt = new C_Text;
    txt->Setup(ID, C_TYPE_NORMAL);
    txt->SetText(Text);
    txt->SetFGColor(color);
    txt->SetFont(Font_);

    item = CreateItem(ID, Type, txt);

    if (item)
    {
        if (ParentID)
        {
            if ( not AddChildItem(Find(ParentID), item))
            {
                txt->Cleanup();
                delete txt;
                delete item;
            }
        }
        else
        {
            if ( not AddItem(Root_, item))
            {
                txt->Cleanup();
                delete txt;
                delete item;
            }
        }
    }
    else
    {
        txt->Cleanup();
        delete txt;
    }
}

void C_TreeList::AddWordWrapItem(long ID, long Type, long ParentID, long TextID, long w, long color)
{
    C_Text *txt;
    TREELIST *item;

    txt = new C_Text;
    txt->Setup(ID, C_TYPE_NORMAL);
    txt->SetText(TextID);
    txt->SetW(w);
    txt->SetFGColor(color);
    txt->SetFont(Font_);
    txt->SetFlagBitOn(C_BIT_WORDWRAP);

    item = CreateItem(ID, Type, txt);

    if (item)
    {
        if (ParentID)
        {
            if ( not AddChildItem(Find(ParentID), item))
            {
                txt->Cleanup();
                delete txt;
                delete item;
            }
        }
        else
        {
            if ( not AddItem(Root_, item))
            {
                txt->Cleanup();
                delete item;
                delete txt;
            }
        }
    }
    else
    {
        txt->Cleanup();
        delete txt;
    }
}

void C_TreeList::AddBitmapItem(long ID, long Type, long ParentID, long ImageID)
{
    C_Bitmap *bmp;
    TREELIST *item;

    bmp = new C_Bitmap;
    bmp->Setup(ID, C_TYPE_NORMAL, ImageID);

    item = CreateItem(ID, Type, bmp);

    if (item)
    {
        if (ParentID)
        {
            if ( not AddChildItem(Find(ParentID), item))
            {
                bmp->Cleanup();
                delete bmp;
                delete item;
            }
        }
        else
        {
            if ( not AddItem(Root_, item))
            {
                bmp->Cleanup();
                delete bmp;
                delete item;
            }
        }
    }
    else
    {
        bmp->Cleanup();
        delete bmp;
    }
}

void C_TreeList::AddHelpItem(long ID, long Type, long ParentID)
{
    C_Help *hlp;
    TREELIST *item;

    hlp = new C_Help;
    hlp->Setup(ID, C_TYPE_NORMAL);

    item = CreateItem(ID, Type, hlp);

    if (item)
    {
        if (ParentID)
        {
            if ( not AddChildItem(Find(ParentID), item))
            {
                hlp->Cleanup();
                delete hlp;
                delete item;
            }
        }
        else
        {
            if ( not AddItem(Root_, item))
            {
                hlp->Cleanup();
                delete hlp;
                delete item;
            }
        }
    }
    else
    {
        hlp->Cleanup();
        delete hlp;
    }
}

void C_TreeList::SetHelpItemImage(long ID, long ImageID, long x, long y)
{
    TREELIST *item;

    item = Find(ID);

    if (item and item->Item_)
    {
        if (item->Item_->_GetCType_() == _CNTL_HELP_)
        {
            ((C_Help*)item->Item_)->SetImage(x, y, ImageID);
        }
    }
}

void C_TreeList::SetHelpItemText(long ID, long TextID, long x, long y, long w, long color)
{
    TREELIST *item;

    item = Find(ID);

    if (item and item->Item_)
    {
        if (item->Item_->_GetCType_() == _CNTL_HELP_)
        {
            ((C_Help*)item->Item_)->SetText(x, y, w, TextID);
            ((C_Help*)item->Item_)->SetFgColor(color);
        }
    }
}

void C_TreeList::SetHelpFlagOn(long ID, long Flag)
{
    TREELIST *item;

    item = Find(ID);

    if (item and item->Item_)
    {
        if (item->Item_->_GetCType_() == _CNTL_HELP_)
        {
            item->Item_->SetFlagBitOn(Flag);
        }
    }
}

void C_TreeList::SetHelpFlagOff(long ID, long Flag)
{
    TREELIST *item;

    item = Find(ID);

    if (item and item->Item_)
    {
        if (item->Item_->_GetCType_() == _CNTL_HELP_)
        {
            item->Item_->SetFlagBitOff(Flag);
        }
    }
}

void C_TreeList::SetHelpItemFont(long ID, long FontID)
{
    TREELIST *item;

    item = Find(ID);

    if (item and item->Item_)
    {
        if (item->Item_->_GetCType_() == _CNTL_HELP_)
        {
            item->Item_->SetFont(FontID);
        }
    }
}

#ifdef _UI95_PARSER_
short C_TreeList::LocalFind(char *token)
{
    short i = 0;

    while (C_Tl_Tokens[i])
    {
        if (strnicmp(token, C_Tl_Tokens[i], strlen(C_Tl_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_TreeList::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CTL_SETUP:
            Setup(P[0], (short)P[1]);
            break;

        case CTL_SETXOFFSET:
            SetXOffset((long)P[0]);
            break;

        case CTL_SETYOFFSET:
            SetMinYOffset((long)P[0]);
            break;

        case CTL_SETIMAGES:
            SetImages(P[0], P[1], P[2]);
            break;

        case CTL_SETSORT:
            SetSortType((short)P[0]);
            break;

        case CTL_ADDTEXTITEM:
            AddTextItem(P[0], P[1], P[2], P[3], P[4] bitor (P[5] << 8) bitor (P[6] << 16));
            break;

        case CTL_ADDBITMAPITEM:
            AddBitmapItem(P[0], P[1], P[2], P[3]);
            break;

        case CTL_ADDHELPITEM:
            AddHelpItem(P[0], P[1], P[2]);
            break;

        case CTL_HELPITEMIMAGE:
            SetHelpItemImage(P[0], P[1], P[2], P[3]);
            break;

        case CTL_HELPITEMTEXT:
            SetHelpItemText(P[0], P[1], P[2], P[3], P[4], P[5] bitor (P[6] << 8) bitor (P[7] << 16));
            break;

        case CTL_HELPITEMFONT:
            SetHelpItemFont(P[0], P[1]);
            break;

        case CTL_HELPITEMFLAGON:
            SetHelpFlagOn(P[0], P[1]);
            break;

        case CTL_HELPITEMFLAGOFF:
            SetHelpFlagOff(P[0], P[1]);
            break;

        case CTL_ADDWORDWRAPITEM:
            AddWordWrapItem(P[0], P[1], P[2], P[3], (long)P[4], P[5] bitor (P[6] << 8) bitor (P[7] << 16));
            break;
    }
}

#endif // PARSER
