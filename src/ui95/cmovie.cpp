#include <windows.h>
#include <ddraw.h>
#include "debuggr.h"
#include "dxutil/ddutil.h"
#include "ui/include/targa.h"
#include "chandler.h"

#ifdef _UI95_PARSER_

enum
{
    CMOV_NOTHING = 0,
    CMOV_ADDMOVIE,
    CMOV_ADDSUBTITLE,
};

char *C_Mov_Tokens[] =
{
    "[NOTHING]",
    "[MOVIE]",
    "[SUBTITLE]",
    0,
};

#endif // PARSER

extern C_Handler *gMainHandler;
C_Movie *gMovieMgr = NULL;

void PlayMovie(char *filename, int left, int top, int w, int h, void *surface);

C_Movie::C_Movie()
{
    x_ = 0;
    y_ = 0;
    Root_ = NULL;
    flags_ = _NO_FLAGS_;
}

C_Movie::~C_Movie()
{
    if (Root_)
        Cleanup();
}

void C_Movie::Setup()
{
    if (Root_)
        Cleanup();

    Root_ = NULL;
}

void C_Movie::Cleanup()
{
    MOVIE_RES *cur, *prev;

    cur = Root_;

    while (cur)
    {
        prev = cur;
        cur = cur->Next;
        delete prev;
    }

    Root_ = NULL;
}

BOOL C_Movie::AddMovie(long ID, char *fname)
{
    MOVIE_RES *newentry, *cur;

    if (fname == NULL)
        return(FALSE);

    if (GetMovie(ID))
    {
        //MonoPrint("[ID %1ld] Already used (AddImage)\n",ID);
        return(FALSE);
    }

    newentry = new MOVIE_RES;
    newentry->ID = ID;
    _tcscpy(newentry->Movie, fname);
    newentry->SubTitle[0] = 0;
    newentry->Next = NULL;

    if (Root_ == NULL)
    {
        Root_ = newentry;
        return(TRUE);
    }
    else
    {
        cur = Root_;

        while (cur->Next)
            cur = cur->Next;

        cur->Next = newentry;
        return(TRUE);
    }

    return(FALSE);
}

BOOL C_Movie::AddSubTitle(long ID, char *fname)
{
    MOVIE_RES *cur;

    if (fname == NULL)
        return(FALSE);

    cur = GetMovie(ID);

    if (cur)
    {
        _tcscpy(cur->SubTitle, fname);
        return(TRUE);
    }

    return(FALSE);
}

MOVIE_RES *C_Movie::GetMovie(long ID)
{
    MOVIE_RES *cur;

    cur = Root_;

    while (cur)
    {
        if (cur->ID == ID)
            return(cur);

        cur = cur->Next;
    }

    return(NULL);
}

BOOL C_Movie::RemoveMovie(long ID)
{
    MOVIE_RES *cur, *prev;

    if (Root_ == NULL)
        return(FALSE);

    if (Root_->ID == ID)
    {
        cur = Root_;
        Root_ = Root_->Next;
        delete cur;
        return(TRUE);
    }
    else
    {
        cur = Root_;

        while (cur->Next)
        {
            if (cur->Next->ID == ID)
            {
                prev = cur->Next;
                cur->Next = prev->Next;
                delete prev;
            }

            cur = cur->Next;
        }

        return(TRUE);
    }

    return(FALSE);
}

BOOL C_Movie::Play(long ID)
{
    MOVIE_RES *cur;

    cur = GetMovie(ID);

    if (cur)
    {
        PlayMovie(cur->Movie, x_, y_, 0, 0, gMainHandler->GetPrimary()->frontSurface());
        return(TRUE);
    }

    return(FALSE);
}

#ifdef _UI95_PARSER_
short C_Movie::LocalFind(char *token)
{
    short i = 0;

    while (C_Mov_Tokens[i])
    {
        if (strnicmp(token, C_Mov_Tokens[i], strlen(C_Mov_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Movie::LocalFunction(short ID, long P[], _TCHAR *str, C_Handler *)
{
    switch (ID)
    {
        case CMOV_ADDMOVIE:
            AddMovie(P[0], str);
            break;

        case CMOV_ADDSUBTITLE:
            AddSubTitle(P[0], str);
            break;
    }
}

#endif // PARSER
