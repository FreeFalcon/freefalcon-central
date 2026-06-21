// controlsxml.cpp - #53 XML control settings. The only place that uses tinyxml2.
// controls.xml (catalog) is generated offline; profile bindings are read/written here.
#include "stdhdr.h"

#include <map>
#include <string>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>

// tinyxml2 BEFORE falclib.h: falclib does #define new DEBUG_NEW, which breaks the inline methods
// of tinyxml2::XMLDocument in tinyxml2.h (XMLDocument becomes "incomplete"). STL above - before windows/min-max.
#include "extlibs/tinyxml2/tinyxml2.h"   // include path ..\.. = src\

// Same preamble as the working siloop.cpp: falclib + sinput expand the windows/dinput/types
// needed by simio.h (otherwise simio.h's body - DeviceAxis/AxisMapping/SIM_NUMDEVICES - won't expand).
#include "falclib.h"
#include "sinput.h"
#include "simio.h"                         // AxisMapping / DeviceAxis / SIM_NUMDEVICES
#include "datadir.h"                        // FalconDataDirectory

#include "controlsxml.h"

// NOT "using namespace tinyxml2" - there is another global XMLDocument (MSXML/Falcon headers
// via falclib/windows) -> C2872 ambiguity. We qualify the types as tinyxml2::.

// ===========================================================================
// Catalog (name -> label/category)
// ===========================================================================
static std::map<std::string, std::string> g_label;
static std::map<std::string, std::string> g_category;

static void cfgPath(char *out, const char *rel)
{
    sprintf(out, "%s\\config\\%s", FalconDataDirectory, rel);
}

int ControlsXml_LoadCatalog(void)
{
    g_label.clear();
    g_category.clear();

    char path[_MAX_PATH];
    cfgPath(path, "controls.xml");

    tinyxml2::XMLDocument doc;

    if (doc.LoadFile(path) != tinyxml2::XML_SUCCESS)
        return 0;

    tinyxml2::XMLElement *root  = doc.FirstChildElement("controls");
    tinyxml2::XMLElement *funcs = root ? root->FirstChildElement("functions") : NULL;

    if (not funcs)
        return 0;

    int cnt = 0;

    for (tinyxml2::XMLElement *fe = funcs->FirstChildElement("function"); fe; fe = fe->NextSiblingElement("function"))
    {
        const char *name = fe->Attribute("name");

        if (not name)
            continue;

        const char *label = fe->Attribute("label");
        const char *cat   = fe->Attribute("category");

        g_label[name] = label ? label : name;

        if (cat)
            g_category[name] = cat;

        cnt++;
    }

    return cnt;
}

const char *ControlsXml_GetLabel(const char *funcName)
{
    if (not funcName)
        return NULL;

    std::map<std::string, std::string>::iterator it = g_label.find(funcName);
    return (it != g_label.end()) ? it->second.c_str() : NULL;
}

const char *ControlsXml_GetCategory(const char *funcName)
{
    if (not funcName)
        return NULL;

    std::map<std::string, std::string>::iterator it = g_category.find(funcName);
    return (it != g_category.end()) ? it->second.c_str() : "";
}

// ===========================================================================
// Helpers
// ===========================================================================
static int iattr(tinyxml2::XMLElement *e, const char *n, int def)
{
    const char *a = e->Attribute(n);
    return a ? (int)strtol(a, NULL, 0) : def;   // base 0: hex (0x..), dec, negatives
}

// Active profile token. Defaults to "default"; the logbook switches it per pilot via
// ControlsXml_SelectProfileForCallsign / ControlsXml_CreateProfile.
char g_controlsProfile[64] = "default";

void ControlsXml_SetProfile(const char *name)
{
    if (name and name[0])
    {
        strncpy(g_controlsProfile, name, sizeof(g_controlsProfile) - 1);
        g_controlsProfile[sizeof(g_controlsProfile) - 1] = 0;
    }
}

bool ControlsXml_ActiveProfilePath(char *out, int outSize)
{
    _snprintf(out, outSize, "%s\\config\\profiles\\%s", FalconDataDirectory, g_controlsProfile);
    out[outSize - 1] = 0;
    return true;
}

static void ensureProfileDir(void)
{
    char p[_MAX_PATH];
    sprintf(p, "%s\\config\\profiles", FalconDataDirectory);
    _mkdir(p);
    ControlsXml_ActiveProfilePath(p, sizeof(p));
    _mkdir(p);
}

// ===========================================================================
// profiles.xml - list of pilot profiles. Each created pilot gets a numeric folder
// dir (1,2,3...) under config\profiles\<dir>\. The default pilot (logbook never
// customised) lives in config\profiles\default\. g_controlsProfile holds the active
// folder token ("default" or a number) - all profile paths are built from it.
// ===========================================================================
struct CxProfileRec { std::string name; int dir; };

static void profilesXmlPath(char *out) { cfgPath(out, "profiles.xml"); }

static void profileDirPathFor(char *out, int outSize, const char *token)
{
    _snprintf(out, outSize, "%s\\config\\profiles\\%s", FalconDataDirectory, token);
    out[outSize - 1] = 0;
}

