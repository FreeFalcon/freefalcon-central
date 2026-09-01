#include <ciso646>
#include "sim/include/stdhdr.h"
#include <tchar.h>
#include <time.h>
#include "logbook.h"
#include "f4find.h"
#include "classtbl.h"
#include "playerop.h"
#include "chandler.h"
#include "uicomms.h"
#include "campmiss.h"
#include "f4thread.h"
#include "cmpclass.h"
#include "textids.h"
#include "f4version.h"
#include "sim/include/controlsxml.h" // Artscout - 2026: per-pilot profile (logbook.xml + controls)

#pragma warning(disable : 4244) // for all the short += short's

class LogBookData LogBook;

// Artscout - 2026: bind the pilot to its profile dir (config\profiles\<dir>) so the logbook
// and the control bindings live together per pilot. The default placeholder pilot ("Viper",
// logbook never customised) maps to profiles\default; any real callsign gets a numbered dir.
// createIfNew=true (on save) registers a brand-new pilot in profiles.xml and seeds its folder.
static void SyncPilotProfile(const _TCHAR *callsign, bool createIfNew)
{
    // The default pilot "Viper" always occupies dir 0 (folder "default"); any other callsign gets
    // a numbered dir on first save. createIfNew=true (save) registers a new pilot.
    const _TCHAR *cs = (callsign and callsign[0]) ? callsign : _T("Viper");

    if (_tcsicmp(cs, _T("Viper")) == 0)
        ControlsXml_EnsureDefaultPilot(
            "Viper"); // dir 0 / folder default, dedup
    else if (createIfNew)
        ControlsXml_CreateProfile(cs); // creates dir if new, else selects it
    else
        ControlsXml_SelectProfileForCallsign(cs);
}

// Password XOR masks (also used by EncryptPwd below). The pilot password is stored XOR-encrypted
// in LB_PILOT; the XOR is symmetric, so the same pass both encrypts and decrypts.
static char PwdMask[] = "Who needs a password";
static char PwdMask2[] = "Repent, FreeFalcon is coming";

// Artscout - 2026: convert the password buffer between stored (encrypted) and plain text. We keep
// the logbook.xml password in the clear so an unset password is just "" (not the "E..." garbage
// you get from writing the encrypted empty string into a text attribute).
static void XorPassword(char *buf)
{
    for (int i = 0; i < PASSWORD_LEN; i++)
    {
        buf[i] ^= PwdMask[i % strlen(PwdMask)];
        buf[i] ^= PwdMask2[i % strlen(PwdMask2)];
    }
}

// Artscout - 2026: map between the binary LB_PILOT and the readable CxLogbook mirror used for
// profiles\<dir>\logbook.xml (so the saved logbook is human-readable, not an opaque blob).
static void PilotToCx(const LB_PILOT &p, CxLogbook &c)
{
    memset(&c, 0, sizeof(c));
    strncpy(c.name, p.Name, sizeof(c.name) - 1);
    strncpy(c.callsign, p.Callsign, sizeof(c.callsign) - 1);
    {
        char pw[PASSWORD_LEN + 1];
        memcpy(pw, p.Password, PASSWORD_LEN);
        XorPassword(pw);
        pw[PASSWORD_LEN] = 0;
        strncpy(c.password, pw, sizeof(c.password) - 1);
    } // decrypt -> plain text for the XML
    strncpy(c.commissioned, p.Commissioned, sizeof(c.commissioned) - 1);
    strncpy(c.optionsFile, p.OptionsFile, sizeof(c.optionsFile) - 1);
    strncpy(c.picture, p.Picture, sizeof(c.picture) - 1);
    strncpy(c.patch, p.Patch, sizeof(c.patch) - 1);
    strncpy(c.personal, p.Personal, sizeof(c.personal) - 1);
    strncpy(c.squadron, p.Squadron, sizeof(c.squadron) - 1);
    c.flightHours = p.FlightHours;
    c.aceFactor = p.AceFactor;
    c.rank = (int)p.Rank;
    c.voice = p.voice;
    c.pictureResource = p.PictureResource;
    c.patchResource = p.PatchResource;
    for (int i = 0; i < NUM_MEDALS and i < 8; i++)
        c.medals[i] = p.Medals[i];
    c.df_matchesWon = p.Dogfight.MatchesWon;
    c.df_matchesLost = p.Dogfight.MatchesLost;
    c.df_matchesWonVHum = p.Dogfight.MatchesWonVHum;
    c.df_matchesLostVHum = p.Dogfight.MatchesLostVHum;
    c.df_kills = p.Dogfight.Kills;
    c.df_killed = p.Dogfight.Killed;
    c.df_humanKills = p.Dogfight.HumanKills;
    c.df_killedByHuman = p.Dogfight.KilledByHuman;
    c.cmp_gamesWon = p.Campaign.GamesWon;
    c.cmp_gamesLost = p.Campaign.GamesLost;
    c.cmp_gamesTied = p.Campaign.GamesTied;
    c.cmp_missions = p.Campaign.Missions;
    c.cmp_totalScore = p.Campaign.TotalScore;
    c.cmp_totalMissionScore = p.Campaign.TotalMissionScore;
    c.cmp_consecMissions = p.Campaign.ConsecMissions;
    c.cmp_kills = p.Campaign.Kills;
    c.cmp_killed = p.Campaign.Killed;
    c.cmp_humanKills = p.Campaign.HumanKills;
    c.cmp_killedByHuman = p.Campaign.KilledByHuman;
    c.cmp_killedBySelf = p.Campaign.KilledBySelf;
    c.cmp_airToGround = p.Campaign.AirToGround;
    c.cmp_static = p.Campaign.Static;
    c.cmp_naval = p.Campaign.Naval;
    c.cmp_friendliesKilled = p.Campaign.FriendliesKilled;
    c.cmp_missSinceLastFriendlyKill = p.Campaign.MissSinceLastFriendlyKill;
}

