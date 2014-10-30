#include <windows.h>
#include <math.h>
#include "chandler.h"
#include "f4error.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CWIN_NOTHING = 0,
    CWIN_SETUP,
    CWIN_SETX,
    CWIN_SETY,
    CWIN_SETXY,
    CWIN_SETW,
    CWIN_SETH,
    CWIN_SETRANGES,
    CWIN_SETCLIENTAREA,
    CWIN_SETFONT,
    CWIN_SETGROUP,
    CWIN_SETFLAGBITON,
    CWIN_SETFLAGBITOFF,
    CWIN_SETDEPTH,
    CWIN_SETMENUID,
    CWIN_SETCLIENTMENUID,
    CWIN_SETCURSOR,
    CWIN_SETDRAGH,
    CWIN_SETCLIENTFLAGS,
};

char *C_Win_Tokens[] =
{
    "[NOTHING]",
    "[Setup]",
    "[X]",
    "[Y]",
    "[XY]",
    "[W]",
    "[H]",
    "[Ranges]",
    "[ClientArea]",
    "[Font]",
    "[Group]",
    "[FLAGBITON]",
    "[FLAGBITOFF]",
    "[DEPTH]",
    "[OPENMENU]",
    "[OPENCLIENTMENU]",
    "[CURSOR]",
    "[DRAGH]",
    "[CLIENTFLAG]",
    0,
};

#endif

enum
{
    WINDOW_TITLE_BAR = 67676767,
};

// Pattern for drawing Hi-lited translucent rectangles
// over controls

short SubTable[16] =
{

    8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1,
};

short TableVal[] =
{
    0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
};

extern C_Handler *gMainHandler;
extern WORD UIColorTable[200][256];
extern WORD rShift[], bShift[], gShift[];

extern int FloatToInt32(float);

//XX
extern DWORD RGB565toRGB8(WORD);
extern WORD RGB8toRGB565(DWORD);


// NEVER CALL THESE FUNCTIONS YOURSELF
F4CSECTIONHANDLE* UI_Enter(C_Window *Parent)
{
    if (Parent and Parent->GetCritical())
    {
        F4EnterCriticalSection(Parent->GetCritical());
        return(Parent->GetCritical());
    }

    return(NULL);
}

void UI_Leave(F4CSECTIONHANDLE* Section)
{
    if (Section)
        F4LeaveCriticalSection(Section);
}

// 8th of a circle (0->45 degrees)
// Multiply these values by the radius to get the x bitand y values
// Skip points or smaller circles
// (The smaller it gets, the more points you skip)
// Then do x,y swaps bitand sign swaps to do the other 8 sections of a full circle
// Connect the dots with drawline()
//
// Referenced in cthread.cpp... so if modified, must modify #define there also
#define CIRCLE_POINTS 5
float Circle8[CIRCLE_POINTS][2] =
{
    { 0.000000000f, 1.000000000f },
    { 0.195090322f, 0.980785280f },
    { 0.382683432f, 0.923879532f },
    { 0.555570233f, 0.831469612f },
    { 0.707106781f, 0.707106781f },
};

C_Window::C_Window()
{
    Critical = NULL;
    Handler_ = NULL;
    ControlCount_ = 0;
    Controls_ = NULL;
    CurControl_ = NULL;
    BgColor_ = 0;
    MinX_ = 0;
    MinY_ = 0;
    MaxX_ = 0;
    MaxY_ = 0;
    DragH_ = 0;
    update_ = C_DRAW_NOTHING;
    Flags_ = C_BIT_NOTHING;
    Group_ = 0; // Nothing
    DefaultFlags_ = C_BIT_REMOVE;
    memset(ClientMenuID_, 0, sizeof(long)*WIN_MAX_CLIENTS);
    memset(ClientFlags_, C_BIT_ENABLED, sizeof(long)*WIN_MAX_CLIENTS);
    Depth_ = 10;
    Font_ = 1;
    CursorID_ = 0;
    MenuID_ = 0;
    MenuFlags_ = 0;
    BorderLite = 0xeeeeee;
    BorderMedium = 0x777777;
    BorderDark = 0x222222;
    BGColor = 0;
    SelectBG = 0xbbbbbb;
    NormalText = 0;
    ReverseText = 0xcccccc;
    DisabledText = 0x888888;
    TextColor_ = 0;
    BgColor_ = 0;
    DragCallback_ = NULL;
    KBCallback_ = NULL;
    Owner_ = NULL;
    Hash_ = NULL;
    Section_ = 0; // JPO initialise
}


void C_Window::Setup(long wID, short Type, short w, short h)
{
    short i;

    if (w < 1)
        w = 1;

    if (h < 1)
        h = 1;

    imgBuf_ = gMainHandler->GetFront();

    SetDefaultFlags();
    Type_ = Type;
    ID_ = wID;
    Width_ = w;
    Height_ = h;
    x_ = 0;
    y_ = 0;
    w_ = Width_;
    h_ = Height_;
    MinX_ = 0;
    MaxX_ = 800;
    MinY_ = 0;
    MaxY_ = 600;
    MinW_ = w_;
    MinH_ = h_;
    MaxW_ = w_;
    MaxH_ = h_;
    Area_.left = 0;
    Area_.top = 0;
    Area_.right = w_;
    Area_.bottom = h_;
    Handler_ = NULL;
    DragCallback_ = NULL;
    SetXY(0, 0);

    if (Hash_)
    {
        Hash_->Cleanup();
        delete Hash_;
    }

    Hash_ = new C_Hash;
    Hash_->Setup(WIN_HASH_SIZE);

    for (i = 0; i < WIN_MAX_CLIENTS; i++)
    {
        VScroll_[i] = NULL;
        HScroll_[i] = NULL;
        FullClientArea_[i] = Area_;
        ClientArea_[i] = Area_;
        SetVirtual(ClientArea_[i].left, ClientArea_[i].top, ClientArea_[i].right - ClientArea_[i].left, ClientArea_[i].bottom - ClientArea_[i].top, i);
    }

    CurControl_ = NULL;
    Font_ = 1;

    GetScreenFormat();

    rectcount_ = 0;
}

void C_Window::ResizeSurface(short w, short h)
{
    Width_ = w;
    Height_ = h;
    MaxW_ = w;
    MaxH_ = h;
    w_ = w;
    h_ = h;
}

void C_Window::SetFlags(long flag)
{
    Flags_ = flag;
}

void C_Window::SetFlagBitOn(long flag)
{
    Flags_ or_eq flag;
}

void C_Window::SetFlagBitOff(long flag)
{
    Flags_ and_eq (0xffffffff xor flag);
}

void C_Window::Cleanup()
{
    CONTROLLIST *cur, *last;

    if (Controls_ and not (Flags_ bitand C_BIT_NOCLEANUP))
    {
        cur = Controls_;

        while (cur)
        {
            last = cur;
            cur = cur->Next;

            if (last->Control_->GetFlags() bitand C_BIT_REMOVE)
            {
                last->Control_->Cleanup();
                delete last->Control_;
            }

            delete last;
        }

        Controls_ = NULL;
    }

    if (Hash_)
    {
        Hash_->Cleanup();
        delete Hash_;
        Hash_ = NULL;
    }

    imgBuf_ = NULL;
}

void C_Window::SetXY(short NewX, short NewY)
{
    SetX(NewX);
    SetY(NewY);
}

void C_Window::SetX(short NewX)
{
    F4CSECTIONHANDLE* Leave;

    if (NewX < MinX_)
        NewX = MinX_;

    if (NewX > MaxX_)
        NewX = MaxX_;

    Leave = UI_Enter(this);
    x_ = NewX;
    UI_Leave(Leave);
}

void C_Window::SetW(short NewW)
{
    F4CSECTIONHANDLE* Leave;

    if (NewW < MinW_)
        NewW = MinW_;

    if (NewW > MaxW_)
        NewW = MaxW_;

    Leave = UI_Enter(this);
    w_ = NewW;
    UI_Leave(Leave);
}

void C_Window::SetH(short NewH)
{
    F4CSECTIONHANDLE* Leave;

    if (NewH < MinH_)
        NewH = MinH_;

    if (NewH > MaxH_)
        NewH = MaxH_;

    Leave = UI_Enter(this);
    h_ = NewH;
    UI_Leave(Leave);
}

void C_Window::SetY(short NewY)
{
    F4CSECTIONHANDLE* Leave;

    if (NewY < MinY_)
        NewY = MinY_;

    if (NewY > MaxY_)
        NewY = MaxY_;

    Leave = UI_Enter(this);
    y_ = NewY;
    UI_Leave(Leave);
}

short C_Window::GetPrimaryW()
{
    long lResult = (Handler_ ? (Handler_->GetW()) : 800); // why 800? default screen width?
    // if(Handler_)
    // return(Handler_->GetW());
    // return(800);
    return static_cast<short>(lResult);
}

void C_Window::SetRanges(short x1, short y1, short x2, short y2, short w, short h)
{
    MinX_ = x1;
    MinY_ = y1;
    MaxX_ = x2;
    MaxY_ = y2;
    MinW_ = w;
    MinH_ = h;
}

void C_Window::ScanClientArea(long client)
{
    CONTROLLIST *cur;

    if (client >= WIN_MAX_CLIENTS)
        return;

    if ( not VScroll_[client] and not HScroll_[client])
        return;

    VW_[client] = 0;
    VH_[client] = 0;

    cur = Controls_;

    while (cur)
    {
        if (cur->Control_ and not (cur->Control_->GetFlags() bitand C_BIT_ABSOLUTE) and not (cur->Control_->GetFlags() bitand C_BIT_INVISIBLE) and cur->Control_->GetClient() == client)
        {
            if ((cur->Control_->GetX() + cur->Control_->GetW()) > VW_[client])
                VW_[client] = cur->Control_->GetX() + cur->Control_->GetW();

            if ((cur->Control_->GetY() + cur->Control_->GetH()) > VH_[client])
                VH_[client] = cur->Control_->GetY() + cur->Control_->GetH();
        }

        cur = cur->Next;
    }

    if (HScroll_[client])
    {
        HScroll_[client]->ClearVW();
        HScroll_[client]->SetVirtualW(VW_[client]);

        if (VW_[client] > (ClientArea_[client].right - ClientArea_[client].left))
        {
            if (VX_[client] < -(VW_[client] - ClientArea_[client].right))
                VX_[client] = -(VW_[client] - ClientArea_[client].right);
        }
        else
            VX_[client] = ClientArea_[client].left;

        HScroll_[client]->UpdatePosition();

        if (VW_[client] > (ClientArea_[client].right - ClientArea_[client].left))
        {
            HScroll_[client]->SetFlagBitOff(C_BIT_INVISIBLE);
            HScroll_[client]->Refresh();
        }
        else
        {
            HScroll_[client]->Refresh();
            HScroll_[client]->SetFlagBitOn(C_BIT_INVISIBLE);
        }
    }

    if (VScroll_[client])
    {
        VScroll_[client]->ClearVH();
        VScroll_[client]->SetVirtualH(VH_[client]);

        if (VH_[client] > (ClientArea_[client].bottom - ClientArea_[client].top))
        {
            if (VY_[client] < -(VH_[client] - ClientArea_[client].bottom))
                VY_[client] = -(VH_[client] - ClientArea_[client].bottom);
        }
        else
            VY_[client] = ClientArea_[client].top;

        VScroll_[client]->UpdatePosition();

        if (VH_[client] > (ClientArea_[client].bottom - ClientArea_[client].top))
        {
            VScroll_[client]->SetFlagBitOff(C_BIT_INVISIBLE);
            VScroll_[client]->Refresh();
        }
        else
        {
            VScroll_[client]->Refresh();
            VScroll_[client]->SetFlagBitOn(C_BIT_INVISIBLE);
        }
    }
}

