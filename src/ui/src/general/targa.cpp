//
//
//
// TARGA.CPP
//
//
// Created by Kyle Granger, October 23, 1995
//
//
//
//

#include <windows.h>
#include <stdio.h>
#include "falclib.h"

#ifdef USE_SH_POOLS
#include "SmartHeap/Include/shmalloc.h"
#include "SmartHeap/Include/smrtheap.hpp"
#endif

#include "targa.h"
#include "ui.h"
#include "F4Thread.h"
#include "f4find.h"

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
// ALL RESMGR CODE ADDITIONS AND END HERE

typedef struct
{
    char Header[12];
    WORD Width;
    WORD Height;
    char Junk[2];
} BOGUS_HEADER;

BOOL LoadTargaFile(char *filename, char **image, BITMAPINFO *bmi)
{
    UI_HANDLE hFile;
    char *data;
    BOGUS_HEADER Targa;
    long bytesToRead;

    hFile = UI_OPEN(filename, "rb");

    if (hFile == NULL)
        return FALSE;

    // For 15-bit Targa file, skip first 12 bytes.
    if (UI_READ(&Targa, sizeof(BOGUS_HEADER), 1, hFile) not_eq 1)
    {
        UI_CLOSE(hFile);
        return NULL;
    }

    // Read in image data
    F4Assert( not (Targa.Width > 3000 or Targa.Height > 3000));

    bytesToRead = Targa.Width * Targa.Height * 2;

    if ( not bytesToRead)
        return(NULL);

    data = new char [bytesToRead];
    F4Assert(data);

    if (data == NULL)
        return(NULL);

    if (UI_READ(data, bytesToRead, 1, hFile) not_eq 1)
    {
        UI_CLOSE(hFile);
        return NULL;
    }

    UI_CLOSE(hFile);

    *image = data;

    bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi->bmiHeader.biWidth = Targa.Width;
    bmi->bmiHeader.biHeight = Targa.Height;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biBitCount = 16;
    bmi->bmiHeader.biCompression = BI_RGB;
    bmi->bmiHeader.biSizeImage = 0;
    bmi->bmiHeader.biXPelsPerMeter = 72;
    bmi->bmiHeader.biYPelsPerMeter = 72;
    bmi->bmiHeader.biClrUsed = 0;
    bmi->bmiHeader.biClrImportant = 0;

    return TRUE;
}

BOOL NonResLoadTargaFile(char *filename, char **image, BITMAPINFO *bmi)
{
    HANDLE hFile;
    char buf[16];
    char *data;
    WORD width, height;
    DWORD bytesToRead, bytesread;

    hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    if ( not ReadFile(hFile, buf, 12, &bytesread, NULL))
    {
        CloseHandle(hFile);
        return(FALSE);
    }

    // Read width
    if ( not ReadFile(hFile, &width, sizeof(width), &bytesread, NULL))
    {
        CloseHandle(hFile);
        return(FALSE);
    }

    // Read height
    if ( not ReadFile(hFile, &height, sizeof(height), &bytesread, NULL))
    {
        CloseHandle(hFile);
        return(FALSE);
    }

    // For 15-bit Targa file, skip last 2 bytes.
    if ( not ReadFile(hFile, buf, 2, &bytesread, NULL))
    {
        CloseHandle(hFile);
        return(FALSE);
    }

    // Read in image data
    bytesToRead = width * height * 2;
    data = new char [bytesToRead];

    if ( not ReadFile(hFile, data, bytesToRead, &bytesread, NULL))
    {
        CloseHandle(hFile);
        return(FALSE);
    }

    CloseHandle(hFile);

    *image = data;

    bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi->bmiHeader.biWidth = width;
    bmi->bmiHeader.biHeight = height;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biBitCount = 16;
    bmi->bmiHeader.biCompression = BI_RGB;
    bmi->bmiHeader.biSizeImage = 0;
    bmi->bmiHeader.biXPelsPerMeter = 72;
    bmi->bmiHeader.biYPelsPerMeter = 72;
    bmi->bmiHeader.biClrUsed = 0;
    bmi->bmiHeader.biClrImportant = 0;

    return TRUE;
}


