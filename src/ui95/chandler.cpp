#include <cISO646>
#include <windows.h>
#include <process.h>
#include "dispcfg.h"
#include "chandler.h"
#include "sim/include/ascii.h"


// OW needed for restoring textures after task switch
#include "graphics/include/texbank.h"
#include "graphics/include/fartex.h"
#include "graphics/include/terrtex.h"

#define UI_ABS(x) (((x) < 0)? -(x):(x))

long _LOAD_ART_RESOURCES_ = 1;

// Take Screenshot HACK stuff
long gUI_TakeScreenShot = 0;
// MN 020104 always allow UI screenshots
long gScreenShotEnabled = 1;
WORD *gScreenShotBuffer = NULL;
void SaveScreenShot();

// OW 11-08-2000
extern bool g_bCheckBltStatusBeforeFlip;
// M.N. 2001-11-13
extern bool g_bHiResUI;

extern void Transmit(int com);

extern int gameCompressionRatio;
extern int g_nMaxUIRefresh; // 2002-02-23 S.G.


extern WORD RGB8toRGB565(DWORD);//XX
extern DWORD RGB565toRGB8(WORD sc);



extern C_Handler *gMainHandler;

C_Handler::C_Handler()
{
    UI_Critical = NULL;
    MouseCallback_ = NULL;
    Root_ = NULL; // Root pointer to windows in handler
    AppWindow_ = NULL;
    Primary_ = NULL; // primary surface pointer
    Front_ = NULL; // Surface to blit to
    FrontRect_.left = 0;
    FrontRect_.top = 0;
    FrontRect_.right = 0;
    FrontRect_.bottom = 0;
    // PrimaryRect_.left=0;
    // PrimaryRect_.top=0;
    // PrimaryRect_.right=0;
    // PrimaryRect_.bottom=0;
    surface_.mem = NULL;
    surface_.width = 0;
    surface_.height = 0;
    MouseDown_ = 0;
    MouseDownTime_ = 0;
    LastUp_ = 0;
    LastUpTime_ = 0;
    DoubleClickTime_ = GetDoubleClickTime();
    TimerLoop_ = 0;
    ControlLoop_ = 0;
    OutputLoop_ = 0;
    OutputWait_ = 80;
    TimerSleep_ = 1;
    ControlSleep_ = 1;
    TimerThread_ = 0;
    ControlThread_ = 0;
    OutputThread_ = 0;
    WakeOutput_ = NULL;
    WakeControl_ = NULL;
    memset(&Grab_, 0, sizeof(GRABBER));
    memset(&Drag_, 0, sizeof(GRABBER));
    OverControl_ = NULL;
    OverLast_.Control_ = NULL;
    OverLast_.Tip_ = NULL;
    OverLast_.Time_ = 0;
    OverLast_.HelpFont_ = 17;
    OverLast_.HelpOn_ = 0;
    MouseControl_ = NULL;
    CurWindow_ = NULL;
    UpdateFlag = 0;
    HandlingMessage = 0;
    UserRoot_ = NULL;
    KeyboardMode_ = FALSE;
    DrawFlags = 0;
}

C_Handler::~C_Handler()
{
    if (Root_)
        Cleanup();
}

void C_Handler::EnterCritical()
{
    F4EnterCriticalSection(UI_Critical);
}

void C_Handler::LeaveCritical()
{
    F4LeaveCriticalSection(UI_Critical);
}

static ImageBuffer *m_pMouseImage;
RECT m_rcMouseImage;

void C_Handler::Setup(HWND hwnd, ImageBuffer *, ImageBuffer *Primary)
{
    Primary_ = Primary;
    // PrimaryRect_=PrimaryRect;
    AppWindow_ = hwnd;

    int dispXres = 800, dispYres = 600;

    if (g_bHiResUI)
    {
        dispXres = 1024;
        dispYres = 768;
    }

    // OW V2
#if 1

    if (FalconDisplay.displayFullScreen)
    {
        // OW 11-08-2000
#if 0
        Front_ = Primary_;
#else
        Front_ = new ImageBuffer;


        BOOL bResult;

        if (FalconDisplay.theDisplayDevice.IsHardware())
            bResult = Front_->Setup(&FalconDisplay.theDisplayDevice, dispXres, dispYres, VideoMem, None, FALSE);
        else
            bResult  = Front_->Setup(&FalconDisplay.theDisplayDevice, dispXres, dispYres, SystemMem, None, FALSE);

#endif

        // OW now handled by running in software mode on V1 and V2
#if 0
        // OW FIXME: hack export a function from device or somewhere
        DXContext *pCtx = FalconDisplay.theDisplayDevice.GetDefaultRC();

        if ( not (pCtx->m_pcapsDD->dwCaps2 bitand DDCAPS2_CANRENDERWINDOWED))
        {
            int nWidth = 16;
            int nHeight = 16;

            m_rcMouseImage.left = 0;
            m_rcMouseImage.top = 0;
            m_rcMouseImage.bottom = nWidth;
            m_rcMouseImage.right = nHeight;

            // prolly voodoo 1 or 2
            m_pMouseImage = new ImageBuffer;

            if (m_pMouseImage->Setup(&FalconDisplay.theDisplayDevice, nWidth, nHeight, SystemMem, None, AppWindow_, FALSE, FALSE))
            {
                WORD *pSurface = (WORD *) m_pMouseImage->Lock();
                int nStride = m_pMouseImage->targetStride() / m_pMouseImage->PixelSize();
                int nHeight = m_pMouseImage->targetYres();

                ShiAssert(pSurface);

                if (pSurface)
                {
                    static WORD cursorImage[16][16] =
                    {
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xffff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xffff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xffff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xffff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xffff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xffff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xffff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xffff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xffff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xffff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xffff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xffff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    };

                    memcpy(pSurface, cursorImage, 256 * 2);
                    m_pMouseImage->Unlock();

                    m_pMouseImage->SetChromaKey(0);
                }
            }
        }

#endif
    }
    //windowed
    else
    {
        Front_ = new ImageBuffer;

        BOOL bResult;

        if (FalconDisplay.theDisplayDevice.IsHardware())
            bResult = Front_->Setup(&FalconDisplay.theDisplayDevice, dispXres, dispYres, VideoMem, None, FALSE);
        else
            bResult  = Front_->Setup(&FalconDisplay.theDisplayDevice, dispXres, dispYres, SystemMem, None, FALSE);

        if ( not bResult)
        {
            MonoPrint("Can't create back surface for UI\n");
            Front_ = Primary_;
        }
    }

#else
    Front_ = new ImageBuffer;

    BOOL bResult;

    if (FalconDisplay.theDisplayDevice.IsHardware())
        bResult = Front_->Setup(&FalconDisplay.theDisplayDevice, 800, 600, VideoMem, None, FALSE);
    else
        bResult  = Front_->Setup(&FalconDisplay.theDisplayDevice, 800, 600, SystemMem, None, FALSE);

    if ( not bResult)
    {
        MonoPrint("Can't create back surface for UI\n");
        Front_ = Primary_;
    }

#endif

    // OW - WM_MOVE does not get sent to the app in response to SetWindowPos
    if ( not FalconDisplay.displayFullScreen)
    {
        RECT dest;
        GetClientRect(hwnd, &dest);
        ClientToScreen(hwnd, (LPPOINT)&dest);
        ClientToScreen(hwnd, (LPPOINT)&dest + 1);

        if (Primary_ and Primary_->frontSurface())
        {
            Primary_->UpdateFrontWindowRect(&dest);
        }

        InvalidateRect(hwnd, &dest, FALSE);
    }

    // This will be correct UNLESS the work surface is a FLIPPING backbuffer of the primary
    // AND the application is running in a window.  Not likly...
    // By the way, this means this rect could really go away entirely...
    FrontRect_.top = 0;;
    FrontRect_.left = 0;;
    FrontRect_.bottom = Front_->targetYres();
    FrontRect_.right = Front_->targetXres();

    Root_ = NULL;
    rectcount_ = 0;
    UI_Critical = F4CreateCriticalSection("UI_Critical");
    StartOutputThread();
    StartControlThread(80);
}

void C_Handler::Cleanup()
{
    WHLIST *cur, *prev;
    CBLIST *cbs, *last;

    // Make ALL winders invisible so they won't draw
    cur = Root_;

    while (cur)
    {
        cur->Flags and_eq compl C_BIT_ENABLED;
        cur = cur->Next;
    }

    // Kill the threads
    EndControlThread();
    EndOutputThread();
    EndTimerThread();

    EnterCritical();

    cur = Root_;

    while (cur)
    {
        prev = cur;
        cur = cur->Next;

        if (prev->Flags bitand C_BIT_REMOVE)
        {
            prev->win->Cleanup();
            delete prev->win;
        }

        delete prev;
    }

    Root_ = NULL;

    cbs = UserRoot_;

    while (cbs)
    {
        last = cbs;
        cbs = cbs->Next;
        delete last;
    }

    UserRoot_ = NULL;

    if (Front_ and Front_ not_eq Primary_)
    {
        Front_->Cleanup();
        delete Front_;
    }

    Primary_ = NULL;
    Front_ = NULL;

    if (m_pMouseImage)
    {
        m_pMouseImage->Cleanup();
        delete m_pMouseImage;
        m_pMouseImage = NULL;
    }

    LeaveCritical();

    F4DestroyCriticalSection(UI_Critical);
    UI_Critical = NULL; // JB 010108
}

void *C_Handler::Lock()
{
    if (Front_)
    {
        surface_.mem = (WORD *)Front_->Lock();

        // surface_.width = (short)Front_->targetXres(); //
        surface_.width = (short)Front_->targetStride() / Front_->PixelSize(); // OW

        surface_.height = (short)Front_->targetYres(); //
        //XX
        surface_.bpp = Front_->PixelSize() << 3;//bytes->bits
        surface_.owner = Front_;
    }

    return(surface_.mem);
}