void C_Window::ScanClientAreas()
{
    short i;

    for (i = 0; i < WIN_MAX_CLIENTS; i++)
        ScanClientArea(i);
}

void C_Window::Minimize()
{
    if (Handler_)
        Handler_->SetBehindWindow(this);

    SetW(MinW_);
    SetH(MinH_);
    RefreshWindow();
}

void C_Window::Maximize()
{
    SetW(MaxW_);
    SetH(MaxH_);

    if (GetX() + GetW() > Handler_->GetW())
        SetX(static_cast<short>(Handler_->GetW() - GetW()));

    if (GetY() + GetH() > Handler_->GetH())
        SetY(static_cast<short>(Handler_->GetH() - GetH()));

    RefreshWindow();
}

void C_Window::AddUpdateRect(long x1, long y1, long x2, long y2)
{
    long i, use;

    if (x1 >= x2 or y1 >= y2)
        return;

    use = -1;

    for (i = 0; i < rectcount_; i++)
        if ( not rectflag_[i])
        {
            use = i;
            i = rectcount_ + 1;
        }

    if (use == -1)
    {
        use = rectcount_;
        rectcount_++;
    }

    if (use < WIN_MAX_RECTS)
    {
        rectflag_[use] = 1;
        rectlist_[use].left = x1;
        rectlist_[use].top = y1;
        rectlist_[use].right = x2;
        rectlist_[use].bottom = y2;
    }
    else
    {
        rectcount_ = 1;
        memset(rectflag_, 0, sizeof(rectflag_));
        rectflag_[0] = 1;
        rectlist_[0].left = 0;
        rectlist_[0].top = 0;
        rectlist_[0].right = GetW();
        rectlist_[0].bottom = GetH();
        //MonoPrint("AddUpdateRect: [use] index >= WIN_MAX_RECTS");
    }
}

long C_Window::SetCheckedUpdateRect(long x1, long y1, long x2, long y2)
{
    long i, clipflag;

    if (rectcount_ < WIN_MAX_RECTS)
    {
        for (i = 0; (i < rectcount_) and (x1 <= x2) and (y1 <= y2); i++)
        {
            if (rectflag_[i])
            {
                if (x1 >= rectlist_[i].right or x2 <= rectlist_[i].left or y1 >= rectlist_[i].bottom or y2 <= rectlist_[i].top)
                {
                    // rects don't intersect
                    continue;
                }
                else
                {
                    clipflag = 0;

                    if (x1 >= rectlist_[i].left)
                        clipflag or_eq _CHR_CLIP_LEFT;

                    if (y1 >= rectlist_[i].top)
                        clipflag or_eq _CHR_CLIP_TOP;

                    if (x2 <= rectlist_[i].right)
                        clipflag or_eq _CHR_CLIP_RIGHT;

                    if (y2 <= rectlist_[i].bottom)
                        clipflag or_eq _CHR_CLIP_BOTTOM;

                    if (clipflag == (_CHR_CLIP_LEFT bitor _CHR_CLIP_TOP bitor _CHR_CLIP_RIGHT bitor _CHR_CLIP_BOTTOM))
                        return(0); // new rect is inside another rect

                    if ( not clipflag)
                        continue;

                    if (clipflag == (_CHR_CLIP_RIGHT bitor _CHR_CLIP_BOTTOM))
                    {
                        // case 1
                        SetCheckedUpdateRect(x1, y1, rectlist_[i].left, y2);
                        x1 = rectlist_[i].left;
                        y2 = rectlist_[i].top;
                    }
                    else if (clipflag == (_CHR_CLIP_LEFT bitor _CHR_CLIP_BOTTOM))
                    {
                        // case 2
                        SetCheckedUpdateRect(x1, y1, x2, rectlist_[i].top);
                        x1 = rectlist_[i].right;
                        y1 = rectlist_[i].top;
                    }
                    else if (clipflag == (_CHR_CLIP_RIGHT bitor _CHR_CLIP_TOP))
                    {
                        // case 3
                        SetCheckedUpdateRect(x1, y1, rectlist_[i].left, y2);
                        x1 = rectlist_[i].left;
                        y1 = rectlist_[i].bottom;
                    }
                    else if (clipflag == (_CHR_CLIP_LEFT bitor _CHR_CLIP_TOP))
                    {
                        // case 4
                        SetCheckedUpdateRect(x1, rectlist_[i].bottom, x2, y2);
                        x1 = rectlist_[i].right;
                        y2 = rectlist_[i].bottom;
                    }
                    else if (clipflag == _CHR_CLIP_BOTTOM)
                    {
                        // case 5
                        rectlist_[i].top = y2;
                    }
                    else if (clipflag == _CHR_CLIP_RIGHT)
                    {
                        // case 6
                        rectlist_[i].left = x2;
                    }
                    else if (clipflag == _CHR_CLIP_TOP)
                    {
                        // case 7
                        rectlist_[i].bottom = y1;
                    }
                    else if (clipflag == _CHR_CLIP_LEFT)
                    {
                        // case 8
                        rectlist_[i].right = x1;
                    }
                    else if (clipflag == (_CHR_CLIP_LEFT bitor _CHR_CLIP_RIGHT bitor _CHR_CLIP_BOTTOM))
                    {
                        // case 9
                        y2 = rectlist_[i].top;
                    }
                    else if (clipflag == (_CHR_CLIP_TOP bitor _CHR_CLIP_RIGHT bitor _CHR_CLIP_BOTTOM))
                    {
                        // case 10
                        x2 = rectlist_[i].left;
                    }
                    else if (clipflag == (_CHR_CLIP_TOP bitor _CHR_CLIP_LEFT bitor _CHR_CLIP_RIGHT))
                    {
                        // case 11
                        y1 = rectlist_[i].bottom;
                    }
                    else if (clipflag == (_CHR_CLIP_LEFT bitor _CHR_CLIP_TOP bitor _CHR_CLIP_BOTTOM))
                    {
                        // case 12
                        x1 = rectlist_[i].right;
                    }
                    else if (clipflag == (_CHR_CLIP_TOP bitor _CHR_CLIP_BOTTOM))
                    {
                        // case 15
                        SetCheckedUpdateRect(x1, y1, rectlist_[i].left, y2);
                        x1 = rectlist_[i].right;
                    }
                    else if (clipflag == (_CHR_CLIP_LEFT bitor _CHR_CLIP_RIGHT))
                    {
                        // case 16
                        SetCheckedUpdateRect(x1, y1, x2, rectlist_[i].top);
                        y1 = rectlist_[i].bottom;
                    }
                }
            }
        }

        if (x1 < x2 and y1 < y2)
        {
            AddUpdateRect(x1, y1, x2, y2);
            return(1);
        }
    }
    else
        return(-1);

    return(0);
}

void C_Window::SetUpdateRect(long x1, long y1, long x2, long y2, long flags, long client)
{
    short i;
    F4CSECTIONHANDLE* Leave;

    if (Handler_ == NULL) return;

    if ( not (flags bitand C_BIT_ABSOLUTE))
    {
        x1 += VX_[client];
        y1 += VY_[client];
        x2 += VX_[client];
        y2 += VY_[client];

        if (x1 > ClientArea_[client].right or y1 > ClientArea_[client].bottom or x2 < ClientArea_[client].left or y2 < ClientArea_[client].top)
            return;

        // original code
        if (x1 < ClientArea_[client].left) x1 = ClientArea_[client].left;

        if (y1 < ClientArea_[client].top) y1 = ClientArea_[client].top;

        if (x2 > ClientArea_[client].right) x2 = ClientArea_[client].right;

        if (y2 > ClientArea_[client].bottom) y2 = ClientArea_[client].bottom;
    }
    else
    {
        if (x1 > GetW() or y1 > GetH() or x2 < 0 or y2 < 0)
            return;

        if (x1 < 0) x1 = 0;

        if (y1 < 0) y1 = 0;

        if (x2 > GetW()) x2 = GetW();

        if (y2 > GetH()) y2 = GetH();
    }

    Leave = UI_Enter(this);

    if (rectcount_)
    {
        if (SetCheckedUpdateRect(x1, y1, x2, y2) < 0)
        {
            for (i = 0; i < WIN_MAX_RECTS; i++)
                rectflag_[i] = 0;

            rectcount_ = 0;
            AddUpdateRect(0, 0, w_, h_);
        }
    }
    else
        AddUpdateRect(x1, y1, x2, y2);

    update_ or_eq C_DRAW_COPYWINDOW bitor C_DRAW_REFRESH;
    Handler_->UpdateFlag or_eq C_DRAW_REFRESH;

    UI_Leave(Leave);
}

