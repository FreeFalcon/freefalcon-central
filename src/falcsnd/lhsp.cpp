/* ------------------------------------------------------------------------

  LHSP.cpp

 Lernout bitand Hauspie Speech compression

   Version 1.02

 Written by Jim DiZoglio (x257)       (c) 1997 Microprose
 Rewritten by Dave Power

------------------------------------------------------------------------ */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "fsound.h"
#include "FalcVoice.h"
#include "debuggr.h"
#include "F4Find.h"
#pragma pack(1)
#include "landh/include/st80.h"
#pragma pack()
#include "LHSP.h"

void *map_file(char *filename);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

LHSP::LHSP(void)
{

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

LHSP::~LHSP(void)
{
    CleanupLHSP();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void LHSP::InitializeLHSP(void)
{
    CODECINFOEX CodecInfoExStruct;

    hAccess = NULL;
    PMSIZE = 0;
    CODESIZE = 0;

    ST80_GetCodecInfoEx( &CodecInfoExStruct, sizeof( CODECINFOEX ) );
    PMSIZE = CodecInfoExStruct.wInputBufferSize;
    CODESIZE = CodecInfoExStruct.wCodedBufferSize;

    if ( ( hAccess = ST80_Open_Decoder( LINEAR_PCM_16_BIT ) ) == NULL )
    {
        return;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

long LHSP::ReadLHSPFile(COMPRESSION_DATA *input, unsigned char **buffer)
{
    unsigned char *outputPtr;
    LH_ERRCODE errorCode;
    long loopCount, compDecodeSize = 0;

    if (hAccess == NULL)
        return 0;

    if (input->bytesRead >= input->compFileLength)
        return 0;

    loopCount = input->compFileLength - input->bytesRead;

    if (loopCount > MAX_INDECODE_SIZE)
    {
        loopCount = MAX_INDECODE_SIZE;
    }

    outputPtr = *buffer;

    while (loopCount > 0)
    {
        // ST80_Decode takes LPWORD (16-bit) for in/out -- use WORD locals,
        // long casts clobbered the high word with garbage (see issue #35).
        WORD inLen  = (WORD)((loopCount > CODESIZE) ? CODESIZE : loopCount);
        WORD outLen = (WORD)PMSIZE;

        /* I must check if Decode adjusts the output buffer size to use for channel struct */
        errorCode = ST80_Decode
        (
            hAccess,
            (LPBYTE)input->dataPtr,
            &inLen,
            outputPtr,
            &outLen
        );

        if (errorCode != LH_SUCCESS)
            break;

        if (inLen == 0)
            break;

        input->dataPtr += inLen;
        outputPtr += outLen;
        loopCount -= inLen;
        input->bytesRead += inLen;
        compDecodeSize += outLen;
    }

    return(compDecodeSize);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void LHSP::CleanupLHSP(void)
{
    //delete lpInputUncoded;

    if (hAccess)
    {
        ST80_Close_Decoder( hAccess );
        hAccess = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/*
void LHSP::VoiceClose( FILE *falconVoiceFile )
{
 //if (falconVoiceFile)
 // fclose( falconVoiceFile );
}*/
