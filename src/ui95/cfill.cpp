#include <windows.h>
#include "chandler.h"
#include "shi/ConvFtoI.h"

#ifdef _UI95_PARSER_ // List of Keywords bitand functions to handle them

enum
{
    CFIL_NOTHING = 0,
    CFIL_SETUP,
    CFIL_SETCOLOR,
    CFIL_SETGRADIENT,
    CFIL_SETPERCENT,
    CFIL_SETDITHER,
};

char *C_Fill_Tokens[] =
{
    "[NOTHING]",
    "[SETUP]",
    "[COLOR]",
    "[GRADIENT]",
    "[PERCENT]",
    "[DITHER]",
    0,
};

#endif

C_Fill::C_Fill() : C_Base()
{
    _SetCType_(_CNTL_FILL_);
    Color_ = 0;
    Start_ = 0;
    End_ = 0;
    Area_ = 1;
    Step_ = 0.0f;
    DitherSize_ = 0;
    DitherPattern_ = NULL;
    DefaultFlags_ = C_BIT_ENABLED bitor C_BIT_REMOVE;
}

C_Fill::C_Fill(char **stream) : C_Base(stream)
{
}

C_Fill::C_Fill(FILE *fp) : C_Base(fp)
{
}

C_Fill::~C_Fill()
{
    if (DitherPattern_)
        Cleanup();
}

long C_Fill::Size()
{
    return(0);
}

void C_Fill::Setup(long ID, short Type)
{
    SetID(ID);
    SetType(Type);
    SetDefaultFlags();
    SetReady(1);
}

void C_Fill::SetGradient(long s, long e)
{
    long delta;
    F4CSECTIONHANDLE* Leave = UI_Enter(Parent_);

    Start_ = static_cast<short>(s);
    End_ = static_cast<short>(e);

    if (s not_eq e)
        SetFlagBitOn(C_BIT_USEGRADIENT);
    else
    {
        UI_Leave(Leave);
        return;
    }

    delta = e - s;

    if (delta < 0)
        delta = -delta;

    if (Type_ == C_TYPE_HORIZONTAL)
    {
        Area_ = (short)(((float)GetW() / (float)delta) + 0.5);

        if (Area_ < 1) Area_ = 1;

        if (GetW())
            Step_ = (float)(e - s) / (float)GetW();
    }
    else
    {
        Area_ = (short)(((float)GetH() / (float)delta) + 0.5);

        if (Area_ < 1) Area_ = 1;

        if (GetH())
            Step_ = (float)(e - s) / (float)GetH();
    }

    UI_Leave(Leave);
}

void C_Fill::SetDither(short size, short range)
{
    short i, j;

    if ((size < 3) or not (range))
        size = 0;

    if (size not_eq DitherSize_)
    {
        if (DitherPattern_)
        {
#ifdef USE_SH_POOLS
            MemFreePtr(DitherPattern_);
#else
            delete DitherPattern_;
#endif
            DitherPattern_ = NULL;
        }
    }

    DitherSize_ = size;

    if (DitherSize_)
    {
#ifdef USE_SH_POOLS
        DitherPattern_ = (char*)MemAllocPtr(UI_Pools[UI_ART_POOL], sizeof(char) * (DitherSize_ * DitherSize_), FALSE);
#else
        DitherPattern_ = new char[DitherSize_ * DitherSize_];
#endif

        if (DitherPattern_)
        {
            for (i = 0; i < size; i++)
                for (j = 0; j < size; j++)
                    DitherPattern_[i * size + j] = (char)((rand() % range) - range / 2);
        }
        else
            DitherSize_ = 0;
    }
}


void C_Fill::Cleanup(void)
{
    if (DitherPattern_ and DitherSize_)
    {
#ifdef USE_SH_POOLS
        MemFreePtr(DitherPattern_);
#else
        delete DitherPattern_;
#endif
        DitherPattern_ = NULL;
    }
}