void C_Window::ClearCheckedUpdateRect(long x1, long y1, long x2, long y2)
{
    short i, clipflag;
    UI95_RECT oldrect;

    if (rectcount_ < WIN_MAX_RECTS)
    {
        for (i = 0; (i < rectcount_) and (x1 < x2) and (y1 < y2); i++)
        {
            if (rectflag_[i])
            {
                if (x1 >= rectlist_[i].right or x2 <= rectlist_[i].left or y1 >= rectlist_[i].bottom or y2 <= rectlist_[i].top)
                {
                    // rects don't intersect
                    continue;
                }
                else
                {
                    clipflag = 0;

                    if (x1 >= rectlist_[i].left)
                        clipflag or_eq _CHR_CLIP_LEFT;

                    if (y1 >= rectlist_[i].top)
                        clipflag or_eq _CHR_CLIP_TOP;

                    if (x2 <= rectlist_[i].right)
                        clipflag or_eq _CHR_CLIP_RIGHT;

                    if (y2 <= rectlist_[i].bottom)
                        clipflag or_eq _CHR_CLIP_BOTTOM;

                    oldrect = rectlist_[i];

                    if ( not clipflag)
                    {
                        // clear rect contains rect... remove cur
                        rectflag_[i] = 0;
                    }

                    if (clipflag == (_CHR_CLIP_LEFT bitor _CHR_CLIP_TOP bitor _CHR_CLIP_RIGHT bitor _CHR_CLIP_BOTTOM))
                    {
                        // clear rect is totally inside current rect... break into 4
                        rectflag_[i] = 0;
                        SetCheckedUpdateRect(oldrect.left, oldrect.top, oldrect.right, y1);
                        SetCheckedUpdateRect(oldrect.left, y1, x1, y2);
                        SetCheckedUpdateRect(x2, y1, oldrect.right, y2);
                        SetCheckedUpdateRect(oldrect.left, y2, oldrect.right, oldrect.bottom);
                    }
                    else if (clipflag == (_CHR_CLIP_RIGHT bitor _CHR_CLIP_BOTTOM))
                    {
                        // case 1
                        rectlist_[i].top = y2;
                        SetCheckedUpdateRect(x2, oldrect.top, oldrect.right, y2);
                    }
                    else if (clipflag == (_CHR_CLIP_LEFT bitor _CHR_CLIP_BOTTOM))
                    {
                        // case 2
                        rectlist_[i].top = y2;
                        SetCheckedUpdateRect(oldrect.left, oldrect.top, x1, y2);
                    }
                    else if (clipflag == (_CHR_CLIP_RIGHT bitor _CHR_CLIP_TOP))
                    {
                        // case 3
                        rectlist_[i].bottom = y1;
                        SetCheckedUpdateRect(x2, y1, oldrect.right, oldrect.bottom);
                    }
                    else if (clipflag == (_CHR_CLIP_LEFT bitor _CHR_CLIP_TOP))
                    {
                        // case 4
                        rectlist_[i].bottom = y1;
                        SetCheckedUpdateRect(oldrect.left, y1, x1, oldrect.bottom);
                    }
                    else if (clipflag == _CHR_CLIP_BOTTOM)
                    {
                        // case 5
                        rectlist_[i].top = y2;
                    }
                    else if (clipflag == _CHR_CLIP_RIGHT)
                    {
                        // case 6
                        rectlist_[i].left = x2;
                    }
                    else if (clipflag == _CHR_CLIP_TOP)
                    {
                        // case 7
                        rectlist_[i].bottom = y1;
                    }
                    else if (clipflag == _CHR_CLIP_LEFT)
                    {
                        // case 8
                        rectlist_[i].right = x1;
                    }
                    else if (clipflag == (_CHR_CLIP_LEFT bitor _CHR_CLIP_RIGHT bitor _CHR_CLIP_BOTTOM))
                    {
                        // case 9
                        rectlist_[i].top = y2;
                        SetCheckedUpdateRect(oldrect.left, oldrect.top, x1, y2);
                        SetCheckedUpdateRect(x2, oldrect.top, oldrect.right, y2);
                    }
                    else if (clipflag == (_CHR_CLIP_TOP bitor _CHR_CLIP_RIGHT bitor _CHR_CLIP_BOTTOM))
                    {
                        // case 10
                        rectlist_[i].left = x2;
                        SetCheckedUpdateRect(oldrect.left, oldrect.top, x2, y1);
                        SetCheckedUpdateRect(oldrect.left, y2, x2, oldrect.bottom);
                    }
                    else if (clipflag == (_CHR_CLIP_TOP bitor _CHR_CLIP_LEFT bitor _CHR_CLIP_RIGHT))
                    {
                        // case 11
                        rectlist_[i].bottom = y1;
                        SetCheckedUpdateRect(oldrect.left, y1, x1, oldrect.bottom);
                        SetCheckedUpdateRect(x2, y1, oldrect.right, oldrect.bottom);
                    }
                    else if (clipflag == (_CHR_CLIP_LEFT bitor _CHR_CLIP_TOP bitor _CHR_CLIP_BOTTOM))
                    {
                        // case 12
                        rectlist_[i].right = x1;
                        SetCheckedUpdateRect(x1, oldrect.top, oldrect.right, y1);
                        SetCheckedUpdateRect(x1, y2, oldrect.right, oldrect.bottom);
                    }
                    else if (clipflag == (_CHR_CLIP_TOP bitor _CHR_CLIP_BOTTOM))
                    {
                        // case 15
                        rectflag_[i] = 0;
                        SetCheckedUpdateRect(oldrect.left, oldrect.top, oldrect.right, y1);
                        SetCheckedUpdateRect(oldrect.left, y2, oldrect.right, oldrect.bottom);
                    }
                    else if (clipflag == (_CHR_CLIP_LEFT bitor _CHR_CLIP_RIGHT))
                    {
                        // case 16
                        rectflag_[i] = 0;
                        SetCheckedUpdateRect(oldrect.left, oldrect.top, x1, oldrect.bottom);
                        SetCheckedUpdateRect(x2, oldrect.top, oldrect.right, oldrect.bottom);
                    }
                }
            }
        }
    }
    else
    {
        for (i = 0; i < WIN_MAX_RECTS; i++)
            rectflag_[i] = 0;

        rectcount_ = 0;
        AddUpdateRect(0, 0, w_, h_);
        ClearCheckedUpdateRect(x1, y1, x2, y2);
    }
}

void C_Window::ClearUpdateRect(long x1, long y1, long x2, long y2)
{
    short i;

    if (x1 > GetW() or x2 < 0 or y1 > GetH() or y2 < 0 or not rectcount_)
        return;

    if (x1 <= 0 and y1 <= 0 and x2 >= GetW() and y2 >= GetH())
    {
        rectcount_ = 0;
        return;
    }

    if (x1 < 0) x1 = 0;

    if (y1 < 0) y1 = 0;

    if (x2 > GetW()) x2 = GetW();

    if (y2 > GetH()) y2 = GetH();

    ClearCheckedUpdateRect(x1, y1, x2, y2);

    for (i = 0; i < rectcount_; i++)
        if (rectflag_[i])
            return;

    update_ = 0;
    rectcount_ = 0;
}

#if 0
void *C_Window::Lock()
{
    if (imgBuf_)
        return(imgBuf_->Lock());

    return(NULL);
}

void C_Window::Unlock()
{
    if (imgBuf_)
        imgBuf_->Unlock();
}
#endif

void C_Window::AddScrollBar(C_ScrollBar *scroll)
{
    switch (scroll->GetType())
    {
        case C_TYPE_VERTICAL:
            VScroll_[scroll->GetClient()] = scroll;
            break;

        case C_TYPE_HORIZONTAL:
            HScroll_[scroll->GetClient()] = scroll;
            break;
    }

    AddControl(scroll);
}

void C_Window::AdjustScrollbar(long client)
{
    if (client < WIN_MAX_CLIENTS)
    {
        if (HScroll_[client])
        {
            HScroll_[client]->UpdatePosition();
            HScroll_[client]->Refresh();
        }

        if (VScroll_[client])
        {
            VScroll_[client]->UpdatePosition();
            VScroll_[client]->Refresh();
        }
    }
}

void C_Window::AddControlTop(C_Base *NewControl)
{
    CONTROLLIST *cnt;
    F4CSECTIONHANDLE* Leave;

    if ( not NewControl)
        return;

    if (NewControl->GetID() > 0)
        if (Hash_->Find(NewControl->GetID()))
            return;

#ifdef USE_SH_POOLS
    cnt = (CONTROLLIST *)MemAllocPtr(UI_Pools[UI_CONTROL_POOL], sizeof(CONTROLLIST), FALSE);
#else
    cnt = new CONTROLLIST;
#endif
    cnt->Control_ = NewControl;
    cnt->Prev = NULL;
    cnt->Next = NULL;

    Leave = UI_Enter(this);

    if ( not Controls_)
        Controls_ = cnt;
    else
    {
        cnt->Next = Controls_;
        Controls_ = cnt;
    }

    NewControl->SetParent(this);
    NewControl->SetSubParents(this);

    if ( not (NewControl->GetFlags() bitand C_BIT_ABSOLUTE))
    {
        if (VScroll_[NewControl->GetClient()])
        {
            VScroll_[NewControl->GetClient()]->SetVirtualH(NewControl->GetY() + NewControl->GetH()); //+ClientArea_[NewControl->GetClient()].top);
        }

        if (HScroll_[NewControl->GetClient()])
        {
            HScroll_[NewControl->GetClient()]->SetVirtualW(NewControl->GetX() + NewControl->GetW()); //+ClientArea_[NewControl->GetClient()].left);
        }
    }

    if (NewControl->GetID() > 0)
        Hash_->Add(NewControl->GetID(), cnt);

    UI_Leave(Leave);
}

void C_Window::AddControl(C_Base *NewControl)
{
    CONTROLLIST *cnt;
    F4CSECTIONHANDLE* Leave;

    if ( not NewControl)
        return;

    //F4Assert(NewControl->GetID() >0 or NewControl->GetID() == C_DONT_CARE);

    if (NewControl->GetID() > 0)
    {
        if (Hash_->Find(NewControl->GetID()))
        {
            //MonoPrint ("Duplicate control ID %d\n", NewControl->GetID());
            delete NewControl;
            return;
        }
    }

#ifdef USE_SH_POOLS
    cnt = (CONTROLLIST *)MemAllocPtr(UI_Pools[UI_CONTROL_POOL], sizeof(CONTROLLIST), FALSE);
#else
    cnt = new CONTROLLIST;
#endif
    cnt->Control_ = NewControl;
    cnt->Next = NULL;

    Leave = UI_Enter(this);

    if ( not Controls_)
    {
        Controls_ = cnt;
        Controls_->Prev = NULL;
        Last_ = Controls_;
    }
    else
    {
        Last_->Next = cnt;
        cnt->Prev = Last_;
        Last_ = cnt;
    }

    NewControl->SetParent(this);
    NewControl->SetSubParents(this);
    ControlCount_++;

    if ( not (NewControl->GetFlags() bitand C_BIT_ABSOLUTE))
    {
        if (VScroll_[NewControl->GetClient()])
        {
            VScroll_[NewControl->GetClient()]->SetVirtualH(NewControl->GetY() + NewControl->GetH()); //+ClientArea_[NewControl->GetClient()].top);
        }

        if (HScroll_[NewControl->GetClient()])
        {
            HScroll_[NewControl->GetClient()]->SetVirtualW(NewControl->GetX() + NewControl->GetW()); //+ClientArea_[NewControl->GetClient()].left);
        }
    }

    NewControl->Refresh();

    if (NewControl->GetID() > 0)
        Hash_->Add(NewControl->GetID(), cnt);

    UI_Leave(Leave);
}

void C_Window::RemoveControl(long ID)
{
    CONTROLLIST *cur;
    F4CSECTIONHANDLE* Leave;

    if (Controls_ == NULL or ID == C_DONT_CARE)
        return;

    Leave = UI_Enter(this);
    cur = Controls_;

    while (cur)
    {
        if (cur->Control_->GetID() == ID)
        {
            RemoveControl(cur);
            cur = NULL;
        }
        else
            cur = cur->Next;
    }

    ScanClientAreas();
    UI_Leave(Leave);
}

