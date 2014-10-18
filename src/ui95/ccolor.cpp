#include <windows.h>
#include "chandler.h"
#include "ccolor.h"

static COLORSCHEME ColorScheme_0 =
{
    RGB(0, 170, 0), // Title Text
    RGB(26, 65, 83), // Title background
    RGB(156, 209, 229), // Border Lite color
    RGB(97, 159, 186), // Border med color
    RGB(26, 65, 83),  // Border dark color
    RGB(62, 115, 139),   // Window client area background color
    RGB(0, 0, 0), // Window color for selected item backgrounds
    RGB(0, 0, 0), // Normal Text color
    RGB(255, 255, 255), // Reverse Text color
    RGB(100, 100, 100), // Disabled Text
};

C_ColorMgr *gColorMgr = NULL;

C_ColorMgr::C_ColorMgr()
{
    Root_ = NULL;
}

C_ColorMgr::~C_ColorMgr()
{
}

void C_ColorMgr::Setup()
{
    AddScheme(0, &ColorScheme_0);
}

void C_ColorMgr::Cleanup()
{
    COLORLIST *cur, *prev;

    cur = Root_;

    while (cur)
    {
        prev = cur;
        cur = cur->Next;
        delete prev;
    }

    Root_ = NULL;
}

void C_ColorMgr::AddScheme(long ID, COLORSCHEME *newscheme)
{
    COLORLIST *newcolor, *cur;

    if (FindScheme(ID))
        return;

    newcolor = new COLORLIST;
    newcolor->ID = ID;
    memcpy(&newcolor->Color, newscheme, sizeof(COLORSCHEME));
    newcolor->Next = NULL;

    if (Root_ == NULL)
        Root_ = newcolor;
    else
    {
        cur = Root_;

        while (cur->Next)
            cur = cur->Next;

        cur->Next = newcolor;
    }
}

COLORSCHEME *C_ColorMgr::GetScheme(long ID)
{
    COLORLIST *cur;

    cur = Root_;

    while (cur)
    {
        if (cur->ID == ID)
            return(&cur->Color);

        cur = cur->Next;
    }

    return(&Root_->Color);
}

BOOL C_ColorMgr::FindScheme(long ID)
{
    COLORLIST *cur;

    cur = Root_;

    while (cur)
    {
        if (cur->ID == ID)
            return(TRUE);

        cur = cur->Next;
    }

    return(FALSE);
}
