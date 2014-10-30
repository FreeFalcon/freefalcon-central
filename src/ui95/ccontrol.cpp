#include <windows.h>
#include "chandler.h"

// Parser Stuff
#ifdef _UI95_PARSER_

enum
{
    CNTL_NOTHING = 0,
    CNTL_SETID,
    CNTL_SETTYPE,
    CNTL_SETCLIENT,
    CNTL_SETX,
    CNTL_SETY,
    CNTL_SETW,
    CNTL_SETH,
    CNTL_SETXY,
    CNTL_SETWH,
    CNTL_SETXYWH,
    CNTL_SETGROUP,
    CNTL_SETCLUSTER,
    CNTL_SETFLAGS,
    CNTL_SETFLAGBITON,
    CNTL_SETFLAGBITOFF,
    CNTL_SETFLAGTOGGLE,
    CNTL_SETSOUND,
    CNTL_SETMENU,
    CNTL_USERDATA,
    CNTL_SETFONT,
    CNTL_SETHELP,
    CNTL_CURSOR,
    CNTL_SETMOVIE,
    CNTL_SETHOTKEY,
    CNTL_SETMOUSECOLOR,
    CNTL_SETMOUSEPERC,
    CNTL_SETDRAGCURSOR,
};

char *C_Cntl_Tokens[] =
{
    "[NOTHING]",
    "[ID]",
    "[TYPE]",
    "[CLIENT]",
    "[X]",
    "[Y]",
    "[W]",
    "[H]",
    "[XY]",
    "[WH]",
    "[XYWH]",
    "[GROUP]",
    "[CLUSTER]",
    "[FLAGS]",
    "[FLAGBITON]",
    "[FLAGBITOFF]",
    "[FLAGTOGGLE]",
    "[SOUNDBITE]",
    "[OPENMENU]",
    "[USERDATA]",
    "[FONT]",
    "[HELPTEXT]",
    "[CURSOR]",
    "[PLAYMOVIE]",
    "[HOTKEY]",
    "[OVERCOLOR]",
    "[OVERPERC]",
    "[DRAGCURSOR]",
    0,
};

#endif

static void DelUserDataCB(void *rec)
{
    USERDATA *data;

    data = (USERDATA*)rec;

    if (data->type == C_Base::CSB_IS_CLEANUP_PTR)
        delete data->data.ptr;

    delete data; // JPO memory leak fix.
}