static void ensureProfileDirFor(const char *token)
{
    char p[_MAX_PATH];
    sprintf(p, "%s\\config\\profiles", FalconDataDirectory);
    _mkdir(p);
    profileDirPathFor(p, sizeof(p), token);
    _mkdir(p);
}

static bool copyFileSimple(const char *src, const char *dst)
{
    FILE *fs = fopen(src, "rb");
    if (not fs) return false;
    FILE *fd = fopen(dst, "wb");
    if (not fd) { fclose(fs); return false; }
    char buf[4096];
    size_t r;
    while ((r = fread(buf, 1, sizeof(buf), fs)) > 0) fwrite(buf, 1, r, fd);
    fclose(fs);
    fclose(fd);
    return true;
}

// Copy the shipped default profile's keyboard (config\profiles\default\keyboard.xml) into a
// profile folder that has no layout of its own yet. force=true overwrites (used for a new
// profile). The "default" profile is shipped by the installer, so it is never seeded here.
static void seedKeyboardFor(const char *token, bool force)
{
    if (strcmp(token, "default") == 0)
        return;                          // default ships its own keyboard.xml

    char dir[_MAX_PATH], dst[_MAX_PATH], src[_MAX_PATH];
    profileDirPathFor(dir, sizeof(dir), token);
    sprintf(dst, "%s\\keyboard.xml", dir);

    if (not force)
    {
        FILE *t = fopen(dst, "rb");
        if (t) { fclose(t); return; }   // already has its own - leave it
    }

    profileDirPathFor(src, sizeof(src), "default");      // template = shipped default profile
    strncat(src, "\\keyboard.xml", sizeof(src) - strlen(src) - 1);
    copyFileSimple(src, dst);
}

static bool profilesRead(std::vector<CxProfileRec> &out, std::string *active)
{
    char path[_MAX_PATH];
    profilesXmlPath(path);

    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(path) != tinyxml2::XML_SUCCESS)
        return false;

    tinyxml2::XMLElement *root = doc.FirstChildElement("profiles");
    if (not root)
        return false;

    if (active)
    {
        const char *a = root->Attribute("active");
        *active = a ? a : "";
    }

    for (tinyxml2::XMLElement *p = root->FirstChildElement("profile"); p; p = p->NextSiblingElement("profile"))
    {
        const char *nm = p->Attribute("name");
        if (not nm)
            continue;
        CxProfileRec rec;
        rec.name = nm;
        rec.dir  = iattr(p, "dir", 0);
        out.push_back(rec);
    }

    return true;
}

static bool profilesWrite(const std::vector<CxProfileRec> &list, const char *active)
{
    char cfg[_MAX_PATH];
    sprintf(cfg, "%s\\config", FalconDataDirectory);
    _mkdir(cfg);

    tinyxml2::XMLDocument doc;
    doc.InsertEndChild(doc.NewDeclaration());
    tinyxml2::XMLElement *root = doc.NewElement("profiles");
    if (active and active[0])
        root->SetAttribute("active", active);
    root->SetAttribute("version", 1);
    doc.InsertEndChild(root);

    for (size_t i = 0; i < list.size(); ++i)
    {
        tinyxml2::XMLElement *p = doc.NewElement("profile");
        p->SetAttribute("name", list[i].name.c_str());
        p->SetAttribute("dir", list[i].dir);
        root->InsertEndChild(p);
    }

    char path[_MAX_PATH];
    profilesXmlPath(path);
    return doc.SaveFile(path) == tinyxml2::XML_SUCCESS;
}

// First run: create profiles.xml seeded with the default pilot ("Viper", the engine's default
// callsign) at dir 0 -> folder "default". Real pilots created later get dir 1,2,3... (max+1,
// so the dir-0 floor keeps them >= 1). The pilot LIST in the UI is built from these entries.
static void ensureProfilesXml(void)
{
    char path[_MAX_PATH];
    profilesXmlPath(path);

    FILE *t = fopen(path, "rb");
    if (t) { fclose(t); return; }        // already exists

    std::vector<CxProfileRec> list;
    CxProfileRec def;
    def.name = "Viper";                  // matches LogBookData::Initialize default callsign
    def.dir  = 0;                        // dir 0 == default pilot == folder "default"
    list.push_back(def);
    profilesWrite(list, "Viper");        // default pilot active

    ensureProfileDirFor("default");
    seedKeyboardFor("default", false);
}

// Map a callsign to its profile folder token. Pure lookup, no state change.
// dir 0 is reserved for our default pilot -> folder "default"; real pilots use "1","2",...
static void profileTokenForCallsign(const char *callsign, char *out, int outSize)
{
    _snprintf(out, outSize, "default");
    out[outSize - 1] = 0;

    if (callsign and callsign[0])
    {
        std::vector<CxProfileRec> list;
        if (profilesRead(list, NULL))
            for (size_t i = 0; i < list.size(); ++i)
                if (_stricmp(list[i].name.c_str(), callsign) == 0)
                {
                    if (list[i].dir <= 0)
                        _snprintf(out, outSize, "default");   // dir 0 = default pilot
                    else
                        _snprintf(out, outSize, "%d", list[i].dir);
                    out[outSize - 1] = 0;
                    break;
                }
    }
}