void C_Handler::Unlock()
{
    if (Front_)
    {
        Front_->Unlock();
        surface_.mem = NULL;
        surface_.width = 0;
        surface_.height = 0;
        //XX
        surface_.bpp = 0;
    }
}

BOOL C_Handler::AddWindow(C_Window *thewin, long Flags)
{
    WHLIST *newwin, *cur;

    cur = Root_;

    while (cur)
    {
        if (cur->win == thewin)
            return(FALSE);

        cur = cur->Next;
    }

    newwin = new WHLIST;

    if (newwin == NULL)
        return(FALSE);

    newwin->win = thewin;
    newwin->Flags = Flags;
    newwin->Next = NULL;
    newwin->Prev = NULL;

    EnterCritical();

    if (Root_ == NULL)
    {
        Root_ = newwin;
    }
    else if (thewin->GetDepth() < Root_->win->GetDepth() and thewin->GetDepth() or (thewin->GetFlags() bitand C_BIT_CANTMOVE and not (Root_->win->GetFlags() bitand C_BIT_CANTMOVE)))
    {
        newwin->Next = Root_;
        Root_->Prev = newwin;
        Root_ = newwin;
    }
    else
    {
        cur = Root_;

        while (cur and newwin)
        {
            if (thewin->GetDepth() < cur->win->GetDepth() and thewin->GetDepth() or (thewin->GetFlags() bitand C_BIT_CANTMOVE and not (cur->win->GetFlags() bitand C_BIT_CANTMOVE)))
            {
                newwin->Next = cur;
                newwin->Prev = cur->Prev;
                newwin->Prev->Next = newwin;
                newwin->Next->Prev = newwin;
                newwin = NULL;
            }
            else if (cur->Next == NULL)
            {
                cur->Next = newwin;
                newwin->Prev = cur;
                newwin = NULL;
            }
            else
                cur = cur->Next;
        }
    }

    thewin->SetHandler(this);
    thewin->update_ or_eq C_DRAW_REFRESHALL;
    thewin->RefreshWindow();

    if (thewin->GetFlags() bitand C_BIT_ENABLED)
        CurWindow_ = thewin;

    // sfr: before adding window, correct its constraints to screen
    thewin->ConstraintsCorrection(GetW(), GetH());

    LeaveCritical();
    return(TRUE);
}

BOOL C_Handler::ShowWindow(C_Window *thewin)
{
    WHLIST *cur;

    cur = Root_;

    while (cur)
    {
        if (cur->win == thewin and not (cur->Flags bitand C_BIT_ENABLED))
        {
            cur->win->SetCritical(UI_Critical);
            cur->Flags or_eq C_BIT_ENABLED;
            cur->win->update_ = C_DRAW_REFRESHALL;
            cur->win->RefreshWindow();
            cur->win->SetSection(CurrentSection_);
            return(TRUE);
        }

        cur = cur->Next;
    }

    return(FALSE);
}

BOOL C_Handler::HideWindow(C_Window *thewin)
{
    WHLIST *cur;


    cur = Root_;

    while (cur)
    {
        if (cur->win == thewin and (cur->Flags bitand C_BIT_ENABLED))
        {
            cur->Flags and_eq compl C_BIT_ENABLED;
            SetBehindWindow(cur->win);
            cur->win->SetCritical(NULL);

            if (CurWindow_ == thewin)
                CurWindow_ = NULL;

            return(TRUE);
        }

        cur = cur->Next;
    }

    return(FALSE);
}

C_Window *C_Handler::FindWindow(long ID)
{
    if (F4IsBadReadPtr(this, sizeof(C_Handler))) // JB 010317 CTD
        return NULL;

    WHLIST *cur;

    cur = Root_;

    while (cur)
    {
        if (cur->win->GetID() == ID)
            return(cur->win);

        cur = cur->Next;
    }

    return(NULL);
}

long C_Handler::GetWindowFlags(long ID)
{
    WHLIST *cur;

    cur = Root_;

    while (cur)
    {
        if (cur->win->GetID() == ID)
            return(cur->Flags);

        cur = cur->Next;
    }

    return(0);
}

C_Window *C_Handler::_GetFirstWindow()
{
    EnterCritical();
    LeaveCritical();

    if (Root_)
        return(Root_->win);

    return(NULL);
}

C_Window *C_Handler::_GetNextWindow(C_Window *win)
{
    WHLIST *cur;

    EnterCritical();
    cur = Root_;

    while (cur)
    {
        if (cur->win == win)
        {
            LeaveCritical();

            if (cur->Next == NULL)
                return(NULL);

            return(cur->Next->win);
        }

        cur = cur->Next;
    }

    LeaveCritical();
    return(NULL);
}

BOOL C_Handler::RemoveWindow(C_Window *thewin)
{
    WHLIST *cur;


    EnterCritical();
    SetBehindWindow(thewin);
    thewin->SetCritical(NULL);

    if (Grab_.Window_ == thewin)
    {
        Grab_.Window_ = NULL;
        Grab_.Control_ = NULL;
    }

    if (Drag_.Window_ == thewin)
    {
        Drag_.Window_ = NULL;
        Drag_.Control_ = NULL;
    }

    if (thewin == Root_->win)
    {
        cur = Root_;
        Root_ = Root_->Next;
        Root_->Prev = NULL;
        delete cur;
    }
    else
    {
        cur = Root_->Next;

        while (cur)
        {
            if (cur->win == thewin)
            {
                if (cur->Prev)
                    cur->Prev->Next = cur->Next;

                if (cur->Next)
                    cur->Next->Prev = cur->Prev;

                delete cur;
                cur = NULL;
            }
            else
                cur = cur->Next;
        }
    }

    if (thewin == CurWindow_)
    {
        if (Root_ == NULL)
            CurWindow_ = NULL;
        else
        {
            cur = Root_;

            while (cur)
            {
                CurWindow_ = cur->win;
                cur = cur->Next;
            }
        }
    }

    LeaveCritical();
    return(TRUE);
}

void C_Handler::WindowToFront(C_Window *thewin) // move to end of list
{
    WHLIST *cur, *found;

    if (Root_ == NULL)
        return;

    if (Root_->Next == NULL)
        return;

    CurWindow_ = thewin;

    if (Root_->win == thewin)
    {
        if (Root_->Flags bitand C_BIT_CANTMOVE or not (Root_->Flags bitand C_BIT_ENABLED))
            return;

        found = Root_;
        Root_ = Root_->Next;
        Root_->Prev = NULL;
        cur = Root_;

        while (cur->Next)
            cur = cur->Next;

        EnterCritical();
        cur->Next = found;
        found->Next = NULL;
        found->Prev = cur;
        LeaveCritical();
    }
    else
    {
        cur = Root_;
        found = NULL;

        while (cur->Next)
        {
            if (cur->Next->win == thewin and found == NULL)
            {
                if (cur->Next->Flags bitand C_BIT_CANTMOVE or not (cur->Next->Flags bitand C_BIT_ENABLED))
                    return;

                found = cur->Next;

                if (found->Next)
                {
                    found->Next->Prev = found->Prev;

                    if (found->Prev)
                        found->Prev->Next = found->Next;
                }
                else
                    found = NULL;
            }

            cur = cur->Next;
        }

        if (found == NULL)
            return;

        EnterCritical();
        cur->Next = found;
        found->Prev = cur;
        found->Next = NULL;
        LeaveCritical();
    }

    thewin->SetUpdateRect(0, 0, thewin->GetW(), thewin->GetH(), C_BIT_ABSOLUTE, 0);
}

void C_Handler::HelpOff()
{
    if (OverLast_.HelpOn_)
    {
        RefreshAll(&OverLast_.Area_); // Tell windows to refresh this area
    }

    OverLast_.HelpOn_ = 0;
    OverLast_.Control_ = NULL;
    OverLast_.Time_ = 0;
    OverLast_.Tip_ = NULL;
    OverLast_.MouseX_ = 0;
    OverLast_.MouseY_ = 0;
}

void C_Handler::CheckHelpText(SCREEN *surface)
{
    C_Fontmgr *font;

    if (OverLast_.Control_ and OverLast_.Tip_ and GetCurrentTime() > (DWORD)(OverLast_.Time_ + 1000))
    {
        font = gFontList->Find(OverLast_.HelpFont_);

        if ( not OverLast_.HelpOn_ and font)
        {
            OverLast_.HelpOn_ = 1;

            OverLast_.Area_.left = OverLast_.MouseX_ - font->Width(OverLast_.Tip_) / 2;
            OverLast_.Area_.top = OverLast_.MouseY_ - font->Height() - 4;

            if (OverLast_.Area_.left < 0)
                OverLast_.Area_.left = 0;

            if (OverLast_.Area_.top < 0)
                OverLast_.Area_.top = 0;

            OverLast_.Area_.right = OverLast_.Area_.left + font->Width(OverLast_.Tip_) + 4;
            OverLast_.Area_.bottom = OverLast_.Area_.top + font->Height() + 4;

            if (OverLast_.Area_.right > GetW())
            {
                OverLast_.Area_.left -= OverLast_.Area_.right - GetW();
                OverLast_.Area_.right = GetW();
            }

            if (OverLast_.Area_.bottom > GetH())
            {
                OverLast_.Area_.top -= OverLast_.Area_.bottom - GetH();
                OverLast_.Area_.bottom = GetH();
            }
        }

        if (font)
        {
            Fill(surface, 0xffffff, &OverLast_.Area_);
            Fill(surface, 0, OverLast_.Area_.left, OverLast_.Area_.top, OverLast_.Area_.right, OverLast_.Area_.top + 1);
            Fill(surface, 0, OverLast_.Area_.left, OverLast_.Area_.bottom - 1, OverLast_.Area_.right, OverLast_.Area_.bottom);
            Fill(surface, 0, OverLast_.Area_.left, OverLast_.Area_.top, OverLast_.Area_.left + 1, OverLast_.Area_.bottom);
            Fill(surface, 0, OverLast_.Area_.right - 1, OverLast_.Area_.top, OverLast_.Area_.right, OverLast_.Area_.bottom);

            font->Draw(surface, OverLast_.Tip_, 0, OverLast_.Area_.left + 2, OverLast_.Area_.top + 2);

            SetUpdateRect(&OverLast_.Area_);
        }
    }
}

