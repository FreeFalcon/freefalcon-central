#include "TheaterDef.h"
#include "stdhdr.h"
#include "datafile.h"
#include "simfile.h"
#include <ctype.h>
#include "FalcSnd\voicemapper.h"
#include "entity.h"
#include "f4find.h"
#include "tactics.h"
#include "AIInput.h"
#include "Fsound.h"
#include "dispcfg.h"
#include "Falcsnd\voicefilter.h"
#include "campstr.h"

static const char THEATERLIST[] = "theater.lst";
extern char FalconDataDirectory[];
extern char FalconTerrainDataDir[];
extern char FalconObjectDataDir[];
extern char FalconMiscTexDataDir[];
extern char Falcon3DDataDir[];	// for Korea.* files

extern void TheaterReload (char *theater, char *loddata);
extern void LoadTrails();

TheaterList g_theaters;

TheaterDef *TheaterList::GetTheater(int n)
{
    TheaterDef *ptr;
    for (ptr = m_first; n > 0; n--)
	ptr = ptr->m_next;
    return ptr;
}


void TheaterList::AddTheater(TheaterDef *nt)
{
    TheaterDef **ptr;
    for(ptr = &m_first; *ptr; ptr = &(*ptr)->m_next)
	continue;
    *ptr = nt;
    m_count ++;
}

#define OFFSET(x) offsetof(TheaterDef, x)

static const InputDataDesc theaterdesc[] = {
    {"name", InputDataDesc::ID_STRING, OFFSET(m_name), ""},
    {"desc", InputDataDesc::ID_STRING, OFFSET(m_description), ""},
    {"campaigndir", InputDataDesc::ID_STRING, OFFSET(m_campaign), ""},
    {"terraindir", InputDataDesc::ID_STRING, OFFSET(m_terrain), ""},
    {"artdir", InputDataDesc::ID_STRING, OFFSET(m_artdir), ""},
    {"moviedir", InputDataDesc::ID_STRING, OFFSET(m_moviedir), ""},
    {"uisounddir", InputDataDesc::ID_STRING, OFFSET(m_uisounddir), ""},
    {"objectdir", InputDataDesc::ID_STRING, OFFSET(m_objectdir), ""},
	{"3ddatadir", InputDataDesc::ID_STRING, OFFSET(m_3ddatadir), ""},
    {"misctexdir", InputDataDesc::ID_STRING, OFFSET(m_misctexdir), ""},
    {"bitmap", InputDataDesc::ID_STRING, OFFSET(m_bitmap), ""},
	{"mintacan", InputDataDesc::ID_INT, OFFSET(m_mintacan), "70"},
	{"sounddir", InputDataDesc::ID_STRING, OFFSET(m_sounddir), ""},
    { NULL,},
};

void LoadTheaterDef(char *name)
{
    SimlibFileClass* theaterfile;
    
    theaterfile = SimlibFileClass::Open (name, SIMLIB_READ);
    if (theaterfile == NULL) return;
    
    TheaterDef *theater = new TheaterDef;
    memset(theater, 0, sizeof *theater);
    if (ParseSimlibFile(theater, theaterdesc, theaterfile) == false) {
	delete theater;
    }
    else g_theaters.AddTheater(theater);
    theaterfile->Close();
    delete theaterfile;
}

void LoadTheaterList()
{
    char tlist[_MAX_PATH];
    sprintf (tlist, "%s\\%s", FalconDataDirectory, THEATERLIST);
    FILE *fp = fopen(tlist,"r");
    if (fp == NULL) { // yikes- make something up
	return;
    }
    char line[1024];
    while (fgets(line, sizeof line, fp)) {
	if (line[0] == '\r' || line[0] == '#' || line[0] == ';' || line[0] == '\n')
	    continue;
	char *cp;
	if (cp = strchr(line, '\n'))
	    *cp = '\0';
	LoadTheaterDef(line);
    }
    fclose(fp);

}

TheaterList::~TheaterList()
{
    TheaterDef *ptr, *p2;
    for(ptr = m_first; ptr; ptr = p2) {
	p2 = ptr->m_next;
	delete ptr;
    }
    m_count = 0;
}

bool TheaterList::ChooseTheaterByName(const char *name)
{
    TheaterDef *ptr = FindTheaterByName(name);
    if (ptr)
	return SetNewTheater(ptr);
    return false;
}

#include "codelib\resources\reslib\src\resmgr.h"

// do all required to swap theaters
bool TheaterList::SetNewTheater(TheaterDef *td)
{
	FreeIndex();
    SetPathName(FalconCampaignSaveDirectory, td->m_campaign, FalconDataDirectory);
    SetPathName(FalconCampUserSaveDirectory, td->m_campaign, FalconDataDirectory);
    SetPathName(FalconTerrainDataDir, td->m_terrain, FalconDataDirectory);
    SetPathName(FalconObjectDataDir, td->m_objectdir, FalconDataDirectory);
    SetPathName(FalconMiscTexDataDir, td->m_misctexdir, FalconDataDirectory);
	SetPathName(Falcon3DDataDir, td->m_3ddatadir, FalconDataDirectory);

	ResAddPath (FalconCampaignSaveDirectory, FALSE);
	ReadIndex("Strings");
    ResSetDirectory (FalconDataDirectory);
    LoadTrails ();

    SetCurrentTheater(td);
    return true;
}

void TheaterList::DoSoundSetup() // must be done after END_UI !!
{

}


// helper function so we can have full pathnames if we want.
void TheaterList::SetPathName(char *dest, char *src, char *reldir)
{
    if (src == NULL || src[0] == 0) return; // leave alone
    if ((src[1] == ':' && isalpha(src[0])) ||
	(src[0] == '/' && src[1] == '/')) // probably full pathname
	strcpy(dest, src);
    else
	sprintf(dest, "%s\\%s", reldir, src);
}

void TheaterList::SetCurrentTheater(TheaterDef *td)
{
    HKEY	theKey;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, FALCON_REGISTRY_KEY, 0, KEY_ALL_ACCESS, &theKey);
    
    RegSetValueEx(theKey, "curTheater", 0, REG_SZ, (LPBYTE)td->m_name, strlen(td->m_name));
    RegCloseKey(theKey);
}

TheaterDef * TheaterList::GetCurrentTheater()
{
    char TheaterName[_MAX_PATH];
    HKEY	theKey;
    DWORD	size,type;
    
    
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, FALCON_REGISTRY_KEY, 0, KEY_ALL_ACCESS, &theKey);
    
    size = sizeof (TheaterName);
    if (RegQueryValueEx(theKey, "curTheater", 0, &type, (LPBYTE)TheaterName, &size) !=  ERROR_SUCCESS)
	TheaterName[0] = '\0';
    RegCloseKey(theKey);
    return FindTheaterByName(TheaterName);
}

TheaterDef * TheaterList::FindTheaterByName(const char *name)
{
    TheaterDef *ptr;
    for(ptr = m_first; ptr; ptr = ptr->m_next) {
	if (strcmpi(ptr->m_name, name) == 0) {
	    return ptr;
	}
    }
    return NULL;
}
