#ifndef _PLAYER_STATS_H_
#define _PLAYER_STATS_H_

#pragma pack(1)
struct StatRec
{
	long  IP;
	long  Date;
	long  Rev;
	short aa_kills;
	short ag_kills;
	short an_kills;
	short as_kills;
	short missions;
	char  rating;
	char  CheckSum;
};
#pragma pack()

struct StatList
{
	StatRec data;
	StatList *Next;
};

class PlayerStats
{
	private:
		StatList *Root_;
		char SaveName_[MAX_PATH];

	public:
		PlayerStats();
		~PlayerStats();

		void SetName(char *filename) { strcpy(SaveName_,filename); }

		void LoadStats();
		void SaveStats();

		void AddStat(long IP,long Date,long Rev,short aa,short ag,short an,short as,short missions,char rating);
		StatList *Find(long IP,long Date,long Rev);
		char *GetSaveName() { return(&SaveName_[0]); }
};

#endif