CONTROLLIST *C_Window::RemoveControl(CONTROLLIST *ctrl)
{
    CONTROLLIST *retval;
    F4CSECTIONHANDLE* Leave;

    if ( not Controls_ or not ctrl)
        return(NULL);

    Leave = UI_Enter(this);

    if (CurControl_ == ctrl->Control_)
    {
        CurControl_->Refresh();
        CurControl_ = NULL;
        Handler_->RemovingControl(ctrl->Control_);
    }

    if (Last_ == ctrl)
        Last_ = Last_->Prev;

    if (ctrl == Controls_)
    {
        Controls_ = Controls_->Next;

        if (Controls_)
            Controls_->Prev = NULL;
    }
    else
    {
        if (ctrl->Prev)
            ctrl->Prev->Next = ctrl->Next;

        if (ctrl->Next)
            ctrl->Next->Prev = ctrl->Prev;
    }

    if (ctrl->Control_->GetID() > 0)
        Hash_->Remove(ctrl->Control_->GetID());

    if (ctrl->Control_->GetFlags() bitand C_BIT_REMOVE)
    {
        ctrl->Control_->Cleanup();
        delete ctrl->Control_;
    }

    retval = ctrl->Next;
    delete ctrl;

    UI_Leave(Leave);
    return(retval);
}

void C_Window::RemoveAllControls()
{
    CONTROLLIST *cur, *last;
    F4CSECTIONHANDLE* Leave;

    Leave = UI_Enter(this);
    cur = Controls_;

    while (cur)
    {
        last = cur;
        cur = cur->Next;

        if (last->Control_->GetFlags() bitand C_BIT_REMOVE)
        {
            last->Control_->Cleanup();
            delete last->Control_;
            delete last;
        }
    }

    Controls_ = NULL;
    Hash_->Cleanup();
    Hash_->Setup(WIN_HASH_SIZE);

    ScanClientAreas();
    UI_Leave(Leave);
}

BOOL C_Window::InsideClientWidth(long left, long right, long Client)
{
    if ((left + VX_[Client] + VW_[Client]) < ClientArea_[Client].right and (right + VX_[Client]) >= ClientArea_[Client].left)
        return(TRUE);

    return(FALSE);
}

BOOL C_Window::InsideClientHeight(long top, long bottom, long Client)
{
    if ((top + VY_[Client]) < ClientArea_[Client].bottom and (bottom + VY_[Client]) >= ClientArea_[Client].top)
        return(TRUE);

    return(FALSE);
}

BOOL C_Window::BelowClient(long y, long Client)
{
    if (y + VY_[Client] >= ClientArea_[Client].bottom)
        return(TRUE);

    return(FALSE);
}

void C_Window::RefreshWindow()
{
    rectcount_ = 0;
    SetUpdateRect(0, 0, GetW(), GetH(), C_BIT_ABSOLUTE, 0);
}

void C_Window::RefreshClient(long Client)
{
    ClearUpdateRect(ClientArea_[Client].left, ClientArea_[Client].top, ClientArea_[Client].right, ClientArea_[Client].bottom);
    SetUpdateRect(ClientArea_[Client].left, ClientArea_[Client].top, ClientArea_[Client].right, ClientArea_[Client].bottom, C_BIT_ABSOLUTE, 0);
}

void C_Window::DrawWindow(SCREEN *surface)
{
    long i;
    CONTROLLIST *cur;

    cur = Controls_;

    while (cur)
    {
        for (i = 0; i < rectcount_; i++)
            if (rectflag_[i])
                cur->Control_->Draw(surface, &rectlist_[i]);

        cur = cur->Next;
    }
}

void C_Window::EnableGroup(long ID)
{
    CONTROLLIST *cur;

    cur = Controls_;

    while (cur)
    {
        if (cur->Control_->GetGroup() == ID)
            cur->Control_->EnableGroup(ID);

        cur = cur->Next;
    }
}

void C_Window::DisableGroup(long ID)
{
    CONTROLLIST *cur;

    cur = Controls_;

    while (cur)
    {
        if (cur->Control_->GetGroup() == ID)
            cur->Control_->DisableGroup(ID);

        cur = cur->Next;
    }
}

void C_Window::UnHideGroup(long ID)
{
    CONTROLLIST *cur;

    cur = Controls_;

    while (cur)
    {
        if (cur->Control_->GetGroup() == ID)
        {
            cur->Control_->SetFlagBitOff(C_BIT_INVISIBLE);
            cur->Control_->Refresh();
        }

        cur = cur->Next;
    }

    ScanClientAreas();
}

void C_Window::HideGroup(long ID)
{
    CONTROLLIST *cur;

    cur = Controls_;

    while (cur)
    {
        if (cur->Control_->GetGroup() == ID)
        {
            if ( not (cur->Control_->GetFlags() bitand C_BIT_INVISIBLE))
                cur->Control_->Refresh();

            cur->Control_->SetFlagBitOn(C_BIT_INVISIBLE);
        }

        cur = cur->Next;
    }

    ScanClientAreas();
}

void C_Window::EnableCluster(long ID)
{
    CONTROLLIST *cur;

    cur = Controls_;

    while (cur)
    {
        if (cur->Control_->GetCluster() == ID)
            cur->Control_->EnableGroup(ID);

        cur = cur->Next;
    }
}

void C_Window::DisableCluster(long ID)
{
    CONTROLLIST *cur;

    cur = Controls_;

    while (cur)
    {
        if (cur->Control_->GetCluster() == ID)
            cur->Control_->DisableGroup(ID);

        cur = cur->Next;
    }
}

void C_Window::UnHideCluster(long ID)
{
    CONTROLLIST *cur;

    cur = Controls_;

    while (cur)
    {
        if (cur->Control_->GetCluster() == ID)
        {
            cur->Control_->SetFlagBitOff(C_BIT_INVISIBLE);
            cur->Control_->Refresh();
        }

        cur = cur->Next;
    }

    ScanClientAreas();
}

void C_Window::HideCluster(long ID)
{
    CONTROLLIST *cur;

    cur = Controls_;

    while (cur)
    {
        if (cur->Control_->GetCluster() == ID)
        {
            if ( not (cur->Control_->GetFlags() bitand C_BIT_INVISIBLE))
                cur->Control_->Refresh();

            cur->Control_->SetFlagBitOn(C_BIT_INVISIBLE);
        }

        cur = cur->Next;
    }

    ScanClientAreas();
}

void C_Window::DrawTimerControls()
{
    CONTROLLIST *cur;

    cur = Controls_;

    while (cur)
    {
        if (cur->Control_->GetFlags() bitand C_BIT_TIMER)
            cur->Control_->Refresh();

        cur = cur->Next;
    }
}

void C_Window::ClearWindow(SCREEN *surface, long Client)
{
    BlitFill(surface, BgColor_, &ClientArea_[Client], C_BIT_ABSOLUTE, Client);
    SetUpdateRect(ClientArea_[Client].left, ClientArea_[Client].top, ClientArea_[Client].right, ClientArea_[Client].bottom, C_BIT_ABSOLUTE, 0);
}

void C_Window::ClearArea(SCREEN *surface, long x, long y, long w, long h, long flags, long Client)
{
    UI95_RECT rect;

    rect.left = x;
    rect.top = y;
    rect.right = rect.left + w;
    rect.bottom = rect.top + h;

    BlitFill(surface, BgColor_, &rect, flags, Client);
}

C_Base *C_Window::GetControl(long *ID, long relX, long relY)
{
    C_Base *last = NULL;
    long thisID, lastID = 0;

    // run all controls in window
    for (CONTROLLIST *cur = Controls_; cur not_eq NULL; cur = cur->Next)
    {
        if (cur->Control_->IsControl())
        {
            if (cur->Control_->GetFlags() bitand C_BIT_ABSOLUTE)
            {
                thisID = cur->Control_->CheckHotSpots(relX, relY);

                if (thisID)
                {
                    lastID = thisID;
                    last = cur->Control_;
                }
            }
            else
            {
                if (relX >= ClientArea_[cur->Control_->GetClient()].left and relX <= ClientArea_[cur->Control_->GetClient()].right and relY >= ClientArea_[cur->Control_->GetClient()].top and relY <= ClientArea_[cur->Control_->GetClient()].bottom)
                {
                    thisID = cur->Control_->CheckHotSpots(relX - VX_[cur->Control_->GetClient()], relY - VY_[cur->Control_->GetClient()]);

                    if (thisID)
                    {
                        lastID = thisID;
                        last = cur->Control_;
                    }
                }
            }
        }
    }

    if (lastID)
    {
        *ID = lastID;
        return(last);
    }

    *ID = 0;
    return(NULL);
}

C_Base *C_Window::FindControl(long ID)
{
    CONTROLLIST *cur = NULL;

    if (F4IsBadReadPtr(Hash_, sizeof(C_Hash))) // JB 010404 CTD
        return NULL;

    if (ID > 0)
        cur = (CONTROLLIST*)Hash_->Find(ID);

    if (cur)
        return(cur->Control_);

    return(NULL);
}

CONTROLLIST *C_Window::FindControlInList(C_Base *cntrl)
{
    CONTROLLIST *cur;

    if ( not cntrl) return(NULL);

    if (cntrl->GetID() > 0)
        return((CONTROLLIST*)Hash_->Find(cntrl->GetID()));

    cur = Controls_;

    while (cur)
    {
        if (cur->Control_ == cntrl)
            return(cur);

        cur = cur->Next;
    }

    return(NULL);
}

C_Base *C_Window::MouseOver(long relx, long rely, C_Base *lastover)
{
    CONTROLLIST *cur;
    C_Base *Found_ = NULL;

    cur = Controls_;

    while (cur)
    {
        if (cur->Control_->GetFlags() bitand C_BIT_ABSOLUTE)
        {
            if (cur->Control_->MouseOver(relx, rely, lastover))
            {
                // 99% of the time this will return cur->Control_ except
                // in the case of piggyback controls (like trees)
                Found_ = cur->Control_->GetMe();
            }
        }
        else if (
            relx >= ClientArea_[cur->Control_->GetClient()].left and 
            relx <= ClientArea_[cur->Control_->GetClient()].right and 
            rely >= ClientArea_[cur->Control_->GetClient()].top and 
            rely <= ClientArea_[cur->Control_->GetClient()].bottom
        )
        {
            if (
                cur->Control_->MouseOver(
                    relx - VX_[cur->Control_->GetClient()], rely - VY_[cur->Control_->GetClient()], lastover
                )
            )
            {
                Found_ = cur->Control_->GetMe();
            }
        }

        cur = cur->Next;
    }

    return (Found_);
}

void C_Window::GetScreenFormat()
{
    UI95_GetScreenColorInfo(r_mask_, r_shift_, g_mask_, g_shift_, b_mask_, b_shift_);
    //UI95_GetScreenColorInfo(&r_mask_,&r_shift_,&g_mask_,&g_shift_,&b_mask_,&b_shift_);
    r_max_ = static_cast<WORD>(r_mask_ >> r_shift_);
    g_max_ = static_cast<WORD>(g_mask_ >> g_shift_);
    b_max_ = static_cast<WORD>(b_mask_ >> b_shift_);
}

