///////////////////////////////////////////////////////////////////////////////
// Class wrapper for memory mapped files - Julian Onions
#ifndef _FILE_MEM_MAP_H
#define _FILE_MEM_MAP_H
#include <windows.h>


class FileMemMap {
    HANDLE m_hFile; // the open file
    HANDLE m_hMap; // the mapping handle
    BYTE *m_Data; // the mapped data
    int m_len; // the length of the data
    void Clear();
public:
    FileMemMap();
    ~FileMemMap();

    BOOL Open(const char *filename, BOOL rw = FALSE, BOOL nomap = FALSE); // open it
    void Close(); // release stoarage and stuff
    BYTE *GetData(int offset, int len); // access data at given offset
    HANDLE GetFileHandle() { return m_hFile; };
    BOOL ReadDataAt(DWORD offset, void *buffer, DWORD size);
    BOOL IsReady() { return m_hFile != INVALID_HANDLE_VALUE; };
};

#endif
