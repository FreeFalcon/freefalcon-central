#include <windows.h>
#include "chandler.h"

C_PopupMgr *gPopupMgr = NULL;

C_PopupMgr::C_PopupMgr()
{
    Root_ = NULL;
    Current_ = NULL;
    CurrentType_ = 0;
    CurrentClient_ = 0;
    Handler_ = NULL;
    Flags_ = 0;
    ready_ = 0;
    LastX_ = 0;
    LastY_ = 0;
}

C_PopupMgr::~C_PopupMgr()
{
    if (ready_)
        Cleanup();
}

void C_PopupMgr::Setup(C_Handler *handler)
{
    Handler_ = handler;
    ready_ = 1;
}

void C_PopupMgr::Cleanup()
{
    POPUPMENU *cur, *prev;
    Handler_ = NULL;
    Current_ = NULL;

    if (Root_)
    {
        cur = Root_;

        while (cur)
        {
            prev = cur;
            cur = cur->Next;

            if (prev->Menu)
            {
                prev->Menu->Cleanup();
                delete prev->Menu;
            }

            delete prev;
        }

        Root_ = NULL;
    }

    ready_ = 0;
}

void C_PopupMgr::AddMenu(C_PopupList *newmenu)
{
    POPUPMENU *cur, *nm;

    cur = Root_;

    while (cur)
    {
        if (cur->Menu == newmenu)
            return;

        cur = cur->Next;
    }

    nm = new POPUPMENU;
    nm->Menu = newmenu;
    nm->Next = NULL;

    if (Root_ == NULL)
        Root_ = nm;
    else
    {
        cur = Root_;

        while (cur->Next)
            cur = cur->Next;

        cur->Next = nm;
    }
}

void C_PopupMgr::RemoveMenu(long ID)
{
    POPUPMENU *cur, *prev;

    if (Root_ == NULL)
        return;

    if (Root_->Menu->GetID() == ID)
    {
        prev = Root_;
        Root_ = Root_->Next;
        prev->Menu->Cleanup();
        delete prev->Menu;
        delete prev;
    }
    else
    {
        cur = Root_;

        while (cur->Next)
        {
            if (cur->Next->Menu->GetID() == ID)
            {
                prev = cur->Next;
                cur->Next = cur->Next->Next;
                prev->Menu->Cleanup();
                delete prev->Menu;
                delete prev;
                cur = NULL;
            }
            else
                cur = cur->Next;
        }
    }
}

BOOL C_PopupMgr::OpenMenu(long ID, long x, long y, C_Base *control)
{
    POPUPMENU *cur;

    if ( not ID) return(FALSE);

    CloseMenu();

    if ( not AMenuOpened())
    {
        cur = Root_;

        while (cur)
        {
            if (cur->Menu->GetID() == ID)
            {
                Current_ = cur;
                cur = NULL;
            }
            else
                cur = cur->Next;
        }

        if (Current_)
        {
            CurrentType_ = C_TYPE_CONTROL;
            Control_ = control;

            if (Current_->Menu->OpenCallback_)
                (*Current_->Menu->OpenCallback_)(Current_->Menu, control);

            Current_->Menu->OpenWindow(static_cast<short>(x - 20), static_cast<short>(y - 5), C_TYPE_RIGHT); 

            LastX_ = static_cast<short>(x);
            LastY_ = static_cast<short>(y);
            return(TRUE);
        }
    }

    return(FALSE);
}

BOOL C_PopupMgr::OpenWindowMenu(C_Window *win, long x, long y)
{
    POPUPMENU *cur;
    long i;

    if ( not win) return(FALSE);

    if (win->IsMenu())
        return(FALSE);

    CloseMenu();

    if ( not AMenuOpened())
    {
        for (i = 0; i < WIN_MAX_CLIENTS; i++)
        {
            if ((x - win->GetX()) >= win->ClientArea_[i].left and (y - win->GetY()) >= win->ClientArea_[i].top and 
                (x - win->GetX()) < win->ClientArea_[i].right and (y - win->GetY()) < win->ClientArea_[i].bottom and 
                win->GetClientMenu(i) and (win->GetClientFlags(i) bitand C_BIT_ENABLED))
            {
                cur = Root_;

                while (cur)
                {
                    if (cur->Menu->GetID() == win->GetClientMenu(i))
                    {
                        Current_ = cur;
                        Current_->Menu->OpenWindow(static_cast<short>(x - 20), static_cast<short>(y - 5), C_TYPE_RIGHT); 
                        CurrentType_ = C_TYPE_WINDOW;
                        CurrentClient_ = static_cast<short>(i); 
                        LastX_ = static_cast<short>(x); 
                        LastY_ = static_cast<short>(y); 
                        return(TRUE);
                    }

                    cur = cur->Next;
                }
            }
        }

        cur = Root_;

        while (cur)
        {
            if (cur->Menu->GetID() == win->GetMenu())
            {
                CurrentType_ = C_TYPE_WINDOW;
                CurrentClient_ = -1;
                Current_ = cur;
                Current_->Menu->OpenWindow(static_cast<short>(x - 20), static_cast<short>(y - 5), C_TYPE_RIGHT); 
                LastX_ = static_cast<short>(x); 
                LastY_ = static_cast<short>(y); 
                return(TRUE);
            }

            cur = cur->Next;
        }
    }

    return(FALSE);
}

BOOL C_PopupMgr::AMenuOpened()
{
    if (Current_ == NULL) return(FALSE);

    if (Current_->Menu == NULL) return(FALSE);

    if (Current_->Menu->GetWindow() == NULL)
    {
        Current_ = NULL;
        return(FALSE);
    }

    return(TRUE);
}

BOOL C_PopupMgr::Opened(long ID)
{
    if (Current_)
        if (Current_->Menu->GetID() == ID)
            return(TRUE);

    return(FALSE);
}

void C_PopupMgr::CloseMenu()
{
    if (Current_)
    {
        Current_->Menu->CloseSubMenus();
        Current_->Menu->CloseWindow();
    }

    Current_ = NULL;
}

C_PopupList *C_PopupMgr::GetMenu(long ID)
{
    POPUPMENU *cur;

    cur = Root_;

    while (cur)
    {
        if (cur->Menu->GetID() == ID)
            return(cur->Menu);

        cur = cur->Next;
    }

    return(NULL);
}

