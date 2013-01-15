// ***************************************************************************
// Name.h
//
// Stuff used to deal with naming
// ***************************************************************************

// ===============================
// Global functions
// ===============================

extern void LoadNames (char* filename);

extern void LoadNameStream (void);

extern int SaveNames (char* filename);

extern void FreeNames (void);

extern _TCHAR* ReadNameString (int sid, _TCHAR *wstr, unsigned int len);

extern int AddName (_TCHAR *name);

extern int SetName (int nameid, _TCHAR *name);

extern int FindName (_TCHAR* name);

extern void RemoveName (int nid);

extern void RemoveName (_TCHAR* name);
