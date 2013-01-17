///
// FileMemMap - implementation
// Julian Onions - initial revision

#include "FileMemMap.h"

FileMemMap::FileMemMap()
{
    Clear();
}

FileMemMap::~FileMemMap()
{
    Close();
}

void FileMemMap::Close()
{
    if (m_Data) UnmapViewOfFile(m_Data);
    if (m_hMap != INVALID_HANDLE_VALUE) CloseHandle(m_hMap);
    if (m_hFile != INVALID_HANDLE_VALUE) CloseHandle(m_hFile);
    Clear();
}

void FileMemMap::Clear()
{
    m_hFile = INVALID_HANDLE_VALUE;
    m_hMap = INVALID_HANDLE_VALUE;
    m_Data = NULL;
    m_len = 0;
}

BOOL FileMemMap::Open(const char *filename, BOOL rw, BOOL nomap)
{
    m_hFile = CreateFile(
	filename,
	rw == TRUE ? GENERIC_WRITE : GENERIC_READ,
	rw == TRUE ? FILE_SHARE_WRITE : FILE_SHARE_READ,
	NULL,
	OPEN_EXISTING,
	FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
	NULL
	);
    if (m_hFile == INVALID_HANDLE_VALUE)
	return FALSE;
    m_len = GetFileSize(m_hFile, NULL);
    if (nomap == TRUE) return TRUE; // we don't really want to map it
    m_hMap = CreateFileMapping(
	m_hFile,
	NULL,
	rw == TRUE ? (PAGE_READWRITE) : (PAGE_READONLY | SEC_COMMIT),
	0,
	0,
	NULL
	);
    
    if (m_hMap == NULL)
	return FALSE;
    
    m_Data = (BYTE *)MapViewOfFileEx(
	m_hMap,
	rw == TRUE ? FILE_MAP_WRITE : FILE_MAP_READ,
	0,
	0,
	0,
	NULL
	);
    return m_Data == NULL ? FALSE : TRUE;
}

BYTE *FileMemMap::GetData(int offset, int len)
{
    if (m_hMap == INVALID_HANDLE_VALUE || offset < 0 || offset + len > m_len)
	return NULL;
    return &m_Data[offset];
}

// abstraction for seek/read behaviour
BOOL FileMemMap::ReadDataAt(DWORD offset, void *buffer, DWORD size) 
{
    int	result = SetFilePointer(m_hFile, offset, NULL, FILE_BEGIN);
    if (result == -1) return FALSE;
    DWORD bytesRead;
    result = ReadFile( m_hFile, buffer, size, &bytesRead, NULL );
    if (result == -1 || bytesRead != size)
	return FALSE;
    return TRUE;
}