static void CxToPilot(const CxLogbook &c, LB_PILOT &p)
{
    memset(&p, 0, sizeof(p));
    strncpy(p.Name, c.name, sizeof(p.Name) - 1);
    strncpy(p.Callsign, c.callsign, sizeof(p.Callsign) - 1);
    // Artscout - 2026: the password feature is unused; do NOT load it from XML. A stale/garbage
    // value (e.g. an encrypted blob from an older save) would make CheckPassword("") fail and pop
    // the "password required" prompt on entry. Force the encrypted-empty form so it always passes;
    // the next save rewrites logbook.xml with password="". (void)c.password keeps the field around.
    (void)c.password;
    {
        char pw[PASSWORD_LEN + 1];
        memset(pw, 0, sizeof(pw));
        XorPassword(pw);
        memcpy(p.Password, pw, PASSWORD_LEN);
    }
    strncpy(p.Commissioned, c.commissioned, sizeof(p.Commissioned) - 1);
    strncpy(p.OptionsFile, c.optionsFile, sizeof(p.OptionsFile) - 1);
    strncpy(p.Picture, c.picture, sizeof(p.Picture) - 1);
    strncpy(p.Patch, c.patch, sizeof(p.Patch) - 1);
    strncpy(p.Personal, c.personal, sizeof(p.Personal) - 1);
    strncpy(p.Squadron, c.squadron, sizeof(p.Squadron) - 1);
    p.FlightHours = c.flightHours;
    p.AceFactor = c.aceFactor;
    p.Rank = (LB_RANK)c.rank;
    p.voice = (short)c.voice;
    p.PictureResource = c.pictureResource;
    p.PatchResource = c.patchResource;
    for (int i = 0; i < NUM_MEDALS and i < 8; i++)
        p.Medals[i] = (uchar)c.medals[i];
    p.Dogfight.MatchesWon = (short)c.df_matchesWon;
    p.Dogfight.MatchesLost = (short)c.df_matchesLost;
    p.Dogfight.MatchesWonVHum = (short)c.df_matchesWonVHum;
    p.Dogfight.MatchesLostVHum = (short)c.df_matchesLostVHum;
    p.Dogfight.Kills = (short)c.df_kills;
    p.Dogfight.Killed = (short)c.df_killed;
    p.Dogfight.HumanKills = (short)c.df_humanKills;
    p.Dogfight.KilledByHuman = (short)c.df_killedByHuman;
    p.Campaign.GamesWon = (short)c.cmp_gamesWon;
    p.Campaign.GamesLost = (short)c.cmp_gamesLost;
    p.Campaign.GamesTied = (short)c.cmp_gamesTied;
    p.Campaign.Missions = (short)c.cmp_missions;
    p.Campaign.TotalScore = c.cmp_totalScore;
    p.Campaign.TotalMissionScore = c.cmp_totalMissionScore;
    p.Campaign.ConsecMissions = (short)c.cmp_consecMissions;
    p.Campaign.Kills = (short)c.cmp_kills;
    p.Campaign.Killed = (short)c.cmp_killed;
    p.Campaign.HumanKills = (short)c.cmp_humanKills;
    p.Campaign.KilledByHuman = (short)c.cmp_killedByHuman;
    p.Campaign.KilledBySelf = (short)c.cmp_killedBySelf;
    p.Campaign.AirToGround = (short)c.cmp_airToGround;
    p.Campaign.Static = (short)c.cmp_static;
    p.Campaign.Naval = (short)c.cmp_naval;
    p.Campaign.FriendliesKilled = (short)c.cmp_friendliesKilled;
    p.Campaign.MissSinceLastFriendlyKill =
        (short)c.cmp_missSinceLastFriendlyKill;
    p.CheckSum = 0;
}

