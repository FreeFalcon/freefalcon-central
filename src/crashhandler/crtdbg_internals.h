/*----------------------------------------------------------------------
John Robbins
Microsoft Systems Journal, October 1997 - Bugslayer
----------------------------------------------------------------------*/

#ifndef _CRTDBG_INTERNALS_H
#define _CRTDBG_INTERNALS_H

#define nNoMansLandSize 4

typedef struct _CrtMemBlockHeader
{
    struct _CrtMemBlockHeader * pBlockHeaderNext        ;
    struct _CrtMemBlockHeader * pBlockHeaderPrev        ;
    char *                      szFileName              ;
    int                         nLine                   ;
    size_t                      nDataSize               ;
    int                         nBlockUse               ;
    long                        lRequest                ;
    unsigned char               gap[nNoMansLandSize]    ;
    /* followed by:
     *  unsigned char           data[nDataSize];
     *  unsigned char           anotherGap[nNoMansLandSize];
     */
} _CrtMemBlockHeader;

#define pbData(pblock) ((unsigned char *) \
                                     ((_CrtMemBlockHeader *)pblock + 1))
#define pHdr(pbData) (((_CrtMemBlockHeader *)pbData)-1)

#endif      // _CRTDBG_INTERNALS_H


