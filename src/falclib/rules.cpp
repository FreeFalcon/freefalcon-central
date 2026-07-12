#include <cISO646>
#include "PlayerOp.h"
//#include "stdhdr.h"
#include "f4find.h"
RulesClass gRules[rNUM_MODES];
RulesModes RuleMode = rINSTANT_ACTION;

// Artscout - 2026: per-pilot rules live in the active profile folder (config\profiles\<dir>\
// rules.rul), not config\<callsign>.rul. Forward-declared to avoid pulling the sim controlsxml
// header into falclib; the symbol resolves at the final exe link.
extern bool ControlsXml_ActiveProfilePath(char *out, int outSize);

int LoadAllRules(char *filename)
{
    size_t success = 0;
    _TCHAR path[_MAX_PATH];
    char  prof[_MAX_PATH];
    long size;
    FILE *fp;

    ControlsXml_ActiveProfilePath(prof, sizeof(prof));
    _stprintf(path, _T("%s\\rules.rul"), prof);

    fp = _tfopen(path, _T("rb"));

    if ( not fp)
    {
        MonoPrint(_T("Couldn't open %s rules file\n"), filename);
        // fallback: shipped default profile, then the legacy config\default.rul
        _stprintf(path, _T("%s\\config\\profiles\\default\\rules.rul"), FalconDataDirectory);
        fp = _tfopen(path, _T("rb"));

        if ( not fp)
        {
            _stprintf(path, _T("%s\\config\\default.rul"), FalconDataDirectory);
            fp = _tfopen(path, "rb");
        }

        if ( not fp)
        {
            MonoPrint(_T("Couldn't open default rules\n"), filename);
            return FALSE;
        }
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size not_eq sizeof(RulesStruct) * rNUM_MODES)
    {
        MonoPrint(_T("%s's rules are in old file format\n"), filename);
        return FALSE;
    }


    RulesClass tempRules[rNUM_MODES];

    success = fread(&tempRules, sizeof(RulesStruct), rNUM_MODES, fp);
    fclose(fp);

    if (success not_eq rNUM_MODES)
    {
        MonoPrint(_T("Failed to read %s's rules file\n"), filename);
        //Initialize();
        return FALSE;
    }

    for (int i = 0; i < rNUM_MODES; i++)
    {
        char dataFileName[_MAX_PATH];
        sprintf(dataFileName, "%s\\atc.ini", FalconCampaignSaveDirectory);
        tempRules[i].BumpTimer = max(0, GetPrivateProfileInt("ATC", "PlayerBumpTime", 10, dataFileName));
        tempRules[i].BumpTimer *= 60000;
        tempRules[i].AiPullTime = max(0, GetPrivateProfileInt("ATC", "AIPullTime", 20, dataFileName));
        tempRules[i].AiPullTime *= 60000;
        tempRules[i].AiPatience = max(0, GetPrivateProfileInt("ATC", "AIPatience", 120, dataFileName));
        tempRules[i].AiPatience *= 1000;
        tempRules[i].AtcPatience = max(0, GetPrivateProfileInt("ATC", "ATCPatience", 180, dataFileName));
        tempRules[i].AtcPatience *= 1000;
    }

    memcpy(&gRules, &tempRules, sizeof(RulesStruct)*rNUM_MODES);
    return TRUE;
}

RulesClass::RulesClass(void)
{
    Initialize();
}

void RulesClass::Initialize(void)
{
    memset(Password, 0, sizeof(_TCHAR)*RUL_PW_LEN);
    MaxPlayers = 16;
    ObjMagnification = 5;
    SimFlags = SIM_RULES_FLAGS; // Sim flags
    SimFlightModel = FMSimplified; // Flight model type
    SimWeaponEffect = WEExaggerated;
    SimAvionicsType = ATEasy;
    SimAutopilotType = APIntelligent;
    SimAirRefuelingMode = ARSimplistic;
    SimPadlockMode = PDEnhanced;
    GeneralFlags = GEN_RULES_FLAGS;

    char dataFileName[_MAX_PATH];
    sprintf(dataFileName, "%s\\atc.ini", FalconCampaignSaveDirectory);
    BumpTimer = max(0, GetPrivateProfileInt("ATC", "PlayerBumpTime", 10, dataFileName));
    BumpTimer *= 60000;
    AiPullTime = max(0, GetPrivateProfileInt("ATC", "AIPullTime", 20, dataFileName));
    AiPullTime *= 60000;
    AiPatience = max(0, GetPrivateProfileInt("ATC", "AIPatience", 120, dataFileName));
    AiPatience *= 1000;
    AtcPatience = max(0, GetPrivateProfileInt("ATC", "ATCPatience", 180, dataFileName));
    AtcPatience *= 1000;
}