BOOL C_Window::ClipToArea(UI95_RECT *src, UI95_RECT *dst, UI95_RECT *ClipArea)
{
    long offset;

    if (dst->left < ClipArea->left)
    {
        offset = ClipArea->left - dst->left;
        src->left += offset;
        dst->left += offset;
    }

    if (dst->top < ClipArea->top)
    {
        offset = ClipArea->top - dst->top;
        src->top += offset;
        dst->top += offset;
    }

    if (dst->right > ClipArea->right)
    {
        offset = dst->right - ClipArea->right;
        src->right -= offset;
        dst->right -= offset;
    }

    if (dst->bottom > ClipArea->bottom)
    {
        offset = dst->bottom - ClipArea->bottom;
        src->bottom -= offset;
        dst->bottom -= offset;
    }

    if (dst->left < dst->right and dst->top < dst->bottom)
        return(TRUE); // Draw it

    return(FALSE);
}

void C_Window::Blend(WORD *front, UI95_RECT *frect, short fwidth, WORD *back, UI95_RECT *brect, short bwidth, WORD *dest, UI95_RECT *drect, short dwidth, short fperc, short bperc)
{
    long i, j;
    long rf, rb, gf, gb, bf, bb;
    long fidx, bidx, didx;
    long fidxstart, bidxstart, didxstart;

    if ( not front or not frect or not fwidth or not back or not brect or not bwidth or not dest or not drect or not dwidth)
        return;

    fidxstart = frect->top * fwidth;
    bidxstart = brect->top * bwidth;
    didxstart = drect->top * dwidth;

    for (i = drect->top; i < drect->bottom; i++)
    {
        fidx = fidxstart + frect->left;
        bidx = bidxstart + brect->left;
        didx = didxstart + drect->left;

        for (j = drect->left; j < drect->right; j++)
        {
            rf = UIColorTable[fperc][(front[fidx] bitand r_mask_) >> r_shift_];
            gf = UIColorTable[fperc][(front[fidx] bitand g_mask_) >> g_shift_];
            bf = UIColorTable[fperc][(front[fidx] bitand b_mask_) >> b_shift_];

            rb = UIColorTable[bperc][(back[bidx] bitand r_mask_) >> r_shift_];
            gb = UIColorTable[bperc][(back[bidx] bitand g_mask_) >> g_shift_];
            bb = UIColorTable[bperc][(back[bidx] bitand b_mask_) >> b_shift_];

            dest[didx] = static_cast<WORD>(rShift[UIColorTable[100][rf + rb]] bitor gShift[UIColorTable[100][gf + gb]] bitor bShift[UIColorTable[100][bf + bb]]); 
            fidx++;
            bidx++;
            didx++;
        }

        fidxstart += fwidth;
        bidxstart += bwidth;
        didxstart += dwidth;
    }
}

void C_Window::BlendTransparent(WORD Mask, WORD *front, UI95_RECT *frect, short fwidth, WORD *back, UI95_RECT *brect, short bwidth, WORD *dest, UI95_RECT *drect, short dwidth, short fperc, short bperc)
{
    long i, j;
    long rf, rb, gf, gb, bf, bb;
    long fidx, bidx, didx;
    long fidxstart, bidxstart, didxstart;

    if ( not front or not frect or not fwidth or not back or not brect or not bwidth or not dest or not drect or not dwidth)
        return;

    fidxstart = frect->top * fwidth;
    bidxstart = brect->top * bwidth;
    didxstart = drect->top * dwidth;

    for (i = drect->top; i < drect->bottom; i++)
    {
        fidx = fidxstart + frect->left;
        bidx = bidxstart + brect->left;
        didx = didxstart + drect->left;

        for (j = drect->left; j < drect->right; j++)
        {
            if (front[fidx] not_eq Mask)
            {
                rf = UIColorTable[fperc][(front[fidx] bitand r_mask_) >> r_shift_];
                gf = UIColorTable[fperc][(front[fidx] bitand g_mask_) >> g_shift_];
                bf = UIColorTable[fperc][(front[fidx] bitand b_mask_) >> b_shift_];

                rb = UIColorTable[bperc][(back[bidx] bitand r_mask_) >> r_shift_];
                gb = UIColorTable[bperc][(back[bidx] bitand g_mask_) >> g_shift_];
                bb = UIColorTable[bperc][(back[bidx] bitand b_mask_) >> b_shift_];

                dest[didx] = static_cast<WORD>(rShift[UIColorTable[100][rf + rb]] bitor gShift[UIColorTable[100][gf + gb]] bitor bShift[UIColorTable[100][bf + bb]]); 
            }
            else
                dest[didx] = back[bidx];

            fidx++;
            bidx++;
            didx++;
        }

        fidxstart += fwidth;
        bidxstart += bwidth;
        didxstart += dwidth;
    }
}

void C_Window::Translucency(WORD *front, UI95_RECT *frect, short fwidth, WORD *dest, UI95_RECT *drect, short dwidth)
{
    long i, j;
    long rf, gf, bf;
    long fidx, fidxstart;
    long didx, didxstart;

    if ( not front or not frect or not fwidth  or not dest or not drect or not dwidth)
        return;

    fidxstart = frect->top * fwidth;
    didxstart = drect->top * dwidth;

    for (i = drect->top; i < drect->bottom; i++)
    {
        fidx = fidxstart + frect->left;
        didx = didxstart + drect->left;

        for (j = drect->left; j < drect->right; j++)
        {
            rf = UIColorTable[0][(front[fidx] bitand r_mask_) >> r_shift_];
            gf = UIColorTable[0][(front[fidx] bitand g_mask_) >> g_shift_];
            bf = UIColorTable[0][(front[fidx] bitand b_mask_) >> b_shift_];

            dest[j + i * dwidth] = static_cast<WORD>(rShift[rf] bitor gShift[gf] bitor bShift[bf]); 
            fidx++;
            didx++;
        }

        fidxstart += fwidth;
        didxstart += dwidth;
    }
}

void C_Window::BlitTranslucent(SCREEN *surface, COLORREF color, long Perc, UI95_RECT *rect, long Flags, long Client)
{

    int i, j;
    long r, g, b;
    long didx, didxstart;
    long rc, gc, bc;
    long bgperc, operc;
    UI95_RECT s, d;

    d = *rect;

    if (Flags bitand C_BIT_ABSOLUTE)
    {
        if ( not ClipToArea(&s, &d, &Area_))
            return;
    }
    else
    {
        d.left += VX_[Client];
        d.top += VY_[Client];
        d.right += VX_[Client];
        d.bottom += VY_[Client];

        if ( not ClipToArea(&s, &d, &ClientArea_[Client]))
            return;
    }

    d.left += GetX();
    d.top += GetY();
    d.right += GetX();
    d.bottom += GetY();

    rc = UIColorTable[Perc][(color >>  3) bitand 0x1f];
    gc = UIColorTable[Perc][(color >> 11) bitand 0x1f];
    bc = UIColorTable[Perc][(color >> 19) bitand 0x1f];

    bgperc = 100 - Perc;

    if (bgperc < 0) bgperc = 0;

    if (bgperc > 100) bgperc = 100;

    operc = Perc + bgperc;

    didxstart = d.top * surface->width;

    for (i = d.top; i < d.bottom; i++)
    {
        didx = didxstart + d.left;

        for (j = d.left; j < d.right; j++)
        {
            DWORD dc;//XX

            if (surface->bpp == 32)
                dc = RGB8toRGB565(((DWORD*)surface->mem)[ didx ]);
            else
                dc = surface->mem[didx];

            r = UIColorTable[operc][rc + UIColorTable[bgperc][(dc >> r_shift_) bitand 0x1f]];
            g = UIColorTable[operc][gc + UIColorTable[bgperc][(dc >> g_shift_) bitand 0x1f]];
            b = UIColorTable[operc][bc + UIColorTable[bgperc][(dc >> b_shift_) bitand 0x1f]];

            if (surface->bpp == 32) //XX
                ((DWORD*)surface->mem)[ didx ] = RGB565toRGB8(static_cast<WORD>((rShift[r] bitor gShift[g] bitor bShift[b])));
            else
                surface->mem[didx] = static_cast<WORD>((rShift[r] bitor gShift[g] bitor bShift[b]));


            didx++;
        }

        didxstart += surface->width;
    }
}

void C_Window::CustomBlitTranslucent(SCREEN *surface, COLORREF color, long Perc, UI95_RECT *rect, long Flags, long Client)
{
    int i, j;
    long r, g, b, rc, gc, bc;
    long didx, didxstart;
    long bgperc, operc;
    UI95_RECT s, d, orig;
    long dl, dt, dr, db, subx, suby, sub;

    d = *rect;

    if (Flags bitand C_BIT_ABSOLUTE)
    {
        orig = d;

        if ( not ClipToArea(&s, &d, &Area_))
            return;
    }
    else
    {
        d.left += VX_[Client];
        d.top += VY_[Client];
        d.right += VX_[Client];
        d.bottom += VY_[Client];
        orig = d;

        if ( not ClipToArea(&s, &d, &ClientArea_[Client]))
            return;
    }

    d.left += GetX();
    d.top += GetY();
    d.right += GetX();
    d.bottom += GetY();
    orig.left += GetX();
    orig.top += GetY();
    orig.right += GetX();
    orig.bottom += GetY();

    rc = (color >>  3) bitand 0x1f;
    gc = (color >> 11) bitand 0x1f;
    bc = (color >> 19) bitand 0x1f;
    bgperc = 100 - Perc;

    if (bgperc < 0) bgperc = 0;

    if (bgperc > 100) bgperc = 100;

    operc = Perc + bgperc;

    didxstart = d.top * surface->width;

    for (i = d.top; i < d.bottom; i++)
    {
        dt = i - orig.top;
        db = orig.bottom - i;
        suby = 0;

        if (dt < 16)
            suby += SubTable[dt];

        if (db < 16)
            suby += SubTable[db];

        didx = didxstart + d.left;

        for (j = d.left; j < d.right; j++)
        {
            dl = j - orig.left;
            dr = orig.right - j;

            subx = 0;

            if (dl < 16)
                subx += SubTable[dl];

            if (dr < 16)
                subx += SubTable[dr];

            if (subx and suby)
                sub = (long)sqrt((float)(subx * subx + suby * suby));
            else
                sub = subx + suby;

            sub = TableVal[sub];

            if (sub < 17)
            {
                DWORD dc;//XX

                if (surface->bpp == 32)
                {
                    dc = RGB8toRGB565(((DWORD*)surface->mem)[didx]);
                }
                else
                {
                    dc = surface->mem[didx];
                }

                r = UIColorTable[operc][UIColorTable[Perc - sub][rc] + UIColorTable[bgperc + sub][(dc >> r_shift_) bitand 0x1f]];
                g = UIColorTable[operc][UIColorTable[Perc - sub][gc] + UIColorTable[bgperc + sub][(dc >> g_shift_) bitand 0x1f]];
                b = UIColorTable[operc][UIColorTable[Perc - sub][bc] + UIColorTable[bgperc + sub][(dc >> b_shift_) bitand 0x1f]];

                if (surface->bpp == 32)//XX
                    ((DWORD*)surface->mem)[didx] = RGB565toRGB8(static_cast<short>(rShift[r] bitor gShift[g] bitor bShift[b]));
                else
                    surface->mem[didx] = static_cast<short>(rShift[r] bitor gShift[g] bitor bShift[b]);

            }

            didx++;
        }

        didxstart += surface->width;
    }
}