// Persist <profiles active="..."> (records the last-selected pilot) without disturbing the list.
static void profilesSetActive(const char *activeCallsign)
{
    std::vector<CxProfileRec> list;
    std::string cur;
    bool have = profilesRead(list, &cur);
    const char *a = activeCallsign ? activeCallsign : "";

    if (have and cur == a)
        return;                       // already current - no rewrite
    if (not have and not a[0])
        return;                       // nothing to persist yet

    profilesWrite(list, a);
}

// Select the active profile by pilot callsign (logbook loads/selects a pilot) and remember it
// as the active pilot in profiles.xml so the choice survives across launches. If the pilot is
// not in profiles.xml we stay on "default" (a numeric folder is created only by CreateProfile).
void ControlsXml_SelectProfileForCallsign(const char *callsign)
{
    ensureProfilesXml();

    char token[64];
    profileTokenForCallsign(callsign, token, sizeof(token));

    ControlsXml_SetProfile(token);
    ensureProfileDirFor(token);
    seedKeyboardFor(token, false);   // default is seeded from config\keyboard.xml if empty
    profilesSetActive(callsign);     // remember the last selection for the next launch
}

// Restore g_controlsProfile from the persisted active pilot (profiles.xml) at startup, so the
// last logbook selection is active everywhere (options, in-sim bindings) even before the logbook
// UI is opened. Safe to call repeatedly; it does not change the stored active.
void ControlsXml_RestoreActiveProfile(void)
{
    ensureProfilesXml();

    std::vector<CxProfileRec> list;
    std::string active;
    profilesRead(list, &active);

    char token[64];
    profileTokenForCallsign(active.c_str(), token, sizeof(token));

    ControlsXml_SetProfile(token);
    ensureProfileDirFor(token);
    seedKeyboardFor(token, false);
}

// Create a profile for a NEW pilot: next dir = max+1, append to profiles.xml,
// make config\profiles\<dir>\, copy the master keyboard there, and make it active.
void ControlsXml_CreateProfile(const char *callsign)
{
    if (not callsign or not callsign[0])
        return;

    ensureProfilesXml();

    std::vector<CxProfileRec> list;
    profilesRead(list, NULL);

    // already exists - just activate it
    for (size_t i = 0; i < list.size(); ++i)
        if (_stricmp(list[i].name.c_str(), callsign) == 0)
        {
            ControlsXml_SelectProfileForCallsign(callsign);
            return;
        }

    int maxDir = 0;
    for (size_t i = 0; i < list.size(); ++i)
        if (list[i].dir > maxDir)
            maxDir = list[i].dir;

    CxProfileRec rec;
    rec.name = callsign;
    rec.dir  = maxDir + 1;
    list.push_back(rec);

    char token[64];
    sprintf(token, "%d", rec.dir);

    profilesWrite(list, callsign);
    ControlsXml_SetProfile(token);
    ensureProfileDirFor(token);
    seedKeyboardFor(token, true);    // fresh copy of the master keyboard into the new profile
}

// Active pilot callsign from profiles.xml (the last-selected pilot). true if non-empty.
bool ControlsXml_GetActiveProfileName(char *out, int outSize)
{
    ensureProfilesXml();

    std::vector<CxProfileRec> list;
    std::string active;
    profilesRead(list, &active);

    _snprintf(out, outSize, "%s", active.c_str());
    out[outSize - 1] = 0;
    return out[0] != 0;
}

// Ensure the default pilot is the single dir-0 entry (folder "default") and make it active.
// Renames whatever sits at dir 0 to `callsign` and drops any other entry with that name, so the
// default pilot can never be duplicated (e.g. a legacy "default" placeholder + a "Viper" copy).
void ControlsXml_EnsureDefaultPilot(const char *callsign)
{
    if (not callsign or not callsign[0])
        callsign = "Viper";

    ensureProfilesXml();

    std::vector<CxProfileRec> list;
    profilesRead(list, NULL);

    std::vector<CxProfileRec> cleaned;
    bool haveZero = false;

    for (size_t i = 0; i < list.size(); ++i)
    {
        if (list[i].dir == 0)
        {
            if (haveZero) continue;            // keep only one dir-0 entry
            CxProfileRec d; d.name = callsign; d.dir = 0;
            cleaned.push_back(d);              // (re)name the dir-0 slot to the default callsign
            haveZero = true;
        }
        else if (_stricmp(list[i].name.c_str(), callsign) == 0)
        {
            continue;                          // drop a spurious non-default copy of the default
        }
        else
        {
            cleaned.push_back(list[i]);
        }
    }

    if (not haveZero)
    {
        CxProfileRec d; d.name = callsign; d.dir = 0;
        cleaned.insert(cleaned.begin(), d);
    }

    profilesWrite(cleaned, callsign);          // default pilot active
    ControlsXml_SetProfile("default");
    ensureProfileDirFor("default");
    seedKeyboardFor("default", false);
}

