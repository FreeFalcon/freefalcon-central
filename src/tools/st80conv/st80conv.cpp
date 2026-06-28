/* ------------------------------------------------------------------------
  st80conv.cpp -- standalone ST80 -> PCM voice-bank transcoder.

  Artscout - 2026. Decodes an ST80 (Lernout & Hauspie StreamTalk80) .tlk voice
  bank (falcon.tlk) into a PCM .tlk (falcon_pcm.tlk) with the SAME index layout
  but already-decoded PCM data (compressedlen == filelen == PCM length). At
  runtime FreeFalcon plays that PCM bank through the passthrough path in
  LHSP::ReadLHSPFile (g_bVoicePcmMode), so the 32-bit-only ST80 codec can be
  dropped from BOTH the x86 and x64 game builds once the bank is generated.

  This is a SELF-CONTAINED x86 console tool: it does NOT include or depend on any
  other FreeFalcon source -- only <windows.h>, the ST80 header, and st80w.lib /
  ST80W.dll. Build it once (Win32), run it once, ship falcon_pcm.tlk.

  Mirrors the in-engine transcoder (TlkFile::TranscodeToPcm in voicemanager.cpp)
  and the decode loop (LHSP::ReadLHSPFile in lhsp.cpp) byte-for-byte, so the
  produced bank is identical to what the engine's -mkvoice path would write.

  .tlk format:
    [0x00] 3 x int32 header (only the index past offset 12 is read at runtime)
    [0x0C] int32 index[count]  -- each entry = byte offset of that fragment's
                                  block, or <= 0 for an empty slot.
    block: { uint32 filelen; uint32 compressedlen; byte data[compressedlen] }

  Usage:  st80conv [input.tlk] [output.tlk]
          defaults: falcon.tlk -> falcon_pcm.tlk (current directory)
------------------------------------------------------------------------ */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(1)
#include "st80.h"
#pragma pack()

// Mirror the engine's LHSP sizing (lhsp.h).
#define MAX_OUTDECODE_SIZE 80960
#define TLK_HEADER_INFO    12

// Read the whole file into a malloc'd buffer. Returns NULL on failure; *outLen set.
static unsigned char *ReadWholeFile(const char *path, long *outLen)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (len <= 0) { fclose(fp); return NULL; }

    unsigned char *buf = (unsigned char *)malloc(len);
    if (!buf) { fclose(fp); return NULL; }

    if (fread(buf, 1, len, fp) != (size_t)len) { free(buf); fclose(fp); return NULL; }
    fclose(fp);

    *outLen = len;
    return buf;
}

// Safe little-endian int32 read from the in-memory .tlk at byte offset 'off'.
static long ReadI32(const unsigned char *buf, long bufLen, long off)
{
    if (off < 0 || off + 4 > bufLen) return 0;
    long v;
    memcpy(&v, buf + off, sizeof(v));
    return v;
}