C_Base::C_Base(char **stream)
{
    short count, i;
    long  idx, value;

    memcpy(&ID_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(Section_, *stream, sizeof(long)*NUM_SECTIONS);
    *stream += sizeof(long) * NUM_SECTIONS;
    memcpy(&Flags_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&_CType_, *stream, sizeof(short));
    *stream += sizeof(short);
    memcpy(&Type_, *stream, sizeof(short));
    *stream += sizeof(short);
    memcpy(&x_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&y_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&w_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&h_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&Client_, *stream, sizeof(short));
    *stream += sizeof(short);
    memcpy(&Ready_, *stream, sizeof(short));
    *stream += sizeof(short);

    User_ = NULL;
    // User Data...
    memcpy(&count, *stream, sizeof(short));
    *stream += sizeof(short);

    for (i = 0; i < count; i++)
    {
        memcpy(&idx, *stream, sizeof(long));
        *stream += sizeof(long);
        memcpy(&value, *stream, sizeof(long));
        *stream += sizeof(long);
        SetUserNumber(idx, value);
    }
}

C_Base::C_Base(FILE *fp)
{
    short count, i;
    long  idx, value;

    fread(&ID_, sizeof(long), 1, fp);
    fread(Section_, sizeof(long)*NUM_SECTIONS, 1, fp);
    fread(&Flags_, sizeof(long), 1, fp);
    fread(&_CType_, sizeof(short), 1, fp);
    fread(&Type_, sizeof(short), 1, fp);
    fread(&x_, sizeof(long), 1, fp);
    fread(&y_, sizeof(long), 1, fp);
    fread(&w_, sizeof(long), 1, fp);
    fread(&h_, sizeof(long), 1, fp);
    fread(&Client_, sizeof(short), 1, fp);
    fread(&Ready_, sizeof(short), 1, fp);

    User_ = NULL;
    // User Data...
    fread(&count, sizeof(short), 1, fp);

    for (i = 0; i < count; i++)
    {
        fread(&idx, sizeof(long), 1, fp);
        fread(&value, sizeof(long), 1, fp);
        SetUserNumber(idx, value);
    }
}

C_Control::C_Control() : C_Base()
{
    RelX_ = 0;
    RelY_ = 0;
    Cursor_ = 0;
    DragCursor_ = 0;
    MenuID_ = 0;
    HelpTextID_ = 0;
    HotKey_ = 0;
    MouseOver_ = 0;
    MouseOverColor_ = 0x00ffffff;
    MouseOverPercent_ = 32;

    Callback_ = NULL;
    Sound_ = NULL;
}

C_Control::C_Control(char **stream) : C_Base(stream)
{
    short count, i;
    long  idx, value;

    memcpy(&Cursor_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&DragCursor_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&MenuID_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&HelpTextID_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&MouseOverColor_, *stream, sizeof(COLORREF));
    *stream += sizeof(COLORREF);
    memcpy(&RelX_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&RelY_, *stream, sizeof(long));
    *stream += sizeof(long);
    memcpy(&HotKey_, *stream, sizeof(short));
    *stream += sizeof(short);
    memcpy(&MouseOver_, *stream, sizeof(short));
    *stream += sizeof(short);
    memcpy(&MouseOverPercent_, *stream, sizeof(short));
    *stream += sizeof(short);

    Sound_ = NULL;
    // User Data...
    memcpy(&count, *stream, sizeof(short));
    *stream += sizeof(short);

    for (i = 0; i < count; i++)
    {
        memcpy(&idx, *stream, sizeof(long));
        *stream += sizeof(long);
        memcpy(&value, *stream, sizeof(long));
        *stream += sizeof(long);
        SetSound(value, (short)idx); // note that the index is the 2nd parameter
    }
}

C_Control::C_Control(FILE *fp) : C_Base(fp)
{
    short count, i;
    long  idx, value;

    fread(&Cursor_, sizeof(long), 1, fp);
    fread(&DragCursor_, sizeof(long), 1, fp);
    fread(&MenuID_, sizeof(long), 1, fp);
    fread(&HelpTextID_, sizeof(long), 1, fp);
    fread(&MouseOverColor_, sizeof(COLORREF), 1, fp);
    fread(&RelX_, sizeof(long), 1, fp);
    fread(&RelY_, sizeof(long), 1, fp);
    fread(&HotKey_, sizeof(short), 1, fp);
    fread(&MouseOver_, sizeof(short), 1, fp);
    fread(&MouseOverPercent_, sizeof(short), 1, fp);

    Sound_ = NULL;
    // User Data...
    fread(&count, sizeof(short), 1, fp);

    for (i = 0; i < count; i++)
    {
        fread(&idx, sizeof(long), 1, fp);
        fread(&value, sizeof(long), 1, fp);
        SetSound(value, (short)idx); // note that the index is the 2nd parameter
    }
}

long C_Base::Size()
{
    long size, curidx;
    C_HASHNODE *cur;
    USERDATA *rec;

    size = sizeof(long)
           + sizeof(long) * NUM_SECTIONS
           + sizeof(long)
           + sizeof(short)
           + sizeof(short)
           + sizeof(long)
           + sizeof(long)
           + sizeof(long)
           + sizeof(long)
           + sizeof(short)
           + sizeof(short)
           // User Data...
           + sizeof(short); // # UserData elements

    if (User_)
    {
        rec = (USERDATA*)User_->GetFirst(&cur, &curidx);

        while (rec)
        {
            if (rec->type == CSB_IS_VALUE)
                size += sizeof(long) * 2;

            rec = (USERDATA*)User_->GetNext(&cur, &curidx);
        }
    }

    return(size);
}

void C_Base::Save(char **stream)
{
    short count;
    long curidx;
    C_HASHNODE *cur;
    USERDATA *rec;

    memcpy(*stream, &ID_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, Section_, sizeof(long)*NUM_SECTIONS);
    *stream += sizeof(long) * NUM_SECTIONS;
    memcpy(*stream, &Flags_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &_CType_, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &Type_, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &x_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &y_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &w_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &h_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &Client_, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &Ready_, sizeof(short));
    *stream += sizeof(short);

    // User Data...
    count = 0;

    if (User_)
    {
        rec = (USERDATA*)User_->GetFirst(&cur, &curidx);

        while (rec)
        {
            if (rec->type == CSB_IS_VALUE)
                count++;

            rec = (USERDATA*)User_->GetNext(&cur, &curidx);
        }

        if (count)
        {
            memcpy(*stream, &count, sizeof(short));
            *stream += sizeof(short);
            rec = (USERDATA*)User_->GetFirst(&cur, &curidx);

            while (rec)
            {
                if (rec->type == CSB_IS_VALUE)
                {
                    memcpy(*stream, &curidx, sizeof(long));
                    *stream += sizeof(long);
                    memcpy(*stream, &rec->data.number, sizeof(long));
                    *stream += sizeof(long);
                }

                rec = (USERDATA*)User_->GetNext(&cur, &curidx);
            }
        }
    }
    else
        memcpy(*stream, &count, sizeof(short));

    *stream += sizeof(short);
}

void C_Base::Save(FILE *fp)
{
    short count;
    long curidx;
    C_HASHNODE *cur;
    USERDATA *rec;

    fwrite(&ID_, sizeof(long), 1, fp);
    fwrite(Section_, sizeof(long)*NUM_SECTIONS, 1, fp);
    fwrite(&Flags_, sizeof(long), 1, fp);
    fwrite(&_CType_, sizeof(short), 1, fp);
    fwrite(&Type_, sizeof(short), 1, fp);
    fwrite(&x_, sizeof(long), 1, fp);
    fwrite(&y_, sizeof(long), 1, fp);
    fwrite(&w_, sizeof(long), 1, fp);
    fwrite(&h_, sizeof(long), 1, fp);
    fwrite(&Client_, sizeof(short), 1, fp);
    fwrite(&Ready_, sizeof(short), 1, fp);

    // User Data...
    count = 0;

    if (User_)
    {
        rec = (USERDATA*)User_->GetFirst(&cur, &curidx);

        while (rec)
        {
            if (rec->type == CSB_IS_VALUE)
                count++;

            rec = (USERDATA*)User_->GetNext(&cur, &curidx);
        }

        if (count)
        {
            fwrite(&count, sizeof(short), 1, fp);
            rec = (USERDATA*)User_->GetFirst(&cur, &curidx);

            while (rec)
            {
                if (rec->type == CSB_IS_VALUE)
                {
                    fwrite(&curidx, sizeof(long), 1, fp);
                    fwrite(&rec->data.number, sizeof(long), 1, fp);
                }

                rec = (USERDATA*)User_->GetNext(&cur, &curidx);
            }
        }
    }
    else
        fwrite(&count, sizeof(short), 1, fp);
}

long C_Control::Size()
{
    long size;
    long curidx;
    C_HASHNODE *cur;
    long rec;

    size = C_Base::Size();

    size += sizeof(long)
            + sizeof(long)
            + sizeof(long)
            + sizeof(long)
            + sizeof(COLORREF)
            + sizeof(long)
            + sizeof(long)
            + sizeof(short)
            + sizeof(short)
            + sizeof(short)
            // Sound Data...
            + sizeof(short);

    if (Sound_)
    {
        rec = (long)Sound_->GetFirst(&cur, &curidx);

        while (rec)
        {
            size += sizeof(long) * 2;
            rec = (long)Sound_->GetNext(&cur, &curidx);
        }
    }

    return(size);
}

void C_Control::Save(char **stream)
{
    short count;
    long curidx;
    C_HASHNODE *cur;
    long rec;

    C_Base::Save(stream);

    memcpy(*stream, &Cursor_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &DragCursor_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &MenuID_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &HelpTextID_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &MouseOverColor_, sizeof(COLORREF));
    *stream += sizeof(COLORREF);
    memcpy(*stream, &RelX_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &RelY_, sizeof(long));
    *stream += sizeof(long);
    memcpy(*stream, &HotKey_, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &MouseOver_, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &MouseOverPercent_, sizeof(short));
    *stream += sizeof(short);

    // Sound Data...
    count = 0;

    if (Sound_)
    {
        rec = (long)Sound_->GetFirst(&cur, &curidx);

        while (rec)
        {
            count++;
            rec = (long)Sound_->GetNext(&cur, &curidx);
        }

        if (count)
        {
            memcpy(*stream, &count, sizeof(short));
            *stream += sizeof(short);
            rec = (long)Sound_->GetFirst(&cur, &curidx);

            while (rec)
            {
                memcpy(*stream, &curidx, sizeof(long));
                *stream += sizeof(long);
                memcpy(*stream, &rec, sizeof(long));
                *stream += sizeof(long);
                rec = (long)Sound_->GetNext(&cur, &curidx);
            }
        }
    }
    else
        memcpy(*stream, &count, sizeof(short));

    *stream += sizeof(short);
}

void C_Control::Save(FILE *fp)
{
    short count;
    long curidx;
    C_HASHNODE *cur;
    long rec;

    C_Base::Save(fp);

    fwrite(&Cursor_, sizeof(long), 1, fp);
    fwrite(&DragCursor_, sizeof(long), 1, fp);
    fwrite(&MenuID_, sizeof(long), 1, fp);
    fwrite(&HelpTextID_, sizeof(long), 1, fp);
    fwrite(&MouseOverColor_, sizeof(COLORREF), 1, fp);
    fwrite(&RelX_, sizeof(long), 1, fp);
    fwrite(&RelY_, sizeof(long), 1, fp);
    fwrite(&HotKey_, sizeof(short), 1, fp);
    fwrite(&MouseOver_, sizeof(short), 1, fp);
    fwrite(&MouseOverPercent_, sizeof(short), 1, fp);

    // Sound Data...
    count = 0;

    if (Sound_)
    {
        rec = (long)Sound_->GetFirst(&cur, &curidx);

        while (rec)
        {
            count++;
            rec = (long)Sound_->GetNext(&cur, &curidx);
        }

        if (count)
        {
            fwrite(&count, sizeof(short), 1, fp);
            rec = (long)Sound_->GetFirst(&cur, &curidx);

            while (rec)
            {
                fwrite(&curidx, sizeof(long), 1, fp);
                fwrite(&rec, sizeof(long), 1, fp);
                rec = (long)Sound_->GetNext(&cur, &curidx);
            }
        }
    }
    else
        fwrite(&count, sizeof(short), 1, fp);
}

void C_Control::SetSound(long ID, short Type)
{
    SOUND_RES *snd;

    snd = gSoundMgr->GetSound(ID);

    if (snd)
    {
        if ( not Sound_)
        {
            Sound_ = new C_Hash;
            Sound_->Setup(1);
        }

        Sound_->Remove(Type);
        Sound_->Add(Type, snd);
    }
}

SOUND_RES *C_Control::GetSound(short Type)
{
    SOUND_RES *snd = NULL;

    if (Sound_)
        snd = (SOUND_RES*)Sound_->Find(Type);

    return(snd);
}

void C_Base::SetUserNumber(long idx, long value)
{
    USERDATA *usr;

    if ( not User_)
    {
        User_ = new C_Hash;
        User_->Setup(1);
        User_->SetFlags(C_BIT_REMOVE);
        User_->SetCallback(DelUserDataCB);
    }

    if (User_)
    {
        usr = (USERDATA*)User_->Find(idx);

        if (usr)
        {
            if (usr->type == CSB_IS_CLEANUP_PTR)
                delete usr->data.ptr;
        }
        else
        {
#ifdef USE_SH_POOLS
            usr = (USERDATA *)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(USERDATA), FALSE);
#else
            usr = new USERDATA;
#endif
            User_->Add(idx, usr);
        }

        usr->type = CSB_IS_VALUE;
        usr->data.number = value;
    }
}