#define _USE_REGISTRY_ 1
#define BAD_READ 2

int MissionResult = 0;
extern int LogState;
extern bool g_bDisableCrashEjectCourtMartials; // JB 010118

void EncryptBuffer(uchar startkey, uchar *buffer, long length);
void DecryptBuffer(uchar startkey, uchar *buffer, long length);

extern long gRanksTxt[NUM_RANKS];

LogBookData::LogBookData(void)
{
    Initialize();
}

// sfr: logbook hack
extern "C" char g_strLgbk[20];
int LogBookData::Load(void)
{
#if _USE_REGISTRY_
    DWORD type, size;
    HKEY theKey;
    long retval;

    char activeCs[64];

    if (strlen(g_strLgbk) not_eq 0)
    {
        sprintf(Pilot.Callsign, "%s", g_strLgbk);
    }
    // Artscout - 2026: prefer the last-selected pilot recorded in profiles.xml (kept current when a
    // pilot is chosen in the logbook). The rest of the pilot is filled in by LoadData from the XML.
    else if (ControlsXml_GetActiveProfileName(activeCs, sizeof(activeCs)))
    {
        _tcsncpy(Pilot.Callsign, activeCs, _CALLSIGN_LEN_);
        Pilot.Callsign[_CALLSIGN_LEN_] = 0;
    }
    else
    {
        retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, FALCON_REGISTRY_KEY, 0,
                              KEY_READ | KEY_WOW64_32KEY, &theKey);
        size = _NAME_LEN_;
        retval = RegQueryValueEx(theKey, "PilotName", 0, &type,
                                 (LPBYTE)Pilot.Name, &size);
        size = _CALLSIGN_LEN_;
        retval = RegQueryValueEx(theKey, "PilotCallsign", 0, &type,
                                 (LPBYTE)Pilot.Callsign, &size);
        RegCloseKey(theKey);

        if (retval not_eq ERROR_SUCCESS)
        {
            MonoPrint(_T("Failed to get registry entries.\n"));
            Initialize();
            return FALSE;
        }
    }

    if (not LoadData(Callsign()))
    {
        return FALSE;
    }

#else
    Initialize();
    return FALSE;
#endif

    return TRUE;
}

LogBookData::~LogBookData(void)
{
    Cleanup();
}


void LogBookData::Initialize(void)
{
    char path[MAX_PATH];

    if (gStringMgr)
    {
        _tcscpy(Pilot.Name, gStringMgr->GetString(TXT_JOE_PILOT));
    }
    else
    {
        _tcscpy(Pilot.Name, _T("Joe Pilot"));
    }

    _tcscpy(Pilot.Callsign, _T("Viper"));
    _tcscpy(Pilot.OptionsFile, _T("Default"));
    _tcscpy(Pilot.Password, _T(""));
    EncryptPwd();
    Pilot.Rank = SEC_LT;
    Pilot.AceFactor = 1.0f;
    Pilot.FlightHours = 0.0F;
    memset(&Pilot.Campaign, 0, sizeof(CAMP_STATS));
    memset(&Pilot.Dogfight, 0, sizeof(DF_STATS));
    memset(Pilot.Medals, 0, sizeof(uchar) * NUM_MEDALS);
    Pilot.Picture[0] = 0;
    Pilot.PictureResource = NOFACE;
    Pilot.Patch[0] = 0;
    Pilot.PatchResource = NOPATCH;
    Pilot.Personal[0] = 0;
    Pilot.Squadron[0] = 0;
    Pilot.voice = 0;

    SYSTEMTIME systime;
    // _TCHAR buf[COMM_LEN + 1];
    // time_t ltime;
    // struct tm *today;

    // time( &ltime );
    // today = localtime( &ltime );
    // strftime( buf, COMM_LEN, "%x", today);
    // german hack... no time
    GetSystemTime(&systime);

    if (gLangIDNum not_eq F4LANG_ENGLISH)
    {
        _stprintf(Pilot.Commissioned, "%02d.%02d.%02d", systime.wDay,
                  systime.wMonth, systime.wYear % 100);
    }
    else
    {
        _stprintf(Pilot.Commissioned, "%02d/%02d/%02d", systime.wMonth,
                  systime.wDay, systime.wYear % 100);
    }

    Pilot.CheckSum = 0;

    if (gCommsMgr)
    {
        char prof[_MAX_PATH];
        ControlsXml_ActiveProfilePath(prof, sizeof(prof));
        sprintf(path, "%s/stats.plc",
                prof); // comms stats in the profile folder
        gCommsMgr->SetStatsFile(path);
    }
}

