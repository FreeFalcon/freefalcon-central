// ***************************************************************************
// GTMObj.cpp
//
// Stuff used to deal with primary and secondary objectives
// ***************************************************************************

#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include "cmpglobl.h"
#include "gtm.h"
#include "objectiv.h"
#include "CampList.h"
#include "gtmobj.h"
#include "Find.h"

// ===============================
// Global current element pointers
// ===============================

// ==============================
// Prototypes	
// ==============================

void AddPODataEntry (Objective po);

// =============================
// Objective Data Lists
// =============================

void CleanupObjList(void)
	{
	ListNode		lp,np;
	POData			pod;
	Objective		o;
	VuListIterator	poit(POList);

	// Eliminate objectives no longer in list
	lp = PODataList->GetFirstElement();;
	while (lp)
		{
		np = lp->GetNext();
		pod = (POData) lp->GetUserData();
		o = FindObjective(pod->objective);
		if (!o || !o->IsPrimary())
			PODataList->Remove(lp);
		lp = np;
		}
	// Add newly promoted objectives
	o = GetFirstObjective(&poit);
	while (o)
		{
		pod = GetPOData(o);
		if (!pod)
			AddPODataEntry(o);
		o = GetNextObjective(&poit);
		}
	}

void DisposeObjList(void)
	{
	delete PODataList;
	PODataList = NULL;
	}

void AddPODataEntry (Objective po)
	{
	POData			pd;
	int				i;

	pd = new PrimaryObjectiveData();
	pd->objective = po->Id();
	for (i=0; i<NUM_TEAMS; i++)
		{
		pd->ground_priority[i] = 0;
		pd->air_priority[i] = 0;
		pd->player_priority[i] = -1;
		pd->ground_assigned[i] = 0;
		pd->flags = 0;
		}
	PODataList->InsertNewElement(po->GetCampID(), pd, 0);
	}

// ===============================
// Retreval functions
// ===============================

POData GetPOData (Objective po)
	{
	ListNode		lp;
	POData			pd;

	lp = PODataList->GetFirstElement();
	while (lp)
		{
		pd = (POData) lp->GetUserData();
		if (pd->objective == po->Id())
			return pd;
		lp = lp->GetNext();
		}
	return NULL;
	}

void ResetObjectiveAssignmentScores(void)
	{
	POData		pod;
	ListNode	lp;
	int			i;

	lp = PODataList->GetFirstElement();
	while (lp)
		{
		pod = (POData) lp->GetUserData();
		for (i=0; i<NUM_TEAMS; i++)
			pod->ground_assigned[i] = 0;
		lp = lp->GetNext();
		}
	}

// ================================
// Sorted unit list components
// ================================

UnitScoreNode::UnitScoreNode (void)
	{
	unit = NULL;
	score = 0;
	distance = 0;
	next = NULL;
	}

#define USN_SORT_BY_SCORE			1
#define USN_SORT_BY_DISTANCE		2

UnitScoreNode* UnitScoreNode::Insert (UnitScoreNode* to_insert, int sort_by)
	{
	USNode		last_node;

	if (to_insert == this)
		return this;

	if (sort_by == USN_SORT_BY_DISTANCE)
		{
		if (to_insert->distance > distance)
			{
			to_insert->next = this;
			return to_insert;
			}

		last_node = this;
		while (last_node->next)
			{
			if (to_insert->distance > last_node->next->distance)
				{
				to_insert->next = last_node->next;
				last_node->next = to_insert;
				return this;
				}
			last_node = last_node->next;
			}
		last_node->next = to_insert;
		}
	else if (sort_by == USN_SORT_BY_SCORE)
		{
		if (to_insert->score > score)
			{
			to_insert->next = this;
			return to_insert;
			}

		last_node = this;
		while (last_node->next)
			{
			if (to_insert->score > last_node->next->score)
				{
				to_insert->next = last_node->next;
				last_node->next = to_insert;
				return this;
				}
			last_node = last_node->next;
			}
		last_node->next = to_insert;
		}
	else
		{
		if (to_insert != this)
			to_insert->next = this;
		else
			to_insert->next = NULL;
		return to_insert;
		}
	return this;
	}

UnitScoreNode* UnitScoreNode::Remove (UnitScoreNode* to_remove)
	{
	USNode		last_node,temp;

	if (to_remove == this)
		{
		temp = next;
		delete this;
		return temp;
		}

	last_node = this;
	while (last_node)
		{
		if (last_node->next == to_remove)
			{
			temp = last_node->next;
			last_node->next = temp->next;
			delete temp;
			return this;
			}
		last_node = last_node->next;
		}
	return this;
	}

UnitScoreNode* UnitScoreNode::Remove (Unit u)
	{
	USNode		last_node,temp;

	if (u == unit)
		{
		temp = next;
		delete this;
		return temp;
		}

	last_node = this;
	while (last_node)
		{
		if (last_node->next->unit == u)
			{
			temp = last_node->next;
			last_node->next = temp->next;
			delete temp;
			return this;
			}
		last_node = last_node->next;
		}
	return this;
	}

UnitScoreNode* UnitScoreNode::Purge (void)
	{
	USNode		temp,cur = next;

	while (cur)
		{
		temp = cur;
		cur = cur->next;
		delete temp;
		}
	delete this;
	return NULL;
	}