void C_Fill::Refresh()
{
    if (GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    F4CSECTIONHANDLE* Leave = UI_Enter(Parent_);
    Parent_->SetUpdateRect(GetX(), GetY(), GetX() + GetW(), GetY() + GetH(), GetFlags(), GetClient());
    UI_Leave(Leave);
}

void C_Fill::Draw(SCREEN *surface, UI95_RECT *cliprect)
{
    UI95_RECT s, d;
    float startg;

    if (GetFlags() bitand C_BIT_INVISIBLE or Parent_ == NULL)
        return;

    d.left = GetX();
    d.top = GetY();
    d.right = d.left + GetW();
    d.bottom = d.top + GetH();

    if ( not (Flags_ bitand C_BIT_ABSOLUTE))
    {
        d.left += Parent_->VX_[GetClient()];
        d.top += Parent_->VY_[GetClient()];
        d.right += Parent_->VX_[GetClient()];
        d.bottom += Parent_->VY_[GetClient()];
    }

    if ( not Parent_->ClipToArea(&s, &d, cliprect))
        return;

    if ( not (Flags_ bitand C_BIT_ABSOLUTE))
        if ( not Parent_->ClipToArea(&s, &d, &Parent_->ClientArea_[GetClient()]))
            return;

    if (Flags_ bitand C_BIT_USEGRADIENT)
    {
        if (Flags_ bitand C_BIT_TRANSLUCENT)
        {
            if (Type_ == C_TYPE_HORIZONTAL)
            {
                s = d;
                d.right = (s.left / Area_ + 1) * Area_;

                startg = Start_ + ((d.left - GetX()) / Area_) * Area_ * Step_;

                while (d.left < s.right)
                {
                    if (d.right > s.right)
                        d.right = s.right;

                    if (startg >= 1.0)
                        Parent_->BlitTranslucent(surface, Color_, (long)startg, &d, C_BIT_ABSOLUTE, 0);

                    startg += (float)Area_ * Step_;
                    d.left = d.right;
                    d.right += Area_;
                }
            }
            else
            {
                s = d;
                d.bottom = (s.top / Area_ + 1) * Area_;

                startg = Start_ + ((d.top - GetY()) / Area_) * Area_ * Step_;

                while (d.top < s.bottom)
                {
                    if (d.bottom > s.bottom)
                        d.bottom = s.bottom;

                    if (startg >= 1.0)
                        Parent_->BlitTranslucent(surface, Color_, (long)startg, &d, C_BIT_ABSOLUTE, 0);

                    startg += (float)Area_ * Step_;
                    d.top = d.bottom;
                    d.bottom += Area_;
                }
            }
        }
        else if (DitherSize_)
        {
            if (Type_ == C_TYPE_HORIZONTAL)
            {
                s = d;
                d.right = (s.left / Area_ + 1) * Area_;

                while (d.left < s.right)
                {
                    if (d.right > s.right)
                        d.right = s.right;

                    startg = Start_ + ((d.left - GetX()) / Area_) * Area_ * Step_;

                    if (startg >= 1.0)
                        Parent_->DitherFill(surface, Color_, FloatToInt32(startg), DitherSize_, DitherPattern_, &d, C_BIT_ABSOLUTE, 0);

                    d.left = d.right;
                    d.right += Area_;
                }
            }
            else
            {
                s = d;
                d.bottom = (s.top / Area_ + 1) * Area_;

                while (d.top < s.bottom)
                {
                    if (d.bottom > s.bottom)
                        d.bottom = s.bottom;

                    startg = Start_ + ((d.top - GetY()) / Area_) * Area_ * Step_;

                    if (startg >= 1.0)
                        Parent_->DitherFill(surface, Color_, FloatToInt32(startg), DitherSize_, DitherPattern_, &d, C_BIT_ABSOLUTE, 0);

                    d.top = d.bottom;
                    d.bottom += Area_;
                }
            }
        }
        else if (Color_)
        {
            if (Type_ == C_TYPE_HORIZONTAL)
            {
                s = d;
                d.right = d.left + (d.left / Area_ + 1) * Area_;
                startg = Start_ + d.left / Area_ * Step_;

                while (d.top < s.bottom)
                {
                    if (d.right > s.right)
                        d.right = s.right;

                    Parent_->GradientFill(surface, Color_, FloatToInt32(startg), &d, C_BIT_ABSOLUTE, 0);

                    d.left += Area_;
                    d.right = d.left + Area_;
                    startg += ((float)Step_ * Area_);
                }
            }
            else
            {
                s = d;
                d.bottom = d.top + (d.top / Area_ + 1) * Area_;
                startg = Start_ + d.top / Area_ * Step_;

                while (d.top < s.bottom)
                {
                    if (d.bottom > s.bottom)
                        d.bottom = s.bottom;

                    Parent_->GradientFill(surface, Color_, (long)startg, &d, C_BIT_ABSOLUTE, 0);

                    d.top += Area_;
                    d.bottom = d.top + Area_;
                    startg += ((float)Step_ * Area_);
                }
            }
        }
        else // No point in gradient... will always be 0
        {
            Parent_->BlitFill(surface, Color_, &d, C_BIT_ABSOLUTE, 0);
        }
    }
    else
    {
        if ((Flags_ bitand C_BIT_TRANSLUCENT) and (Start_ < 100) and Start_)
        {
            Parent_->BlitTranslucent(surface, Color_, Start_, &d, C_BIT_ABSOLUTE, 0);
        }
        else if (DitherSize_)
        {
            Parent_->DitherFill(surface, Color_, Start_, DitherSize_, DitherPattern_, &d, C_BIT_ABSOLUTE, 0);
        }
        else
        {
            Parent_->BlitFill(surface, Color_, &d, C_BIT_ABSOLUTE, 0);
        }
    }
}

#ifdef _UI95_PARSER_
short C_Fill::LocalFind(char *token)
{
    short i = 0;

    while (C_Fill_Tokens[i])
    {
        if (strnicmp(token, C_Fill_Tokens[i], strlen(C_Fill_Tokens[i])) == 0)
            return(i);

        i++;
    }

    return(0);
}

void C_Fill::LocalFunction(short ID, long P[], _TCHAR *, C_Handler *)
{
    switch (ID)
    {
        case CFIL_SETUP:
            Setup(P[0], (short)P[1]);
            break;

        case CFIL_SETCOLOR:
            SetColor(P[0] bitor (P[1] << 8) bitor (P[2] << 16));
            break;

        case CFIL_SETGRADIENT:
            SetGradient((short)P[0], (short)P[1]);
            break;

        case CFIL_SETPERCENT:
            SetGradient((short)P[0], (short)P[0]);
            break;

        case CFIL_SETDITHER:
            SetDither((short)P[0], (short)P[1]);
            break;
    }
}

extern char ParseSave[];
extern char ParseCRLF[];

#endif // PARSER
