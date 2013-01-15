/**********************************************************************************
/*
/* cmpevent.h
/*
/* Campaign event manager
/*
/*********************************************************************************/

#ifndef CMPEVENT_H
#define CMPEVENT_H

// ====================================
// Event class
// ====================================

#define	CE_ONETIME		0x01						// Set if this is a onetime only event
#define	CE_FIRED		0x08						// This event has been used

class EventClass {
	private:
	public:
		short			event;						// The Event ID
		short			flags;						// event flags
	public:
		EventClass (short id);
		//sfr: added rem
		EventClass (uchar **stream, long *rem);
		EventClass (FILE* fp);
		~EventClass (void);
		int Save (FILE* fp);
		void DoEvent (void);
		void SetEvent (int status);
		int HasFired (void)							{ return flags & CE_FIRED; }
	};


/*
// ====================================
// Trigger data
// ====================================

struct Trigger {
	short				flags;						// Trigger flags
	char				type[20];					// The check type
	short				data[4];					// Data associated with this trigger
	};

// ====================================
// Event class
// ====================================

#define	CE_ONETIME		0x01						// Set if this is a onetime only event
#define	CE_FIRED		0x08						// This event has been used

class EventClass {
	private:
	public:
		short			event;						// The Event ID
		short			flags;						// event flags
		short			data[4];					// event data
		char			name[20];					// The event name
		uchar			priority;					// event priority
		uchar			and_trigs;					// Number of triggers
		uchar			or_trigs;					// Number of triggers
		Trigger*		and_triggers;				// AND triggers (all must be met)
		Trigger*		or_triggers;				// OR triggers (any may be met)

	public:
		EventClass (short id);
		EventClass (FILE* fp);
		~EventClass (void);
		int Save (FILE* fp);
		int CheckTrigger (Trigger* trig);
		int CheckTriggers (void);
		void DoEvent (void);
		void ClearEvent (void)						{ flags & ~CE_FIRED; }
		int HasFired (void)							{ return flags & CE_FIRED; }
	};
*/

// ==============================
// Global functions
// ==============================

int CheckTriggers(char *filename);

int NewCampaignEvents (char* filename);

int LoadCampaignEvents (char* filename, char* scenario);

int SaveCampaignEvents (char* filename);

void DisposeCampaignEvents (void);

#endif