void C_Window::DitherFill(SCREEN *surface, COLORREF color, long Perc, short size, char *pattern, UI95_RECT *rect, long Flags, long Client)
{
    long mask;
    long i, j, k, l, ls;
    long rf, gf, bf;
    long didx, didxstart;
    long rc, gc, bc;
    long convcolor;
    UI95_RECT s, d;

    mask = size - 1;

    d = *rect;

    if (Flags bitand C_BIT_ABSOLUTE)
    {
        if ( not ClipToArea(&s, &d, &Area_))
            return;
    }
    else
    {
        d.left += VX_[Client];
        d.top += VY_[Client];
        d.right += VX_[Client];
        d.bottom += VY_[Client];

        if ( not ClipToArea(&s, &d, &ClientArea_[Client]))
            return;
    }

    k = d.top;
    ls = d.left;
    d.left += GetX();
    d.top += GetY();
    d.right += GetX();
    d.bottom += GetY();

    convcolor = UI95_RGB24Bit(color);
    rc = UIColorTable[Perc][(convcolor bitand r_mask_) >> r_shift_];
    gc = UIColorTable[Perc][(convcolor bitand g_mask_) >> g_shift_];
    bc = UIColorTable[Perc][(convcolor bitand b_mask_) >> b_shift_];

    didxstart = d.top * surface->width;

    for (i = d.top; i < d.bottom; i++)
    {
        l = ls;
        didx = didxstart + d.left;

        for (j = d.left; j < d.right; j++)
        {
            rf = (WORD)rc + pattern[(k bitand mask) * size + (l bitand mask)];
            gf = (WORD)gc + pattern[(k bitand mask) * size + (l bitand mask)];
            bf = (WORD)bc + pattern[(k bitand mask) * size + (l bitand mask)];

            if (rf < 0) rf = 0;

            if (gf < 0) gf = 0;

            if (bf < 0) bf = 0;

            if (rf > 0x1f) rf = 0x1f;

            if (gf > 0x1f) gf = 0x1f;

            if (bf > 0x1f) bf = 0x1f;

            if (surface->bpp == 32) //XX
                ((DWORD*)surface->mem)[didx] = RGB565toRGB8(static_cast<short>(rShift[rf] bitor gShift[gf] bitor bShift[bf]));
            else
                surface->mem[didx] = static_cast<short>(rShift[rf] bitor gShift[gf] bitor bShift[bf]);

            l++;
            didx++;
        }

        didxstart += surface->width;
        k++;
    }
}

void C_Window::Fill(SCREEN *surface, WORD Color, UI95_RECT *rect)
{
    long startpos;
    long w, h, addpos;
    WORD *dest;
    int i;

    dest = surface->mem;
    startpos = (rect->top * surface->width + rect->left); // << 1;
    w = rect->right - rect->left;
    h = rect->bottom - rect->top;
    addpos = (surface->width - w); // << 1;

    if (surface->bpp == 32)//XX
    {
        DWORD c = RGB565toRGB8(Color);
        DWORD* dptr = ((DWORD*)surface->mem) + startpos;

        while (h--)
        {
            i = w;

            while (i--)
                *dptr++ = c;

            dptr += addpos;
        }
    }
    else
    {
        //WORD

        __asm
        {
            mov ecx, h
            mov edi, dest
            add edi, startpos
            add edi, startpos
        };
        Loop1:
        __asm
        {
            push ecx
            mov ecx, w
            mov ax, Color
            rep stosw
            add edi, addpos
            add edi, addpos
            pop ecx
            loop Loop1
        };
    }
}


void C_Window::BlitFill(SCREEN *surface, COLORREF Color, UI95_RECT *dst, long Flags, long Client)
{
    UI95_RECT s, d;

    d = *dst;

    if (Flags bitand C_BIT_ABSOLUTE)
    {
        if ( not ClipToArea(&s, &d, &Area_))
            return;
    }
    else
    {
        d.left += VX_[Client];
        d.top += VY_[Client];
        d.right += VX_[Client];
        d.bottom += VY_[Client];

        if ( not ClipToArea(&s, &d, &ClientArea_[Client]))
            return;
    }

    d.left += GetX();
    d.top += GetY();
    d.right += GetX();
    d.bottom += GetY();

    Fill(surface, UI95_RGB24Bit(Color), &d);
}

void C_Window::GradientFill(SCREEN *surface, COLORREF Color, long Perc, UI95_RECT *dst, long Flags, long Client)
{
    UI95_RECT s, d;
    long col, r, g, b;

    d = *dst;

    if (Flags bitand C_BIT_ABSOLUTE)
    {
        if ( not ClipToArea(&s, &d, &Area_))
            return;
    }
    else
    {
        d.left += VX_[Client];
        d.top += VY_[Client];
        d.right += VX_[Client];
        d.bottom += VY_[Client];

        if ( not ClipToArea(&s, &d, &ClientArea_[Client]))
            return;
    }

    d.left += GetX();
    d.top += GetY();
    d.right += GetX();
    d.bottom += GetY();

    col = UI95_RGB24Bit(Color);
    r = UIColorTable[Perc][(col bitand r_mask_) >> r_shift_];
    g = UIColorTable[Perc][(col bitand g_mask_) >> g_shift_];
    b = UIColorTable[Perc][(col bitand b_mask_) >> b_shift_];

    col = rShift[r] bitor gShift[g] bitor bShift[b];

    Fill(surface, (WORD)col, &d);
}

void C_Window::BlitFill(SCREEN *surface, COLORREF Color, long x, long y, long w, long h, long Flags, long Client, UI95_RECT *clip)
{
    UI95_RECT d, s;

    s.left = s.top = s.right = s.bottom = 0; // JPO initialise to something
    d.left = x;
    d.top = y;
    d.right = x + w;
    d.bottom = y + h;

    if (Flags bitand C_BIT_ABSOLUTE)
    {
        if ( not ClipToArea(&s, &d, &Area_))
            return;
    }
    else
    {
        d.left += VX_[Client];
        d.top += VY_[Client];
        d.right += VX_[Client];
        d.bottom += VY_[Client];

        if ( not ClipToArea(&s, &d, &ClientArea_[Client]))
            return;
    }

    if ( not ClipToArea(&s, &d, clip))
        return;

    BlitFill(surface, Color, &d, C_BIT_ABSOLUTE, 0);
}

void C_Window::DrawHLine(SCREEN *surface, COLORREF color, long x, long y, long w, long Flags, long Client, UI95_RECT *clip)
{
    UI95_RECT rect, s;

    rect.left = x;
    rect.right = x + w;
    rect.top = y;
    rect.bottom = y + 1;
    s = rect; // JPO initialise to something

    if (Flags bitand C_BIT_ABSOLUTE)
    {
        if ( not ClipToArea(&s, &rect, &Area_))
            return;
    }
    else
    {
        rect.left += VX_[Client];
        rect.top += VY_[Client];
        rect.right += VX_[Client];
        rect.bottom += VY_[Client];

        if ( not ClipToArea(&s, &rect, clip))
            return;

        if ( not ClipToArea(&s, &rect, &ClientArea_[Client]))
            return;
    }

    if ( not ClipToArea(&s, &rect, clip))
        return;

    BlitFill(surface, color, &rect, C_BIT_ABSOLUTE, 0);
}

void C_Window::DrawVLine(SCREEN *surface, COLORREF color, long x, long y, long h, long Flags, long Client, UI95_RECT *clip)
{
    UI95_RECT rect, s;

    rect.left = x;
    rect.right = x + 1;
    rect.top = y;
    rect.bottom = y + h;
    s = rect; // JPO - initialise to something.

    if (Flags bitand C_BIT_ABSOLUTE)
    {
        if ( not ClipToArea(&s, &rect, &Area_))
            return;
    }
    else
    {
        rect.left += VX_[Client];
        rect.top += VY_[Client];
        rect.right += VX_[Client];
        rect.bottom += VY_[Client];

        if ( not ClipToArea(&s, &rect, &ClientArea_[Client]))
            return;
    }

    if ( not ClipToArea(&s, &rect, clip))
        return;

    BlitFill(surface, color, &rect, C_BIT_ABSOLUTE, 0);
}

BOOL C_Window::CheckLine(long x1, long y1, long x2, long y2, long minx, long miny, long maxx, long maxy)
{
    if (x1 < minx and x2 < minx) return(FALSE);

    if (y1 < miny and y2 < miny) return(FALSE);

    if (x1 > maxx and x2 > maxx) return(FALSE);

    if (y1 > maxy and y2 > maxy) return(FALSE);

    return(TRUE);
}

enum
{
    LINE_CLIP_LEFT = 0x01,
    LINE_CLIP_RIGHT = 0x02,
    LINE_CLIP_TOP = 0x04,
    LINE_CLIP_BOTTOM = 0x08,
};