// Rename a pilot's profiles.xml entry (oldName -> newName), keeping the same dir/folder so the
// profile's data (logbook.xml/keyboard/etc.) stays put. Updates 'active' if it pointed at oldName.
void ControlsXml_RenameProfile(const char *oldName, const char *newName)
{
    if (not oldName or not newName or not newName[0] or _stricmp(oldName, newName) == 0)
        return;

    ensureProfilesXml();

    std::vector<CxProfileRec> list;
    std::string active;
    profilesRead(list, &active);

    bool found = false;
    for (size_t i = 0; i < list.size(); ++i)
        if (_stricmp(list[i].name.c_str(), oldName) == 0)
        {
            list[i].name = newName;
            found = true;
            break;
        }

    if (not found)
        return;

    const char *newActive = (_stricmp(active.c_str(), oldName) == 0) ? newName : active.c_str();
    profilesWrite(list, newActive);
}

// List the pilot callsigns from profiles.xml (for the logbook pilot list). Returns the count.
int ControlsXml_ListProfiles(char out[][24], int maxN)
{
    ensureProfilesXml();

    std::vector<CxProfileRec> list;
    profilesRead(list, NULL);

    int n = 0;
    for (size_t i = 0; i < list.size() and n < maxN; ++i)
    {
        strncpy(out[n], list[i].name.c_str(), 23);
        out[n][23] = 0;
        n++;
    }
    return n;
}

// ===========================================================================
// logbook.xml - pilot data in the active profile folder, as human-readable XML.
// logbook.cpp maps LB_PILOT <-> CxLogbook (a plain mirror), so this module needs no UI headers.
// ===========================================================================
static float fattr(tinyxml2::XMLElement *e, const char *n, float def)
{
    const char *a = e->Attribute(n);
    return a ? (float)atof(a) : def;
}

static void sattr(tinyxml2::XMLElement *e, const char *n, char *out, int outSize)
{
    const char *a = e->Attribute(n);
    if (a) { strncpy(out, a, outSize - 1); out[outSize - 1] = 0; }
    else out[0] = 0;
}

bool ControlsXml_WriteLogbook(const CxLogbook *p)
{
    if (not p)
        return false;

    ensureProfileDir();

    tinyxml2::XMLDocument doc;
    doc.InsertEndChild(doc.NewDeclaration());
    tinyxml2::XMLElement *root = doc.NewElement("logbook");
    root->SetAttribute("version", 2);
    doc.InsertEndChild(root);

    tinyxml2::XMLElement *pilot = doc.NewElement("pilot");
    pilot->SetAttribute("name", p->name);
    pilot->SetAttribute("callsign", p->callsign);
    pilot->SetAttribute("squadron", p->squadron);
    pilot->SetAttribute("commissioned", p->commissioned);
    pilot->SetAttribute("rank", p->rank);
    pilot->SetAttribute("flightHours", p->flightHours);
    pilot->SetAttribute("aceFactor", p->aceFactor);
    pilot->SetAttribute("voice", p->voice);
    pilot->SetAttribute("optionsFile", p->optionsFile);
    pilot->SetAttribute("password", p->password);
    pilot->SetAttribute("picture", p->picture);
    pilot->SetAttribute("pictureRes", p->pictureResource);
    pilot->SetAttribute("patch", p->patch);
    pilot->SetAttribute("patchRes", p->patchResource);
    pilot->SetAttribute("personal", p->personal);
    root->InsertEndChild(pilot);

    tinyxml2::XMLElement *med = doc.NewElement("medals");
    char a[8];
    for (int i = 0; i < 8; i++) { sprintf(a, "m%d", i); med->SetAttribute(a, p->medals[i]); }
    root->InsertEndChild(med);

    tinyxml2::XMLElement *df = doc.NewElement("dogfight");
    df->SetAttribute("matchesWon", p->df_matchesWon);
    df->SetAttribute("matchesLost", p->df_matchesLost);
    df->SetAttribute("matchesWonVHum", p->df_matchesWonVHum);
    df->SetAttribute("matchesLostVHum", p->df_matchesLostVHum);
    df->SetAttribute("kills", p->df_kills);
    df->SetAttribute("killed", p->df_killed);
    df->SetAttribute("humanKills", p->df_humanKills);
    df->SetAttribute("killedByHuman", p->df_killedByHuman);
    root->InsertEndChild(df);

    tinyxml2::XMLElement *cm = doc.NewElement("campaign");
    cm->SetAttribute("gamesWon", p->cmp_gamesWon);
    cm->SetAttribute("gamesLost", p->cmp_gamesLost);
    cm->SetAttribute("gamesTied", p->cmp_gamesTied);
    cm->SetAttribute("missions", p->cmp_missions);
    cm->SetAttribute("totalScore", p->cmp_totalScore);
    cm->SetAttribute("totalMissionScore", p->cmp_totalMissionScore);
    cm->SetAttribute("consecMissions", p->cmp_consecMissions);
    cm->SetAttribute("kills", p->cmp_kills);
    cm->SetAttribute("killed", p->cmp_killed);
    cm->SetAttribute("humanKills", p->cmp_humanKills);
    cm->SetAttribute("killedByHuman", p->cmp_killedByHuman);
    cm->SetAttribute("killedBySelf", p->cmp_killedBySelf);
    cm->SetAttribute("airToGround", p->cmp_airToGround);
    cm->SetAttribute("static", p->cmp_static);
    cm->SetAttribute("naval", p->cmp_naval);
    cm->SetAttribute("friendliesKilled", p->cmp_friendliesKilled);
    cm->SetAttribute("missSinceLastFriendlyKill", p->cmp_missSinceLastFriendlyKill);
    root->InsertEndChild(cm);

    char prof[_MAX_PATH], path[_MAX_PATH];
    ControlsXml_ActiveProfilePath(prof, sizeof(prof));
    sprintf(path, "%s\\logbook.xml", prof);
    return doc.SaveFile(path) == tinyxml2::XML_SUCCESS;
}