void LogBookData::Clear(void)
{
    char path[MAX_PATH];
    _stprintf(path, _T("%s/config/%s.rul"), FalconDataDirectory,
              Pilot.Callsign);
    remove(path);
    //JAM 29Dec03 - Duh.
    // _stprintf(path,_T("%s/config/%s.pop"),FalconDataDirectory,Pilot.Callsign);
    // remove(path);
    _stprintf(path, _T("%s/config/%s.lbk"), FalconDataDirectory,
              Pilot.Callsign);
    remove(path);
    _stprintf(path, _T("%s/config/%s.plc"), FalconDataDirectory,
              Pilot.Callsign);
    remove(path);
    Initialize();
    /*
    Pilot.Rank = SEC_LT;
    Pilot.AceFactor = 1.0f;
    Pilot.FlightHours = 0.0F;
    memset(&Pilot.Campaign,0,sizeof(CAMP_STATS));
    memset(&Pilot.Dogfight,0,sizeof(DF_STATS));
    memset(Pilot.Medals,0,sizeof(uchar)*NUM_MEDALS);
    Pilot.Picture[0] = 0;
    Pilot.Patch[0] = 0;
    Pilot.Personal[0] = 0;
    Pilot.voice = 0;
    _TCHAR buf[COMM_LEN];
    _tstrdate(buf);
    _tcscpy(Pilot.Commissioned,buf);
    */
}

void LogBookData::Cleanup(void)
{
}


int LogBookData::LoadData(_TCHAR *callsign)
{
    DWORD size;
    FILE *fp;
    size_t success = 0;
    _TCHAR path[_MAX_PATH];

    ShiAssert(callsign);

    // Artscout - 2026: select this pilot's profile, then prefer profiles\<dir>\logbook.xml.
    // Legacy config\<callsign>.lbk is still read as a fallback and migrated to XML on success.
    SyncPilotProfile(callsign, false);

    bool loadedFromXml = false;
    {
        CxLogbook cx;

        if (ControlsXml_ReadLogbook(&cx))
        {
            CxToPilot(cx, Pilot);
            loadedFromXml = true;
        }
    }

    if (not loadedFromXml)
    {
        _stprintf(path, _T("%s/config/%s.lbk"), FalconDataDirectory, callsign);

        fp = _tfopen(path, _T("rb"));

        if (not fp)
        {
            MonoPrint(_T("Couldn't open %s's logbook.\n"), callsign);
            Initialize();
            return FALSE;
        }

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (size not_eq sizeof(LB_PILOT))
        {
            MonoPrint(_T("%s's logbook is old file format.\n"), callsign);
            fclose(fp);
            Initialize();
            return FALSE;
        }

        success = fread(&Pilot, sizeof(LB_PILOT), 1, fp);
        fclose(fp);

        if (success not_eq 1)
        {
            MonoPrint(_T("Failed to read %s's logbook.\n"), callsign);
            Initialize();
            return BAD_READ;
        }

        DecryptBuffer(0x58, (uchar *)&Pilot, sizeof(LB_PILOT));

        if (Pilot.CheckSum) // Somebody changed the data... init
        {
            MonoPrint("Failed checksum");
            Initialize();
            return (FALSE);
        }

        // Artscout - 2026: one-time migration of the legacy .lbk into the profile's logbook.xml.
        {
            CxLogbook cx;
            PilotToCx(Pilot, cx);
            ControlsXml_WriteLogbook(&cx);
        }
    }

    if (gCommsMgr)
    {
        char prof[_MAX_PATH];
        ControlsXml_ActiveProfilePath(prof, sizeof(prof));
        sprintf(path, "%s/stats.plc",
                prof); // comms stats in the profile folder
        gCommsMgr->SetStatsFile(path);
    }

    if (this == &LogBook)
    {
        FalconLocalSession->SetPlayerName(NameWRank());
        FalconLocalSession->SetPlayerCallsign(Callsign());
        FalconLocalSession->SetAceFactor(AceFactor());
        FalconLocalSession->SetInitAceFactor(AceFactor());
        FalconLocalSession->SetVoiceID(static_cast<uchar>(Voice()));
        PlayerOptions.LoadOptions();
        LoadAllRules(Callsign());
        LogState or_eq LB_LOADED_ONCE;
    }

    return TRUE;
}

