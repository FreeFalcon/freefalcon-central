#ifndef _CONTROLSXML_H
#define _CONTROLSXML_H

// #53: XML control settings. Isolates tinyxml2 to a single module (controlsxml.cpp).
// Files (catalog generated offline by tools\gen_controls_xml.py; runtime reads/writes):
//   config\controls.xml                      - catalog: name -> BMS label -> category (read-only)
//   config\profiles.xml                      - list of profiles + active one
//   config\profiles\<prof>\keyboard.xml      - keyboard combinations
//   config\profiles\<prof>\<GUID>.xml         - buttons/POV of a specific device
//   config\profiles\<prof>\axismapping.xml   - axes
// Goal: fully drop keystrokes.key/axismapping.dat at runtime.

struct AxisMapping;   // simio.h

// --- Catalog (name -> label/category) --------------------------------------
int  ControlsXml_LoadCatalog(void);                       // from controls.xml; function count
const char *ControlsXml_GetLabel(const char *funcName);   // BMS label or NULL
const char *ControlsXml_GetCategory(const char *funcName);// category or ""

// --- Active profile --------------------------------------------------------
// Active profile token ("default" or a numeric dir). All profile paths derive from it.
// On profile change: store the token here and re-read the bindings/axes.
extern char g_controlsProfile[64];
void ControlsXml_SetProfile(const char *name);

// Full path of the active profile dir (config\profiles\<g_controlsProfile>). true on success.
bool ControlsXml_ActiveProfilePath(char *out, int outSize);

// --- Pilot profiles (profiles.xml) -----------------------------------------
// Each created pilot = a <profile name="callsign" dir="N"/> entry + a folder
// config\profiles\<N>\ (keyboard.xml/<GUID>.xml/axismapping.xml/logbook.xml).
// The default pilot (logbook never customised) -> config\profiles\default\.
// Select the active profile by pilot callsign (logbook loads/selects a pilot).
void ControlsXml_SelectProfileForCallsign(const char *callsign);
// Register a profile for a NEW pilot (next numeric dir) and make it active.
void ControlsXml_CreateProfile(const char *callsign);
// Restore the active profile from profiles.xml at startup (last logbook selection persists).
void ControlsXml_RestoreActiveProfile(void);
// List pilot callsigns from profiles.xml (for the logbook pilot list). Returns the count.
int  ControlsXml_ListProfiles(char out[][24], int maxN);
// Active (last-selected) pilot callsign from profiles.xml. true if non-empty.
bool ControlsXml_GetActiveProfileName(char *out, int outSize);
// Ensure the default pilot occupies the single dir-0 entry (folder "default") and make it active;
// drops any duplicate/placeholder entries so the default never appears twice.
void ControlsXml_EnsureDefaultPilot(const char *callsign);
// Rename a pilot's profiles.xml entry (keeps the same dir/folder and its data).
void ControlsXml_RenameProfile(const char *oldName, const char *newName);

// --- Pilot logbook (logbook.xml in the active profile dir) ------------------
// Readable mirror of LB_PILOT. Plain types only, so controlsxml (which must not pull in the
// UI logbook headers) can serialise it as human-readable XML; logbook.cpp maps to/from LB_PILOT.
struct CxLogbook
{
    char  name[24];
    char  callsign[16];
    char  password[16];
    char  commissioned[16];
    char  optionsFile[16];
    char  picture[40];
    char  patch[40];
    char  personal[128];
    char  squadron[24];
    float flightHours;
    float aceFactor;
    int   rank;
    int   voice;
    int   pictureResource;
    int   patchResource;
    int   medals[8];          // NUM_MEDALS = 6 (padded)
    // dogfight
    int   df_matchesWon, df_matchesLost, df_matchesWonVHum, df_matchesLostVHum;
    int   df_kills, df_killed, df_humanKills, df_killedByHuman;
    // campaign
    int   cmp_gamesWon, cmp_gamesLost, cmp_gamesTied, cmp_missions;
    int   cmp_totalScore, cmp_totalMissionScore;
    int   cmp_consecMissions, cmp_kills, cmp_killed, cmp_humanKills, cmp_killedByHuman;
    int   cmp_killedBySelf, cmp_airToGround, cmp_static, cmp_naval;
    int   cmp_friendliesKilled, cmp_missSinceLastFriendlyKill;
};
bool ControlsXml_WriteLogbook(const CxLogbook *in);   // profiles\<dir>\logbook.xml (readable)
bool ControlsXml_ReadLogbook(CxLogbook *out);         // false if no file

// --- Keyboard combinations (keyboard.xml) ----------------------------------
struct CxKbBind { char func[64]; int k2, m2, k1, m1, cpbtn, mouse, editable; };
int  ControlsXml_ReadKeyboard(CxKbBind *out, int maxN);   // count read
bool ControlsXml_WriteKeyboard(const CxKbBind *in, int n);
// Convenience lookup of a function's combo for the list view (true if an entry with k2>=0 exists).
bool ControlsXml_KeyBindForFunc(const char *func, CxKbBind *out);

// --- Device buttons/POV (<GUID>.xml) ---------------------------------------
struct CxBtnBind { char func[64]; int id, cpbtn, dir; int isPov; };
int  ControlsXml_ReadDevice(const char *guidStr, CxBtnBind *out, int maxN); // 0 if no file
bool ControlsXml_WriteDevice(const char *guidStr, const CxBtnBind *in, int n); // creates the file

// --- Axes (axismapping.xml) ------------------------------------------------
bool ControlsXml_ReadAxes(AxisMapping *out);
bool ControlsXml_WriteAxes(const AxisMapping *in);

#endif // _CONTROLSXML_H