bool ControlsXml_ReadLogbook(CxLogbook *out)
{
    if (not out)
        return false;

    char prof[_MAX_PATH], path[_MAX_PATH];
    ControlsXml_ActiveProfilePath(prof, sizeof(prof));
    sprintf(path, "%s\\logbook.xml", prof);

    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(path) != tinyxml2::XML_SUCCESS)
        return false;

    tinyxml2::XMLElement *root = doc.FirstChildElement("logbook");
    if (not root)
        return false;

    memset(out, 0, sizeof(*out));

    tinyxml2::XMLElement *pilot = root->FirstChildElement("pilot");
    if (pilot)
    {
        sattr(pilot, "name", out->name, sizeof(out->name));
        sattr(pilot, "callsign", out->callsign, sizeof(out->callsign));
        sattr(pilot, "squadron", out->squadron, sizeof(out->squadron));
        sattr(pilot, "commissioned", out->commissioned, sizeof(out->commissioned));
        out->rank = iattr(pilot, "rank", 0);
        out->flightHours = fattr(pilot, "flightHours", 0.f);
        out->aceFactor = fattr(pilot, "aceFactor", 1.f);
        out->voice = iattr(pilot, "voice", 0);
        sattr(pilot, "optionsFile", out->optionsFile, sizeof(out->optionsFile));
        sattr(pilot, "password", out->password, sizeof(out->password));
        sattr(pilot, "picture", out->picture, sizeof(out->picture));
        out->pictureResource = iattr(pilot, "pictureRes", 0);
        sattr(pilot, "patch", out->patch, sizeof(out->patch));
        out->patchResource = iattr(pilot, "patchRes", 0);
        sattr(pilot, "personal", out->personal, sizeof(out->personal));
    }

    tinyxml2::XMLElement *med = root->FirstChildElement("medals");
    if (med)
    {
        char a[8];
        for (int i = 0; i < 8; i++) { sprintf(a, "m%d", i); out->medals[i] = iattr(med, a, 0); }
    }

    tinyxml2::XMLElement *df = root->FirstChildElement("dogfight");
    if (df)
    {
        out->df_matchesWon      = iattr(df, "matchesWon", 0);
        out->df_matchesLost     = iattr(df, "matchesLost", 0);
        out->df_matchesWonVHum  = iattr(df, "matchesWonVHum", 0);
        out->df_matchesLostVHum = iattr(df, "matchesLostVHum", 0);
        out->df_kills           = iattr(df, "kills", 0);
        out->df_killed          = iattr(df, "killed", 0);
        out->df_humanKills      = iattr(df, "humanKills", 0);
        out->df_killedByHuman   = iattr(df, "killedByHuman", 0);
    }

    tinyxml2::XMLElement *cm = root->FirstChildElement("campaign");
    if (cm)
    {
        out->cmp_gamesWon                  = iattr(cm, "gamesWon", 0);
        out->cmp_gamesLost                 = iattr(cm, "gamesLost", 0);
        out->cmp_gamesTied                 = iattr(cm, "gamesTied", 0);
        out->cmp_missions                  = iattr(cm, "missions", 0);
        out->cmp_totalScore                = iattr(cm, "totalScore", 0);
        out->cmp_totalMissionScore         = iattr(cm, "totalMissionScore", 0);
        out->cmp_consecMissions            = iattr(cm, "consecMissions", 0);
        out->cmp_kills                     = iattr(cm, "kills", 0);
        out->cmp_killed                    = iattr(cm, "killed", 0);
        out->cmp_humanKills                = iattr(cm, "humanKills", 0);
        out->cmp_killedByHuman             = iattr(cm, "killedByHuman", 0);
        out->cmp_killedBySelf              = iattr(cm, "killedBySelf", 0);
        out->cmp_airToGround               = iattr(cm, "airToGround", 0);
        out->cmp_static                    = iattr(cm, "static", 0);
        out->cmp_naval                     = iattr(cm, "naval", 0);
        out->cmp_friendliesKilled          = iattr(cm, "friendliesKilled", 0);
        out->cmp_missSinceLastFriendlyKill = iattr(cm, "missSinceLastFriendlyKill", 0);
    }

    return true;
}

