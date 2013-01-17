// Theater defintions file

#ifndef _THEATERDEF_H
#define _THEATERDEF_H

class TheaterList;

struct TheaterDef {
    char *m_name;
    char *m_description;
    char *m_terrain;
    char *m_campaign;
    char *m_bitmap;
    char *m_artdir;
    char *m_uisounddir;
    char *m_moviedir;
    char *m_objectdir;
	char *m_3ddatadir;
    char *m_misctexdir;
    int m_mintacan;
	char *m_sounddir;
	// RV - Biker
	char *m_cockpitdir;
	char *m_zipsdir;
	char *m_tacrefdir;
	char *m_splashdir;
	
    TheaterDef *m_next; // link
};

class TheaterList {
    TheaterDef *m_first;
    int m_count;
public:
	TheaterDef * FindTheaterByName(const char *name);
	TheaterDef * GetCurrentTheater();
	void SetCurrentTheater(TheaterDef *td);
	bool SetNewTheater(TheaterDef *td);
	bool ChooseTheaterByName(const char *name);
    TheaterList() : m_count(0), m_first(0) {};
    ~TheaterList();
    void AddTheater(TheaterDef *ntheater);
	void DoSoundSetup();

    TheaterDef *GetTheater(int n);
    int Count() { return m_count; };
private:
	void SetPathName(char *dest, char *src, char *reldir);
};

extern TheaterList g_theaters;
#endif
