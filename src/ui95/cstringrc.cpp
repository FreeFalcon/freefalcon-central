#include <windows.h>
#include <ddraw.h>
#include "debuggr.h"
#include "dxutil/ddutil.h"
#include "UI/Include/targa.h"
#include "chandler.h"

#ifdef _UI95_PARSER_

enum
{
    CSTR_NOTHING = 0,
    CSTR_ADDTEXT,
};

char *C_Str_Tokens[] =
{
    "[NOTHING]",
    "[ADDTEXT]",
    0,
};

#endif // PARSER

C_String *gStringMgr = NULL;

C_String::C_String()
{
    Root_ = NULL;
    IDTable_ = NULL;
    IDSize_ = 0;
    LastID_ = -1;
}

C_String::~C_String()
{
    if (Root_ or IDTable_)
        Cleanup();
}

void C_String::Setup(long NumIDs)
{
    if (Root_)
        Cleanup();

    Root_ = new C_Hash;
    Root_->Setup(_STR_HASH_SIZE_);
    Root_->SetFlags(C_BIT_REMOVE);

    if (NumIDs > 0)
    {
        IDSize_ = NumIDs;
#ifdef USE_SH_POOLS
        IDTable_ = (long*)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(long) * (IDSize_), FALSE);
#else
        IDTable_ = new long[IDSize_];
#endif
        memset(IDTable_, -1, sizeof(long)*IDSize_);
    }
}

void C_String::Cleanup()
{
    if (Root_)
    {
        Root_->Cleanup();
        delete Root_;
        Root_ = NULL;
    }

    if (IDTable_)
    {
#ifdef USE_SH_POOLS
        MemFreePtr(IDTable_);
#else
        delete IDTable_;
#endif
        IDTable_ = NULL;
    }

    IDSize_ = 0;
}

BOOL C_String::AddString(long ID, _TCHAR *str)
{
    long HashID;

    if (str == NULL)
        return(FALSE);

    HashID = Root_->AddText(str);

    if (HashID < 0)
        return(FALSE);

    if (ID < IDSize_)
        IDTable_[ID] = HashID bitor 0x40000000;

    return(TRUE);
}

_TCHAR *C_String::GetString(long ID)
{
    if (ID < 1) return(NULL);

    if (ID < IDSize_)
    {
        if (IDTable_[ID] >= 0)
            return(Root_->FindText(IDTable_[ID] bitand 0x3fffffff));
    }
    else if (ID > 0)
        return(Root_->FindText(ID bitand 0x3fffffff));

    return(NULL);
}


long C_String::AddText(const _TCHAR *str)
{
    return(Root_->AddText(str) bitor 0x40000000);
}

_TCHAR *C_String::GetText(long ID)
{
    if (ID > 0)
        return(Root_->FindText(ID bitand 0x3fffffff));

    return(NULL);
}

#ifdef _UI95_PARSER_

short C_String::LocalFind(char *token)
{
    short i = 0;

    while (C_Str_Tokens[i])
    {
        if (strnicmp(token, C_Str_Tokens[i], strlen(C_Str_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_String::LocalFunction(short ID, long P[], _TCHAR *str, C_Handler *)
{
    switch (ID)
    {
        case CSTR_ADDTEXT:
            LastID_ = -1;

            if (P[0] < 0)
                LastID_ = AddText(str);
            else
                AddString(P[0], str);

            break;
    }
}

#endif // PARSER