int LogBookData::LoadData(LB_PILOT *NewPilot)
{
    if (NewPilot)
    {
        memcpy(&Pilot, NewPilot, sizeof(LB_PILOT));

        if (this == &LogBook)
        {
            SyncPilotProfile(
                Pilot.Callsign,
                false); // Artscout - 2026: point at this pilot's profile
            FalconLocalSession->SetPlayerName(NameWRank());
            FalconLocalSession->SetPlayerCallsign(Callsign());
            FalconLocalSession->SetAceFactor(AceFactor());
            FalconLocalSession->SetInitAceFactor(AceFactor());
            FalconLocalSession->SetVoiceID(static_cast<uchar>(Voice()));
            PlayerOptions.LoadOptions();
            LoadAllRules(Callsign());
            LogState or_eq LB_LOADED_ONCE;
        }

        return TRUE;
    }

    return FALSE;
}

int LogBookData::SaveData(void)
{
    _TCHAR path[_MAX_PATH];

    // Artscout - 2026: the logbook now lives ONLY in profiles\<dir>\logbook.xml (readable XML).
    // A real pilot's first save registers its profile (numbered dir) and seeds the folder. The
    // legacy encrypted config\<callsign>.lbk is no longer written.
    SyncPilotProfile(Pilot.Callsign, true);
    {
        CxLogbook cx;
        PilotToCx(Pilot, cx);
        ControlsXml_WriteLogbook(&cx);
    }

    if (gCommsMgr)
    {
        // comms stats live in the pilot's profile folder, not config\<callsign>.plc
        char prof[_MAX_PATH];
        ControlsXml_ActiveProfilePath(prof, sizeof(prof));
        sprintf(path, "%s/stats.plc", prof);
        gCommsMgr->SetStatsFile(path);
    }

#if _USE_REGISTRY_
    DWORD size;
    HKEY theKey;
    long retval;

    retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, FALCON_REGISTRY_KEY, 0,
                          KEY_ALL_ACCESS | KEY_WOW64_32KEY, &theKey);
    size = _NAME_LEN_;

    if (retval == ERROR_SUCCESS)
        retval = RegSetValueEx(theKey, "PilotName", 0, REG_BINARY,
                               (LPBYTE)Name(), size);

    size = _CALLSIGN_LEN_;

    if (retval == ERROR_SUCCESS)
        retval = RegSetValueEx(theKey, "PilotCallsign", 0, REG_BINARY,
                               (LPBYTE)Callsign(), size);

    RegCloseKey(theKey);
#endif

    if (this == &LogBook)
    {
        FalconLocalSession->SetPlayerName(NameWRank());
        FalconLocalSession->SetPlayerCallsign(Callsign());
        FalconLocalSession->SetAceFactor(AceFactor());
        FalconLocalSession->SetInitAceFactor(LogBook.AceFactor());
        FalconLocalSession->SetVoiceID(static_cast<uchar>(Voice()));
    }

    return TRUE;
}


short LogBookData::TotalKills(void)
{
    return static_cast<short>(Pilot.Campaign.Kills + Pilot.Dogfight.Kills);
}

short LogBookData::TotalKilled(void)
{
    return static_cast<short>(Pilot.Campaign.Killed + Pilot.Dogfight.Killed);
}

//static char XorMask[]="FreeFalcon Fun for the whole Family";
//static char YorMask[]="Makes other sims look like shit";

void LogBookData::Encrypt(void)
{
#if 0 // This has been replaced by a routine with a checksum to make sure the user isn't modifying the data file
    int i;
    char *ptr;

    ptr = (char *)&Pilot;

    for (i = 0; i < sizeof(LB_PILOT); i++)
    {
        *ptr xor_eq XorMask[i % strlen(XorMask)];
        *ptr xor_eq YorMask[i % strlen(YorMask)];
        ptr++;
    }

#endif
}

void LogBookData::EncryptPwd(void)
{
    int i;
    char *ptr;

    ptr = (char *)Pilot.Password;

    for (i = 0; i < PASSWORD_LEN; i++)
    {
        *ptr xor_eq PwdMask[i % strlen(PwdMask)];
        *ptr xor_eq PwdMask2[i % strlen(PwdMask2)];
        ptr++;
    }
}

