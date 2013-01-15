#include <stdio.h>
#include "CmpGlobl.h"
#include "Unit.h"
#include "Division.h"
#include "CampList.h"
#include "Find.h"
#include "Team.h"
#include "Campaign.h"
#include "CampStr.h"
#include "classtbl.h"
#include "F4Version.h"

Division DivisionData[NUM_TEAMS] = { NULL };

#ifdef USE_SH_POOLS
MEM_POOL DivisionClass::pool;
MEM_POOL gDivVUIDs = NULL;
#endif

#define MAX_DIVISION 255

#ifdef DEBUG
int DivisionSanityCheck (void);
#endif

extern _TCHAR* GetDivisionName (int div, int type, _TCHAR *buffer, int size, int object);

// ============================
// Division class stuff
// ============================

DivisionClass::DivisionClass (void)
	{
	x = y = 0;
	nid = 0;
	owner = 0;
	elements = 0;
	c_element = 0;
	element = NULL;
	next = NULL;
	}
		
DivisionClass::~DivisionClass (void)
	{
	if (element)
		#ifdef USE_SH_POOLS
		MemFreePtr( element );
		#else
		delete element;
		#endif
	element = NULL;
	next = NULL;
	ShiAssert ( this != DivisionData[owner] );
	}

_TCHAR* DivisionClass::GetName (_TCHAR* buffer, int size, int object)
	{
	return GetDivisionName (nid, type, buffer, size, object);
	}

Unit DivisionClass::GetFirstUnitElement (void)
	{
	c_element = 0;
	return GetUnitElement(c_element);
	}

Unit DivisionClass::GetNextUnitElement (void)
	{
	c_element++;
	return GetUnitElement(c_element);
	}

Unit DivisionClass::GetPrevUnitElement (Unit e)
	{
	for (int i=1; i<elements; i++)
		{
		if (element[i] == e->Id())
			return GetUnitElement(i-1);
		}
	return NULL;
	}

Unit DivisionClass::GetUnitElement (int en)
	{
	Unit	ret = NULL;

	if (en < elements && element[c_element])
		{
		ret = (Unit)vuDatabase->Find(element[en]);
		if (!ret || ret->GetDomain() != DOMAIN_LAND)
			RemoveChild(element[en]);
		}
	return ret;
	}

Unit DivisionClass::GetUnitElementByID (int eid)
	{
	if (eid < elements)
		return GetUnitElement(eid);

	return NULL;
	}

void DivisionClass::UpdateDivisionStats (void)
	{
	Unit			u;
	GridIndex		ex,ey;
	uchar			count[50] = { 0 };
	int				bcount=0,btype=0,i;

	x = y = 0;
	u = GetFirstUnitElement();
	while (u)
		{
		u->GetLocation(&ex,&ey);
		x += ex;
		y += ey;
		count[u->GetSType()]++;
		u = GetNextUnitElement();
		}
	if (elements)
		{
		x /= elements;
		y /= elements;
		}
	for (i=0; i<50; i++)
		{
		if (count[i] > bcount)
			{
			bcount = count[i];
			btype = i;
			}
		}
	type = btype;
	}

void DivisionClass::RemoveChildren (void)
	{
	elements = 0;
	c_element = 0;
	if ( element )
		#ifdef USE_SH_POOLS
		MemFreePtr( element );
		#else
		delete element;
		#endif
	element = NULL;
	}

void DivisionClass::RemoveChild (VU_ID	eid)
	{
	int		i=0,j;
	Unit	e;

	while (i < elements)
		{
		if (element[i] == eid)
			{
			for (j=i; j < elements-1; j++)
				{
				element[j] = element[j+1];
				e = FindUnit(element[j]);
				}
			element[j] = FalconNullId;
			elements--;
			}
		else
			i++;
		}
	}


// ========================
// Other functions
// ========================

void DumpDivisionData (int team)
	{
	DivisionClass	*deadd;

	while (DivisionData[team])
		{
		deadd = DivisionData[team];
		DivisionData[team] = deadd->next;
		delete deadd;
#ifdef DEBUG
		DivisionSanityCheck();
#endif
		}
	DivisionData[team] = NULL;
	}

void DumpDivisionData (void)
	{
	for (int i=0; i<NUM_TEAMS; i++)
		DumpDivisionData(i);
	}

