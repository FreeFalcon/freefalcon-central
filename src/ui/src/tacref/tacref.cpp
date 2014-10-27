#include <windows.h>
#include "chandler.h"
#include "tacref.h"

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

#include "cmpclass.h"
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
// This file CONTAINS ALL the class code define in tacref.h
// regardless of which class it was


// TacticalReference Class

TacticalReference::TacticalReference()
{
    Index_ = NULL;
    Size_ = 0;
    Data_ = NULL;
}

TacticalReference::~TacticalReference()
{
    if (Index_ or Data_)
        Cleanup();
}

BOOL TacticalReference::Load(char *filename)
{
    UI_HANDLE fp;

    fp = UI_OPEN(filename, "rb");

    if ( not fp)
        return(FALSE);

    UI_SEEK(fp, 0l, SEEK_END);
    Size_ = RES_FTELL(fp);
    UI_SEEK(fp, 0l, SEEK_SET);

    Data_ = new char [Size_];
    UI_READ(Data_, Size_, 1, fp);
    UI_CLOSE(fp);

    return(TRUE);
}

void TacticalReference::Cleanup()
{
    if (Index_)
    {
        Index_->Cleanup();
        delete Index_;
        Index_ = NULL;
    }

    if (Data_)
    {
        delete Data_;
        Data_ = NULL;
        Size_ = 0;
    }
}

void TacticalReference::MakeIndex(long Size)
{
    long Offset;
    Entity *rec;

    if (Index_)
        Index_->Cleanup();
    else
        Index_ = new C_Hash;

    Index_->Setup(Size);

    rec = GetFirst(&Offset);

    while (rec)
    {
        Index_->Add(rec->EntityID, rec);
        rec = GetNext(&Offset);
    }
}

Entity *TacticalReference::Find(long EntityID)
{
    long Offset;
    Entity *rec;

    if (Index_)
        return((Entity*)Index_->Find(EntityID));

    rec = GetFirst(&Offset);

    while (rec)
    {
        if (rec->EntityID == EntityID)
            return((Entity*)rec);

        rec = GetNext(&Offset);
    }

    return(NULL);
}

Entity *TacticalReference::FindFirst(long GroupID, long SubGroupID)
{
    long Offset;
    Entity *rec;

    rec = GetFirst(&Offset);

    while (rec)
    {
        if (rec->GroupID == GroupID and rec->SubGroupID == SubGroupID)
            return((Entity*)rec);

        rec = GetNext(&Offset);
    }

    return(NULL);
}

Entity *TacticalReference::GetFirst(long *offset)
{
    Header *hdr;

    if ( not Data_)
        return(NULL);

    *offset = 0;
    hdr = (Header*)Data_;
    return((Entity*)hdr->Data);
}

Entity *TacticalReference::GetNext(long *offset)
{
    Header *hdr;

    if ( not Data_)
        return(NULL);

    hdr = (Header*)(Data_ + *offset);
    *offset += sizeof(Header) + hdr->size;
    hdr = (Header*)(Data_ + *offset);

    if (*offset < Size_ and hdr->type == _ENTITY_)  // JPO - reorder condition to stop bad array ref
        return((Entity*)hdr->Data);

    return(NULL);
}

/*********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************/

// Statistics Class

Category *Statistics::GetFirst(long *offset)
{
    if ( not size)
        return(NULL);

    *offset = 0;
    return((Category*)Data);
}

Category *Statistics::GetNext(long *offset)
{
    Category *cat;

    cat = (Category*)&Data[(*offset)];
    (*offset) += sizeof(Header) + cat->size;

    if ((*offset) < size)
        return((Category*)&Data[(*offset)]);

    return(NULL);
}

/*********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************/

// Category Class

CatText *Category::GetFirst(long *offset)
{
    Header *hdr;

    hdr = (Header *)Data;

    if ( not hdr->size)
        return(NULL);

    *offset = 0;
    return((CatText*)&hdr->Data[*offset]);
}

CatText *Category::GetNext(long *offset)
{
    Header *hdr;
    CatText *txt;

    hdr = (Header *)Data;

    txt = (CatText*)&hdr->Data[*offset];
    *offset += sizeof(CatText) + txt->length;

    if (*offset < hdr->size)
        return((CatText*)&hdr->Data[*offset]);

    return(NULL);
}

/*********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************/

// Category Class

TextString *Description::GetFirst(long *offset)
{
    if ( not size)
        return(NULL);

    *offset = 0;
    return((TextString*)Data);
}

TextString *Description::GetNext(long *offset)
{
    TextString *txt;

    txt = (TextString*)&Data[*offset];
    *offset += sizeof(TextString) + txt->length;

    if (*offset < size and txt->length)
        return((TextString*)&Data[*offset]);

    return(NULL);
}

/*********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************/

// RWR Class

Radar *RWR::GetFirst(long *offset)
{
    if ( not size)
        return(NULL);

    *offset = 0;
    return((Radar*)Data);
}

Radar *RWR::GetNext(long *offset)
{
    *offset += sizeof(Radar);

    if (*offset < size)
        return((Radar*)&Data[*offset]);

    return(NULL);
}

/*********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *********************************************************************************/

// Entity Class

Statistics *Entity::GetStats()
{
    Header *hdr;
    long offset;

    offset = 0;
    hdr = (Header*)&Data[offset];

    while (hdr and hdr->type not_eq _STATS_)
    {
        offset += sizeof(Header) + hdr->size;
        hdr = (Header*)&Data[offset];
    }

    return((Statistics*)hdr);
}

Description *Entity::GetDescription()
{
    Header *hdr;
    long offset;

    offset = 0;
    hdr = (Header*)&Data[offset];

    while (hdr and hdr->type not_eq _DESCRIPTION_)
    {
        offset += sizeof(Header) + hdr->size;
        hdr = (Header*)&Data[offset];
    }

    return((Description*)hdr);
}

RWR *Entity::GetRWR()
{
    Header *hdr;
    long offset;

    offset = 0;
    hdr = (Header*)&Data[offset];

    while (hdr and hdr->type not_eq _RWR_MAIN_ and hdr->type not_eq _ENTITY_)
    {
        offset += sizeof(Header) + hdr->size;
        hdr = (Header*)&Data[offset];
    }

    if (hdr->type == _RWR_MAIN_)
        return((RWR*)hdr);

    return(NULL);
}
