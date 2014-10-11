#include "SIM/INCLUDE/stdhdr.h"
#include <tchar.h>
#include <time.h>
#include "logbook.h"
#include "F4find.h"
#include "classtbl.h"
#include "PlayerOp.h"
#include "chandler.h"
#include "uicomms.h"
#include "campmiss.h"
#include "F4Thread.h"
#include "cmpclass.h"
#include "textids.h"
#include "F4Version.h"

#pragma warning(disable : 4244)  // for all the short += short's

class LogBookData LogBook;

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

    if (strlen(g_strLgbk) != 0)
    {
        sprintf(Pilot.Callsign, "%s", g_strLgbk);
    }
    else
    {
        retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, FALCON_REGISTRY_KEY,
                              0, KEY_READ, &theKey);
        size = _NAME_LEN_;
        retval = RegQueryValueEx(theKey, "PilotName", 0, &type, (LPBYTE)Pilot.Name, &size);
        size = _CALLSIGN_LEN_;
        retval = RegQueryValueEx(theKey, "PilotCallsign", 0, &type, (LPBYTE)Pilot.Callsign, &size);
        RegCloseKey(theKey);

        if (retval != ERROR_SUCCESS)
        {
            MonoPrint(_T("Failed to get registry entries.\n"));
            Initialize();
            return FALSE;
        }
    }

    if (!LoadData(Callsign()))
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
    memset(Pilot.Medals, 0, sizeof(uchar)*NUM_MEDALS);
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

    if (gLangIDNum != F4LANG_ENGLISH)
    {
        _stprintf(Pilot.Commissioned, "%02d.%02d.%02d", systime.wDay, systime.wMonth, systime.wYear % 100);
    }
    else
    {
        _stprintf(Pilot.Commissioned, "%02d/%02d/%02d", systime.wMonth, systime.wDay, systime.wYear % 100);
    }

    Pilot.CheckSum = 0;

    if (gCommsMgr)
    {
        sprintf(path, "%s\\config\\%s.plc", FalconDataDirectory, Pilot.Callsign);
        gCommsMgr->SetStatsFile(path);
    }
}

void LogBookData::Clear(void)
{
    char path[MAX_PATH];
    _stprintf(path, _T("%s\\config\\%s.rul"), FalconDataDirectory, Pilot.Callsign);
    remove(path);
    //JAM 29Dec03 - Duh.
    // _stprintf(path,_T("%s\\config\\%s.pop"),FalconDataDirectory,Pilot.Callsign);
    // remove(path);
    _stprintf(path, _T("%s\\config\\%s.lbk"), FalconDataDirectory, Pilot.Callsign);
    remove(path);
    _stprintf(path, _T("%s\\config\\%s.plc"), FalconDataDirectory, Pilot.Callsign);
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

    _stprintf(path, _T("%s\\config\\%s.lbk"), FalconDataDirectory, callsign);

    fp = _tfopen(path, _T("rb"));

    if (!fp)
    {
        MonoPrint(_T("Couldn't open %s's logbook.\n"), callsign);
        Initialize();
        return FALSE;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size != sizeof(LB_PILOT))
    {
        MonoPrint(_T("%s's logbook is old file format.\n"), callsign);
        fclose(fp);
        Initialize();
        return FALSE;
    }

    success = fread(&Pilot, sizeof(LB_PILOT), 1, fp);
    fclose(fp);

    if (success != 1)
    {
        MonoPrint(_T("Failed to read %s's logbook.\n"), callsign);
        Initialize();
        return BAD_READ;
    }

    DecryptBuffer(0x58, (uchar*)&Pilot, sizeof(LB_PILOT));


    if (Pilot.CheckSum) // Somebody changed the data... init
    {
        MonoPrint("Failed checksum");
        Initialize();
        return(FALSE);
    }

    if (gCommsMgr)
    {
        sprintf(path, "%s\\config\\%s.plc", FalconDataDirectory, callsign);
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
        LogState |= LB_LOADED_ONCE;
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
            FalconLocalSession->SetPlayerName(NameWRank());
            FalconLocalSession->SetPlayerCallsign(Callsign());
            FalconLocalSession->SetAceFactor(AceFactor());
            FalconLocalSession->SetInitAceFactor(AceFactor());
            FalconLocalSession->SetVoiceID(static_cast<uchar>(Voice()));
            PlayerOptions.LoadOptions();
            LoadAllRules(Callsign());
            LogState |= LB_LOADED_ONCE;
        }

        return TRUE;
    }

    return FALSE;
}