// ===========================================================================
// keyboard.xml
// ===========================================================================
int ControlsXml_ReadKeyboard(CxKbBind *out, int maxN)
{
    char prof[_MAX_PATH], path[_MAX_PATH];
    ControlsXml_ActiveProfilePath(prof, sizeof(prof));
    sprintf(path, "%s\\keyboard.xml", prof);

    tinyxml2::XMLDocument doc;

    if (doc.LoadFile(path) != tinyxml2::XML_SUCCESS)
        return 0;

    tinyxml2::XMLElement *root = doc.FirstChildElement("keyboard");

    if (not root)
        return 0;

    int n = 0;

    for (tinyxml2::XMLElement *b = root->FirstChildElement("bind"); b and n < maxN; b = b->NextSiblingElement("bind"))
    {
        const char *fn = b->Attribute("function");

        if (not fn)
            continue;

        strncpy(out[n].func, fn, sizeof(out[n].func) - 1);
        out[n].func[sizeof(out[n].func) - 1] = 0;
        out[n].k2 = iattr(b, "k2", -1);
        out[n].m2 = iattr(b, "m2", 0);
        out[n].k1 = iattr(b, "k1", 0);
        out[n].m1 = iattr(b, "m1", 0);
        out[n].cpbtn = iattr(b, "cpbtn", -1);
        out[n].mouse = iattr(b, "mouse", 0);
        out[n].editable = iattr(b, "editable", 1);
        n++;
    }

    return n;
}

bool ControlsXml_WriteKeyboard(const CxKbBind *in, int n)
{
    ensureProfileDir();

    tinyxml2::XMLDocument doc;
    doc.InsertEndChild(doc.NewDeclaration());
    tinyxml2::XMLElement *root = doc.NewElement("keyboard");
    root->SetAttribute("version", 1);
    doc.InsertEndChild(root);

    char buf[16];

    for (int i = 0; i < n; i++)
    {
        tinyxml2::XMLElement *b = doc.NewElement("bind");
        b->SetAttribute("function", in[i].func);
        // negatives (key=-1 / legacy key1=-1) -> decimal, otherwise strtol breaks on read
        if (in[i].k2 >= 0) sprintf(buf, "0x%X", in[i].k2); else sprintf(buf, "%d", in[i].k2);
        b->SetAttribute("k2", buf);
        b->SetAttribute("m2", in[i].m2);
        if (in[i].k1 >= 0) sprintf(buf, "0x%X", in[i].k1); else sprintf(buf, "%d", in[i].k1);
        b->SetAttribute("k1", buf);
        b->SetAttribute("m1", in[i].m1);
        b->SetAttribute("cpbtn", in[i].cpbtn);
        b->SetAttribute("mouse", in[i].mouse);
        b->SetAttribute("editable", in[i].editable);
        root->InsertEndChild(b);
    }

    char prof[_MAX_PATH], path[_MAX_PATH];
    ControlsXml_ActiveProfilePath(prof, sizeof(prof));
    sprintf(path, "%s\\keyboard.xml", prof);
    return doc.SaveFile(path) == tinyxml2::XML_SUCCESS;
}

bool ControlsXml_KeyBindForFunc(const char *func, CxKbBind *out)
{
    // linear scan of keyboard.xml (the list is built rarely - acceptable)
    static CxKbBind cache[1200];
    static int cacheN = -1;

    if (cacheN < 0)
        cacheN = ControlsXml_ReadKeyboard(cache, 1200);

    for (int i = 0; i < cacheN; i++)
        if (strcmp(cache[i].func, func) == 0)
        {
            *out = cache[i];
            return out->k2 >= 0;
        }

    return false;
}

// ===========================================================================
// <GUID>.xml - device buttons/POV
// ===========================================================================
int ControlsXml_ReadDevice(const char *guidStr, CxBtnBind *out, int maxN)
{
    char prof[_MAX_PATH], path[_MAX_PATH];
    ControlsXml_ActiveProfilePath(prof, sizeof(prof));
    sprintf(path, "%s\\%s.xml", prof, guidStr);

    tinyxml2::XMLDocument doc;

    if (doc.LoadFile(path) != tinyxml2::XML_SUCCESS)
        return 0;   // no file - fine (device has no bindings yet)

    tinyxml2::XMLElement *root = doc.FirstChildElement("device");

    if (not root)
        return 0;

    int n = 0;

    for (tinyxml2::XMLElement *e = root->FirstChildElement(); e and n < maxN; e = e->NextSiblingElement())
    {
        const char *fn = e->Attribute("function");

        if (not fn)
            continue;

        strncpy(out[n].func, fn, sizeof(out[n].func) - 1);
        out[n].func[sizeof(out[n].func) - 1] = 0;
        out[n].cpbtn = iattr(e, "cpbtn", -1);

        if (strcmp(e->Name(), "pov") == 0)
        {
            out[n].isPov = 1;
            out[n].id = iattr(e, "hat", 0);
            out[n].dir = iattr(e, "dir", 0);
        }
        else
        {
            out[n].isPov = 0;
            out[n].id = iattr(e, "id", 0);
            out[n].dir = 0;
        }

        n++;
    }

    return n;
}