int LogBookData::CheckPassword(_TCHAR *Pwd)
{
    //if(Pilot.Password[0] == 0)
    //return TRUE;

    EncryptPwd();

    if (_tcscmp(Pwd, Pilot.Password))
    {
        EncryptPwd();
        return FALSE;
    }
    else
    {
        EncryptPwd();
        return TRUE;
    }
}

int LogBookData::SetPassword(_TCHAR *Password)
{
    if (_tcslen(Password) <= PASSWORD_LEN)
    {
        _tcscpy(Pilot.Password, Password);
        EncryptPwd();
        return TRUE;
    }

    return FALSE;
}

int LogBookData::GetPassword(_TCHAR *Pwd)
{
    if (Pilot.Password[0] == 0)
        return FALSE;

    EncryptPwd();
    _tcscpy(Pwd, Pilot.Password);
    EncryptPwd();
    return TRUE;
}

_TCHAR nameWrank[MAX_PATH];

_TCHAR *LogBookData::NameWRank(void)
{
    if (gStringMgr)
    {
        _TCHAR *rank = gStringMgr->GetString(gRanksTxt[Rank()]);
        _stprintf(nameWrank, "%s %s", rank, Pilot.Name);
        return nameWrank;
    }

    return Name();
}

void LogBookData::UpdateDogfight(short MatchWon, float Hours, short VsHuman,
                                 short Kills, short Killed, short HumanKills,
                                 short KilledByHuman)
{
    MissionResult = 0;

    UpdateFlightHours(Hours);

    if (VsHuman)
    {
        if (MatchWon >= 1)
            Pilot.Dogfight.MatchesWonVHum++;
        else if (MatchWon <= -1)
            Pilot.Dogfight.MatchesLostVHum++;
    }

    if (MatchWon >= 1)
        Pilot.Dogfight.MatchesWon++;
    else if (MatchWon <= -1)
        Pilot.Dogfight.MatchesLost++;

    if (Kills > 0)
        Pilot.Dogfight.Kills += Kills;

    if (Killed > 0)
        Pilot.Dogfight.Killed += Killed;

    if (HumanKills > 0)
        Pilot.Dogfight.HumanKills += HumanKills;

    if (KilledByHuman > 0)
        Pilot.Dogfight.KilledByHuman += KilledByHuman;

    SaveData();
}

