#ifndef NTM_H
#define NTM_H

// ==================================
// Naval Tasking Manager class
// ==================================

#include "falcmesg.h"
#include "unit.h"
#include "objectiv.h"
#include "team.h"
#include "CampList.h"

class NavalTaskingManagerClass : public CampManagerClass {
	private:
	public:
		short			flags;
		// These don't need to be transmitted
		F4PFList		unitList;							// Collection of available ground assets
		short			tod;									// Time of day (temp variable)
		short			topPriority;						// Highest PO priority (for scaling)
		short			done;									// Flagged when all units assigned
	public:
		// constructors
		NavalTaskingManagerClass(ushort type, Team t);
		//sfr: added rem
		NavalTaskingManagerClass(VU_BYTE **stream, long *rem);
		NavalTaskingManagerClass(FILE *file);
		virtual ~NavalTaskingManagerClass();
		virtual int SaveSize (void);
		virtual int Save (VU_BYTE **stream);
		virtual int Save (FILE *file);

		// Required pure virtuals
		virtual int Task();
		virtual void DoCalculations();
		virtual int Handle(VuFullUpdateEvent *event);

		// core functions
		void Setup(void);
		void Cleanup(void);
		void SendNTMMessage (VU_ID from, short message, short data1, short data2, VU_ID data3);
	};

typedef NavalTaskingManagerClass *NavalTaskingManager;
typedef NavalTaskingManagerClass *NTM;

// ==========================================
// Global functions
// ==========================================

#endif
