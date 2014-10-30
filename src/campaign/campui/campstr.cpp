//
// Campaign strings file.
//
// Most of these are for my tool, so don't need to be double wide, but others of
// these should be double wide and come from files.
//
// KCK, Dec 9, 1996
//

#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include "Mission.h"
#include "ClassTbl.h"
#include "CampWp.h"
#include "CampStr.h"
#include "Campaign.h"
#include "Brief.h"
#include "F4Version.h"

#define NUM_CAMERA_LABELS 16

// These are used in my camp tool, so don't need to be UNICODE
#ifdef DEBUG // 2001-10-22 MODIFIED BY S.G. Used to be CAMPTOOL but gave link error when CAMPTOOL was undefined.
#include "GndUnit.h"

char TOTStr[7][6] = { "NA", "<", "<=", "=", ">=", ">", "NA" };
char TargetTypeStr[7][15] = { "Location", "Objective", "Unit" };
char OrderStr[GORD_LAST][15] = { "Reserve", "Capture", "Secure", "Assault", "Airborne", "Commando", "Defend", "Support", "Repair", "Air Defense", "Recon", "Radar" };
char FormStr[3][15] = { "None", "Line Abreast", "Wedge" };
char Side[NUM_COUNS][3] = { "XX", "US", "SK", "JA", "RU", "CH", "NK", "GO" };
#endif

char SpecialStr[3][15] = { "General", "Air to Air", "Air to Ground" };

// These are used by FreeFalcon text string builders
_TCHAR ObjectiveStr[33][20];
_TCHAR MissStr[AMIS_OTHER][20];
_TCHAR WPActStr[WP_LAST][20];
_TCHAR AirSTypesStr[20][20];
_TCHAR GroundSTypesStr[20][20];
_TCHAR NavalSTypesStr[20][20];
_TCHAR CountryNameStr[NUM_COUNS][20];
_TCHAR gUnitNameFormat[40];

// These are used by the Sim or somewhere else in the UI
char CompressionStr[5][20];
char CameraLabel[NUM_CAMERA_LABELS][40] = {"FLY-BY CAMERA", "CHASE CAMERA", "ORBIT CAMERA", "SATELLITE CAMERA", "WEAPON CAMERA", "TARGET TO WEAPON CAMERA", "ENEMY AIRCRAFT CAMERA", "FRIENDLY AIRCRAFT CAMERA", "ENEMY GROUND UNIT CAMERA", "FRIENDLY GROUND UNIT CAMERA", "INCOMING MISSILE CAMERA", "TARGET CAMERA", "TARGET TO SELF CAMERA", "ACTION CAMERA", "RECORDING"};

// Index and string information
ushort *StringIndex;
_TCHAR *StringTable;

//
// Functions
//

char* GetSTypeName(int domain, int type, int stype, char buffer[])
{
    if (domain == DOMAIN_AIR)
        _stprintf(buffer, AirSTypesStr[stype]);
    else if (domain == DOMAIN_LAND)
        _stprintf(buffer, GroundSTypesStr[stype]);
    else if (domain == DOMAIN_SEA)
        _stprintf(buffer, NavalSTypesStr[stype]);
    else
        _stprintf(buffer, AirSTypesStr[0]);

    return buffer;
}

_TCHAR* GetNumberName(int nameid, _TCHAR *buffer)
{
    _TCHAR tmp[5];

    if (nameid % 10 == 1 and nameid not_eq 11)
    {
        if (gLangIDNum == F4LANG_FRENCH)
        {
            if (nameid not_eq 1)
                ReadIndexedString(16, tmp, 5);
            else
                ReadIndexedString(15, tmp, 5);
        }
        else
            ReadIndexedString(15, tmp, 5);
    }
    else if (nameid % 10 == 2 and nameid not_eq 12)
        ReadIndexedString(16, tmp, 5);
    else if (nameid % 10 == 3 and nameid not_eq 13)
        ReadIndexedString(17, tmp, 5);
    else
        ReadIndexedString(18, tmp, 5);

    _stprintf(buffer, "%d%s", nameid, tmp);
    return buffer;
}