int LogBookData::SaveData(void)
{
    FILE *fp;
    _TCHAR path[_MAX_PATH];

    _stprintf(path, _T("%s\\config\\%s.lbk"), FalconDataDirectory, Pilot.Callsign);

    if ((fp = _tfopen(path, _T("wb"))) == NULL)
    {
        MonoPrint(_T("Couldn't save logbook"));
        return FALSE;
    }

    EncryptBuffer(0x58, (uchar*)&Pilot, sizeof(LB_PILOT));

    fwrite(&Pilot, sizeof(LB_PILOT), 1, fp);
    fclose(fp);

    DecryptBuffer(0x58, (uchar*)&Pilot, sizeof(LB_PILOT));

    if (gCommsMgr)
    {
        sprintf(path, "%s\\config\\%s.plc", FalconDataDirectory, Pilot.Callsign);
        gCommsMgr->SetStatsFile(path);
    }

#if _USE_REGISTRY_
    DWORD size;
    HKEY theKey;
    long retval;

    retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, FALCON_REGISTRY_KEY,
                          0, KEY_ALL_ACCESS, &theKey);
    size = _NAME_LEN_;

    if (retval == ERROR_SUCCESS)
        retval = RegSetValueEx(theKey, "PilotName", 0, REG_BINARY, (LPBYTE)Name(), size);

    size = _CALLSIGN_LEN_;

    if (retval == ERROR_SUCCESS)
        retval = RegSetValueEx(theKey, "PilotCallsign", 0, REG_BINARY, (LPBYTE)Callsign(), size);

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

//static char XorMask[]="FreeFalcon Fun for the whole Family!!!";
//static char YorMask[]="Makes other sims look like shit!";

void LogBookData::Encrypt(void)
{
#if 0 // This has been replaced by a routine with a checksum to make sure the user isn't modifying the data file
    int i;
    char *ptr;

    ptr = (char *)&Pilot;

    for (i = 0; i < sizeof(LB_PILOT); i++)
    {
        *ptr ^= XorMask[i % strlen(XorMask)];
        *ptr ^= YorMask[i % strlen(YorMask)];
        ptr++;
    }

#endif
}

static char PwdMask[] = "Who needs a password!";
static char PwdMask2[] = "Repent, FreeFalcon is coming!";

void LogBookData::EncryptPwd(void)
{
    int i;
    char *ptr;

    ptr = (char *)Pilot.Password;

    for (i = 0; i < PASSWORD_LEN; i++)
    {
        *ptr ^= PwdMask[i % strlen(PwdMask)];
        *ptr ^= PwdMask2[i % strlen(PwdMask2)];
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

void LogBookData::UpdateDogfight(short MatchWon, float Hours, short VsHuman , short Kills, short Killed, short HumanKills, short KilledByHuman)
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
        Pilot.Dogfight.MatchesWon ++;
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

    if (MissStats->Flags & CRASH_UNDAMAGED && !g_bDisableCrashEjectCourtMartials) // JB 010118
    {
        Pilot.Campaign.TotalScore -= 25;
        MissionResult |= CM_CRASH | COURT_MARTIAL;
    }

    if (MissStats->Flags & EJECT_UNDAMAGED && !g_bDisableCrashEjectCourtMartials) // JB 010118
    {
        Pilot.Campaign.TotalScore -= 50;
        MissionResult |= CM_EJECT | COURT_MARTIAL;
    }

    short FrKills = static_cast<short>(MissStats->FriendlyFireKills);

    while (FrKills)
    {
        if (Pilot.Campaign.FriendliesKilled == 0)
        {
            Pilot.Campaign.TotalScore -= 100;
            MissionResult |= CM_FR_FIRE1 | COURT_MARTIAL;
        }
        else if (Pilot.Campaign.FriendliesKilled == 1)
        {
            Pilot.Campaign.TotalScore -= 200;
            MissionResult |= CM_FR_FIRE2 | COURT_MARTIAL;
        }
        else
        {
            Pilot.Campaign.TotalScore -= 200;

            if (MissStats->Flags & FR_HUMAN_KILLED)
            {
                Pilot.Campaign.TotalScore = 0;
                Pilot.Rank = SEC_LT;
                MissionResult |= CM_FR_FIRE3 | COURT_MARTIAL;
            }
            else
                MissionResult |= CM_FR_FIRE2 | COURT_MARTIAL;
        }

        Pilot.Campaign.FriendliesKilled++;
        FrKills--;
    }

    //calculate new score using complexity, no mission pts if you get court martialed!
    if (!(MissionResult & COURT_MARTIAL))
        Pilot.Campaign.TotalScore += FloatToInt32(MissStats->Score * MissionComplexity(MissStats) * CampaignDifficulty() *
                                     PlayerOptions.Realism / 30.0F + MissStats->FlightHours);

    if (Pilot.Campaign.TotalScore < 0)
        Pilot.Campaign.TotalScore = 0;

    if (!(MissStats->Flags & DONT_SCORE_MISSION))
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

        if (!(MissionResult & COURT_MARTIAL))
            AwardMedals(MissStats);
    }

    CalcRank();

    SaveData();
}

