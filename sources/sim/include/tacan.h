#ifndef _TACAN_H
#define _TACAN_H

#include "vu2.h"

//---------------------------------------------------------------
// Constants and Defines
//---------------------------------------------------------------

const int	NUM_CHANNELS			= 126;
const	int	MIN_TACAN_CHANNEL		= 70; // superceeded by global
const	int	NUM_TACAN_FIELDS		= 4;

extern int g_nMinTacanChannel;
//---------------------------------------------------------------
// Class Definition
//---------------------------------------------------------------

class TacanList {

public:

	//---------------------------------------------------------------
	// Local Type Definitions
	//---------------------------------------------------------------

	enum StationSet {X, Y, NumSets};		// Tacan Stations are numbered 1 - 126 and are grouped in two sets X and Y band.

	enum Domain {AA, AG, NumDomains};

	struct TacanCampStr
	{
		int			channel;
		StationSet	set;
		short		campaignID;
		int			callsign;
		int range;
		int tactype;
		float ilsfreq;
	};

	struct LinkedCampStationStr
	{
		LinkedCampStationStr	*p_next;
		TacanCampStr			*p_station;
	};

	struct LinkedTacanVUStr
	{
		LinkedTacanVUStr*	p_next;
		LinkedTacanVUStr*	p_previous;
		int					channel;
		StationSet			set;
		Domain				domain;
		VU_ID					vuID;
		short camp_id;
	};
	
private:
	//---------------------------------------------------------------
	// Airbase File Data
	//---------------------------------------------------------------
	
	TacanCampStr		**mpCampList;		 							// List Sorted By CampId
	int					mCampListTally;								// Total Num of airbases

	//---------------------------------------------------------------
	// THE Dynamically Assigned Tanker/Carrier List
	//---------------------------------------------------------------
	LinkedTacanVUStr	*mpAssigned;	// All Tacans will be in Y band
	LinkedTacanVUStr	*mpRetired;		// All Tacans will be in Y band
	int					mLastUnused;	// Valid Numbers 126 ... 70

	//---------------------------------------------------------------
	// THE Airbase TACAN LIST
	//---------------------------------------------------------------
	
	LinkedTacanVUStr	*mpTList;

	//---------------------------------------------------------------
	// Initialization Functions for Airbase List
	//---------------------------------------------------------------

	// Used by the class only during initialization, operates upon mpCampList
	BOOL StoreStation(LinkedCampStationStr**listp, short staionid, int band, StationSet set, int callsign,
	    int range, int type, float ilsfreq);		
	// Used by the class only during initialization, operates upon mpCampList
	void ResolveStationList(LinkedCampStationStr**, TacanCampStr***, int);	
	friend int SearchForChannel(void*, void**);										// Compare function for bsearch routine

	//---------------------------------------------------------------
	// Utility Functions for Airbase List
	//---------------------------------------------------------------
	
	BOOL GetChannelFromCampID(int*, StationSet*, short);														// Looks up channel given the CampID, operates upon mpCampList
	BOOL GetPointerFromVUID(LinkedTacanVUStr*, VU_ID, LinkedTacanVUStr**, LinkedTacanVUStr**);	// Used for searching for a particular VU_ID, returns a pointer to the entry.
	BOOL GetCampTacanFromVUID(TacanCampStr **tacaninfo, short camp_id);
	void InsertIntoTacanList(LinkedTacanVUStr**, LinkedTacanVUStr**, VU_ID vuid, short camp_id, int digits, StationSet xy, Domain);	// Used by AddTacan for inserting tacan stations into mpTList

	//---------------------------------------------------------------
	// Utility Functions for Dynamically Assigned List
	//---------------------------------------------------------------

	void					InitDynamicChans(void);
	void					CleanupDynamicChans(void);
	int					AssignChannel(VU_ID, Domain, short camp_id);		// Get the next retired tacan number, otherwise create one from the mLastUnused value
	void					RetireChannel(VU_ID);				// Move the channel from the Assigned list into the retired list

public:

	//---------------------------------------------------------------
	// Run Time Functions
	//---------------------------------------------------------------

	void AddTacan(CampBaseClass *);														// Add a tacan station to the list
	void RemoveTacan(VU_ID, int);																// Remove a tacan station from the list
	
	BOOL GetVUIDFromChannel(int digits, StationSet xy, Domain dom, 
	    VU_ID*vuid, int *rangep, int *ttype, float *ilsfreq);					// Given the channel and band, we can get the VU_ID of a tacan station
	BOOL GetVUIDFromLocation(float x, float y, Domain domain, VU_ID*vuid, int *rangep, int *ttype, float *ilsfreq);						// Works for airbases only.  Find the closest tacan to this point
	BOOL GetChannelFromVUID(VU_ID, int*, StationSet*, Domain*,
	    int *range, int *ttype, float *ilsfreq);					// Works for airbases only.  Given a VU_ID, we can get the channel and band of a tacan station
	BOOL GetCallsignFromCampID(short campId, int*);									// Works for airbases only.  Given a campId, we can get the callsign of the respective objective, 0 if not found

	// utility function
	static int ChannelToFrequency(StationSet set, int channel);
	//---------------------------------------------------------------
	// Constructors and Destructors
	//---------------------------------------------------------------

	TacanList();
	~TacanList();
};

//---------------------------------------------------------------
// Global Constants
//---------------------------------------------------------------

extern TacanList*		gTacanList;
extern const char*	gpTacanFileName;

#endif