bool ControlsXml_WriteDevice(const char *guidStr, const CxBtnBind *in, int n)
{
    ensureProfileDir();

    char prof[_MAX_PATH], path[_MAX_PATH];
    ControlsXml_ActiveProfilePath(prof, sizeof(prof));
    sprintf(path, "%s\\%s.xml", prof, guidStr);

    if (n <= 0)
    {
        remove(path);   // no bindings - file not needed
        return true;
    }

    tinyxml2::XMLDocument doc;
    doc.InsertEndChild(doc.NewDeclaration());
    tinyxml2::XMLElement *root = doc.NewElement("device");
    root->SetAttribute("guid", guidStr);
    root->SetAttribute("version", 1);
    doc.InsertEndChild(root);

    for (int i = 0; i < n; i++)
    {
        if (in[i].isPov)
        {
            tinyxml2::XMLElement *e = doc.NewElement("pov");
            e->SetAttribute("function", in[i].func);
            e->SetAttribute("hat", in[i].id);
            e->SetAttribute("dir", in[i].dir);
            e->SetAttribute("cpbtn", in[i].cpbtn);
            root->InsertEndChild(e);
        }
        else
        {
            tinyxml2::XMLElement *e = doc.NewElement("button");
            e->SetAttribute("function", in[i].func);
            e->SetAttribute("id", in[i].id);
            e->SetAttribute("cpbtn", in[i].cpbtn);
            root->InsertEndChild(e);
        }
    }

    return doc.SaveFile(path) == tinyxml2::XML_SUCCESS;
}

// ===========================================================================
// axismapping.xml
// ===========================================================================
// #57 the GameAxis_t lets us also persist the per-axis "soft" properties (reversed / center=AB
// detent / cutoff=idle / smoothing) that used to live in the binary config/joystick.cal — those
// are now stored here in axismapping.xml, so joystick.cal is gone.
struct AxisField { const char *name; DeviceAxis AxisMapping::*ptr; GameAxis_t axis; };

static const AxisField kAxisFields[] =
{
    {"Pitch", &AxisMapping::Pitch, AXIS_PITCH}, {"Bank", &AxisMapping::Bank, AXIS_ROLL}, {"Yaw", &AxisMapping::Yaw, AXIS_YAW},
    {"Throttle", &AxisMapping::Throttle, AXIS_THROTTLE}, {"Throttle2", &AxisMapping::Throttle2, AXIS_THROTTLE2},
    {"BrakeLeft", &AxisMapping::BrakeLeft, AXIS_BRAKE_LEFT}, {"BrakeRight", &AxisMapping::BrakeRight, AXIS_BRAKE_RIGHT},
    {"FOV", &AxisMapping::FOV, AXIS_FOV}, {"PitchTrim", &AxisMapping::PitchTrim, AXIS_TRIM_PITCH},
    {"YawTrim", &AxisMapping::YawTrim, AXIS_TRIM_YAW}, {"BankTrim", &AxisMapping::BankTrim, AXIS_TRIM_ROLL},
    {"AntElev", &AxisMapping::AntElev, AXIS_ANT_ELEV}, {"RngKnob", &AxisMapping::RngKnob, AXIS_RANGE_KNOB},
    {"CursorX", &AxisMapping::CursorX, AXIS_CURSOR_X}, {"CursorY", &AxisMapping::CursorY, AXIS_CURSOR_Y},
    {"Comm1Vol", &AxisMapping::Comm1Vol, AXIS_COMM_VOLUME_1}, {"Comm2Vol", &AxisMapping::Comm2Vol, AXIS_COMM_VOLUME_2},
    {"MSLVol", &AxisMapping::MSLVol, AXIS_MSL_VOLUME}, {"ThreatVol", &AxisMapping::ThreatVol, AXIS_THREAT_VOLUME},
    {"InterComVol", &AxisMapping::InterComVol, AXIS_INTERCOM_VOLUME}, {"HudBrt", &AxisMapping::HudBrt, AXIS_HUD_BRIGHTNESS},
    {"RetDepr", &AxisMapping::RetDepr, AXIS_RET_DEPR}, {"Zoom", &AxisMapping::Zoom, AXIS_ZOOM},
};
static const int kAxisCount = (int)(sizeof(kAxisFields) / sizeof(kAxisFields[0]));

static void hexToGuid(const char *hex, GUID *g)
{
    unsigned char *b = (unsigned char *)g;

    for (int i = 0; i < (int)sizeof(GUID); i++)
    {
        unsigned int v = 0;
        sscanf(hex + i * 2, "%2x", &v);
        b[i] = (unsigned char)v;
    }
}

static void guidToHex(const GUID *g, char *out)
{
    const unsigned char *b = (const unsigned char *)g;

    for (int i = 0; i < (int)sizeof(GUID); i++)
        sprintf(out + i * 2, "%02X", b[i]);

    out[2 * sizeof(GUID)] = 0;
}