void C_Handler::CheckTranslucentWindows()
{
    WHLIST *cur, *infront;
    UI95_RECT src;
    short i;

    EnterCritical();
    cur = Root_;

    while (cur)
    {
        infront = cur->Next;

        if (cur->Flags bitand C_BIT_ENABLED)
            while (infront)
            {
                if ((infront->win->GetFlags() bitand C_BIT_TRANSLUCENT)
                   and (infront->win->update_ bitand (C_DRAW_COPYWINDOW bitor C_DRAW_REFRESH bitor C_DRAW_REFRESHALL))
                   and (infront->Flags bitand C_BIT_ENABLED))
                {
                    for (i = 0; i < infront->win->rectcount_; i++)
                    {
                        if (infront->win->rectflag_[i])
                        {
                            src = infront->win->rectlist_[i];

                            src.left  += infront->win->GetX() - cur->win->GetX();
                            src.top   += infront->win->GetY() - cur->win->GetY();
                            src.right += infront->win->GetX() - cur->win->GetX();
                            src.bottom += infront->win->GetY() - cur->win->GetY();

                            cur->win->SetUpdateRect(src.left, src.top, src.right, src.bottom, C_BIT_ABSOLUTE, 0);
                        }
                    }
                }

                infront = infront->Next;
            }

        cur = cur->Next;
    }

    LeaveCritical();
}

enum
{
    _CHR_ClipLeft = 0x01,
    _CHR_ClipTop = 0x02,
    _CHR_ClipRight = 0x04,
    _CHR_ClipBottom = 0x08,
    _CHR_RemoveAll = _CHR_ClipLeft bitor _CHR_ClipTop bitor _CHR_ClipRight bitor _CHR_ClipBottom,
};

BOOL C_Handler::ClipRect(UI95_RECT *src, UI95_RECT *dst, UI95_RECT *ClientArea)
{
    long offset;

    if (dst->left < ClientArea->left)
    {
        offset = ClientArea->left - dst->left;
        src->left += offset;
        dst->left += offset;
    }

    if (dst->top < ClientArea->top)
    {
        offset = ClientArea->top - dst->top;
        src->top += offset;
        dst->top += offset;
    }

    if (dst->right > ClientArea->right)
    {
        offset = dst->right - ClientArea->right;
        src->right -= offset;
        dst->right -= offset;
    }

    if (dst->bottom > ClientArea->bottom)
    {
        offset = dst->bottom - ClientArea->bottom;
        src->bottom -= offset;
        dst->bottom -= offset;
    }

    if (dst->left < dst->right and dst->top < dst->bottom)
        return(TRUE); // Draw it

    return(FALSE);
}

void C_Handler::SetUpdateRect(UI95_RECT *upd)
{
    short i;

    if (rectcount_ < 1)
    {
        rectlist_[rectcount_] = *upd;
        rectcount_++;
        UpdateFlag or_eq C_DRAW_COPYWINDOW;
    }
    else if (rectcount_ < HND_MAX_RECTS)
    {
        for (i = 0; i < rectcount_; i++)
        {
            if (rectlist_[i].left <= upd->left and rectlist_[i].top <= upd->top and rectlist_[i].right > upd->right and rectlist_[i].bottom >= upd->bottom)
                return;
        }

        rectlist_[rectcount_].left = upd->left;
        rectlist_[rectcount_].top = upd->top;
        rectlist_[rectcount_].right = upd->right;
        rectlist_[rectcount_].bottom = upd->bottom;
        rectcount_++;
        UpdateFlag or_eq C_DRAW_COPYWINDOW;
    }
    else
    {
        rectcount_ = 1;
        rectlist_[0].left = 0;
        rectlist_[0].top = 0;
        rectlist_[0].right = GetW();
        rectlist_[0].bottom = GetH();
        UpdateFlag or_eq C_DRAW_COPYWINDOW;
    }
}

void C_Handler::RefreshAll(UI95_RECT *updaterect)
{
    WHLIST *cur;
    UI95_RECT rect;

    cur = Root_;

    while (cur)
    {
        if (cur->Flags bitand C_BIT_ENABLED)
        {
            rect = *updaterect;
            rect.left -= cur->win->GetX();
            rect.right -= cur->win->GetX();
            rect.top -= cur->win->GetY();
            rect.bottom -= cur->win->GetY();

            if (rect.left < 0) rect.left = 0;

            if (rect.top < 0) rect.top = 0;

            if (rect.right > 0 and rect.bottom > 0)
                cur->win->SetUpdateRect(rect.left, rect.top, rect.right, rect.bottom, C_BIT_ABSOLUTE, 0);
        }

        cur = cur->Next;
    }
}

void C_Handler::ClearHiddenRects(WHLIST *me)
{
    WHLIST *cur;

    if (me)
    {
        cur = me->Next;

        while (cur)
        {
            if ((cur->Flags bitand C_BIT_ENABLED) and not (cur->win->GetFlags() bitand C_BIT_TRANSLUCENT))
            {
                me->win->ClearUpdateRect(cur->win->GetX() - me->win->GetX(),
                                         cur->win->GetY() - me->win->GetY(),
                                         cur->win->GetX() + cur->win->GetW() - me->win->GetX(),
                                         cur->win->GetY() + cur->win->GetH() - me->win->GetY());
            }

            cur = cur->Next;
        }
    }
}

void C_Handler::ClearAllHiddenRects()
{
    WHLIST *me, *cur;

    me = Root_;

    while (me)
    {
        if (me->Flags bitand C_BIT_ENABLED)
        {
            cur = me->Next;

            while (cur)
            {
                if ((cur->Flags bitand C_BIT_ENABLED) and not (cur->win->GetFlags() bitand C_BIT_TRANSLUCENT))
                {
                    me->win->ClearUpdateRect(cur->win->GetX() - me->win->GetX(),
                                             cur->win->GetY() - me->win->GetY(),
                                             cur->win->GetX() + cur->win->GetW() - me->win->GetX(),
                                             cur->win->GetY() + cur->win->GetH() - me->win->GetY());
                }

                cur = cur->Next;
            }
        }

        me = me->Next;
    }
}

void C_Handler::Fill(SCREEN *surface, COLORREF Color, long x1, long y1, long x2, long y2)
//void C_Handler::Fill(SCREEN *surface,COLORREF Color,short x1,short y1,short x2,short y2)
{
    UI95_RECT dst;

    dst.left = x1;
    dst.top = y1;
    dst.right = x2;
    dst.bottom = y2;

    Fill(surface, Color, &dst);
}

void C_Handler::Fill(SCREEN *surface, COLORREF Color, UI95_RECT *dst)
{
    if (surface->bpp == 32) //XX
    {
        long start, i, len;
        DWORD color;
        DWORD *dest;

        dest = (DWORD*) surface->mem;
        color = RGB565toRGB8(UI95_RGB24Bit(Color));
        i = dst->top;
        len = dst->right - dst->left;

        start = (dst->top * surface->width + dst->left) * sizeof(DWORD);

        while (i < dst->bottom)
        {
            __asm
            {
                mov eax, color
                mov ecx, len
                mov edi, dest
                add edi, start
                rep stosd
            };

            i++;
            start += surface->width * sizeof(DWORD);
        }

    }
    else
    {
        long start, i, len;
        WORD color;
        WORD *dest;

        dest = surface->mem;
        color = UI95_RGB24Bit(Color);
        i = dst->top;
        len = dst->right - dst->left;

        start = (dst->top * surface->width + dst->left) * sizeof(WORD);

        while (i < dst->bottom)
        {
            __asm
            {
                mov AX, color
                mov ECX, len
                mov EDI, dest
                add EDI, start
                rep stosw
            };

            i++;
            start += surface->width * sizeof(WORD);
        }
    }
}

void C_Handler::CheckDrawThrough()
{
    WHLIST *me, *cur;
    UI95_RECT src;
    short i;

    me = Root_;

    while (me)
    {
        if ((me->Flags bitand C_BIT_ENABLED) and (me->win->update_ bitand C_DRAW_REFRESH))
        {
            cur = me->Next;

            while (cur)
            {
                if ((cur->Flags bitand C_BIT_ENABLED) and (cur->win->GetFlags() bitand C_BIT_TRANSLUCENT))
                {
                    for (i = 0; i < me->win->rectcount_; i++)
                    {
                        if (me->win->rectflag_[i])
                        {
                            src = me->win->rectlist_[i];
                            src.left += (me->win->GetX() - cur->win->GetX());
                            src.right += (me->win->GetX() - cur->win->GetX());
                            src.top += (me->win->GetY() - cur->win->GetY());
                            src.bottom += (me->win->GetY() - cur->win->GetY());

                            cur->win->SetUpdateRect(src.left, src.top, src.right, src.bottom, C_BIT_ABSOLUTE, 0);
                        }
                    }
                }

                cur = cur->Next;
            }
        }

        me = me->Next;
    }
}