BOOL C_Window::ClipLine(long *x1, long *y1, long *x2, long *y2, UI95_RECT *clip)
{
    char flag1, flag2;
    float slope1 = 0.0, slope2 = 0.0; 

    flag1 = 0;
    flag2 = 0;

    if (*x1 < clip->left)
        flag1 or_eq LINE_CLIP_LEFT;

    if (*x2 < clip->left)
        flag2 or_eq LINE_CLIP_LEFT;

    if (*y1 < clip->top)
        flag1 or_eq LINE_CLIP_TOP;

    if (*y2 < clip->top)
        flag2 or_eq LINE_CLIP_TOP;

    if (*x1 > clip->right)
        flag1 or_eq LINE_CLIP_RIGHT;

    if (*x2 > clip->right)
        flag2 or_eq LINE_CLIP_RIGHT;

    if (*y1 > clip->bottom)
        flag1 or_eq LINE_CLIP_BOTTOM;

    if (*y2 > clip->bottom)
        flag2 or_eq LINE_CLIP_BOTTOM;

    if ( not flag1 and not flag2) // return, because both points are inside clip rect
        return(TRUE);

    if (((flag1 bitand flag2) bitand LINE_CLIP_LEFT) or // If both points are on the same side of the clip rect... don't draw
        ((flag1 bitand flag2) bitand LINE_CLIP_TOP) or
        ((flag1 bitand flag2) bitand LINE_CLIP_RIGHT) or
        ((flag1 bitand flag2) bitand LINE_CLIP_BOTTOM))
        return(FALSE);

    if (*x1 == *x2) // if x's are the same... no math required
    {
        if (flag1 bitand LINE_CLIP_TOP)
            *y1 = clip->top;
        else if (flag1 bitand LINE_CLIP_BOTTOM)
            *y1 = clip->bottom;

        if (flag2 bitand LINE_CLIP_TOP)
            *y2 = clip->top;
        else if (flag2 bitand LINE_CLIP_BOTTOM)
            *y2 = clip->bottom;
    }
    else if (*y1 == *y2)
    {
        if (flag1 bitand LINE_CLIP_LEFT)
            *x1 = clip->left;
        else if (flag1 bitand LINE_CLIP_RIGHT)
            *x1 = clip->right;

        if (flag2 bitand LINE_CLIP_LEFT)
            *x2 = clip->left;
        else if (flag2 bitand LINE_CLIP_RIGHT)
            *x2 = clip->right;
    }
    else
    {
        if ((flag1 bitor flag2) bitand (LINE_CLIP_LEFT bitor LINE_CLIP_RIGHT))
            slope1 = (float)(*y2 - *y1) / (float)(*x2 - *x1);

        if ((flag1 bitor flag2) bitand (LINE_CLIP_TOP bitor LINE_CLIP_BOTTOM))
            slope2 = (float)(*x2 - *x1) / (float)(*y2 - *y1);

        if (flag1 bitand LINE_CLIP_LEFT)
        {
            *y1 = FloatToInt32(*y1 + (clip->left - *x1) * slope1);
            *x1 = clip->left;
        }
        else if (flag1 bitand LINE_CLIP_RIGHT)
        {
            *y1 = FloatToInt32(*y1 + (clip->right - *x1) * slope1);
            *x1 = clip->right;
        }
        else if (flag1 bitand LINE_CLIP_TOP)
        {
            *x1 = FloatToInt32(*x1 + (clip->top - *y1) * slope2);
            *y1 = clip->top;
        }
        else if (flag1 bitand LINE_CLIP_BOTTOM)
        {
            *x1 = FloatToInt32(*x1 + (clip->bottom - *y1) * slope2);
            *y1 = clip->bottom;
        }

        if (flag2 bitand LINE_CLIP_LEFT)
        {
            *y2 = FloatToInt32(*y2 + (clip->left - *x2) * slope1);
            *x2 = clip->left;
        }
        else if (flag2 bitand LINE_CLIP_RIGHT)
        {
            *y2 = FloatToInt32(*y2 + (clip->right - *x2) * slope1);
            *x2 = clip->right;
        }
        else if (flag2 bitand LINE_CLIP_TOP)
        {
            *x2 = FloatToInt32(*x2 + (clip->top - *y2) * slope2);
            *y2 = clip->top;
        }
        else if (flag2 bitand LINE_CLIP_BOTTOM)
        {
            *x2 = FloatToInt32(*x2 + (clip->bottom - *y2) * slope2);
            *y2 = clip->bottom;
        }

        if ((*x1 < clip->left or *x2 < clip->left) or (*x1 > clip->right or *x2 > clip->right))
            return(FALSE);

        if ((*y1 < clip->top or *y2 < clip->top) or (*y1 > clip->bottom or *y2 > clip->bottom))
            return(FALSE);
    }

    return(TRUE);
}

void C_Window::DrawLine(SCREEN *surface, COLORREF color, long x1, long y1, long x2, long y2, long Flags, long Client, UI95_RECT *clip)
{
    WORD drawcolor;
    UI95_RECT clipper;

    clipper = *clip;

    if ( not (Flags bitand C_BIT_ABSOLUTE))
    {
        x1 += VX_[Client];
        y1 += VY_[Client];
        x2 += VX_[Client];
        y2 += VY_[Client];

        if (clipper.left < ClientArea_[Client].left) clipper.left = ClientArea_[Client].left;

        if (clipper.top < ClientArea_[Client].top) clipper.top = ClientArea_[Client].top;

        if (clipper.right > ClientArea_[Client].right) clipper.right = ClientArea_[Client].right;

        if (clipper.bottom > ClientArea_[Client].bottom) clipper.bottom = ClientArea_[Client].bottom;
    }

    x1 += GetX();
    y1 += GetY();
    x2 += GetX();
    y2 += GetY();
    clipper.left += GetX();
    clipper.top += GetY();
    clipper.right += GetX();
    clipper.bottom += GetY();

    drawcolor = (WORD)UI95_RGB24Bit(color);
    DrawClipLine(surface, x1, y1, x2, y2, &clipper, drawcolor);
}

void C_Window::DrawClipLine(SCREEN *surface, long x1, long y1, long x2, long y2, UI95_RECT *clip, WORD color)
{
    long x_unit, y_unit;
    long offset, y_dir;
    long xdiff, ydiff;
    long error_term;
    long length, i;

    if ( not ClipLine(&x1, &y1, &x2, &y2, clip))
        if ( not ClipLine(&x1, &y1, &x2, &y2, clip))
            return;

    ydiff = y2 - y1;

    if (ydiff < 0)
    {
        ydiff = -ydiff;
        y_unit = -surface->width;
        y_dir = -1;
    }
    else
    {
        y_unit = surface->width;
        y_dir = 1;
    }

    xdiff = x2 - x1;

    if (xdiff < 0)
    {
        xdiff = -xdiff;
        x_unit = -1;
    }
    else
        x_unit = 1;

    offset = y1 * surface->width + x1;
    error_term = 0;

    if (xdiff > ydiff)
    {
        length = xdiff + 1;

        for (i = 0; i < length; i++)
        {
            if (surface->bpp == 32) //XX
                ((DWORD*)surface->mem)[offset] = RGB565toRGB8(color);
            else
                surface->mem[offset] = color;

            error_term += ydiff;

            offset += x_unit;
            x1 += x_unit;

            if (error_term > xdiff)
            {
                error_term -= xdiff;
                offset += y_unit;
                y1 += y_dir;
            }
        }
    }
    else
    {
        length = ydiff + 1;

        for (i = 0; i < length; i++)
        {
            if (surface->bpp == 32) //XX
                ((DWORD*)surface->mem)[offset] = RGB565toRGB8(color);
            else
                surface->mem[offset] = color;

            error_term += xdiff;
            offset += y_unit;
            y1 += y_dir;

            if (error_term > 0)
            {
                error_term -= ydiff;
                offset += x_unit;
                x1 += x_unit;
            }
        }
    }
}

void C_Window::DrawCircle(SCREEN *surface, COLORREF color, long x, long y, float radius, long Flags, long Client, UI95_RECT *clip)
{
    short i, j;
    long x1, y1, x2, y2;
    WORD linecolor;
    UI95_RECT clipper;

    clipper = *clip;

    if ( not (Flags bitand C_BIT_ABSOLUTE))
    {
        x += VX_[Client];
        y += VY_[Client];

        if (clipper.left < ClientArea_[Client].left) clipper.left = ClientArea_[Client].left;

        if (clipper.top < ClientArea_[Client].top) clipper.top = ClientArea_[Client].top;

        if (clipper.right > ClientArea_[Client].right) clipper.right = ClientArea_[Client].right;

        if (clipper.bottom > ClientArea_[Client].bottom) clipper.bottom = ClientArea_[Client].bottom;
    }

    x1 = x - (long)radius;
    y1 = y - (long)radius;
    x2 = x + (long)radius;
    y2 = y + (long)radius;

    if ( not CheckLine(x1, y1, x2, y2, clipper.left, clipper.top, clipper.right, clipper.bottom))
        return;

    x += GetX();
    y += GetY();
    clipper.left += GetX();
    clipper.top += GetY();
    clipper.right += GetX();
    clipper.bottom += GetY();

    linecolor = (WORD)UI95_RGB24Bit(color);

    for (i = 0; i < 8; i++)
    {
        x1 = (long)(Circle8[0][0] * radius);
        y1 = (long)(Circle8[0][1] * radius);

        for (j = 1; j < CIRCLE_POINTS; j++)
        {
            x2 = (long)(Circle8[j][0] * radius);
            y2 = (long)(Circle8[j][1] * radius);

            DrawClipLine(surface, x + x1, y + y1, x + x2, y + y2, &clipper, linecolor);
            DrawClipLine(surface, x - x1, y + y1, x - x2, y + y2, &clipper, linecolor);
            DrawClipLine(surface, x + x1, y - y1, x + x2, y - y2, &clipper, linecolor);
            DrawClipLine(surface, x - x1, y - y1, x - x2, y - y2, &clipper, linecolor);

            DrawClipLine(surface, x + y1, y + x1, x + y2, y + x2, &clipper, linecolor);
            DrawClipLine(surface, x - y1, y + x1, x - y2, y + x2, &clipper, linecolor);
            DrawClipLine(surface, x + y1, y - x1, x + y2, y - x2, &clipper, linecolor);
            DrawClipLine(surface, x - y1, y - x1, x - y2, y - x2, &clipper, linecolor);

            x1 = x2;
            y1 = y2;
        }
    }
}

void C_Window::DrawArc(SCREEN *surface, COLORREF color, long x, long y, float radius, short section, long Flags, long Client, UI95_RECT *clip)
{
    short i, j;
    float x1, y1, x2, y2;

    for (i = 0; i < 8; i++)
    {
        x1 = Circle8[0][0] * radius;
        y1 = Circle8[0][1] * radius;

        for (j = 1; j < CIRCLE_POINTS; j++)
        {
            x2 = Circle8[j][0] * radius;
            y2 = Circle8[j][1] * radius;

            switch (section)
            {
                case 0:
                        DrawLine(surface, color, x + (long)x1, y - (long)y1, x + (long)x2, y - (long)y2, Flags, Client, clip);
                    break;

                case 1:
                        DrawLine(surface, color, x + (long)y1, y - (long)x1, x + (long)y2, y - (long)x2, Flags, Client, clip);
                    break;

                case 2:
                        DrawLine(surface, color, x + (long)y1, y + (long)x1, x + (long)y2, y + (long)x2, Flags, Client, clip);
                    break;

                case 3:
                        DrawLine(surface, color, x + (long)x1, y + (long)y1, x + (long)x2, y + (long)y2, Flags, Client, clip);
                    break;

                case 4:
                        DrawLine(surface, color, x - (long)x1, y + (long)y1, x - (long)x2, y + (long)y2, Flags, Client, clip);
                    break;

                case 5:
                        DrawLine(surface, color, x - (long)y1, y + (long)x1, x - (long)y2, y + (long)x2, Flags, Client, clip);
                    break;

                case 6:
                        DrawLine(surface, color, x - (long)y1, y - (long)x1, x - (long)y2, y - (long)x2, Flags, Client, clip);
                    break;

                case 7:
                        DrawLine(surface, color, x - (long)x1, y - (long)y1, x - (long)x2, y - (long)y2, Flags, Client, clip);
                    break;
            }

            x1 = x2;
            y1 = y2;
        }
    }
}

BOOL C_Window::UpdateTimerControls()
{
    BOOL retval = FALSE;
    CONTROLLIST *cur, *me;
    F4CSECTIONHANDLE *Leave;

    Leave = UI_Enter(this);
    cur = Controls_;

    while (cur)
    {
        me = cur;
        cur = cur->Next;

        if (me->Control_->GetFlags() bitand C_BIT_TIMER)
            if (me->Control_->TimerUpdate())
                retval = TRUE;
    }

    UI_Leave(Leave);
    return(retval);
}

