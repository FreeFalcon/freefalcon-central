#include "chandler.h"

C_Hash::C_Hash()
{
    flags_ = 0;
    TableSize_ = 0;
    Table_ = NULL;
    Check_ = 0;

    Callback_ = NULL;
}

C_Hash::~C_Hash()
{
    if (TableSize_ or Table_)
        Cleanup();
}

void C_Hash::Setup(long Size)
{
    short i;
    TableSize_ = Size;

    //edg note: even if new is overloaded to use specific pool, when alloc'ing
    // and array of things, SH seems to still put into default pool( not ?)
#ifdef USE_SH_POOLS
    Table_ = (C_HASHROOT *)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(C_HASHROOT) * TableSize_, FALSE);
#else
    Table_ = new C_HASHROOT[TableSize_];
#endif

    for (i = 0; i < TableSize_; i++)
        Table_[i].Root_ = NULL;
}

void C_Hash::Cleanup()
{
    long i;
    C_HASHNODE *cur, *prev;

    if (TableSize_ or Table_)
    {
        for (i = 0; i < TableSize_; i++)
        {
            cur = Table_[i].Root_;

            while (cur)
            {
                prev = cur;
                cur = cur->Next;

                if (flags_ bitand C_BIT_REMOVE)
                {
                    if (Callback_)
                        (*Callback_)(prev->Record);
                    else
#ifdef USE_SH_POOLS
                        MemFreePtr(prev->Record);

#else
                        delete prev->Record;
#endif
                }

                delete prev;
            }
        }

        delete []Table_; // JPO fix
        Table_ = NULL;
        TableSize_ = 0;
    }
}

void *C_Hash::Find(long ID)
{
    long idx;
    C_HASHNODE *cur;

    if ( not TableSize_ or not Table_ or ID < 0) return(NULL);

    idx = ID % TableSize_;
    cur = Table_[idx].Root_;

    while (cur)
    {
        if (cur->ID == ID)
        {
            cur->Check = Check_;
            return(cur->Record);
        }

        cur = cur->Next;
    }

    return(NULL);
}

void C_Hash::Add(long ID, void *rec)
{
    long idx;
    C_HASHNODE *cur, *newhash;

    if ( not TableSize_ or not Table_ or not rec or ID < 0)
        return;

    if (Find(ID))
        return;

    newhash = new C_HASHNODE;
    newhash->ID = ID;
    newhash->Record = rec;
    newhash->Check = Check_;
    newhash->Next = NULL;

    idx = ID % TableSize_;

    if ( not Table_[idx].Root_)
    {
        Table_[idx].Root_ = newhash;
    }
    else
    {
        cur = Table_[idx].Root_;

        while (cur->Next)
            cur = cur->Next;

        cur->Next = newhash;
    }
}

long C_Hash::AddText(const _TCHAR *string)
{
    unsigned long idx, ID, i;
    C_HASHNODE *cur, *newhash;
    _TCHAR *data;

    if ( not TableSize_ or not Table_ or not string) return(-1);

    ID = 0;
    idx = _tcsclen(string);

    for (i = 0; i < idx; i++)
        ID += string[i];

    idx = ID % TableSize_;

    cur = Table_[idx].Root_;

    while (cur)
    {
        if (_tcscmp(string, (_TCHAR *)cur->Record) == 0)
            return(cur->ID);

        cur = cur->Next;
    }

    i = _tcsclen(string);
    newhash = new C_HASHNODE;
#ifdef USE_SH_POOLS
    data = (_TCHAR*)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(_TCHAR) * (i + 1), FALSE);
#else
    data = new _TCHAR[i + 1];
#endif
    _tcsncpy(data, string, i);
    data[i] = 0;
    newhash->Record = data;
    newhash->Check = Check_;
    newhash->Next = NULL;

    ID = idx << 16;

    if ( not Table_[idx].Root_)
    {
        Table_[idx].Root_ = newhash;
        newhash->ID = idx << 16;
    }
    else
    {
        ID++;
        cur = Table_[idx].Root_;

        while (cur->Next)
        {
            ID++;
            cur = cur->Next;
        }

        cur->Next = newhash;
        newhash->ID = ID;
    }

    return(newhash->ID);
}

