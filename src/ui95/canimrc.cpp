#include <windows.h>
#include "falclib.h"
#include "chandler.h"
// ALL RESMGR CODE ADDITIONS START HERE
#define _USE_RES_MGR_ 1

#ifndef _USE_RES_MGR_ // DON'T USE RESMGR

#define UI_HANDLE FILE *
#define UI_OPEN   fopen
#define UI_READ   fread
#define UI_CLOSE  fclose
#define UI_SEEK   fseek
#define UI_TELL   ftell

#else // USE RESMGR

#include "campaign/include/cmpclass.h"
extern "C"
{
#include "codelib/resources/reslib/src/resmgr.h"
}

#define UI_HANDLE FILE *
#define UI_OPEN   RES_FOPEN
#define UI_READ   RES_FREAD
#define UI_CLOSE  RES_FCLOSE
#define UI_SEEK   RES_FSEEK
#define UI_TELL   RES_FTELL

#endif

long UI_FILESIZE(UI_HANDLE fp);

#ifdef _UI95_PARSER_

enum
{
    CANM_NOTHING = 0,
    CANM_LOADANIM,
};

char *C_Anim_Tokens[] =
{
    "[NOTHING]",
    "[LOADANIM]",
    0,
};

#endif // PARSER

C_Animation *gAnimMgr = NULL;

C_Animation::C_Animation()
{
    Root_ = NULL;
}

C_Animation::~C_Animation()
{
    if (Root_)
        Cleanup();
}

void C_Animation::Setup()
{
    if (Root_)
        Cleanup();

    Root_ = NULL;
}

void C_Animation::Cleanup()
{
    ANIM_RES *cur, *prev;

    cur = Root_;

    while (cur)
    {
        prev = cur;
        cur = cur->Next;
        delete prev->Anim;
        delete prev;
    }

    Root_ = NULL;
}

ANIM_RES *C_Animation::LoadAnim(long ID, char *filename)
{
    ANIM_RES *cur, *NewAnim;
    UI_HANDLE ifp;
    long size;

    if (GetAnim(ID))
        return(NULL);

    ifp = UI_OPEN(filename, "rb");

    if (ifp == NULL)
        return(NULL);

    size = UI_FILESIZE(ifp);

    if ( not size)
    {
        UI_CLOSE(ifp);
        return(FALSE);
    }

    NewAnim = new ANIM_RES;
    NewAnim->ID = ID;
    NewAnim->Anim = (ANIMATION *)((void*)new char [size + 1]);
    NewAnim->flags = 0;
    NewAnim->Next = NULL;

    if (NewAnim->Anim == NULL)
    {
        delete NewAnim;
        UI_CLOSE(ifp);
        return(NULL);
    }

    if (UI_READ(NewAnim->Anim, size, 1, ifp) not_eq 1)
    {
        delete NewAnim->Anim;
        delete NewAnim;
        UI_CLOSE(ifp);
        return(NULL);
    }

    UI_CLOSE(ifp);

    ConvertAnim(NewAnim->Anim);
    cur = Root_;

    if (cur == NULL)
    {
        Root_ = NewAnim;
    }
    else
    {
        while (cur->Next)
            cur = cur->Next;

        cur->Next = NewAnim;
    }

    return(NewAnim);
}

void C_Animation::ConvertAnim(ANIMATION *Data)
{
    switch (Data->BytesPerPixel)
    {
        case 2:
            switch (Data->Compression)
            {
                case 0:
                    Convert16Bit(Data);
                    break;

                case 1:
                case 2:
                case 3:
                case 4:
                    Convert16BitRLE(Data);
                    break;
            }
    }
}
void C_Animation::Convert16BitRLE(ANIMATION *Data)
{
    ANIM_FRAME *AnimPtr;
    long i, cnt;
    WORD *dptr;

    AnimPtr = (ANIM_FRAME *)&Data->Start[0];

    for (i = 0; i < Data->Frames; i++)
    {
        dptr = (WORD *)&AnimPtr->Data[0];

        while ( not (*dptr bitand RLE_END))
        {
            if ( not (*dptr bitand RLE_KEYMASK))
            {
                cnt = *dptr;
                dptr++;

                while (cnt > 0)
                {
                    *dptr = UI95_RGB15Bit(*dptr);
                    dptr++;
                    cnt--;
                }
            }
            else if (*dptr bitand RLE_REPEAT)
            {
                dptr++;
                *dptr = UI95_RGB15Bit(*dptr);
                dptr++;
            }
            else
                dptr++;
        }

        AnimPtr = (ANIM_FRAME *)&AnimPtr->Data[AnimPtr->Size];
    }
}


void C_Animation::Convert16Bit(ANIMATION *Data)
{
    ANIM_FRAME *AnimPtr;
    long i, j;
    WORD *dptr;


    AnimPtr = (ANIM_FRAME *)&Data->Start[0];

    for (i = 0; i < Data->Frames; i++)
    {
        dptr = (WORD *)&AnimPtr->Data[0];

        for (j = 0; j < Data->Height * Data->Width; j++)
        {
            *dptr = UI95_RGB15Bit(*dptr);
            dptr++;
        }

        AnimPtr = (ANIM_FRAME *)&AnimPtr->Data[AnimPtr->Size];
    }
}

ANIM_RES *C_Animation::GetAnim(long ID)
{
    ANIM_RES *cur;

    cur = Root_;

    while (cur)
    {
        if (cur->ID == ID)
            return(cur);

        cur = cur->Next;
    }

    return(NULL);
}

void C_Animation::SetFlags(long ID, long flags)
{
    ANIM_RES *anim;

    anim = GetAnim(ID);

    if (anim)
        anim->flags = flags;
}

long C_Animation::GetFlags(long ID)
{
    ANIM_RES *anim;

    anim = GetAnim(ID);

    if (anim)
        return(anim->flags);

    return(0);
}


BOOL C_Animation::RemoveAnim(long ID)
{
    ANIM_RES *cur, *prev;

    if (Root_ == NULL)
        return(FALSE);

    if (Root_->ID == ID)
    {
        cur = Root_;
        Root_ = Root_->Next;
        delete cur->Anim;
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
                delete prev->Anim;
                delete prev;
            }

            cur = cur->Next;
        }

        return(TRUE);
    }

    return(FALSE);
}

#ifdef _UI95_PARSER_
short C_Animation::LocalFind(char *token)
{
    short i = 0;

    while (C_Anim_Tokens[i])
    {
        if (strnicmp(token, C_Anim_Tokens[i], strlen(C_Anim_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Animation::LocalFunction(short ID, long P[], _TCHAR *str, C_Handler *)
{
    switch (ID)
    {
        case CANM_LOADANIM:
            LoadAnim(P[0], str);
            break;
    }
}

#endif // PARSER