void C_Handler::Update()
{
    WHLIST *cur;
    //#if 0
    WHLIST *infront;
    //#endif
    UI95_RECT src, dst;
    short i;

    if ( not (UpdateFlag bitand C_DRAW_REFRESH) or not DrawFlags)
        return;

    Lock();

    if ( not surface_.mem)
        return;

    // CheckDrawThrough();
    CheckTranslucentWindows();
    // ClearAllHiddenRects();

    cur = Root_;

    while (cur)
    {
        if ((cur->Flags bitand C_BIT_ENABLED) and (cur->win->update_ bitand C_DRAW_REFRESH))
        {
            if (cur->win->update_ bitand C_DRAW_REFRESH)
            {
                //#if 0
                ClearHiddenRects(cur);
                //#endif
                cur->win->DrawWindow(&surface_);
            }

            if (cur->win->update_ bitand C_DRAW_COPYWINDOW)
            {
                for (i = 0; i < cur->win->rectcount_; i++)
                {
                    if (cur->win->rectflag_[i])
                    {
                        src = cur->win->rectlist_[i];

                        dst = src;
                        dst.left += cur->win->GetX();
                        dst.top += cur->win->GetY();
                        dst.right += cur->win->GetX();
                        dst.bottom += cur->win->GetY();

                        SetUpdateRect(&dst);
                        //#if 0
                        infront = cur->Next;

                        while (infront)
                        {
                            if (infront->Flags bitand C_BIT_ENABLED and (infront->win->GetFlags() bitand C_BIT_TRANSLUCENT))
                                infront->win->SetUpdateRect(dst.left - infront->win->GetX(), dst.top - infront->win->GetY(), dst.right - infront->win->GetX(), dst.bottom - infront->win->GetY(), C_BIT_ABSOLUTE, 0);

                            infront = infront->Next;
                        }

                        //#endif
                    }
                }
            }

            cur->win->rectcount_ = 0;
            cur->win->update_ = 0;
        }

        cur = cur->Next;
    }

    if (OverLast_.Time_)
        CheckHelpText(&surface_);

    if (gScreenShotEnabled and gUI_TakeScreenShot == 1)
    {
        // Copy Front_ surface to a secondary buffer
        int xsize = 800;

        if (g_bHiResUI)
            xsize = 1024;

        //memcpy(gScreenShotBuffer,surface_.mem,surface_.width * surface_.height * sizeof(WORD));
        // MN somehow this D3D stuff for surface_.width creates a width of 1024 for an 800x600 UI...

        if (surface_.bpp == 32)//XX
        {
            for (int i = 0; i < xsize * surface_.height; i++)
                gScreenShotBuffer[i] = RGB8toRGB565(((DWORD*)surface_.mem)[ i ]);
        }
        else
            memcpy(gScreenShotBuffer, surface_.mem, xsize * surface_.height * sizeof(WORD));

        gUI_TakeScreenShot = 2;
    }

    Unlock();
}

void C_Handler::CopyToPrimary()
{
    short i;
    RECT src, dest;

    if ( not DrawFlags)
        return;

    // OW now handled by running in software mode on V1 and V2
#if 0

    if (Primary_ == Front_)
    {
        if (FalconDisplay.displayFullScreen and rectcount_ <= 1)
        {
            Primary_->SwapBuffers(true); // dont flip, it would interfere with our clipping / update mechanics
            rectcount_ = 0;
            UpdateFlag = 0;
            return;
        }
    }

#endif

    // Make sure the drivers isnt buffering any data
    if (g_bCheckBltStatusBeforeFlip)
    {
        while (true)
        {
            HRESULT hres = Primary_->frontSurface()->GetBltStatus(DDGBS_ISBLTDONE);

            if (hres not_eq DDERR_WASSTILLDRAWING)
            {
                break;
            }

            // Let all the other threads have some CPU.
            Sleep(0);
        }
    }

    for (i = 0; i < rectcount_; i++)
    {
        src.left = rectlist_[i].left;
        src.top = rectlist_[i].top;
        src.right = rectlist_[i].right;
        src.bottom = rectlist_[i].bottom;

        dest = src;
        // ImageBuf should handle this...
        // dest.left+=PrimaryRect_.left;
        // dest.right+=PrimaryRect_.left;
        // dest.top+=PrimaryRect_.top;
        // dest.bottom+=PrimaryRect_.top;

        // ShowCursor(FALSE);
        if (src.left < src.right and src.top < src.bottom)
            Primary_->Compose(Front_, &src, &dest);

        // ShowCursor(TRUE);
        // MonoPrint("CopyToPrimary(%1d,%1d,%1d,%1d)\n",dest.left,dest.top,dest.right,dest.bottom);
    }

    rectcount_ = 0;

    UpdateFlag = 0;
    // MonoPrint("HDNLR->CopyToPrimary() Done\n");

    // OW now handled by running in software mode on V1 and V2
#if 0

    if (m_pMouseImage)
    {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(AppWindow_, &pt);

        // center
        pt.x -= m_pMouseImage->targetXres() / 2;
        pt.y -= m_pMouseImage->targetYres() / 2;

        // clip
        if (pt.x < 0) pt.x = 0;
        else if (pt.x > Primary_->targetXres() - m_pMouseImage->targetXres()) pt.x = Primary_->targetXres() - m_pMouseImage->targetXres();

        if (pt.y < 0) pt.y = 0;
        else if (pt.y > Primary_->targetYres() - m_pMouseImage->targetYres()) pt.y = Primary_->targetYres() - m_pMouseImage->targetYres();

        m_rcMouseImage.left = pt.x;
        m_rcMouseImage.top = pt.y;
        m_rcMouseImage.right = m_rcMouseImage.left + m_pMouseImage->targetXres();
        m_rcMouseImage.bottom = m_rcMouseImage.top + m_pMouseImage->targetYres();

        RECT rcSrc = { 0, 0, m_pMouseImage->targetXres(), m_pMouseImage->targetYres() };
        Primary_->ComposeTransparent(m_pMouseImage, &rcSrc, &m_rcMouseImage);

        // refresh next time
        UI95_RECT upme;
        upme.left = m_rcMouseImage.left;
        upme.top = m_rcMouseImage.top;
        upme.right = m_rcMouseImage.right;
        upme.bottom = m_rcMouseImage.bottom;
        SetUpdateRect(&upme);
    }

#endif
}

void C_Handler::UpdateTimerControls(void)
{
    WHLIST *cur;

    cur = Root_;

    while (cur)
    {
        if (cur->Flags bitand C_BIT_ENABLED)
        {
            if (cur->win->UpdateTimerControls())
                cur->win->DrawTimerControls();
        }

        cur = cur->Next;
    }

    if (UpdateFlag bitand (C_DRAW_REFRESH bitor C_DRAW_REFRESHALL))
        SetEvent(WakeOutput_);
}

void C_Handler::EnableGroup(long ID)
{
    WHLIST *cur;

    cur = Root_;

    while (cur)
    {
        cur->win->EnableGroup(ID);
        cur = cur->Next;
    }
}

void C_Handler::DisableGroup(long ID)
{
    WHLIST *cur;

    cur = Root_;

    while (cur)
    {
        cur->win->DisableGroup(ID);
        cur = cur->Next;
    }
}

void C_Handler::EnableWindowGroup(long ID)
{
    WHLIST *cur, *next;

    EnterCritical();
    cur = Root_;

    while (cur)
    {
        if (cur->win->GetGroup() == ID and not (cur->Flags bitand C_BIT_ENABLED))
        {
            ShowWindow(cur->win);

            if ( not (cur->win->GetFlags() bitand C_BIT_CANTMOVE))
            {
                next = cur->Next;
                WindowToFront(cur->win);
                cur->win->SetUpdateRect(0, 0, cur->win->GetW(), cur->win->GetH(), C_BIT_ABSOLUTE, 0);
                cur = next;
                continue;
            }
        }

        cur = cur->Next;
    }

    LeaveCritical();
}

void C_Handler::DisableWindowGroup(long ID)
{
    WHLIST *cur;

    EnterCritical();
    cur = Root_;

    while (cur)
    {
        if (cur->win->GetGroup() == ID and (cur->Flags bitand C_BIT_ENABLED))
            HideWindow(cur->win);

        cur = cur->Next;
    }

    LeaveCritical();
}

void C_Handler::DisableSection(long ID)
{
    WHLIST *cur;

    EnterCritical();
    cur = Root_;

    while (cur)
    {
        if (cur->win->GetSection() == ID and (cur->Flags bitand C_BIT_ENABLED))
            HideWindow(cur->win);

        cur = cur->Next;
    }

    LeaveCritical();
}

BOOL C_Handler::AddUserCallback(void (*cb)())
{
    CBLIST *cur, *newcb;

    if (cb == NULL) return(FALSE);

    cur = UserRoot_;

    while (cur)
    {
        if (cur->Callback == cb)
            return(FALSE);

        cur = cur->Next;
    }

    newcb = new CBLIST;
    newcb->Callback = cb;
    newcb->Next = NULL;

    EnterCritical();

    if (UserRoot_ == NULL)
    {
        UserRoot_ = newcb;
    }
    else
    {
        cur = UserRoot_;

        while (cur->Next)
            cur = cur->Next;

        cur->Next = newcb;
    }

    LeaveCritical();
    return(TRUE);
}

BOOL C_Handler::RemoveUserCallback(void (*cb)())
{
    CBLIST *cur, *last;
    BOOL retval = FALSE;

    if (cb == NULL or UserRoot_ == NULL) return(FALSE);

    EnterCritical();

    if (UserRoot_->Callback == cb)
    {
        SuspendTimer();
        last = UserRoot_;
        UserRoot_ = UserRoot_->Next;
        delete last;
        ResumeTimer();
        retval = TRUE;
    }
    else
    {
        cur = UserRoot_;
        last = UserRoot_;

        while ((cur) and (cur->Next) and ( not retval))
        {
            if (cur->Next->Callback == cb)
            {
                SuspendTimer();
                last = cur->Next;
                cur->Next = cur->Next->Next;
                delete last;
                ResumeTimer();
                retval = TRUE;
            }
            else
                cur = cur->Next;
        }
    }

    LeaveCritical();
    return(retval);
}
C_Window *C_Handler::GetWindow(short x, short y)
{
    WHLIST *cur;
    C_Window *overme;

    overme = NULL;

    cur = Root_;

    while (cur)
    {
        if (cur->Flags bitand C_BIT_ENABLED)
        {
            if (x >= cur->win->GetX() and y >= cur->win->GetY() and 
                x <= (cur->win->GetX() + cur->win->GetW()) and 
                y <= (cur->win->GetY() + cur->win->GetH()))
                overme = cur->win;

            if (cur->win->GetType() == C_TYPE_EXCLUSIVE)
                overme = cur->win;
        }

        cur = cur->Next;
    }

    return(overme);
}