long C_Hash::AddTextID(long TextID, _TCHAR *string)
{
    unsigned long idx, ID, i;
    C_HASHNODE *cur, *newhash;
    _TCHAR *data;

    if ( not TableSize_ or not Table_ or not string) return(-1);

    ID = 0;
    idx = _tcsclen(string);

    for (i = 0; i < idx; i++)
        ID += string[i];

    idx = ID % TableSize_;

    cur = Table_[idx].Root_;

    while (cur)
    {
        if (_tcscmp(string, (_TCHAR *)cur->Record) == 0)
            return(cur->ID);

        cur = cur->Next;
    }

    i = _tcsclen(string);
    newhash = new C_HASHNODE;
#ifdef USE_SH_POOLS
    data = (_TCHAR*)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(_TCHAR) * (i + 1), FALSE);
#else
    data = new _TCHAR[i + 1];
#endif
    _tcsncpy(data, string, i);
    data[i] = 0;
    newhash->ID = TextID;
    newhash->Record = data;
    newhash->Check = Check_;
    newhash->Next = NULL;

    ID = idx << 16;

    if ( not Table_[idx].Root_)
    {
        Table_[idx].Root_ = newhash;
    }
    else
    {
        ID++;
        cur = Table_[idx].Root_;

        while (cur->Next)
        {
            ID++;
            cur = cur->Next;
        }

        cur->Next = newhash;
    }

    return(ID);
}

_TCHAR *C_Hash::FindText(long ID)
{
    unsigned long idx, i;
    C_HASHNODE *cur;

    if ( not TableSize_ or not Table_ or (ID < 0)) return(NULL);

    idx = ID >> 16;
    i = ID bitand 0x0000ffff;

    if (idx >= (unsigned long)TableSize_) 
        return(NULL);

    cur = Table_[idx].Root_;

    for (i = 0; i < (unsigned long)(ID bitand 0x0000ffff) and cur; i++)
        cur = cur->Next;

    if (cur)
        return((_TCHAR *)cur->Record);

    return(NULL);
}

long C_Hash::FindTextID(_TCHAR *string)
{
    unsigned long idx, i;
    unsigned long ID;
    C_HASHNODE *cur;

    if ( not TableSize_ or not Table_ or not string) return(-1);

    ID = 0;
    idx = _tcsclen(string);

    for (i = 0; i < idx; i++)
        ID += string[i];

    idx = ID % TableSize_;

    cur = Table_[idx].Root_;

    while (cur)
    {
        if (_tcscmp(string, (_TCHAR *)cur->Record) == 0)
            return(cur->ID);

        cur = cur->Next;
    }

    return(-1);
}

long C_Hash::FindTextID(long ID)
{
    long idx, i;
    C_HASHNODE *cur;

    if ( not TableSize_ or not Table_ or (ID < 0)) return(NULL);

    idx = ID >> 16;
    i = ID bitand 0x0000ffff;

    if (idx >= TableSize_)
        return(-1);

    cur = Table_[idx].Root_;

    for (i = 0; i < (ID bitand 0x0000ffff) and cur; i++)
        cur = cur->Next;

    if (cur)
        return(cur->ID);

    return(-1);
}

void C_Hash::Remove(long ID)
{
    long idx;
    C_HASHNODE *cur, *prev;

    if ( not TableSize_ or not Table_ or (ID < 0)) return;

    idx = ID % TableSize_;

    if ( not Table_[idx].Root_) return;

    Table_[idx].Root_;

    if (Table_[idx].Root_->ID == ID)
    {
        prev = Table_[idx].Root_;
        Table_[idx].Root_ = Table_[idx].Root_->Next;

        if (flags_ bitand C_BIT_REMOVE)
        {
            if (Callback_)
                (*Callback_)(prev->Record);
            else
#ifdef USE_SH_POOLS
                MemFreePtr(prev->Record);

#else
                delete prev->Record;
#endif
        }

        delete prev;
    }
    else
    {
        cur = Table_[idx].Root_;

        while (cur->Next)
        {
            if (cur->Next->ID == ID)
            {
                prev = cur->Next;
                cur->Next = cur->Next->Next;

                if (flags_ bitand C_BIT_REMOVE)
                {
                    if (Callback_)
                        (*Callback_)(prev->Record);
                    else
#ifdef USE_SH_POOLS
                        MemFreePtr(prev->Record);

#else
                        delete prev->Record;
#endif
                }

                delete prev;
                return;
            }

            cur = cur->Next;
        }
    }
}