void LogBookData::UpdateCampaign(CAMP_MISS_STRUCT *MissStats)
{
    MissionResult = 0;

    UpdateFlightHours(MissStats->FlightHours);

    if (MissStats->Flags bitand CRASH_UNDAMAGED and
        not g_bDisableCrashEjectCourtMartials) // JB 010118
    {
        Pilot.Campaign.TotalScore -= 25;
        MissionResult or_eq CM_CRASH bitor COURT_MARTIAL;
    }

    if (MissStats->Flags bitand EJECT_UNDAMAGED and
        not g_bDisableCrashEjectCourtMartials) // JB 010118
    {
        Pilot.Campaign.TotalScore -= 50;
        MissionResult or_eq CM_EJECT bitor COURT_MARTIAL;
    }

    short FrKills = static_cast<short>(MissStats->FriendlyFireKills);

    while (FrKills)
    {
        if (Pilot.Campaign.FriendliesKilled == 0)
        {
            Pilot.Campaign.TotalScore -= 100;
            MissionResult or_eq CM_FR_FIRE1 bitor COURT_MARTIAL;
        }
        else if (Pilot.Campaign.FriendliesKilled == 1)
        {
            Pilot.Campaign.TotalScore -= 200;
            MissionResult or_eq CM_FR_FIRE2 bitor COURT_MARTIAL;
        }
        else
        {
            Pilot.Campaign.TotalScore -= 200;

            if (MissStats->Flags bitand FR_HUMAN_KILLED)
            {
                Pilot.Campaign.TotalScore = 0;
                Pilot.Rank = SEC_LT;
                MissionResult or_eq CM_FR_FIRE3 bitor COURT_MARTIAL;
            }
            else
                MissionResult or_eq CM_FR_FIRE2 bitor COURT_MARTIAL;
        }

        Pilot.Campaign.FriendliesKilled++;
        FrKills--;
    }

    //calculate new score using complexity, no mission pts if you get court martialed
    if (not(MissionResult bitand COURT_MARTIAL))
        Pilot.Campaign.TotalScore += FloatToInt32(
            MissStats->Score * MissionComplexity(MissStats) *
                CampaignDifficulty() * PlayerOptions.Realism / 30.0F +
            MissStats->FlightHours);

    if (Pilot.Campaign.TotalScore < 0)
        Pilot.Campaign.TotalScore = 0;

    if (not(MissStats->Flags bitand DONT_SCORE_MISSION))
    {
        Pilot.Campaign.Missions++;

        ShiAssert(MissStats->Score >= 0);
        Pilot.Campaign.TotalMissionScore += MissStats->Score;

        Pilot.Campaign.TotalScore -= MissStats->WingmenLost * 5;

        ShiAssert(MissStats->Kills >= 0);

        if (MissStats->Kills > 0)
            Pilot.Campaign.Kills += MissStats->Kills;

        if (Pilot.Campaign.Kills < 0)
            Pilot.Campaign.Kills = 0;

        ShiAssert(MissStats->Killed >= 0);

        if (MissStats->Killed > 0)
            Pilot.Campaign.Killed += MissStats->Killed;

        if (Pilot.Campaign.Killed < 0)
            Pilot.Campaign.Killed = 0;

        ShiAssert(MissStats->HumanKills >= 0);

        if (MissStats->HumanKills > 0)
            Pilot.Campaign.HumanKills += MissStats->HumanKills;

        if (Pilot.Campaign.HumanKills < 0)
            Pilot.Campaign.HumanKills = 0;

        ShiAssert(MissStats->KilledByHuman >= 0);

        if (MissStats->KilledByHuman > 0)
            Pilot.Campaign.KilledByHuman += MissStats->KilledByHuman;

        if (Pilot.Campaign.KilledByHuman < 0)
            Pilot.Campaign.KilledByHuman = 0;

        ShiAssert(MissStats->KilledBySelf >= 0);

        if (MissStats->KilledBySelf > 0)
            Pilot.Campaign.KilledBySelf += MissStats->KilledBySelf;

        if (Pilot.Campaign.KilledBySelf < 0)
            Pilot.Campaign.KilledBySelf = 0;

        ShiAssert(MissStats->GroundUnitsKilled >= 0);

        if (MissStats->GroundUnitsKilled > 0)
            Pilot.Campaign.AirToGround += MissStats->GroundUnitsKilled;

        if (Pilot.Campaign.AirToGround < 0)
            Pilot.Campaign.AirToGround = 0;

        ShiAssert(MissStats->FeaturesDestroyed >= 0);

        if (MissStats->FeaturesDestroyed > 0)
            Pilot.Campaign.Static += MissStats->FeaturesDestroyed;

        if (Pilot.Campaign.Static < 0)
            Pilot.Campaign.Static = 0;

        ShiAssert(MissStats->NavalUnitsKilled >= 0);

        if (MissStats->NavalUnitsKilled > 0)
            Pilot.Campaign.Naval += MissStats->NavalUnitsKilled;

        if (Pilot.Campaign.Naval < 0)
            Pilot.Campaign.Naval = 0;

        if (not(MissionResult bitand COURT_MARTIAL))
            AwardMedals(MissStats);
    }

    CalcRank();

    SaveData();
}

void LogBookData::CalcRank(void)
{
    LB_RANK NewRank = SEC_LT;

    if ((Pilot.Campaign.TotalScore > 3200) and Pilot.Campaign.GamesWon)
    {
        NewRank = COLONEL;
    }
    else if ((Pilot.Campaign.TotalScore > 1600) and
             (Pilot.Campaign.GamesWon or Pilot.Campaign.GamesTied))
    {
        NewRank = LT_COL;
    }
    else if ((Pilot.Campaign.TotalScore > 800) and
             (Pilot.Campaign.GamesWon or Pilot.Campaign.GamesTied or
              Pilot.Campaign.GamesLost))
    {
        NewRank = MAJOR;
    }
    else if (Pilot.Campaign.TotalScore > 300)
    {
        NewRank = CAPTAIN;
    }
    else if (Pilot.Campaign.TotalScore > 150)
    {
        NewRank = LEIUTENANT;
    }
    else
    {
        NewRank = SEC_LT;
    }

    if (NewRank > Pilot.Rank)
    {
        MissionResult or_eq PROMOTION;
        Pilot.Rank = NewRank;
    }
}

