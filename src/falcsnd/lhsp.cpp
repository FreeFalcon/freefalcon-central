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
//#include "landh/include/st80.h"
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
    //CODECINFOEX CodecInfoExStruct;
    //
    //ST80_GetCodecInfoEx( &CodecInfoExStruct, sizeof( CODECINFOEX ) );
    //PMSIZE = CodecInfoExStruct.wInputBufferSize;
    //CODESIZE = CodecInfoExStruct.wCodedBufferSize;
    //
    //// lpInputUncoded = new unsigned char[ MAXCODESIZE ];
    ////lpInputUncoded = new unsigned char[ MAX_INDECODE_SIZE ];
    //
    //if ( ( hAccess = ST80_Open_Decoder( LINEAR_PCM_16_BIT ) ) == NULL )
    //{
    // return;
    //}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

long LHSP::ReadLHSPFile(COMPRESSION_DATA *input, unsigned char **buffer)
{
    unsigned char *outputPtr;
    //unsigned long errorCode;
    long outputCodedSize;
    long loopCount, decodeSize, compDecodeSize = 0;

    if (input->bytesRead >= input->compFileLength)
        return 0;

    loopCount = input->compFileLength - input->bytesRead;

    if (loopCount > MAX_INDECODE_SIZE)
    {
        loopCount = MAX_INDECODE_SIZE;
    }

    outputCodedSize = PMSIZE;
    outputPtr = *buffer;

    while (loopCount > 0)
    {
        decodeSize = loopCount;

        if (decodeSize > CODESIZE)
            decodeSize = CODESIZE;

        /* I must check if Decode adjusts the output buffer size to use for channel struct */
        //errorCode = ST80_Decode
        //(
        // hAccess,
        // (unsigned char *)input->dataPtr,
        // ( LPWORD )&decodeSize,
        // outputPtr,
        // ( LPWORD )&outputCodedSize
        //);

        /* if ( errorCode == LH_EBADARG )
         {
         return 0;
         }
         if ( errorCode == LH_BADHANDLE )
         {
         return 0;
         }*/

        input->dataPtr += decodeSize;
        outputPtr += outputCodedSize;
        loopCount -= decodeSize;
        input->bytesRead += decodeSize;
        compDecodeSize += outputCodedSize;
    }

    return(compDecodeSize);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void LHSP::CleanupLHSP(void)
{
    //delete lpInputUncoded;

    //ST80_Close_Decoder( hAccess );
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