void C_Handler::SetBehindWindow(C_Window *thewin)
{
    long x, y, w, h;
    WHLIST *cur;

    if (thewin == NULL)
        return;

    x = thewin->GetX();
    y = thewin->GetY();
    w = x + thewin->GetW();
    h = y + thewin->GetH();

    cur = Root_;

    while (cur)
    {
        if (cur->win not_eq thewin)
        {
            if (cur->Flags bitand C_BIT_ENABLED)
                cur->win->SetUpdateRect(x - cur->win->GetX(), y - cur->win->GetY(), w - cur->win->GetX(), h - cur->win->GetY(), C_BIT_ABSOLUTE, 0);

            cur = cur->Next;
        }
        else
            cur = NULL;
    }
}

void C_Handler::PostTimerMessage()
{
    while (TimerLoop_ > 0)
    {
        PostMessage(AppWindow_, FM_TIMER_UPDATE, 0, 0);
        // JB 020217 Speed up UI refresh when using a higher time acceleration
        Sleep(TimerSleep_ / min(max(gameCompressionRatio, 1), g_nMaxUIRefresh)); // 2002-02-23 MODIFIED BY S.G. Added the min(... g_nMaxUIRefresh) to prevent the UI to refresh too much and end up running out of resources because it can't keep up
    }

    TimerLoop_ = -1;
}

void C_Handler::DoControlLoop()
{
    while (ControlLoop_ > 0)
    {
        WaitForSingleObject(WakeControl_, ControlSleep_);

        if (ControlLoop_ > 0)
        {
            PostMessage(AppWindow_, C_WM_TIMER, 0, 0);
            EnterCritical();
            UpdateTimerControls();
            LeaveCritical();
        }
    }

    ControlLoop_ = -1;
}

unsigned int __stdcall C_Handler::ControlLoop(void *myself)
{
#ifdef _MSC_VER
    // Set the FPU to Truncate
    _controlfp(_RC_CHOP, MCW_RC);

    // Set the FPU to 24bit precision
    _controlfp(_PC_24, MCW_PC);
#endif
    ((C_Handler *)myself)->DoControlLoop();
    _endthreadex(0);
    return (0);
}

unsigned int __stdcall C_Handler::TimerLoop(void *myself)
{
#ifdef _MSC_VER
    // Set the FPU to Truncate
    _controlfp(_RC_CHOP, MCW_RC);

    // Set the FPU to 24bit precision
    _controlfp(_PC_24, MCW_PC);
#endif
    ((C_Handler *)myself)->PostTimerMessage();
    _endthreadex(0);
    return (0);
}

unsigned int __stdcall C_Handler::OutputLoop(void *myself)
{
#ifdef _MSC_VER
    // Set the FPU to Truncate
    _controlfp(_RC_CHOP, MCW_RC);

    // Set the FPU to 24bit precision
    _controlfp(_PC_24, MCW_PC);
#endif
    ((C_Handler *)myself)->DoOutputLoop();
    _endthreadex(0);
    return (0);
}

void C_Handler::DoOutputLoop()
{
    while (OutputLoop_ > 0)
    {
        WaitForSingleObject(WakeOutput_, 40/*OutputWait_*/);

        if (OutputLoop_ > 0)
        {
            // OW
            DXContext *pCtx = FalconDisplay.theDisplayDevice.GetDefaultRC();
            HRESULT hr = pCtx->TestCooperativeLevel();

            if (FAILED(hr))
                continue;

            if (hr == S_FALSE)
            {
                Primary_->RestoreAll();
                TheTextureBank.RestoreAll();
                TheTerrTextures.RestoreAll();
                TheFarTextures.RestoreAll();

                // Surfaces have been restored
                UI95_RECT upme;
                upme.left = FrontRect_.left;
                upme.top = FrontRect_.top;
                upme.right = FrontRect_.right;
                upme.bottom = FrontRect_.bottom;
                rectcount_ = 0;
                RefreshAll(&upme);
            }

            EnterCritical();

            if (UpdateFlag bitand C_DRAW_REFRESH)
                Update();

            if (UpdateFlag bitand C_DRAW_COPYWINDOW)
                CopyToPrimary();

            LeaveCritical();
        }
    }

    OutputLoop_ = -1;
}

void C_Handler::ProcessUserCallbacks()
{
    CBLIST *cur, *me;

    EnterCritical();
    cur = UserRoot_;

    while (cur)
    {
        me = cur;
        cur = cur->Next;

        if (me->Callback)
            (*me->Callback)();
    }

    LeaveCritical();
}

void C_Handler::StartOutputThread()
{
    if (OutputThread_)
        return;

    WakeOutput_ = CreateEvent(NULL, FALSE, FALSE, "Awaken Output Thread");

    if ( not WakeOutput_)
        return;

    OutputLoop_ = 1;
    OutputThread_ = (HANDLE)_beginthreadex(NULL, 0, OutputLoop, this, 0, &OutputID_);
}

#pragma auto_inline (off)
void C_Handler::EndOutputThread()
{
    if (OutputThread_)
    {
        OutputLoop_ = 0;
        SetEvent(WakeOutput_);

        // Wait for thread to end
        while (OutputLoop_ == 0)
        {
            Sleep(1);
        }

        CloseHandle(OutputThread_);
        OutputThread_ = 0;
    }

    if (WakeOutput_)
    {
        CloseHandle(WakeOutput_);
        WakeOutput_ = NULL;
    }
}
#pragma auto_inline (on)

void C_Handler::SuspendOutput()
{
    EnterCritical();

    if (TimerThread_)
    {
        SuspendThread(OutputThread_);
    }

    LeaveCritical();
}

void C_Handler::ResumeOutput()
{
    EnterCritical();

    if (TimerThread_)
    {
        ResumeThread(OutputThread_);
    }

    LeaveCritical();
}

void C_Handler::StartControlThread(long sleeptime)
{
    if (ControlThread_)
        return;

    WakeControl_ = CreateEvent(NULL, FALSE, FALSE, "Awaken Control Thread");

    if ( not WakeControl_)
    {
        return;
    }

    ControlLoop_ = 1;
    ControlSleep_ = sleeptime;
    ControlThread_ = (HANDLE)_beginthreadex(NULL, 0, ControlLoop, this, 0, &ControlID_);
}

#pragma auto_inline (off)
void C_Handler::EndControlThread()
{
    if (ControlThread_)
    {
        ControlLoop_ = 0;
        SetEvent(WakeControl_);

        // Wait for thread to end
        while (ControlLoop_ == 0)
            Sleep(1);

        CloseHandle(ControlThread_);
        ControlThread_ = 0;
    }

    if (WakeControl_)
    {
        CloseHandle(WakeControl_);
        WakeControl_ = NULL;
    }
}
#pragma auto_inline (on)

void C_Handler::SuspendControl()
{
    EnterCritical();

    if (TimerThread_)
        SuspendThread(ControlThread_);

    LeaveCritical();
}

void C_Handler::ResumeControl()
{
    EnterCritical();

    if (TimerThread_)
        ResumeThread(ControlThread_);

    LeaveCritical();
}

void C_Handler::StartTimerThread(long interval)
{
    if (TimerThread_)
        return;

    TimerLoop_ = 1;
    TimerSleep_ = interval;
    TimerThread_ = (HANDLE)_beginthreadex(NULL, 0, TimerLoop, this, 0, &TimerID_);
}

#pragma auto_inline (off)

void C_Handler::EndTimerThread()
{
    if (TimerThread_)
    {
        TimerLoop_ = 0;

        // Wait for thread to end
        while (TimerLoop_ == 0)
            Sleep(1);

        CloseHandle(TimerThread_);
        TimerThread_ = 0;
    }
}
#pragma auto_inline (on)

void C_Handler::SuspendTimer()
{
    if (TimerThread_)
        SuspendThread(TimerThread_);
}

void C_Handler::ResumeTimer()
{
    if (TimerThread_)
        ResumeThread(TimerThread_);
}

BOOL C_Handler::CheckHotKeys(unsigned char DKScanCode, unsigned char Ascii, unsigned char ShiftStates, long RepeatCount)
{
    WHLIST *cur;

    cur = Root_;

    if (cur == NULL)
        return(FALSE);

    while (cur->Next)
        cur = cur->Next;

    while (cur)
    {
        if (cur->Flags bitand C_BIT_ENABLED)
            if (cur->win->CheckHotKeys(DKScanCode, Ascii, ShiftStates, RepeatCount))
                return(TRUE);

        if (cur->win->GetType() == C_TYPE_EXCLUSIVE)
            cur = NULL;
        else
            cur = cur->Prev;
    }

    return(FALSE);
}

long C_Handler::GetDragX(WORD MouseX) //
{
    long retval = Drag_.ItemX_ + MouseX - Drag_.StartX_;

    return(retval);
}

long C_Handler::GetDragY(WORD MouseY) //
{
    long retval = Drag_.ItemY_ + MouseY - Drag_.StartY_;

    return(retval);
}