UnitScoreNode* UnitScoreNode::Sort (int sort_by)
	{
	USNode		temp,last,head=NULL;

	last = this;
	temp = next;
	while (temp)
		{
		last->next = NULL;
		if (head)
			head = head->Insert(last, sort_by);
		else
			head = last->Insert(last, sort_by);
		last = temp;
		temp = temp->next;
		}
	if (head)
		head = head->Insert(last, sort_by);
	else
		head = last->Insert(last, sort_by);
	return head;
	}

// ================================
// Sorted objective list components
// ================================

#define GODN_SORT_BY_PRIORITY		1
#define GODN_SORT_BY_OPTIONS		2

GndObjDataType::GndObjDataType (void)
	{
	obj = NULL;
	priority_score = 0;
	unit_options = 0;
	unit_list = NULL;
	next = NULL;
	}

GndObjDataType::~GndObjDataType (void)
	{
	if (unit_list)
		unit_list->Purge();
	}

GODNode GndObjDataType::Insert (GODNode to_insert, int sort_by)
	{
	GODNode temp;

	if (sort_by == GODN_SORT_BY_PRIORITY)
		{
		if (to_insert->priority_score > priority_score)
			{
			to_insert->next = this;
			ShiAssert(this != to_insert);
			return to_insert;
			}
		if (!next || to_insert->priority_score > next->priority_score)
			{
			to_insert->next = next;
			ShiAssert(next != to_insert);
			if (to_insert != this)
				next = to_insert;
			else
				next = NULL;
			return this;
			}
		temp = next;
		while (temp->next)
			{
			if (to_insert->priority_score > temp->next->priority_score)
				{
				to_insert->next = temp->next;
				ShiAssert(temp->next != to_insert);
				temp->next = to_insert;
				return this;
				}
			temp = temp->next;
			}
		temp->next = to_insert;
		}
	else if (sort_by == GODN_SORT_BY_OPTIONS)
		{
		// Sort by best option
		if (to_insert->unit_options < unit_options)
			{
			to_insert->next = this;
			ShiAssert(this != to_insert);
			return to_insert;
			}
		if (!next || to_insert->unit_options < next->unit_options)
			{
			to_insert->next = next;
			ShiAssert(next != to_insert);
			if (to_insert != this)
				next = to_insert;
			else
				next = NULL;
			return this;
			}
		temp = next;
		while (temp->next)
			{
			if (to_insert->unit_options < temp->next->unit_options)
				{
				to_insert->next = temp->next;
				ShiAssert(temp->next != to_insert);
				temp->next = to_insert;
				return this;
				}
			temp = temp->next;
			}
		temp->next = to_insert;
		}
	else
		{
		if (to_insert != this)
			to_insert->next = this;
		else
			to_insert->next = NULL;
		return to_insert;
		}
	return this;
	}

GODNode GndObjDataType::Remove (GODNode to_remove)
	{
	GODNode temp;

	if (to_remove == this)
		{
		temp = next;
		delete this;
		return temp;
		}
	if (!next)
		return this;
	temp = this;
	while (temp->next)
		{
		if (temp->next == to_remove)
			{
			temp->next = to_remove->next;
			delete to_remove;
			return this;
			}
		temp = temp->next;
		}
	return this;
	}

GODNode GndObjDataType::Remove (Objective o)
	{
	GODNode temp,to_remove;

	if (o == obj)
		{
		temp = next;
		delete this;
		return temp;
		}
	if (!next)
		return this;
	temp = this;
	while (temp->next)
		{
		if (temp->next->obj == o)
			{
			to_remove = temp->next;
			temp->next = to_remove->next;
			delete to_remove;
			return this;
			}
		temp = temp->next;
		}
	return this;
	}

GODNode GndObjDataType::Purge (void)
	{
	GODNode		temp;

	while (next)
		{
		temp = next;
		next = next->next;
		delete temp;
		}
	delete this;
	return NULL;
	}

GODNode GndObjDataType::Sort (int sort_by)
	{
	// Reorder by # of options
	GODNode		temp,last,head=NULL;

	last = this;
	temp = next;
	while (temp)
		{
		last->next = NULL;
		if (head)
			head = head->Insert(last, sort_by);
		else
			head = last->Insert(last, sort_by);
		last = temp;
		temp = temp->next;
		}
	if (head)
		head = head->Insert(last, sort_by);
	else
		head = last->Insert(last, sort_by);
	return head;
	}

void GndObjDataType::InsertUnit (Unit u, int s, int d)
	{
	USNode		new_node;
		
	if (s <= 0 || d <= 0)
		return;

	new_node = new UnitScoreNode;
	new_node->unit = u;
	new_node->score = s;
	new_node->distance = d;
	new_node->next = NULL;
	unit_options += GetOptions(new_node->distance);
	if (!unit_list)
		unit_list = new_node;
	unit_list = unit_list->Insert(new_node, USN_SORT_BY_DISTANCE);
	}

UnitScoreNode* GndObjDataType::RemoveUnit (Unit u)
	{
	USNode		temp = unit_list;

	while (temp)
		{
		if (temp->unit == u)
			{
			unit_options -= GetOptions(temp->distance);
			unit_list = unit_list->Remove(u);
			return unit_list;
			}
		temp = temp->next;
		}
	return unit_list;
	}

void GndObjDataType::RemoveUnitFromAll (Unit u)
	{
	GODNode		cur = this;

	while (cur)
		{
		cur->RemoveUnit(u);
		cur = cur->next;
		}
	}

void GndObjDataType::PurgeUnits (void)
	{
	if (unit_list)
		unit_list->Purge();
	}

// =======================================
// Support functions
// =======================================

int GetOptions (int score)
	{
	if (score > 0)
		return (score/26)+1;
	return 0;
	}