void C_Base::SetUserPtr(long idx, void *value)
{
    USERDATA *usr;

    if ( not User_)
    {
        User_ = new C_Hash;
        User_->Setup(1);
        User_->SetFlags(C_BIT_REMOVE);
        User_->SetCallback(DelUserDataCB);
    }

    if (User_)
    {
        usr = (USERDATA*)User_->Find(idx);

        if (usr)
        {
            if (usr->type == CSB_IS_CLEANUP_PTR)
                delete usr->data.ptr;
        }
        else
        {
#ifdef USE_SH_POOLS
            usr = (USERDATA *)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(USERDATA), FALSE);
#else
            usr = new USERDATA;
#endif
            User_->Add(idx, usr);
        }

        usr->type = CSB_IS_PTR;
        usr->data.ptr = value;
    }
}

void C_Base::SetUserCleanupPtr(long idx, void *value)
{
    USERDATA *usr;

    if ( not User_)
    {
        User_ = new C_Hash;
        User_->Setup(1);
        User_->SetFlags(C_BIT_REMOVE);
        User_->SetCallback(DelUserDataCB);
    }

    if (User_)
    {
        usr = (USERDATA*)User_->Find(idx);

        if (usr)
        {
            if (usr->type == CSB_IS_CLEANUP_PTR)
                delete usr->data.ptr;
        }
        else
        {
#ifdef USE_SH_POOLS
            usr = (USERDATA *)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(USERDATA), FALSE);
#else
            usr = new USERDATA;
#endif
            User_->Add(idx, usr);
        }

        usr->type = CSB_IS_CLEANUP_PTR;
        usr->data.ptr = value;
    }
}