// returns FALSE if NOT grabbing a control
BOOL C_Handler::GrabItem(WORD MouseX, WORD MouseY, C_Window *overme, long GrabType)
//not BOOL C_Handler::GrabItem(WORD MouseX,WORD MouseY,C_Window *overme,short GrabType)
{
    Grab_.Control_ = overme->GetControl(&Grab_.ID_, MouseX - overme->GetX(), MouseY - overme->GetY());
    Grab_.StartX_ = MouseX;
    Grab_.StartY_ = MouseY;
    Grab_.GrabType_ = GrabType;

    if (Grab_.Control_)
    {
        Grab_.Window_ = overme;
        Grab_.Control_->GetItemXY(Grab_.ID_, &Grab_.ItemX_, &Grab_.ItemY_);
    }
    else
    {
        if (overme->GetFlags() bitand C_BIT_DRAGABLE)
        {
            if (MouseX < overme->GetX() or MouseY < overme->GetY() or MouseX > (overme->GetX() + overme->GetW()) or MouseY > (overme->GetY() + overme->GetDragH()))
            {
                Grab_.Window_ = NULL;
                return(FALSE);
            }

            Grab_.Window_ = overme;
            Grab_.ItemX_ = overme->GetX();
            Grab_.ItemY_ = overme->GetY();
        }

        return(FALSE);
    }

    return(TRUE);
}

void C_Handler::ReleaseControl(C_Base *control)
{
    if (OverControl_ == control)
        OverControl_ = NULL;

    if (OverLast_.Control_ == control)
        HelpOff();

    if (Grab_.Control_ == control)
        Grab_.Control_ = NULL;

    if (Drag_.Control_ == control)
        Drag_.Control_ = NULL;
}

void C_Handler::StartDrag()
{
    if (Grab_.GrabType_ not_eq C_TYPE_LMOUSEDOWN)
        return;

    if (Grab_.Control_)
    {
        if (Grab_.Control_->Dragable(Grab_.ID_))
        {
            Drag_ = Grab_;
            //ShowCursor(FALSE);
        }
    }
    else if (Grab_.Window_)
    {
        if (Grab_.Window_->GetFlags() bitand C_BIT_DRAGABLE)
        {
            Drag_ = Grab_;
            //ShowCursor(FALSE);
        }
    }
}

BOOL C_Handler::DragItem(WORD MouseX, WORD MouseY, C_Window *overme)
{
    if (Drag_.GrabType_ not_eq C_TYPE_LMOUSEDOWN)
        return(FALSE);

    if (Drag_.Control_)
        return(Drag_.Control_->Drag(&Drag_, MouseX, MouseY, overme));
    else if (Drag_.Window_)
        return(Drag_.Window_->Drag(&Drag_, MouseX, MouseY, overme));

    return(FALSE);
}

BOOL C_Handler::DropItem(WORD MouseX, WORD MouseY, C_Window *overme)
{
    long relX, relY; //
    BOOL retval = FALSE;

    relX = MouseX - overme->GetX();
    relY = MouseY - overme->GetY();

    if (Drag_.Control_)
    {
        retval = Drag_.Control_->Drop(&Drag_, MouseX, MouseY, overme);
        //ShowCursor(TRUE);
    }
    else if (Drag_.Window_)
    {
        retval = Drag_.Window_->Drop(&Drag_, MouseX, MouseY, overme);
        //ShowCursor(TRUE);
    }

    Drag_.Control_ = NULL;
    Drag_.Window_ = NULL;
    Drag_.GrabType_ = 0;
    return(retval);
}

void C_Handler::BlitWindowNow(C_Window *win)
{
    RECT src, dest;

    if (win == NULL)
        return;

    src.left = 0;
    src.top = 0;
    src.right = win->GetW();
    src.bottom = win->GetH();

    dest = src;
    dest.left += win->GetX();
    dest.right += win->GetX();
    dest.top += win->GetY();
    dest.bottom += win->GetY();
    // ImageBuf should handle this...
    // dest.left+=PrimaryRect_.left;
    // dest.right+=PrimaryRect_.left;
    // dest.top+=PrimaryRect_.top;
    // dest.bottom+=PrimaryRect_.top;

    Primary_->Compose(win->GetSurface(), &src, &dest);
}

void C_Handler::PostUpdate()
{
    // UpdateFlag or_eq C_DRAW_UPDATE;
    // PostMessage(AppWindow_,C_WM_UPDATE,0,0);
}

void C_Handler::SendUpdate()
{
    // UpdateFlag or_eq C_DRAW_UPDATE;
    // SendMessage(AppWindow_,C_WM_UPDATE,0,0);
}

// Check to make sure message post time < 1 second for mouse/keyboard input... if not... ignore it :)
BOOL C_Handler::OldInputMessage()
{
    if (GetMessageTime() < EnabledTime_)
        return(TRUE);

    return(FALSE);
}

void C_Handler::DropControl()
{
    Grab_.Control_ = NULL;
    Grab_.Window_ = NULL;
}

void C_Handler::RemovingControl(C_Base *control)
{
    // Warn chandler that a control it might be referncing is being deleted
    if (MouseControl_ == control)
    {
        MouseControl_->Refresh();
        MouseControl_ = NULL;
    }

    if (OverControl_ == control)
        OverControl_ = NULL;

    if (Grab_.Control_ == control)
    {
        Grab_.Control_ = NULL;
        Grab_.Window_ = NULL;
    }

    if (Drag_.Control_ == control)
    {
        Drag_.Control_ = NULL;
        Drag_.Window_ = NULL;
    }

    if (gPopupMgr->AMenuOpened())
    {
        if (gPopupMgr->GetCallingControl() and gPopupMgr->GetCallingControl() ==  control)
            gPopupMgr->CloseMenu();
    }
}