void C_Hash::RemoveOld()
{
    long i;
    C_HASHNODE *cur, *prev;

    if ( not TableSize_ or not Table_) return;

    for (i = 0; i < TableSize_; i++)
    {
        while (Table_[i].Root_ and Table_[i].Root_->Check not_eq Check_)
        {
            prev = Table_[i].Root_;
            Table_[i].Root_ = Table_[i].Root_->Next;

            if (flags_ bitand C_BIT_REMOVE)
            {
                if (Callback_)
                    (*Callback_)(prev->Record);
                else
#ifdef USE_SH_POOLS
                    MemFreePtr(prev->Record);

#else
                    delete prev->Record;
#endif
            }

            delete prev;
        }

        cur = Table_[i].Root_;

        if (cur)
        {
            while (cur->Next)
            {
                if (cur->Next->Check not_eq Check_)
                {
                    prev = cur->Next;
                    cur->Next = cur->Next->Next;

                    if (flags_ bitand C_BIT_REMOVE)
                    {
                        if (Callback_)
                            (*Callback_)(prev->Record);
                        else
#ifdef USE_SH_POOLS
                            MemFreePtr(prev->Record);

#else
                            delete prev->Record;
#endif
                    }

                    delete prev;
                }
                else
                    cur = cur->Next;
            }
        }
    }
}

void *C_Hash::GetFirstOld(C_HASHNODE **current, long *curidx)
{
    C_HASHNODE *cur;

    *curidx = 0;
    *current = NULL;

    cur = Table_[*curidx].Root_;

    while (cur)
    {
        if (cur->Record and cur->Check not_eq Check_)
        {
            *current = cur;
            return(cur->Record);
        }

        cur = cur->Next;

        while ( not cur and *curidx < (TableSize_ - 1))
        {
            (*curidx)++;
            cur = Table_[*curidx].Root_;
        }
    }

    return(NULL);
}

void *C_Hash::GetNextOld(C_HASHNODE **current, long *curidx)
{
    C_HASHNODE *cur;

    if ( not (*current))
        return(NULL);

    cur = (*current)->Next;

    while ( not cur and *curidx < (TableSize_ - 1))
    {
        (*curidx)++;
        cur = Table_[*curidx].Root_;
    }

    while (cur and cur->Check == Check_ and cur->Record)
    {
        cur = cur->Next;

        while ( not cur and *curidx < (TableSize_ - 1))
        {
            (*curidx)++;
            cur = Table_[*curidx].Root_;
        }
    }

    *current = cur;

    if (cur)
        return(cur->Record);

    return(NULL);
}

void *C_Hash::GetFirst(C_HASHNODE **current, long *curidx)
{
    C_HASHNODE *cur;

    *curidx = 0;
    *current = NULL;

    ShiAssert(curidx);

    if ( not curidx)
        return NULL;

    cur = Table_[*curidx].Root_;

    while ( not cur and *curidx < (TableSize_ - 1))
    {
        (*curidx)++;
        cur = Table_[*curidx].Root_;
    }

    if (*curidx < TableSize_)
    {
        *current = cur;

        if (cur)
            return(cur->Record);
    }

    *current = NULL;
    return(NULL);
}

void *C_Hash::GetNext(C_HASHNODE **current, long *curidx)
{
    C_HASHNODE *cur;

    if ( not *current)
        return(NULL);

    cur = (*current)->Next;

    while ( not cur and *curidx < (TableSize_ - 1))
    {
        (*curidx)++;
        cur = Table_[*curidx].Root_;
    }

    *current = cur;

    if (cur)
        return(cur->Record);

    return(NULL);
}