long C_Base::GetUserNumber(long idx)
{
    USERDATA *usr;

    if (User_)
    {
        usr = (USERDATA*)User_->Find(idx);

        if (usr and usr->type == CSB_IS_VALUE)
            return(usr->data.number);
    }

    return(0);
}

void *C_Base::GetUserPtr(long idx)
{
    USERDATA *usr;

    if (User_)
    {
        usr = (USERDATA*)User_->Find(idx);

        if (usr and (usr->type == CSB_IS_PTR or usr->type == CSB_IS_CLEANUP_PTR))
            return(usr->data.ptr);
    }

    return(NULL);
}

BOOL C_Control::MouseOver(long relx, long rely, C_Base *me)
{
    if (GetFlags() bitand C_BIT_INVISIBLE or not (GetFlags() bitand C_BIT_ENABLED) or not Ready())
        return(FALSE);

    if (relx >= GetX() and rely >= GetY() and relx <= (GetX() + GetW()) and rely <= GetY() + GetH())
    {
        if ((C_Base*)this not_eq me)
            gSoundMgr->PlaySound(GetSound(C_TYPE_MOUSEOVER));

        return(TRUE);
    }

    return(FALSE);
}

void C_Control::HighLite(SCREEN *surface, UI95_RECT *cliprect)
{
    UI95_RECT clip, tmp;

    clip.left = GetX();
    clip.top = GetY();

    if (Flags_ bitand C_BIT_RIGHT)
        clip.left -= GetW();
    else if (Flags_ bitand C_BIT_HCENTER)
        clip.left -= GetW() / 2;

    if (Flags_ bitand C_BIT_BOTTOM)
        clip.top -= GetH();
    else if (Flags_ bitand C_BIT_VCENTER)
        clip.top -= GetH() / 2;

    if ( not (Flags_ bitand C_BIT_ABSOLUTE))
    {
        clip.left += Parent_->VX_[Client_];
        clip.top += Parent_->VY_[Client_];
    }

    clip.right = clip.left + GetW();
    clip.bottom = clip.top + GetH();
    tmp = clip; // JPO fix so it has some valid data

    if ( not Parent_->ClipToArea(&tmp, &clip, cliprect))
        return;

    if ( not (Flags_ bitand C_BIT_ABSOLUTE))
        if ( not Parent_->ClipToArea(&tmp, &clip, &Parent_->ClientArea_[Client_]))
            return;

    Parent_->BlitTranslucent(surface, MouseOverColor_, MouseOverPercent_, &clip, C_BIT_ABSOLUTE, 0);
}