long C_Handler::EventHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    C_Window *overme;
    C_Base   *OldMouseControl_;
    WORD MouseX, MouseY;
    RECT dest;
    long retval = 1;
    BOOL ret = TRUE;
    unsigned char Key;
    unsigned char Ascii;
    unsigned char ShiftStates;
    long Repeat;
    long MessageType, DblClk;
    static int InTimer = 0;

    HandlingMessage = message;

    switch (message)
    {
            // sfr: added mouse wheel
        case WM_MOUSEWHEEL:
            {
                WORD MouseZ;
                HelpOff();

                if (OldInputMessage())
        {
            retval = 0;
            break;
        }

    MessageType = C_TYPE_MOUSEWHEEL;

    // we have to correct mouse position, since this is relative to screen
    // lparam is cursor x/y (low hi)
    POINT p;
    p.x = LOWORD(lParam);
    p.y = HIWORD(lParam);
    ScreenToClient(hwnd, &p);
    MouseX = (WORD)p.x;
    MouseY = (WORD)p.y;
    overme = GetWindow(MouseX, MouseY);

    if (overme == NULL)
    {
        break;
    }

    // so we are rolling wheel inside an active window

    // grab component we are over
    if ( not GrabItem(MouseX, MouseY, overme, MessageType))
    {
        break;
    }

    // wparam is distance rolled/ key modifiers (low hi)
    // here we found a component. process event
    // check hi bit of MouseZ, since word is unsigned
    MouseZ = 1 << ((sizeof(WORD) * 8) - 1);
    MouseZ and_eq HIWORD(wParam);
    // here we invert, since positive in mouse wheel
    // is forward, and forward is up in screen coordinates (neg values)
    Grab_.Control_->Wheel(MouseZ ? 1 : -1, MouseX, MouseY);
    ret = TRUE;
}
    break;

    case WM_LBUTTONDOWN:
            HelpOff();

            if (OldInputMessage())
            {
                retval = 0;
                break;
            }

            MessageType = C_TYPE_LMOUSEDOWN;

            MouseDown_ = C_TYPE_LMOUSEDOWN;
            MouseDownTime_ = GetCurrentTime();
            MouseX = LOWORD(lParam);
            MouseY = HIWORD(lParam);
            overme = GetWindow(MouseX, MouseY);

            if (overme == NULL)
            {
                retval = 0;
                break;
            }

            if (CurWindow_ and CurWindow_ not_eq overme)
            {
                CurWindow_->Deactivate();
                overme->Activate();
            }

            CurWindow_ = overme;

            if (gPopupMgr->AMenuOpened() and not overme->IsMenu())
                gPopupMgr->CloseMenu();

            if (Dragging())
            {
                DropItem(MouseX, MouseY, overme);
            }

            if (GrabItem(MouseX, MouseY, overme, MessageType))
            {
                if (MouseCallback_)
                {
                    ret = (*MouseCallback_)(Grab_.Control_, MouseX, MouseY, overme, (short)MessageType); //
                }
                else
                {
                    ret = TRUE;
                }

                if (ret)
                {
                    overme->SetControl(Grab_.ID_);
                    WindowToFront(overme);
                    Grab_.Control_->Process(Grab_.ID_, (short)MessageType); //
                }
            }
            else
            {
                overme->DeactivateControl();

                if (MouseCallback_)
                    ret = (*MouseCallback_)(NULL, MouseX, MouseY, overme, (short)MessageType); //

                if (ret)
                    WindowToFront(overme);
            }

            retval = 0;
            break;

        case WM_LBUTTONUP:
                if (OldInputMessage())
                {
                    retval = 0;
                    break;
                }

            MouseDown_ = 0;
            MouseX = LOWORD(lParam);
            MouseY = HIWORD(lParam);
            overme = GetWindow(MouseX, MouseY);

            if (overme == NULL)
            {
                retval = 0;
                break;
            }

            if (LastUp_ == C_TYPE_LMOUSEUP and (GetMessageTime() - LastUpTime_) < DoubleClickTime_)
                DblClk = C_TYPE_LMOUSEDBLCLK;
            else
                DblClk = 0;

            MessageType = C_TYPE_LMOUSEUP;
            LastUp_ = MessageType;
            LastUpTime_ = GetMessageTime();

            if (Dragging())
            {
                DropItem(MouseX, MouseY, overme);
                MessageType = C_TYPE_LDROP;
            }

            if (Grab_.Control_)
            {
                if (this not_eq gMainHandler)
                {
                    ret = TRUE;
                }

                if (MouseCallback_ and Grab_.Control_)
                {
                    ret = (*MouseCallback_)(Grab_.Control_, MouseX, MouseY, overme, (short)MessageType); //
                }
                else
                {
                    ret = TRUE;
                }

                if (this not_eq gMainHandler)
                {
                    ret = TRUE;
                }

                if (ret and Grab_.Control_)
                {
                    if (Grab_.Control_->GetFlags() bitand C_BIT_ABSOLUTE)
                    {
                        Grab_.Control_->SetRelXY(
                            MouseX - Grab_.Window_->GetX() - Grab_.Control_->GetX(),
                            MouseY - Grab_.Window_->GetY() - Grab_.Control_->GetY()
                        );
                    }
                    else
                    {
                        Grab_.Control_->SetRelXY(
                            MouseX - Grab_.Window_->GetX() -
                            Grab_.Window_->ClientArea_[Grab_.Control_->GetClient()].left -
                            Grab_.Control_->GetX(),
                            MouseY - Grab_.Window_->GetY() -
                            Grab_.Window_->ClientArea_[Grab_.Control_->GetClient()].top -
                            Grab_.Control_->GetY()
                        );
                    }

                    Grab_.Control_->Process(Grab_.ID_, (short)MessageType); //

                    if (DblClk and Grab_.Control_)
                    {
                        Grab_.Control_->Process(Grab_.ID_, (short)DblClk); //
                    }
                }

                if (this not_eq gMainHandler)
                {
                    ret = TRUE;
                }

                Grab_.Control_ = NULL;
                Grab_.Window_ = NULL;
            }
            else
            {
                overme->DeactivateControl();

                if (MouseCallback_)
                {
                    ret = (*MouseCallback_)(NULL, MouseX, MouseY, overme, (short)MessageType); //
                }

                if (ret)
                {
                    WindowToFront(overme);
                }

                if (MouseX < overme->GetX() or MouseY < overme->GetY() or MouseX > (overme->GetX() + overme->GetW()) or MouseY > (overme->GetY() + overme->GetH()))
                {
                    if (overme->GetOwner())
                    {
                        if (overme->GetOwner()->CloseWindow())
                        {
                            overme = NULL;
                        }
                    }
                }

                Grab_.Window_ = NULL;
            }

            overme = GetWindow(MouseX, MouseY);

            if (overme)
            {
                OverControl_ = overme->MouseOver(MouseX - overme->GetX(), MouseY - overme->GetY(), OverControl_);

                if (OverLast_.Control_ not_eq OverControl_)
                {
                    HelpOff();
                    OverLast_.Control_ = OverControl_;

                    if (OverControl_)
                    {
                        OverLast_.Time_ = GetCurrentTime();
                        OverLast_.Tip_ = gStringMgr->GetString(OverControl_->GetHelpText());
                        OverLast_.MouseX_ = MouseX;
                        OverLast_.MouseY_ = MouseY;
                    }
                }
            }

            retval = 0;
            break;

        case WM_LBUTTONDBLCLK:
                break;
            HelpOff();

            if (OldInputMessage())
            {
                retval = 0;
                break;
            }

            MouseX = LOWORD(lParam);
            MouseY = HIWORD(lParam);
            overme = GetWindow(MouseX, MouseY);

            if (overme == NULL)
            {
                retval = 0;
                break;
            }

            CurWindow_ = overme;

            if (Dragging())
                DropItem(MouseX, MouseY, overme);

            if (GrabItem(MouseX, MouseY, overme, C_TYPE_LMOUSEDBLCLK))
            {
                if (MouseCallback_)
                    ret = (*MouseCallback_)(Grab_.Control_, MouseX, MouseY, overme, C_TYPE_LMOUSEDBLCLK);
                else
                    ret = TRUE;

                if (ret)
                {
                    overme->SetControl(Grab_.ID_);
                    WindowToFront(overme);
                    Grab_.Control_->Process(Grab_.ID_, C_TYPE_LMOUSEDBLCLK);
                }
            }
            else
            {
                if (MouseCallback_)
                    ret = (*MouseCallback_)(NULL, MouseX, MouseY, overme, C_TYPE_LMOUSEDBLCLK);
                else
                    ret = TRUE;

                if (ret)
                    WindowToFront(overme);

                Grab_.Window_ = NULL;
            }

            retval = 0;
            break;

        case WM_MOUSEMOVE:
                HelpOff();

            if (OldInputMessage())
            {
                retval = 0;
                break;
            }

            MessageType = C_TYPE_MOUSEMOVE;
            LastUp_ = 0;
            LastUpTime_ = 0;
            OldMouseControl_ = MouseControl_;
            MouseX = LOWORD(lParam);
            MouseY = HIWORD(lParam);
            overme = GetWindow(MouseX, MouseY);

            if (overme == NULL)
            {
                if (OldMouseControl_ and OldMouseControl_->GetFlags() bitand C_BIT_MOUSEOVER)
                {
                    OldMouseControl_->SetMouseOver(0);
                    OldMouseControl_->Refresh();
                }

                retval = 0;
                break;
            }

            if ( not Dragging() and (Grab_.Control_ or Grab_.Window_) and Grab_.GrabType_ == C_TYPE_LMOUSEDOWN)
                StartDrag();

            if (Dragging())
            {
                if (DragItem(MouseX, MouseY, overme) and Drag_.Control_)
                    Drag_.Window_->RefreshWindow();

                OverControl_ = NULL;

                if (Drag_.Control_)
                {
                    if (Drag_.Control_->GetDragCursorID())
                        SetCursor(gCursors[Drag_.Control_->GetDragCursorID()]);
                    else if (Drag_.Control_->GetCursorID())
                        SetCursor(gCursors[Drag_.Control_->GetCursorID()]);
                    else
                        SetCursor(gCursors[overme->GetCursorID()]);
                }
                else
                    SetCursor(gCursors[overme->GetCursorID()]);
            }
            else if (MouseDown_)
            {
                if (Grab_.Control_ and Grab_.Window_)
                {
                    if (Grab_.Control_->GetFlags() bitand C_BIT_ABSOLUTE)
                        Grab_.Control_->SetRelXY(MouseX - Grab_.Window_->GetX() - Grab_.Control_->GetX(), MouseY - Grab_.Window_->GetY() - Grab_.Control_->GetY());
                    else
                        Grab_.Control_->SetRelXY(MouseX - Grab_.Window_->GetX()
                                                 - Grab_.Window_->ClientArea_[Grab_.Control_->GetClient()].left
                                                 - Grab_.Control_->GetX(),
                                                 MouseY - Grab_.Window_->GetY()
                                                 - Grab_.Window_->ClientArea_[Grab_.Control_->GetClient()].top
                                                 - Grab_.Control_->GetY());
                }

                OverControl_ = NULL;
            }
            else
            {
                MouseControl_ = overme->MouseOver(MouseX - overme->GetX(), MouseY - overme->GetY(), OldMouseControl_);

                if (OldMouseControl_ and MouseControl_ not_eq OldMouseControl_ and OldMouseControl_->GetFlags() bitand C_BIT_MOUSEOVER)
                {
                    OldMouseControl_->SetMouseOver(0);
                    OldMouseControl_->Refresh();
                }

                if (MouseControl_ and MouseControl_ not_eq OldMouseControl_ and MouseControl_->GetFlags() bitand C_BIT_MOUSEOVER)
                {
                    MouseControl_->SetMouseOver(1);
                    MouseControl_->Refresh();
                }

                OverControl_ = MouseControl_;

                if (OverControl_)
                {
                    if (OverControl_->GetCursorID())
                        SetCursor(gCursors[OverControl_->GetCursorID()]);
                    else
                        SetCursor(gCursors[overme->GetCursorID()]);
                }
                else
                {
                    SetCursor(gCursors[overme->GetCursorID()]);
                }
            }

            if (OverLast_.Control_ not_eq OverControl_)
            {
                HelpOff();
                OverLast_.Control_ = OverControl_;

                if (OverControl_)
                {
                    OverLast_.Time_ = GetCurrentTime();
                    OverLast_.Tip_ = gStringMgr->GetString(OverControl_->GetHelpText());
                    OverLast_.MouseX_ = MouseX;
                    OverLast_.MouseY_ = MouseY;
                }
            }

            if (MouseCallback_)
                (*MouseCallback_)(NULL, MouseX, MouseY, overme, (short)MessageType); //

            retval = 0;
            break;

        case WM_RBUTTONDOWN:
                HelpOff();

            if (OldInputMessage())
            {
                retval = 0;
                break;
            }

            MessageType = C_TYPE_RMOUSEDOWN;

            MouseX = LOWORD(lParam);
            MouseY = HIWORD(lParam);
            overme = GetWindow(MouseX, MouseY);

            if (overme == NULL)
            {
                retval = 0;
                break;
            }

            if (CurWindow_ and CurWindow_ not_eq overme)
            {
                CurWindow_->Deactivate();
                overme->Activate();
            }

            CurWindow_ = overme;

            if (Dragging())
                DropItem(MouseX, MouseY, overme);

            if (GrabItem(MouseX, MouseY, overme, MessageType))
            {
                if (MouseCallback_)
                    ret = (*MouseCallback_)(Grab_.Control_, MouseX, MouseY, overme, (short)MessageType); //
                else
                    ret = TRUE;

                if (ret)
                {
                    overme->SetControl(Grab_.ID_);
                    WindowToFront(overme);
                    Grab_.Control_->Process(Grab_.ID_, (short)MessageType); //
                }
            }
            else
            {
                overme->DeactivateControl();

                if (MouseCallback_)
                    ret = (*MouseCallback_)(NULL, MouseX, MouseY, overme, (short)MessageType); //
                else
                    ret = TRUE;

                // if(ret)
                // WindowToFront(overme);
                Grab_.Window_ = NULL;
            }

            retval = 0;
            break;

        case WM_RBUTTONUP:
                if (OldInputMessage())
                {
                    retval = 0;
                    break;
                }

            if (LastUp_ == C_TYPE_RMOUSEUP and (GetMessageTime() - LastUpTime_) < DoubleClickTime_)
                DblClk = C_TYPE_RMOUSEDBLCLK;
            else
                DblClk = 0;

            MessageType = C_TYPE_RMOUSEUP;
            LastUp_ = MessageType;
            LastUpTime_ = GetMessageTime();

            MouseDown_ = 0;
            MouseX = LOWORD(lParam);
            MouseY = HIWORD(lParam);
            overme = GetWindow(MouseX, MouseY);

            if (overme == NULL)
            {
                retval = 0;
                break;
            }

            if (Dragging())
            {
                DropItem(MouseX, MouseY, overme);
            }
            else if (Grab_.Control_)
            {
                if (MouseCallback_)
                    ret = (*MouseCallback_)(Grab_.Control_, MouseX, MouseY, overme, (short)MessageType); //
                else
                    ret = TRUE;

                if (ret and overme->IsMenu())
                {
                    Grab_.Control_->Process(Grab_.ID_, (short)MessageType); //

                    if (DblClk and Grab_.Control_)
                        Grab_.Control_->Process(Grab_.ID_, (short)DblClk); //

                    Grab_.Control_ = NULL;
                }
                else if (UI_ABS(MouseX - Grab_.StartX_) < 3 and UI_ABS(MouseY - Grab_.StartY_) < 3)
                    gPopupMgr->OpenMenu(Grab_.Control_->GetMenu(), MouseX, MouseY, Grab_.Control_);
            }
            else
            {
                overme->DeactivateControl();

                if (MouseCallback_)
                    ret = (*MouseCallback_)(NULL, MouseX, MouseY, overme, (short)MessageType); //

                gPopupMgr->OpenWindowMenu(overme, MouseX, MouseY);
            }

            overme = GetWindow(MouseX, MouseY);

            if (overme)
            {
                OverControl_ = overme->MouseOver(MouseX - overme->GetX(), MouseY - overme->GetY(), OverControl_);

                if (OverLast_.Control_ not_eq OverControl_)
                {
                    HelpOff();
                    OverLast_.Control_ = OverControl_;

                    if (OverControl_)
                    {
                        OverLast_.Time_ = GetCurrentTime();
                        OverLast_.Tip_ = gStringMgr->GetString(OverControl_->GetHelpText());
                        OverLast_.MouseX_ = MouseX;
                        OverLast_.MouseY_ = MouseY;
                    }
                }
            }

            retval = 0;
            break;

        case WM_RBUTTONDBLCLK:
                break;
            HelpOff();

            if (OldInputMessage())
            {
                retval = 0;
                break;
            }

            MouseX = LOWORD(lParam);
            MouseY = HIWORD(lParam);
            overme = GetWindow(MouseX, MouseY);

            if (overme == NULL)
            {
                retval = 0;
                break;
            }

            CurWindow_ = overme;

            if (Dragging())
                DropItem(MouseX, MouseY, overme);

            if (GrabItem(MouseX, MouseY, overme, C_TYPE_RMOUSEDBLCLK))
            {
                if (MouseCallback_)
                    ret = (*MouseCallback_)(Grab_.Control_, MouseX, MouseY, overme, C_TYPE_RMOUSEDBLCLK);
                else
                    ret = TRUE;

                if (ret)
                {
                    overme->SetControl(Grab_.ID_);
                    WindowToFront(overme);
                    Grab_.Control_->Process(Grab_.ID_, C_TYPE_RMOUSEDBLCLK);
                }
            }
            else
            {
                if (MouseCallback_)
                    ret = (*MouseCallback_)(NULL, MouseX, MouseY, overme, C_TYPE_RMOUSEDBLCLK);

                if (ret)
                    WindowToFront(overme);

                Grab_.Window_ = NULL;
            }

            retval = 0;
            break;

        case C_WM_TIMER:
                if (MouseDown_ and (GetCurrentTime() - MouseDownTime_) > 250)
                {

                    if (Grab_.Control_ and not InTimer)
                    {
                        if (GetAsyncKeyState(VK_LBUTTON))
                        {
                            InTimer = 1;
                            Grab_.Control_->Process(Grab_.ID_, C_TYPE_REPEAT);
                            InTimer = 0;
                        }
                        else
                        {
                            InTimer = 1;
                            MouseDown_ = 0;

                            if (Drag_.Window_ not_eq NULL and Drag_.Control_ not_eq NULL)
                            {
                                DropItem((WORD)(Drag_.Window_->GetX() + Drag_.Control_->GetX()), //
                                         (WORD)(Drag_.Window_->GetY() + Drag_.Control_->GetY()), //
                                         Drag_.Window_);
                                Grab_.Control_->Process(Grab_.ID_, C_TYPE_LDROP);
                            }
                            else
                            {
                                Grab_.Control_->Process(Grab_.ID_, C_TYPE_LMOUSEUP);
                            }

                            Grab_.Control_ = NULL;
                            InTimer = 0;
                        }
                    }
                }

            if (gScreenShotEnabled and gUI_TakeScreenShot == 2)
            {
                SaveScreenShot();
                gUI_TakeScreenShot = 0;
            }

            break;

        case WM_SYSKEYUP:
            case WM_KEYUP:
                    Transmit(0);// voice stuff me123

            if (wParam == VK_SNAPSHOT) // fall through to KEYDOWN also
            {
                if (gScreenShotEnabled)
                    gUI_TakeScreenShot = 1; // Set to take screen shot after screen is refreshed (2=Save to file)...

                lParam = (lParam bitand 0xff00ffff) bitor DIK_SYSRQ;
            }
            else

                break;

        case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
                    ShiftStates = 0;

            if (OldInputMessage())
            {
                retval = 0;
                break;
            }

            Repeat = lParam bitand 0xffff;

            Key = (uchar)(((lParam >> 16) bitand 0xff) bitor ((lParam >> 17) bitand 0x80)); //

            if (GetKeyState(VK_SHIFT) bitand 0x80)
                ShiftStates or_eq _SHIFT_DOWN_;

            if (GetKeyState(VK_MENU) bitand 0x80)
                ShiftStates or_eq _ALT_DOWN_;

            if (GetKeyState(VK_CONTROL) bitand 0x80)
                ShiftStates or_eq _CTRL_DOWN_;

            if (GetKeyState(VK_CAPITAL) bitand 0x01)
                if ((Key >= DIK_Q and Key <= DIK_P) or (Key >= DIK_A and Key <= DIK_L) or (Key >= DIK_Z and Key <= DIK_M))
                    ShiftStates xor_eq _SHIFT_DOWN_;

            if (GetKeyState(VK_NUMLOCK) bitand 0x01)
                if ((Key >= DIK_NUMPAD7 and Key <= DIK_NUMPAD9) or (Key >= DIK_NUMPAD4 and Key <= DIK_NUMPAD6) or (Key >= DIK_NUMPAD1 and Key <= DIK_DECIMAL))
                    ShiftStates or_eq _SHIFT_DOWN_;

            Ascii = AsciiChar(Key, ShiftStates);

            // Handle Hot Keys bitand Keyboard input
            if (CurWindow_)
            {
                if ( not CurWindow_->CheckKeyboard(Key, Ascii, ShiftStates, Repeat))
                    CheckHotKeys(Key, Ascii, ShiftStates, Repeat);
            }
            else
                CheckHotKeys(Key, Ascii, ShiftStates, Repeat);

            retval = 0;

            ////// me123 voice stuff added
            if (Key == DIK_F1)
                Transmit(1);
            else if (Key == DIK_F2)
                Transmit(2);

            /////////////////
            break;

            /* case WM_CHAR: // NOLONGER USED
             retval=0;
             if(OldInputMessage())
             break;
             // Handle Hot Keys bitand Keyboard input
             if(CurWindow_ == NULL)
             break;

             if( not CurWindow_->CheckKeyboard(message,wParam,lParam))
             CheckHotKeys(message,wParam,lParam);
             retval=0;
             break;
            */
        case C_WM_UPDATE:
                //if(UpdateFlag bitand C_DRAW_REFRESH)
                // SetEvent(WakeOutput_);
                retval = 0;
            break;

        case WM_MOVE:

                // get the client rectangle
                if (FalconDisplay.displayFullScreen)
                {
                    SetRect(&dest, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
                }
                else
                {
                    GetClientRect(hwnd, &dest);
                    ClientToScreen(hwnd, (LPPOINT)&dest);
                    ClientToScreen(hwnd, (LPPOINT)&dest + 1);

                    if (Primary_ and Primary_->frontSurface())
                    {
                        Primary_->UpdateFrontWindowRect(&dest);
#if 0 // Don't know how to get rc

                        if (rc)
                            MPRSwapBuffers(rc, fronthandle);

#endif
                    }

                    InvalidateRect(hwnd, &dest, FALSE);
                }

            break;

        case WM_PAINT:
                if (GetUpdateRect(hwnd, &dest, FALSE))
                {
                    ValidateRect(hwnd, NULL);

                    if (Primary_ not_eq Front_)
                    {
                        UI95_RECT upme;

                        upme.left = FrontRect_.left;
                        upme.top = FrontRect_.top;
                        upme.right = FrontRect_.right;
                        upme.bottom = FrontRect_.bottom;

                        EnterCritical();
                        rectcount_ = 0;
                        RefreshAll(&upme);
                        LeaveCritical();
                        //if(UpdateFlag bitand C_DRAW_REFRESH)
                        // SetEvent(WakeOutput_);
                    }
                }

            retval = 0;
            break;
    }

    HandlingMessage = 0;
    return(retval);
}