int main(int argc, char **argv)
{
    const char *inPath  = (argc > 1) ? argv[1] : "falcon.tlk";
    const char *outPath = (argc > 2) ? argv[2] : "falcon_pcm.tlk";

    printf("st80conv: %s -> %s\n", inPath, outPath);

    long srcLen = 0;
    unsigned char *src = ReadWholeFile(inPath, &srcLen);
    if (!src)
    {
        fprintf(stderr, "ERROR: cannot read '%s'\n", inPath);
        return 1;
    }

    // ---- Derive fragment count + first block offset (same scan as TranscodeToPcm).
    // The index is int32[] starting at TLK_HEADER_INFO; its lowest positive entry
    // points at the first block, which sits right after the index.
    long firstOff = 0x7fffffff;
    int  count = 0;
    for (int i = 0; (long)(TLK_HEADER_INFO + (long)sizeof(long) * i) < firstOff; i++)
    {
        long pos = TLK_HEADER_INFO + (long)sizeof(long) * i;
        if (pos + 4 > srcLen) break;
        long off = ReadI32(src, srcLen, pos);
        if (off > 0 && off < firstOff) firstOff = off;
        count = i + 1;
    }

    if (count <= 0)
    {
        fprintf(stderr, "ERROR: '%s' has no fragments (not a valid .tlk?)\n", inPath);
        free(src);
        return 1;
    }
    printf("st80conv: %d fragments\n", count);

    // ---- Open the ST80 decoder (same flags as LHSP::InitializeLHSP).
    CODECINFOEX info;
    ST80_GetCodecInfoEx(&info, sizeof(CODECINFOEX));
    const long PMSIZE   = info.wInputBufferSize;   // PCM bytes per decode call
    const long CODESIZE = info.wCodedBufferSize;   // coded bytes per decode call

    HANDLE hDec = ST80_Open_Decoder(LINEAR_PCM_16_BIT);
    if (hDec == NULL)
    {
        fprintf(stderr, "ERROR: ST80_Open_Decoder failed (is ST80W.dll next to the exe?)\n");
        free(src);
        return 1;
    }

    long *index = (long *)calloc(count, sizeof(long));
    FILE *fp = fopen(outPath, "wb");
    if (!index || !fp)
    {
        fprintf(stderr, "ERROR: cannot create '%s'\n", outPath);
        if (fp) fclose(fp);
        free(index); free(src);
        ST80_Close_Decoder(hDec);
        return 1;
    }

    // 12-byte header (runtime only reads the index from offset 12).
    long header[3] = { (long)count, 0, 0 };
    fwrite(header, sizeof(header), 1, fp);

    long indexPos = ftell(fp);                 // == TLK_HEADER_INFO (12)
    fwrite(index, sizeof(long), count, fp);    // placeholder; rewritten at the end

    unsigned char *outBuf = (unsigned char *)malloc(PMSIZE + 16); // one decode call
    long  pcmCap = MAX_OUTDECODE_SIZE * 4;
    unsigned char *pcm = (unsigned char *)malloc(pcmCap);         // whole fragment

    int decoded = 0, empty = 0, failed = 0;

    for (int i = 0; i < count; i++)
    {
        long blockOff = ReadI32(src, srcLen, TLK_HEADER_INFO + (long)sizeof(long) * i);
        if (blockOff <= 0 || blockOff + 8 > srcLen)
        {
            index[i] = 0;   // empty slot
            empty++;
            continue;
        }

        // long filelen      = ReadI32(src, srcLen, blockOff);          // uncompressed (unused)
        long compressedlen   = ReadI32(src, srcLen, blockOff + 4);
        const unsigned char *dataPtr = src + blockOff + 8;

        if (compressedlen <= 0 || blockOff + 8 + compressedlen > srcLen)
        {
            index[i] = 0;
            empty++;
            continue;
        }

        // ---- Decode the whole fragment (same inner loop as LHSP::ReadLHSPFile).
        long pcmLen = 0;
        long bytesRead = 0;
        bool ok = true;
        while (bytesRead < compressedlen)
        {
            // ST80_Decode takes LPWORD (16-bit) for in/out lengths -- use WORD locals,
            // long casts clobber the high word with garbage (FreeFalcon issue #35).
            WORD inLen  = (WORD)((compressedlen - bytesRead > CODESIZE) ? CODESIZE : (compressedlen - bytesRead));
            WORD outLen = (WORD)PMSIZE;

            LH_ERRCODE err = ST80_Decode(hDec,
                                         (LPBYTE)(dataPtr + bytesRead), &inLen,
                                         outBuf, &outLen);
            if (err != LH_SUCCESS) { ok = false; break; }
            if (inLen == 0) break;

            if (pcmLen + outLen > pcmCap)
            {
                while (pcmLen + outLen > pcmCap) pcmCap *= 2;
                pcm = (unsigned char *)realloc(pcm, pcmCap);
            }
            memcpy(pcm + pcmLen, outBuf, outLen);
            pcmLen    += outLen;
            bytesRead += inLen;
        }

        if (!ok || pcmLen <= 0)
        {
            index[i] = 0;
            failed++;
            continue;
        }

        index[i] = ftell(fp);
        unsigned long len = (unsigned long)pcmLen;
        fwrite(&len, sizeof(len), 1, fp);   // filelen      (== PCM length)
        fwrite(&len, sizeof(len), 1, fp);   // compressedlen(== PCM length in the PCM bank)
        fwrite(pcm, 1, pcmLen, fp);
        decoded++;
    }

    // Rewrite the real index now that block offsets are known.
    fseek(fp, indexPos, SEEK_SET);
    fwrite(index, sizeof(long), count, fp);
    fclose(fp);

    ST80_Close_Decoder(hDec);
    free(pcm); free(outBuf); free(index); free(src);

    printf("st80conv: done -- %d decoded, %d empty, %d failed\n", decoded, empty, failed);
    return (decoded > 0) ? 0 : 2;
}