bool ControlsXml_ReadAxes(AxisMapping *out)
{
    char prof[_MAX_PATH], path[_MAX_PATH];
    ControlsXml_ActiveProfilePath(prof, sizeof(prof));
    sprintf(path, "%s\\axismapping.xml", prof);

    tinyxml2::XMLDocument doc;

    if (doc.LoadFile(path) != tinyxml2::XML_SUCCESS)
        return false;

    tinyxml2::XMLElement *root = doc.FirstChildElement("axismapping");

    if (not root)
        return false;

    *out = AxisMapping();   // defaults (axes -1/100, GUIDs zeroed)
    out->FlightControlDevice = iattr(root, "flightControlDevice", -1);
    out->totalDeviceCount    = iattr(root, "totalDeviceCount", 0);

    tinyxml2::XMLElement *guids = root->FirstChildElement("deviceGuids");

    if (guids)
        for (tinyxml2::XMLElement *g = guids->FirstChildElement("guid"); g; g = g->NextSiblingElement("guid"))
        {
            int idx = iattr(g, "index", -1);
            const char *val = g->Attribute("value");

            if (idx >= 0 and idx < SIM_NUMDEVICES and val and strlen(val) >= 2 * sizeof(GUID))
                hexToGuid(val, &out->DeviceGUIDs[idx]);
        }

    // #57 the SetupDIJoystick sanity check (siloop) compares AxisMap.FlightControllerGUID against the
    // current flight controller's guidInstance. The XML doesn't store FlightControllerGUID separately,
    // so derive it from the persisted per-device GUID (== guidInstance). Without this it stays zero ->
    // mismatch -> IO.Reset() -> axes dead at startup until the controls window calls SetupGameAxis.
    if (out->FlightControlDevice >= 0 and out->FlightControlDevice < SIM_NUMDEVICES)
        out->FlightControllerGUID = out->DeviceGUIDs[out->FlightControlDevice];

    tinyxml2::XMLElement *axes = root->FirstChildElement("axes");

    if (axes)
        for (tinyxml2::XMLElement *a = axes->FirstChildElement("axis"); a; a = a->NextSiblingElement("axis"))
        {
            const char *nm = a->Attribute("name");

            if (not nm)
                continue;

            for (int i = 0; i < kAxisCount; i++)
                if (strcmp(kAxisFields[i].name, nm) == 0)
                {
                    DeviceAxis &ax = out->*(kAxisFields[i].ptr);
                    ax.Device     = iattr(a, "device", -1);
                    ax.Axis       = iattr(a, "axis", -1);
                    ax.Deadzone   = iattr(a, "deadzone", 100);
                    ax.Saturation = iattr(a, "saturation", -1);
                    // #57 soft properties (formerly in joystick.cal)
                    {
                        const GameAxis_t ga = kAxisFields[i].axis;
                        IO.SetAnalogIsReversed(ga, iattr(a, "reversed", 0) != 0);
                        IO.analog[ga].center          = iattr(a, "center", 0);
                        IO.analog[ga].cutoff          = iattr(a, "cutoff", 15000);   // matches IO::Reset default
                        IO.analog[ga].smoothingFactor = iattr(a, "smoothing", 0);
                    }
                    break;
                }
        }

    return true;
}

bool ControlsXml_WriteAxes(const AxisMapping *in)
{
    ensureProfileDir();

    tinyxml2::XMLDocument doc;
    doc.InsertEndChild(doc.NewDeclaration());
    tinyxml2::XMLElement *root = doc.NewElement("axismapping");
    root->SetAttribute("version", 1);
    root->SetAttribute("flightControlDevice", in->FlightControlDevice);
    root->SetAttribute("totalDeviceCount", in->totalDeviceCount);
    doc.InsertEndChild(root);

    tinyxml2::XMLElement *guids = doc.NewElement("deviceGuids");
    root->InsertEndChild(guids);

    static const GUID zero = {0};
    char hex[2 * sizeof(GUID) + 1];

    for (int i = 0; i < SIM_NUMDEVICES; i++)
        if (memcmp(&in->DeviceGUIDs[i], &zero, sizeof(GUID)) != 0)
        {
            guidToHex(&in->DeviceGUIDs[i], hex);
            tinyxml2::XMLElement *g = doc.NewElement("guid");
            g->SetAttribute("index", i);
            g->SetAttribute("value", hex);
            guids->InsertEndChild(g);
        }

    tinyxml2::XMLElement *axes = doc.NewElement("axes");
    root->InsertEndChild(axes);

    for (int i = 0; i < kAxisCount; i++)
    {
        const DeviceAxis &ax = in->*(kAxisFields[i].ptr);
        const GameAxis_t ga = kAxisFields[i].axis;
        tinyxml2::XMLElement *a = doc.NewElement("axis");
        a->SetAttribute("name", kAxisFields[i].name);
        a->SetAttribute("device", ax.Device);
        a->SetAttribute("axis", ax.Axis);
        a->SetAttribute("deadzone", ax.Deadzone);
        a->SetAttribute("saturation", ax.Saturation);
        // #57 soft properties (formerly in joystick.cal)
        a->SetAttribute("reversed", IO.AnalogIsReversed(ga) ? 1 : 0);
        a->SetAttribute("center", (int)IO.analog[ga].center);
        a->SetAttribute("cutoff", (int)IO.analog[ga].cutoff);
        a->SetAttribute("smoothing", (int)IO.analog[ga].smoothingFactor);
        axes->InsertEndChild(a);
    }

    char prof[_MAX_PATH], path[_MAX_PATH];
    ControlsXml_ActiveProfilePath(prof, sizeof(prof));
    sprintf(path, "%s\\axismapping.xml", prof);
    return doc.SaveFile(path) == tinyxml2::XML_SUCCESS;
}