int RulesClass::LoadRules(char *filename)
{
    size_t success = 0;
    _TCHAR path[_MAX_PATH];
    char  prof[_MAX_PATH];
    long size;
    FILE *fp;

    ControlsXml_ActiveProfilePath(prof, sizeof(prof));
    _stprintf(path, _T("%s\\rules.rul"), prof);

    fp = _tfopen(path, _T("rb"));

    if ( not fp)
    {
        MonoPrint(_T("Couldn't open %s rules file\n"), filename);
        // fallback: shipped default profile, then the legacy config\default.rul
        _stprintf(path, _T("%s\\config\\profiles\\default\\rules.rul"), FalconDataDirectory);
        fp = _tfopen(path, _T("rb"));

        if ( not fp)
        {
            _stprintf(path, _T("%s\\config\\default.rul"), FalconDataDirectory);
            fp = _tfopen(path, "rb");
        }

        if ( not fp)
        {
            MonoPrint(_T("Couldn't open default rules\n"), filename);
            Initialize();
            return FALSE;
        }
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size not_eq sizeof(RulesStruct) * rNUM_MODES)
    {
        MonoPrint(_T("%s's rules are in old file format\n"), filename);
        return FALSE;
    }


    RulesClass tempRules[rNUM_MODES];

    success = fread(&tempRules, sizeof(RulesStruct), rNUM_MODES, fp);
    fclose(fp);

    if (success not_eq rNUM_MODES)
    {
        MonoPrint(_T("Failed to read %s's rules file\n"), filename);
        //Initialize();
        return FALSE;
    }

    char dataFileName[_MAX_PATH];
    sprintf(dataFileName, "%s\\atc.ini", FalconCampaignSaveDirectory);
    tempRules[RuleMode].BumpTimer = max(0, GetPrivateProfileInt("ATC", "PlayerBumpTime", 10, dataFileName));
    tempRules[RuleMode].BumpTimer *= 60000;
    tempRules[RuleMode].AiPullTime = max(0, GetPrivateProfileInt("ATC", "AIPullTime", 20, dataFileName));
    tempRules[RuleMode].AiPullTime *= 60000;
    tempRules[RuleMode].AiPatience = max(0, GetPrivateProfileInt("ATC", "AIPatience", 120, dataFileName));
    tempRules[RuleMode].AiPatience *= 1000;
    tempRules[RuleMode].AtcPatience = max(0, GetPrivateProfileInt("ATC", "ATCPatience", 180, dataFileName));
    tempRules[RuleMode].AtcPatience *= 1000;

    memcpy(this, &(tempRules[RuleMode]), sizeof(RulesStruct));
    return TRUE;
}

void RulesClass::LoadRules(RulesStruct *rules)
{
    if (rules)
        memcpy(this, rules, sizeof(RulesStruct));

    /*
    _tcscpy(Password,rules->Password);
    MaxPlayers = rules->MaxPlayers;
    ObjMagnification = rules->ObjMagnification;
    SimFlags = rules->SimFlags; // Sim flags
    SimFlightModel = rules->SimFlightModel; // Flight model type
    SimWeaponEffect = rules->SimWeaponEffect;
    SimAvionicsType = rules->SimAvionicsType;
    SimAutopilotType = rules->SimAutopilotType;
    SimAirRefuelingMode = rules->SimAirRefuelingMode;
    SimPadlockMode = rules->SimPadlockMode;
    GeneralFlags = rules->GeneralFlags;*/
}

int RulesClass::SaveRules(_TCHAR *filename)
{
    FILE *fp;
    _TCHAR path[_MAX_PATH];
    char  prof[_MAX_PATH];
    size_t success = 0;

    ControlsXml_ActiveProfilePath(prof, sizeof(prof));   // rules in the profile folder
    _stprintf(path, _T("%s\\rules.rul"), prof);
    (void)filename;

    if ((fp = _tfopen(path, "wb")) == NULL)
    {
        MonoPrint(_T("Couldn't save rules"));
        return FALSE;
    }

    success = fwrite(gRules, sizeof(RulesStruct), rNUM_MODES, fp);
    fclose(fp);

    if (success not_eq rNUM_MODES)
    {
        MonoPrint(_T("Couldn't save rules"));
        return FALSE;
    }

    return TRUE;
}

static char PwdMask[] = "Blood makes the grass grow, kill, kill, kill";
static char PwdMask2[] = "ojodp^&SANDsfsl,[poe5487wqer1]@&$N";

void RulesClass::EncryptPwd(void)
{
    int i;
    _TCHAR *ptr;

    ptr = Password;

    for (i = 0; i < RUL_PW_LEN; i++)
    {
        *ptr xor_eq PwdMask[i % strlen(PwdMask)];
        *ptr xor_eq PwdMask2[i % strlen(PwdMask2)];
        ptr++;
    }
}

int RulesClass::CheckPassword(_TCHAR *Pwd)
{
    //if(Pilot.Password[0] == 0)
    //return TRUE;

    //EncryptPwd();
    if (_tcscmp(Pwd, Password))
    {
        //EncryptPwd();
        return FALSE;
    }
    else
    {
        //EncryptPwd();
        return TRUE;
    }
}

int RulesClass::SetPassword(_TCHAR *newPassword)
{
    if (_tcslen(newPassword) <= RUL_PW_LEN)
    {
        _tcscpy(Password, newPassword);
        //EncryptPwd();
        return TRUE;
    }

    return FALSE;
}

int RulesClass::GetPassword(_TCHAR *Pwd)
{
    //EncryptPwd();
    _tcscpy(Pwd, Password);
    //EncryptPwd();
    return TRUE;
}
