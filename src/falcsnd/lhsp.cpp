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
    hAccess = NULL;
    PMSIZE = 0;
    CODESIZE = 0;

    // Artscout - 2026 (x64): ST80 (Lernout & Hauspie StreamTalk80) is a 32-bit-only codec DLL with
    // no x64 build. On x64 NO_ST80 keeps hAccess NULL -> ReadLHSPFile returns 0 -> voice chatter is
    // silent (use subtitles meanwhile). TODO: replace with a modern codec / TTS path.
#ifndef NO_ST80
    CODECINFOEX CodecInfoExStruct;

    ST80_GetCodecInfoEx( &CodecInfoExStruct, sizeof( CODECINFOEX ) );
    PMSIZE = CodecInfoExStruct.wInputBufferSize;
    CODESIZE = CodecInfoExStruct.wCodedBufferSize;

    if ( ( hAccess = ST80_Open_Decoder( LINEAR_PCM_16_BIT ) ) == NULL )
    {
        return;
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Artscout - 2026: when the voice data is the pre-transcoded PCM .tlk (falcon_pcm.tlk), the block
// "data" is already-decoded PCM (no ST80). VoiceManager sets this after opening that file. In PCM
// mode ReadLHSPFile is a plain chunked copy -> voice works on BOTH x86 and x64 with no ST80 dep.
bool g_bVoicePcmMode = false;

long LHSP::ReadLHSPFile(COMPRESSION_DATA *input, unsigned char **buffer)
{
    unsigned char *outputPtr;
    LH_ERRCODE errorCode;
    long loopCount, compDecodeSize = 0;

    // Artscout - 2026: PCM passthrough (transcoded falcon_pcm.tlk). Copy up to MAX_OUTDECODE_SIZE
    // already-decoded PCM bytes per call; the VoiceManager streaming loop keeps calling until
    // bytesRead == compFileLength. No ST80 needed.
    if (g_bVoicePcmMode)
    {
        long remaining = input->compFileLength - input->bytesRead;

        if (remaining <= 0)
            return 0;

        long chunk = (remaining > MAX_OUTDECODE_SIZE) ? MAX_OUTDECODE_SIZE : remaining;
        memcpy(*buffer, input->dataPtr, chunk);
        input->dataPtr  += chunk;
        input->bytesRead += chunk;
        return chunk;
    }

    if (hAccess == NULL)
        return 0;   // Artscout - 2026 (x64/NO_ST80): always NULL -> silent, ST80 path below never built/run

#ifdef NO_ST80
    return 0;
#else
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
#endif // NO_ST80
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void LHSP::CleanupLHSP(void)
{
    //delete lpInputUncoded;

#ifndef NO_ST80
    if (hAccess)
    {
        ST80_Close_Decoder( hAccess );
        hAccess = NULL;
    }
#endif
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
