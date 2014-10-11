#include "F4Thread.h"
#include "falclib.h"
#include "dispcfg.h"
#include "sinput.h"

static const struct dilookup
{
    HRESULT hr;
    char *value;
} DiErrTable[] =
{
#define Err(x) { x, #x }
    Err(DI_BUFFEROVERFLOW),
    Err(DI_DOWNLOADSKIPPED),
    Err(DI_EFFECTRESTARTED),
    Err(DI_NOEFFECT),
    Err(DI_NOTATTACHED),
    Err(DI_OK),
    Err(DI_POLLEDDEVICE),
    Err(DI_PROPNOEFFECT),
#ifdef DI_SETTINGSNOTSAVED
    Err(DI_SETTINGSNOTSAVED),
#endif
    Err(DI_TRUNCATED),
    Err(DI_TRUNCATEDANDRESTARTED),
#ifdef DI_WRITEPROTECT
    Err(DI_WRITEPROTECT),
#endif
    Err(DIERR_ACQUIRED),
    Err(DIERR_ALREADYINITIALIZED),
    Err(DIERR_BADDRIVERVER),
    Err(DIERR_BETADIRECTINPUTVERSION),
    Err(DIERR_DEVICEFULL),
    Err(DIERR_DEVICENOTREG),
    Err(DIERR_EFFECTPLAYING),
    Err(DIERR_GENERIC),
    Err(DIERR_HANDLEEXISTS),
    Err(DIERR_HASEFFECTS),
    Err(DIERR_INCOMPLETEEFFECT),
    Err(DIERR_INPUTLOST),
    Err(DIERR_INVALIDPARAM),
#ifdef DIERR_MAPFILEFAIL
    Err(DIERR_MAPFILEFAIL),
#endif
    Err(DIERR_MOREDATA),
    Err(DIERR_NOAGGREGATION),
    Err(DIERR_NOINTERFACE),
    Err(DIERR_NOTACQUIRED),
    Err(DIERR_NOTBUFFERED),
    Err(DIERR_NOTDOWNLOADED),
    Err(DIERR_NOTEXCLUSIVEACQUIRED),
    Err(DIERR_NOTFOUND),
    Err(DIERR_NOTINITIALIZED),
    Err(DIERR_OBJECTNOTFOUND),
    Err(DIERR_OLDDIRECTINPUTVERSION),
    Err(DIERR_OTHERAPPHASPRIO),
    Err(DIERR_OUTOFMEMORY),
    Err(DIERR_READONLY),
    Err(DIERR_REPORTFULL),
    Err(DIERR_UNPLUGGED),
    Err(DIERR_UNSUPPORTED),
    Err(E_HANDLE),
    Err(E_PENDING),
    Err(E_POINTER)
};
#undef Err
static const int MAX_ERR = sizeof(DiErrTable) / sizeof(DiErrTable[0]);

BOOL VerifyResult(HRESULT hResult)
{

    if (hResult == DI_OK)
    {
        return TRUE;
    }

    /*
    #ifdef DEBUG
    for (int i = 0; i < MAX_ERR; i++) {
    if (DiErrTable[i].hr == hResult) {
        MonoPrint ("Sim Input//DInput Error: %x, %s\n", hResult, DiErrTable[i].value);
        return FALSE;
    }
    }
    MonoPrint ("Sim Input//DInput Error: %x, Unknown err\n", hResult);
    #endif
    */
    return(FALSE);
}

BOOL DIMessageBox(int ErrNum, int Type, char* pErrStr)
{
    int Response;
    char pOutStr[100];

    sprintf(pOutStr, "Sim Input//DInput Error: %d, %s\n", ErrNum, pErrStr);
    Response = MessageBox(FalconDisplay.appWin, pErrStr, "FreeFalcon Sim Input Error", Type);

    return (Response == IDYES);
}