void C_Window::SetGroupState(long GroupID, short state)
{
    CONTROLLIST *cur;

    cur = Controls_;

    while (cur)
    {
        if (cur->Control_->GetGroup() == GroupID and cur->Control_->IsControl())
        {
            if (cur->Control_->GetState() not_eq state)
            {
                cur->Control_->SetState(state);
                cur->Control_->Refresh();
            }
        }

        cur = cur->Next;
    }
}


BOOL C_Window::KeyboardMode()
{
    if (Handler_)
        return(Handler_->KeyboardMode());

    return(FALSE);
}

void C_Window::DeactivateControl()
{
    if (CurControl_)
    {
        CurControl_->Deactivate();
        CurControl_ = NULL;
    }
}

void C_Window::RemovingControl(C_Base *control)
{
    if (CurControl_ == control)
        CurControl_ = NULL;

    Handler_->RemovingControl(control);
}

void C_Window::SetControl(long ID) // Called when mouse is used over this control
{
    C_Base *cur;

    cur = FindControl(ID);

    if (cur)
    {
        if (cur->GetFlags() bitand C_BIT_SELECTABLE)
        {
            if (CurControl_)
            {
                if (cur not_eq CurControl_)
                {
                    CurControl_->Deactivate();
                    CurControl_ = cur;
                    CurControl_->Activate();
                }
            }
            else
            {
                CurControl_ = cur;
                CurControl_->Activate();
            }
        }
        else
        {
            if (CurControl_)
            {
                CurControl_->Deactivate();
                CurControl_ = NULL;
            }
        }
    }
    else
    {
        if (CurControl_)
        {
            CurControl_->Deactivate();
            CurControl_ = NULL;
        }
    }
}

void C_Window::SetPrevControl() // Called when SHIFT bitand TAB are pressed
{
    CONTROLLIST *cur;

    if (CurControl_)
        CurControl_->Deactivate();

    if (CurControl_ == NULL)
    {
        cur = Last_;

        while (cur)
        {
            if (cur->Control_->GetFlags() bitand C_BIT_SELECTABLE and cur->Control_->GetFlags() bitand C_BIT_ENABLED and not (cur->Control_->GetFlags() bitand C_BIT_INVISIBLE))
            {
                CurControl_ = cur->Control_;
                CurControl_->Activate();
                return;
            }

            cur = cur->Prev;
        }
    }
    else
    {
        cur = FindControlInList(CurControl_);

        if (cur == NULL)
            return;

        if (cur->Prev == NULL)
            cur = Last_;
        else
            cur = cur->Prev;

        while (cur->Control_ not_eq CurControl_)
        {
            if (cur->Control_->GetFlags() bitand C_BIT_SELECTABLE and cur->Control_->GetFlags() bitand C_BIT_ENABLED and not (cur->Control_->GetFlags() bitand C_BIT_INVISIBLE))
            {
                CurControl_ = cur->Control_;
                CurControl_->Activate();
                return;
            }

            cur = cur->Prev;

            if (cur == NULL)
                cur = Last_;
        }
    }
}

void C_Window::SetNextControl() // Called when TAB is pressed
{
    CONTROLLIST *cur;

    if (CurControl_)
        CurControl_->Deactivate();

    if (CurControl_ == NULL)
    {
        cur = Controls_;

        while (cur)
        {
            if (cur->Control_->GetFlags() bitand C_BIT_SELECTABLE and cur->Control_->GetFlags() bitand C_BIT_ENABLED and not (cur->Control_->GetFlags() bitand C_BIT_INVISIBLE))
            {
                CurControl_ = cur->Control_;
                CurControl_->Activate();
                return;
            }

            cur = cur->Next;
        }
    }
    else
    {
        cur = FindControlInList(CurControl_);

        if (cur == NULL)
            return;

        if (cur->Next == NULL)
            cur = Controls_;
        else
            cur = cur->Next;

        while (cur->Control_ not_eq CurControl_)
        {
            if (cur->Control_->GetFlags() bitand C_BIT_SELECTABLE and cur->Control_->GetFlags() bitand C_BIT_ENABLED and not (cur->Control_->GetFlags() bitand C_BIT_INVISIBLE))
            {
                CurControl_ = cur->Control_;
                CurControl_->Activate();
                return;
            }

            cur = cur->Next;

            if (cur == NULL)
                cur = Controls_;
        }
    }
}

void C_Window::Activate()
{
    C_Base *wintitle;

    wintitle = FindControl(WINDOW_TITLE_BAR);

    if (wintitle)
    {
        wintitle->SetState(1);
        wintitle->Refresh();
    }

    if (CurControl_)
        CurControl_->Activate();
}

void C_Window::Deactivate()
{
    C_Base *wintitle;

    wintitle = FindControl(WINDOW_TITLE_BAR);

    if (wintitle)
    {
        wintitle->SetState(0);
        wintitle->Refresh();
    }

    if (CurControl_)
        CurControl_->Deactivate();
}

BOOL C_Window::CheckKeyboard(unsigned char DKScanCode, unsigned char Ascii, unsigned char ShiftStates, long RepeatCount)
{
    if (KBCallback_)
        if ((*KBCallback_)(DKScanCode, Ascii, ShiftStates, RepeatCount))
            return(TRUE);

    if (Ascii)
    {
        if (CurControl_ and CurControl_->CheckKeyboard(DKScanCode, Ascii, ShiftStates, RepeatCount))
            return(TRUE);
    }
    else
    {
        switch (DKScanCode)
        {
            case DIK_TAB:
                    if (ShiftStates == _SHIFT_DOWN_)
                        SetPrevControl();
                    else if ( not ShiftStates)
                        SetNextControl();

                return(TRUE);
                break;

            case DIK_NUMPAD0:
                case DIK_INSERT:
                        if (Handler_->KeyboardMode())
                            Handler_->SetKeyboardMode(FALSE);
                        else
                            Handler_->SetKeyboardMode(TRUE);

                return(TRUE);
                break;

            default:
                    if (CurControl_ == NULL)
                        break;

                if (CurControl_->CheckKeyboard(DKScanCode, Ascii, ShiftStates, RepeatCount))
                    return(TRUE);

                break;
        }
    }

    return(FALSE);
}

//BOOL C_Window::CheckHotKeys(unsigned char DKScanCode,unsigned char Ascii,unsigned char ShiftStates,long RepeatCount)
BOOL C_Window::CheckHotKeys(unsigned char DKScanCode, unsigned char, unsigned char ShiftStates, long)
{
    CONTROLLIST *cur;

    cur = Controls_;

    while (cur)
    {
        if (cur->Control_->GetHotKey() == (DKScanCode bitor (ShiftStates << 8)))
        {
            cur->Control_->Process(cur->Control_->GetID(), C_TYPE_LMOUSEDOWN);
            cur->Control_->Process(cur->Control_->GetID(), C_TYPE_LMOUSEUP);
            return(TRUE);
        }

        cur = cur->Next;
    }

    return(FALSE);
}

//BOOL C_Window::Drag(GRABBER *Drag,WORD MouseX,WORD MouseY,C_Window *over)
BOOL C_Window::Drag(GRABBER *Drag, WORD MouseX, WORD MouseY, C_Window*)
{
    long x, y;
    F4CSECTIONHANDLE* Leave;

    x = Drag->ItemX_ + (MouseX - Drag->StartX_);
    y = Drag->ItemY_ + (MouseY - Drag->StartY_);

    if (x < MinX_) x = MinX_;

    if ((x + GetW()) > MaxX_) x = MaxX_ - GetW();

    if (y < MinY_) y = MinY_;

    if ((y + GetH()) > MaxY_) y = MaxY_ - GetH();

    Leave = UI_Enter(this);
    Handler_->SetBehindWindow(this);
    SetXY((short)x, (short)y);

    if (DragCallback_)
        (*DragCallback_)(this);

    RefreshWindow();
    UI_Leave(Leave);
    return(TRUE);
}

//BOOL C_Window::Drop(GRABBER *Drag,WORD MouseX,WORD MouseY,C_Window *over)
BOOL C_Window::Drop(GRABBER*, WORD, WORD, C_Window*)
{
    return(FALSE);
}

// sfr: added for constraints
void C_Window::ConstraintsCorrection(long w, long h)
{
    // clip window dimensions to screen
    if (this->x_ + this->Area_.right >= w)
    {
        this->Area_.right -= ((this->x_ + this->Area_.right) - w + 1);
    }

    if (this->y_ + this->Area_.bottom >= h)
    {
        this->Area_.bottom -= ((this->y_ + this->Area_.bottom) - h + 1);
    }
}

#ifdef _UI95_PARSER_
short C_Window::LocalFind(char *token)
{
    short i = 0;

    while (C_Win_Tokens[i])
    {
        if (strnicmp(token, C_Win_Tokens[i], strlen(C_Win_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Window::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CWIN_SETUP:
                Setup(P[0], (short)P[1], (short)P[2], (short)P[3]);
            break;

        case CWIN_SETX:
                SetX((short)P[0]);
            break;

        case CWIN_SETY:
                SetY((short)P[0]);
            break;

        case CWIN_SETXY:
                SetXY((short)P[0], (short)P[1]);
            break;

        case CWIN_SETW:
                SetW((short)P[0]);
            break;

        case CWIN_SETH:
                SetH((short)P[0]);
            break;

        case CWIN_SETRANGES:
                SetRanges((short)P[0], (short)P[1], (short)P[2], (short)P[3], (short)P[4], (short)P[5]);
            break;

        case CWIN_SETFONT:
                SetFont(P[0]);
            break;

        case CWIN_SETGROUP:
                SetGroup(P[0]);
            break;

        case CWIN_SETCLIENTAREA:
                SetClientArea((short)P[0], (short)P[1], (short)P[2], (short)P[3], (short)P[4]);
            break;

        case CWIN_SETFLAGBITON:
                SetFlagBitOn(P[0]);
            break;

        case CWIN_SETFLAGBITOFF:
                SetFlagBitOff(P[0]);
            break;

        case CWIN_SETDEPTH:
                SetDepth((short)P[0]);
            break;

        case CWIN_SETMENUID:
                SetMenu(P[0]);
            break;

        case CWIN_SETCLIENTMENUID:
                SetClientMenu(P[0], P[1]);
            break;

        case CWIN_SETCURSOR:
                SetCursorID(P[0]);
            break;

        case CWIN_SETDRAGH:
                SetDragH((short)P[0]);
            break;

        case CWIN_SETCLIENTFLAGS:
                SetClientFlags(P[0], P[1]);
            break;
    }
}

extern char ParseCRLF[];
extern char ParseSave[];

void C_Window::SaveTextControls(HANDLE ofp, C_Parser *Parser)
{
    CONTROLLIST *cntrl;
    DWORD bw;

    cntrl = Controls_;

    while (cntrl)
    {
        cntrl->Control_->SaveText(ofp, Parser);
        WriteFile(ofp, ParseCRLF, strlen(ParseCRLF), &bw, NULL);
        cntrl = cntrl->Next;
    }
}

#endif // PARSER