void LogBookData::CalcRank(void)
{
    LB_RANK NewRank = SEC_LT;

    if ((Pilot.Campaign.TotalScore > 3200) && Pilot.Campaign.GamesWon)
    {
        NewRank = COLONEL;
    }
    else if ((Pilot.Campaign.TotalScore > 1600) && \
             (Pilot.Campaign.GamesWon || Pilot.Campaign.GamesTied))
    {
        NewRank = LT_COL;
    }
    else if ((Pilot.Campaign.TotalScore > 800) && \
             (Pilot.Campaign.GamesWon || Pilot.Campaign.GamesTied || Pilot.Campaign.GamesLost))
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
        MissionResult |= PROMOTION;
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


    return (Duration / 3.0F + WeapExpended / 6.0F + ShotsAt / 10.0F + AircrftInPkg / 16.0F + 3.0F);
}

//returns value from 16 to 20
float LogBookData::CampaignDifficulty(void)
{
    return ((13.0F - TheCampaign.GroundRatio - TheCampaign.AirRatio -
             TheCampaign.AirDefenseRatio - TheCampaign.NavalRatio / 4.0F) / 39.0F  +
            (TheCampaign.EnemyAirExp + TheCampaign.EnemyADExp) / 12.0F) * 5.0F + 15.0F;
}

void LogBookData::AwardMedals(CAMP_MISS_STRUCT *MissStats)
{
    if (MissStats->Score >= 3)
    {
        int MedalPts = 0;

        if (MissStats->Flags & DESTROYED_PRIMARY)
            MedalPts += 2;

        if (MissStats->Flags & LANDED_AIRCRAFT)
            MedalPts++;

        if (!MissStats->WingmenLost)
            MedalPts++;

        MedalPts += MissStats->NavalUnitsKilled + MissStats->Kills +
                    min(10, MissStats->FeaturesDestroyed / 2) + min(10, MissStats->GroundUnitsKilled / 2);

        MedalPts = FloatToInt32(PlayerOptions.Realism * MedalPts * CampaignDifficulty() * MissStats->Score * MissionComplexity(MissStats));

        if ((MedalPts > 9600) && (PlayerOptions.Realism > 0.9f) && MissStats->Score >= 4)
        {
            MissionResult |= AWARD_MEDAL | MDL_AFCROSS;
            Pilot.Medals[AIR_FORCE_CROSS]++;
            Pilot.Campaign.TotalScore += 20;
        }
        else if ((MedalPts > 7800) && (PlayerOptions.Realism > 0.7f))
        {
            MissionResult |= AWARD_MEDAL | MDL_SILVERSTAR;
            Pilot.Medals[SILVER_STAR]++;
            Pilot.Campaign.TotalScore += 15;
        }
        else if (MedalPts > 6000 && (PlayerOptions.Realism > 0.5f))
        {
            MissionResult |= AWARD_MEDAL | MDL_DIST_FLY;
            Pilot.Medals[DIST_FLY_CROSS]++;
            Pilot.Campaign.TotalScore += 10;
        }
        else if (MedalPts > 4800)
        {
            MissionResult |= AWARD_MEDAL | MDL_AIR_MDL;
            Pilot.Medals[AIR_MEDAL]++;
            Pilot.Campaign.TotalScore += 5;
        }
    }


    if (MissStats->Killed || MissStats->KilledByHuman || MissStats->KilledBySelf)
    {
        Pilot.Campaign.ConsecMissions = 0;
    }
    else
    {
        if (!PlayerOptions.InvulnerableOn())
            Pilot.Campaign.ConsecMissions++;
    }

    if (Pilot.Campaign.ConsecMissions >= 100)
    {
        MissionResult |= AWARD_MEDAL | MDL_LONGEVITY;
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
        MissionResult |= AWARD_MEDAL | MDL_KOR_CAMP;
    }
    else if (WonLostTied < 0)
    {
        Pilot.Campaign.GamesLost++;
    }
    else
        Pilot.Campaign.GamesTied++;

    SaveData();
}