//returns value from 4 to 6
float LogBookData::MissionComplexity(CAMP_MISS_STRUCT *MissStats)
{
    float Duration;
    float WeapExpended, ShotsAt, AircrftInPkg;

    //determine mission complexity
    if (MissStats->FlightHours > 1.5f)
        Duration = 1.5f;
    else
        Duration = MissStats->FlightHours;

    if (MissStats->WeaponsExpended > 6)
        WeapExpended = 6;
    else
        WeapExpended = static_cast<float>(MissStats->WeaponsExpended);

    if (MissStats->ShotsAtPlayer > 10)
        ShotsAt = 10;
    else
        ShotsAt = static_cast<float>(MissStats->ShotsAtPlayer);

    if (MissStats->AircraftInPackage > 8)
        AircrftInPkg = 8;
    else
        AircrftInPkg = static_cast<float>(MissStats->AircraftInPackage);


    return (Duration / 3.0F + WeapExpended / 6.0F + ShotsAt / 10.0F +
            AircrftInPkg / 16.0F + 3.0F);
}

//returns value from 16 to 20
float LogBookData::CampaignDifficulty(void)
{
    return ((13.0F - TheCampaign.GroundRatio - TheCampaign.AirRatio -
             TheCampaign.AirDefenseRatio - TheCampaign.NavalRatio / 4.0F) /
                39.0F +
            (TheCampaign.EnemyAirExp + TheCampaign.EnemyADExp) / 12.0F) *
               5.0F +
           15.0F;
}

void LogBookData::AwardMedals(CAMP_MISS_STRUCT *MissStats)
{
    if (MissStats->Score >= 3)
    {
        int MedalPts = 0;

        if (MissStats->Flags bitand DESTROYED_PRIMARY)
            MedalPts += 2;

        if (MissStats->Flags bitand LANDED_AIRCRAFT)
            MedalPts++;

        if (not MissStats->WingmenLost)
            MedalPts++;

        MedalPts += MissStats->NavalUnitsKilled + MissStats->Kills +
                    min(10, MissStats->FeaturesDestroyed / 2) +
                    min(10, MissStats->GroundUnitsKilled / 2);

        MedalPts = FloatToInt32(PlayerOptions.Realism * MedalPts *
                                CampaignDifficulty() * MissStats->Score *
                                MissionComplexity(MissStats));

        if ((MedalPts > 9600) and (PlayerOptions.Realism > 0.9f) and
            MissStats->Score >= 4)
        {
            MissionResult or_eq AWARD_MEDAL bitor MDL_AFCROSS;
            Pilot.Medals[AIR_FORCE_CROSS]++;
            Pilot.Campaign.TotalScore += 20;
        }
        else if ((MedalPts > 7800) and (PlayerOptions.Realism > 0.7f))
        {
            MissionResult or_eq AWARD_MEDAL bitor MDL_SILVERSTAR;
            Pilot.Medals[SILVER_STAR]++;
            Pilot.Campaign.TotalScore += 15;
        }
        else if (MedalPts > 6000 and (PlayerOptions.Realism > 0.5f))
        {
            MissionResult or_eq AWARD_MEDAL bitor MDL_DIST_FLY;
            Pilot.Medals[DIST_FLY_CROSS]++;
            Pilot.Campaign.TotalScore += 10;
        }
        else if (MedalPts > 4800)
        {
            MissionResult or_eq AWARD_MEDAL bitor MDL_AIR_MDL;
            Pilot.Medals[AIR_MEDAL]++;
            Pilot.Campaign.TotalScore += 5;
        }
    }


    if (MissStats->Killed or MissStats->KilledByHuman or
        MissStats->KilledBySelf)
    {
        Pilot.Campaign.ConsecMissions = 0;
    }
    else
    {
        if (not PlayerOptions.InvulnerableOn())
            Pilot.Campaign.ConsecMissions++;
    }

    if (Pilot.Campaign.ConsecMissions >= 100)
    {
        MissionResult or_eq AWARD_MEDAL bitor MDL_LONGEVITY;
        Pilot.Campaign.ConsecMissions = 0;
        Pilot.Medals[LONGEVITY]++;
    }
}

void LogBookData::FinishCampaign(short WonLostTied)
{
    Pilot.Campaign.TotalScore += 10;

    if (WonLostTied > 0)
    {
        Pilot.Campaign.GamesWon++;
        Pilot.Campaign.TotalScore += 10;
        Pilot.Medals[KOREA_CAMPAIGN]++;
        MissionResult or_eq AWARD_MEDAL bitor MDL_KOR_CAMP;
    }
    else if (WonLostTied < 0)
    {
        Pilot.Campaign.GamesLost++;
    }
    else
        Pilot.Campaign.GamesTied++;

    SaveData();
}