#ifdef _UI95_PARSER_

extern char ParseSave[];
extern char ParseCRLF[];

short C_Base::BaseFind(char *token)
{
    short i = 0;

    while (C_Cntl_Tokens[i])
    {
        if (strnicmp(token, C_Cntl_Tokens[i], strlen(C_Cntl_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Base::BaseFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CNTL_SETID:
            SetID(P[0]);
            break;

        case CNTL_SETTYPE:
            SetType((short)P[0]);
            break;

        case CNTL_SETCLIENT:
            SetClient((short)P[0]);
            break;

        case CNTL_SETX:
            SetX(P[0]);
            break;

        case CNTL_SETY:
            SetY(P[0]);
            break;

        case CNTL_SETW:
            SetW(P[0]);
            break;

        case CNTL_SETH:
            SetH(P[0]);
            break;

        case CNTL_SETXY:
            SetXY(P[0], P[1]);
            break;

        case CNTL_SETWH:
            SetWH(P[0], P[1]);
            break;

        case CNTL_SETXYWH:
            SetXYWH(P[0], P[1], P[2], P[3]);
            break;

        case CNTL_SETGROUP:
            SetGroup(P[0]);
            break;

        case CNTL_SETCLUSTER:
            SetCluster(P[0]);
            break;

        case CNTL_SETFLAGS:
            SetFlags(P[0]);
            break;

        case CNTL_SETFLAGBITON:
            SetFlags(GetFlags() bitor P[0]);
            break;

        case CNTL_SETFLAGBITOFF:
            SetFlags(GetFlags() bitand compl P[0]);
            break;

        case CNTL_SETFLAGTOGGLE:
            SetFlags(GetFlags() xor P[0]);
            break;

        case CNTL_SETFONT:
            SetFont(P[0]);
            break;

        case CNTL_SETSOUND:
            SetSound(P[0], (short)P[1]);
            break;

        case CNTL_SETMENU:
            SetMenu(P[0]);
            break;

        case CNTL_USERDATA:
            SetUserNumber(P[0], P[1]);
            break;

        case CNTL_SETHOTKEY:
            SetHotKey((WORD)(P[0] bitor P[1] bitor P[2] bitor P[3] bitor P[4] bitor P[5] bitor P[6]));
            break;

        case CNTL_CURSOR:
            SetCursorID(P[0]);
            break;

        case CNTL_SETHELP:
            SetHelpText(P[0]);
            break;

        case CNTL_SETMOUSECOLOR:
            SetMouseOverColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CNTL_SETMOUSEPERC:
            SetMouseOverPerc((short)P[0]);
            break;

        case CNTL_SETDRAGCURSOR:
            SetDragCursorID(P[0]);
            break;
    }
}

#endif // PARSER