_TCHAR* GetTimeString(CampaignTime time, _TCHAR buffer[], int seconds)
{
    int d, h, m, s;
    _TCHAR format[MAX_STRLEN_PER_TOKEN], hour[3], minute[3], second[3];

    d = (int)(time / CampaignDay);
    time -= d * CampaignDay;
    h = (int)(time / CampaignHours);
    _stprintf(hour, "%2.2d", h);
    time -= h * CampaignHours;
    m = (int)(time / CampaignMinutes);
    _stprintf(minute, "%2.2d", m);

    if (seconds)
    {
        time -= m * CampaignMinutes;
        s = (int)(time / CampaignSeconds);
        _stprintf(second, "%2.2d", s);
    }

    // Construct the string
    if (seconds)
        ReadIndexedString(50, format, MAX_STRLEN_PER_TOKEN);
    else
        ReadIndexedString(57, format, MAX_STRLEN_PER_TOKEN);

    ConstructOrderedSentence(10, buffer, format, hour, minute, second); // PJW: my size is 10 characters
    return buffer;
}

void ReadIndex(char* filename)
{
    FILE *fp;
    short max, i;
    int size;

    if ((fp = OpenCampFile(filename, "idx", "rb")) == NULL)
        return;

    fread(&max, sizeof(ushort), 1, fp);
    StringIndex = new ushort[max];
    fread(StringIndex, sizeof(ushort), max, fp);
    CloseCampFile(fp);

    size = StringIndex[max - 1];

    if ((fp = OpenCampFile(filename, "wch", "rb")) == NULL)
        return;

    StringTable = new _TCHAR[size];
    fread(StringTable, size, 1, fp);
    CloseCampFile(fp);

    // Now fill some of our string arrays.
    for (i = 0; i <= NumObjectiveTypes; i++)
        ReadIndexedString(500 + i, ObjectiveStr[i], 19);

    for (i = 0; i < AMIS_OTHER; i++)
        ReadIndexedString(300 + i, MissStr[i], 19);

    for (i = 0; i < WP_LAST; i++)
        ReadIndexedString(350 + i, WPActStr[i], 19);

    for (i = 0; i < 20; i++)
        ReadIndexedString(540 + i, AirSTypesStr[i], 19);

    for (i = 0; i < 20; i++)
        ReadIndexedString(560 + i, GroundSTypesStr[i], 19);

    for (i = 0; i < 20; i++)
        ReadIndexedString(580 + i, NavalSTypesStr[i], 19);

    for (i = 0; i < NUM_COUNS; i++)
        ReadIndexedString(40 + i, CountryNameStr[i], 19);

    for (i = 0; i < 5; i++)
        ReadIndexedString(75 + i, CompressionStr[i], 19);

    for (i = 0; i < NUM_CAMERA_LABELS; i++)
        ReadIndexedString(60 + i, CameraLabel[i], 39);

    ReadIndexedString(58, gUnitNameFormat, 39);
}

void FreeIndex(void)
{
    delete [] StringIndex;
    StringIndex = NULL;
    delete [] StringTable;
    StringTable = NULL;
}

void ReadIndexedString(int sid, _TCHAR *wstr, int len)
{
    ushort size, rlen;

    size = StringIndex[sid + 1] - StringIndex[sid];
    rlen = size / sizeof(_TCHAR);

    if (rlen >= len)
        rlen = len - 1;

    memcpy(wstr, &StringTable[StringIndex[sid]], rlen);
    wstr[rlen] = 0;
}

void ForeignToUpper(_TCHAR *buffer)
{
    int i = 0;

    while (buffer[i])
    {
        if (gLangIDNum not_eq F4LANG_ENGLISH)
        {
            // Check for special characters
            if ((uchar)(buffer[i]) >= 224 and (uchar)(buffer[i]) <= 253)
                buffer[i] -= 32;
            else if (_istlower(buffer[i]))
                buffer[i] = _toupper(buffer[i]);

            /*
             switch (buffer[i])
             {
             case 'è':
             case 'é':
             case 'ë':
             case 'ê':
             buffer[i] = 'E';
             break;
             case 'â':
             case 'à':
             case 'á':
             case 'ä':
             buffer[i] = 'A';
             break;
             case 'ì':
             case 'í':
             case 'î':
             case 'ï':
             buffer[i] = 'I';
             break;
             case 'û':
             case 'ú':
             case 'ù':
             case 'ü':
             buffer[i] = 'U';
             break;
             case 'ç':
             buffer[i] = 'C';
             break;
             case 'ñ':
             buffer[i] = 'N';
             break;
             case 'ô':
             case 'ö':
             buffer[i] = 'O';
             break;
             default:
             if (_istlower(buffer[i]))
             buffer[i] = _toupper(buffer[i]);
             }
            */
        }
        else if (_istlower(buffer[i]))
            buffer[i] = _toupper(buffer[i]);

        i++;
    }
}