void BuildDivisionData (void)
{
	Division		dc;
	Division		dd[NUM_TEAMS] = { NULL };
	Unit			u;
	int				d,t;
	uchar			divels[NUM_TEAMS][MAX_DIVISION] = { 0 };
	uchar tempteam; // JB 010220 CTD
	uchar tempdivision; // JB 010220 CTD

	#ifdef USE_SH_POOLS
	if ( gDivVUIDs == NULL )
		gDivVUIDs = MemPoolInit( 0 );
	#endif

	CampEnterCriticalSection();
	DumpDivisionData();

	// Count # of elements in each division
	{
		VuListIterator	myit(AllParentList);
		u = (Unit) myit.GetFirst();
		while (u){
			//if (u->VuState() != VU_MEM_DELETED)
			//{
			if (u->GetDomain() == DOMAIN_LAND)
			{
				tempteam = u->GetTeam(); // JB 010220 CTD
				tempdivision = u->GetUnitDivision(); // JB 010220 CTD
				if (tempteam >= 0 && tempteam < NUM_TEAMS && tempdivision >= 0 && tempdivision < MAX_DIVISION) // JB 010220 CTD
					divels[tempteam][tempdivision]++; // JB 010220 CTD
					//divels[u->GetTeam()][u->GetUnitDivision()]++; // JB 010220 CTD
			}
			//}
			u = (Unit) myit.GetNext();
		}
		// Create/add to the divisions
		u = (Unit) myit.GetFirst();
		while (u)
		{
			if (u->GetDomain() == DOMAIN_LAND)
			{
				d = u->GetUnitDivision();
				if (d > 0)
				{
					t = u->GetTeam();
					dc = dd[t];
					while (dc)
						{
						// Try to find existing one
						if (dc->nid == d)
							break;
						dc = dc->next;
					}

					if (!dc)
					{
						// Create a new one
						dc = new DivisionClass();
						dc->nid = d;
						dc->owner = u->GetOwner();
						ShiAssert(divels[t][d]);
						#ifdef USE_SH_POOLS
						dc->element = (VU_ID *)MemAllocPtr( gDivVUIDs, sizeof( VU_ID ) * divels[t][d], FALSE);
						#else
						if (t >= 0 && t < NUM_TEAMS && d >= 0 && d < MAX_DIVISION) // JB 010223 CTD
							dc->element = new VU_ID[divels[t][d]];
						#endif
						dc->next = dd[t];
						dd[t] = dc;
					}

					if (!F4IsBadWritePtr(dc, sizeof(DivisionClass)) && dc->elements >= 0 && !F4IsBadWritePtr(&(dc->element[dc->elements]), sizeof(VU_ID))) // JB 010223 CTD
					{
						dc->element[dc->elements] = u->Id();
						dc->elements++;
					}
				}
			}
			u = (Unit) myit.GetNext();
		}
	}

	// Set the division type and location
	for (t=0; t<NUM_TEAMS; t++)
	{
		dc = dd[t];
		while (dc)
		{
			if (F4IsBadReadPtr(dc, sizeof(Division))) // JB 010223 CTD
				break;

			ShiAssert (dc->elements == divels[t][dc->nid]);
			dc->UpdateDivisionStats();
			dc = dc->next;
		}
		// Assign to lists only after we've finalized it.
		// UI doesn't critical section with us.
		DivisionData[t] = dd[t];
	}
	CampLeaveCriticalSection();

#ifdef DEBUG
	DivisionSanityCheck();
#endif
}


/*
void BuildDivisionData (void)
	{
	DivisionClass	*dc,*nextd,*lastd;
	Unit			u;
	int				c,d,t;
	VuListIterator	myit(AllParentList);
	uchar			divlist[256];

	memset(divlist,0,256);

	// Find out what we got, first;
	for (t=0; t<NUM_TEAMS; t++)
		{
		dc = GetFirstDivision(t);
		while (dc)
			{
			if (!dc->BuildDivision(dc->owner,dc->nid))
				{
				// No longer needed, kill off
				if (dc == DivisionData[t])
					DivisionData[t] = dc->next;
				else if (lastd)
					lastd->next = dc->next;
				nextd = dc->next;
				delete dc;
#ifdef DEBUG
				DivisionSanityCheck();
#endif
				dc = nextd;
				}
			else
				{
				divlist[dc->nid] |= (1 << dc->owner);
				lastd = dc;
				dc = dc->next;
				}
			}
		}

	// Now add any newly found stuff
	u = GetFirstUnit(&myit);
	while (u)
		{
		if (u->GetDomain() == DOMAIN_LAND && u->GetUnitDivision())
			{
			d = u->GetUnitDivision();
			c = u->GetOwner();
			if (!(divlist[d] & (1 << c)))
				{
//				CampEnterCriticalSection();
				dc = new DivisionClass();
				if (!dc->BuildDivision(c,d))
					{
					delete dc;
#ifdef DEBUG
					DivisionSanityCheck();
#endif
					}
				else
					{
					dc->next = DivisionData[u->GetTeam()];
					DivisionData[u->GetTeam()] = dc;
					}
//				CampLeaveCriticalSection();
				divlist[d] |= (1 << c);
				}
			}
		u = GetNextUnit(&myit);
		}
	}
*/

Division GetFirstDivision (int team)
	{
	return DivisionData[team];
	}

Division GetNextDivision (Division d)
	{
	return d->next;
	}

Division GetFirstDivisionByCountry (int country)
	{
	Division	d = DivisionData[GetTeam(country)];

	while (d)
		{
		if (d->owner == country)
			return d;
		d = d->next;
		}
	return NULL;
	}

Division GetNextDivisionByCountry (Division d, int country)
	{
	d = d->next;
	while (d)
		{
		if (d->owner == country)
			return d;
		d = d->next;
		}
	return NULL;
	}

Division GetDivisionByUnit (Unit u)
	{
	Division d;

	d = GetFirstDivision(u->GetTeam());
	while (d)
		{
		if (d->nid == u->GetUnitDivision() && d->owner == u->GetOwner())
			return d;
		d = d->next;
		}
	return NULL;
	}

Division FindDivisionByXY (GridIndex x, GridIndex y)
	{
	int			t;
	Division	d=NULL;

	for (t=0; t<NUM_TEAMS; t++)
		{
		d = GetFirstDivision(t);
		while (d)
			{
			if (d->x == x && d->y == y)
				return d;
			d = d->next;
			}
		}
	return NULL;
	}

int DivisionSanityCheck (void)
	{
	Division	tmp;
	int			i;

	for (i=0; i<NUM_TEAMS; i++)
		{
		tmp = DivisionData[i];
		while (tmp)
			{
			ShiAssert ((int)tmp != 0xdddddddd);
			tmp = tmp->next;
			}
		}
	return 0;
	}
