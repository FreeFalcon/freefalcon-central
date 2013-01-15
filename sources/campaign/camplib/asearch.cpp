#include <stddef.h>
#include "cmpglobl.h"
#include "asearch.h"
//sfr: added for checks
#include "InvalidBufferException.h"

// A-Search is an A* Search algorythm which can be used to find the best-cost
// path between two nodes in a directed graph or a grid map (specialized directed
// graph, essentially.

// User's extend function should take a location and find the following
// information on that location's neighbors (and call ASFillNode)
// * cost to move from location to neighbor;
// * estimated cost to go (lower bound);
// * the direction taken to get to the neighbor;
// * a pointer to either the coordinate or objective we're dealing with
//   or zero if no such neighbor exists.

// ==========================================
// Our Global search Structure
// ==========================================

// Allocation moved to SystemLevelInit().
ASData      ASD;

// ==========================================
// Path class
// ==========================================

PathClass::PathClass (void)
{
	max_length = MAX_DISTANCE;
	path = path_pool;
	length = current_location = 0;
	cost = 0.0F;
}

int PathClass::SaveSize (void)
{
	return sizeof(uchar) + sizeof(uchar) + sizeof(uchar) + sizeof(uchar)*(length+1)/PATH_DIV;
}

SmallPathClass::SmallPathClass(void)
{
	max_length = SMALL_PATH_MAX_DISTANCE;
	path = path_pool;
	length = current_location = 0;
	cost = 0.0F;
}
//sfr: stream functions
SmallPathClass::SmallPathClass(uchar **stream, long *rem) : BasePathClass(stream, rem){
	memcpychk(path_pool, stream, sizeof(path_pool), rem);
}

int SmallPathClass::Save(uchar **stream){
	BasePathClass::Save(stream);
	memcpy(*stream, path_pool, sizeof(path_pool));
	*stream += sizeof(path_pool);
	return SaveSize();
}

int SmallPathClass::SaveSize (void)
{
	//return sizeof(uchar) + sizeof(uchar) + sizeof(uchar) + sizeof(uchar)*(length+1)/PATH_DIV;
	return BasePathClass::SaveSize() + sizeof(path_pool);
}

BasePathClass::BasePathClass (void)
{
	max_length = MAX_DISTANCE;
	length = current_location = 0;
	cost = 0.0F;
}

BasePathClass::BasePathClass (uchar **stream, long *rem){
	memcpychk(&max_length, stream,sizeof(uchar), rem);					
	memcpychk(&length,stream,sizeof(uchar), rem);						
	memcpychk(&current_location, stream, sizeof(uchar), rem);
	memcpychk(path, stream, sizeof(uchar)*(length+1)/PATH_DIV, rem);		
	cost = 0.0F;			// Cost is pretty meaningless by this point
}

BasePathClass::~BasePathClass ()
{
	//	delete path;
}

int BasePathClass::Save (uchar **stream)
{
	memcpy(*stream,&max_length,sizeof(uchar));					*stream += sizeof(uchar);
	memcpy(*stream,&length,sizeof(uchar));						*stream += sizeof(uchar);
	memcpy(*stream,&current_location,sizeof(uchar));			*stream += sizeof(uchar);
	memcpy(*stream,path,sizeof(uchar)*(length+1)/PATH_DIV);	*stream += sizeof(uchar)*(length+1)/PATH_DIV;
	return SaveSize();
}

int BasePathClass::SaveSize (void)
{
	return sizeof(uchar) + sizeof(uchar) + sizeof(uchar) + sizeof(uchar)*(length+1)/PATH_DIV;
}

int BasePathClass::GetNextDirection (void)
{
	if (current_location >= length)
		return MAX_NEIGHBORS;
	int	i = current_location/PATH_DIV;
	int o = current_location%PATH_DIV;
	return (path[i] >> (o*PATH_BITS)) & PATH_MASK;
}

int BasePathClass::GetPreviousDirection (int num)
{
	int l = current_location - num;
	if (l < 0)
		return MAX_NEIGHBORS;
	int	i = l/PATH_DIV;
	int o = l%PATH_DIV;
	return (path[i] >> (o*PATH_BITS)) & PATH_MASK;
}

int BasePathClass::GetDirection (int num)
{
	if (num >= length)
		return MAX_NEIGHBORS;
	int	i = num/PATH_DIV;
	int o = num%PATH_DIV;
	return (path[i] >> (o*PATH_BITS)) & PATH_MASK;
}

void BasePathClass::SetDirection (int num, int d)
{
	if (d < 0)
		d = MAX_NEIGHBORS;
	int	i = num/PATH_DIV;
	int o = num%PATH_DIV;
	uchar temp = (unsigned char)(PATH_MASK << (o*PATH_BITS));
	path[i] &= ~temp;
	path[i] |= d << (o*PATH_BITS);
}

void BasePathClass::StepPath (void)
{
	current_location++;
}

void BasePathClass::SetNextDirection (int d)
{
	SetDirection(current_location, d);
}

int BasePathClass::AddToPath (int d, costtype c)
{
	if (length == max_length)
		return 0;

	SetDirection(length, d);
	cost = c;
	length++;
	return length;
}

void BasePathClass::ClearPath (void)
{
	length = 0;
}

int BasePathClass::CopyPath (BasePathClass *from_path)
{
	cost = from_path->cost;
	current_location = from_path->current_location;
	if (from_path->length > max_length)
	{
		length = max_length;
		if (from_path->current_location > max_length)
			current_location = max_length;
		memcpy(path,from_path->path,((max_length*PATH_BITS)+7)/8);
		return 0;
	}
	else
	{
		length = from_path->length;
		memcpy(path,from_path->path,((from_path->length*PATH_BITS)+7)/8);
		return 1;
	}
}

// ==========================================
// AS_Node class
// ==========================================

/*
   AS_NodeClass::AS_NodeClass (void)
   {
   next = NULL;
   where = NULL;
   }

   AS_NodeClass::~AS_NodeClass (void)
   {
   }
 */

// ==========================================
// AS_Data class
// ==========================================

AS_DataClass::AS_DataClass (void)
{
	int			i;

	queue = NULL;
	tried = NULL;
	cs = NULL;
#ifdef AS_THREAD_SAFE
	cs = F4CreateCriticalSection ("AS_CS");
#endif
	// Allocate a big chunk of nodes
	node_ptr = new AS_NodeClass[MAX_SEARCH];
	// We want our first node to be the current location
	location = &node_ptr[0];
	location->cost = 0.0F;
	location->to_go = 0.0F;
	location->where = 0;
	location->next = NULL;
	// Build the waste list from the remaining chunks
	for (i=1; i<MAX_SEARCH-1; i++)
		node_ptr[i].next = &node_ptr[i+1];
	node_ptr[MAX_SEARCH-1].next = NULL;
	waste = &node_ptr[1];
}

AS_DataClass::~AS_DataClass (void)
{
#ifdef AS_THREAD_SAFE
	F4DestroyCriticalSection (cs);
	cs = NULL; // JB 010108
#endif

#ifdef DEBUG
	// Make sure we didn't lose any nodes during our run
	int		count = 1;		// 1 for location
	ASNode	T;
	T = queue;
	while (T)
	{
		count++;
		T = T->next;
	}
	T = tried;
	while (T)
	{
		count++;
		T = T->next;
	}
	T = waste;
	while (T)
	{
		count++;
		T = T->next;
	}
	ShiAssert (count == MAX_SEARCH);
#endif

	delete [] node_ptr;
}

// This is the main search routine. It must be passed an extension funtion
// return values: -1 on error, 0 if partial path found, 1 if full path found
int AS_DataClass::ASSearch (Path p, void* origin, void* target, void (*extend)(AS_DataClass* asd, void* o, void* t), int flags, int maxSearch, costtype maxCost)
{
	int      	count = 0, retval = -1, max_length;
	ASNode   	T;
	float		best;

	if (origin == target)
		return 1;
	if (target == NULL)
		return -1;

#ifdef AS_THREAD_SAFE
	F4EnterCriticalSection(cs);
#endif
	AS_attach_queues();
	location->where = origin;
	location->cost = 0.0F;
	location->next = NULL;
	location->path.ClearPath();
	p->ClearPath();
	max_length = p->GetMaxLength();
	while (count < maxSearch && waste)
	{
		(*extend)(this, location->where, target);
		AS_merge(count);
		if (queue)
		{
			T = queue;
			queue = T->next;
			T->next = NULL;
			location->next = tried;
			tried = location;
			location = T;
		}
		else								
		{
			// No possible moves
#ifdef AS_THREAD_SAFE
			F4LeaveCriticalSection(cs);
#endif
			return retval;
		}

		// Check for solution
		if (location->where == target)
		{
			// We're done - Found full path
			p->CopyPath(&location->path);
#ifdef AS_THREAD_SAFE
			F4LeaveCriticalSection(cs);
#endif
			return 1;
		}

		// Check for exceeding the passed path's length
		if (location->path.GetLength() >= max_length)
		{
			if (flags & RETURN_PARTIAL_ON_MAX)
			{
				// return a partial path
				p->CopyPath(&location->path);
				retval = 0;
			}

#ifdef AS_THREAD_SAFE
			F4LeaveCriticalSection(cs);
#endif
			return retval;
		}

		// Check for exceeding max cost and abort if so
		if (maxCost && location->cost > maxCost)
			count = maxSearch;

		count++;
	}

	// No solution found (timed out).
	if (flags & RETURN_PARTIAL_ON_FAIL)	
	{
		// Return a path to the best location so far
		T = queue;
		best = 99999.0F;
		while (T != NULL)
		{
			if (T->cost > 0 && T->to_go < best)
			{
				best = T->to_go;
				p->CopyPath(&T->path);
			}
			T = T->next;
		}
		T = tried;
		while (T != NULL)
		{
			if (T->cost > 0 && T->to_go < best)
			{
				best = T->to_go;
				p->CopyPath(&T->path);
			}
			T = T->next;
		}
		retval = 0;
	}
#ifdef AS_THREAD_SAFE
	F4LeaveCriticalSection(cs);
#endif
	return retval;
}

// Our User's extend function calls this to update our data structure.
// Required arguments are:
// node:    which neighbor we're filling
// cost:    how much it 'costs' to move from our location to the neighbor
// to_go:   Lower bound guess as to how far to our destination
// dir:     The direction taken to get to this neighbor
// where:   pointer to the new neighbor (Coordinate or Objective)
//
//void AS_DataClass::ASFillNode (int node, costtype *cost, costtype *to_go, char dir, void* where)
void AS_DataClass::ASFillNode (int node, costtype *cost, costtype *to_go, char, void* where)
{
	neighbors[node].where = where;
	if (where)
	{
		neighbors[node].cost = *cost + location->cost;
		neighbors[node].to_go = *to_go;
	}
}

void AS_DataClass::AS_dispose_queue (ASNode N)
{
	ASNode   T;

	while (N != NULL)
	{
		T = N->next;
		delete N;
		N = T;
	}
}

// slops old queues onto the waste pile
void AS_DataClass::AS_attach_queues(void)
{
	ASNode   N;

	if (queue != NULL)
	{
		N = queue;
		while (N->next != NULL)
			N = N->next;
		N->next = waste;
		waste = queue;
		queue = NULL;
	}
	if (tried != NULL)
	{
		N = tried;
		while (N->next != NULL)
			N = N->next;
		N->next = waste;
		waste = tried;
		tried = NULL;
	}
}

// reattaches a node to the waste pile                     
void AS_DataClass::AS_reattach(ASNode n)
{
	n->next = waste;
	waste = n;
}

//void AS_DataClass::AS_merge (int count)
void AS_DataClass::AS_merge (int)
{
	int      n;
	ASNode   new_node=NULL, T, insert_after;

	for (n=0; n<MAX_NEIGHBORS; n++)
	{
		if (!neighbors[n].where)
			continue;
		if (queue == NULL || neighbors[n].cost + neighbors[n].to_go < queue->cost + queue->to_go)
		{
			new_node = AS_get_new_node(n);
			if (!new_node)
				continue;
			new_node->next = queue;
			queue = new_node;
			continue;
		}
		insert_after = NULL;
		T = queue;
		while (T->next)
		{
			if (neighbors[n].where == T->next->where)
			{
				if (insert_after)
				{
					// We've got a better version than this one, move this queue entry to waste
					ASNode	temp = T->next;
					T->next = temp->next;
					AS_reattach(temp);
				}
				// Either way, quit searching
				break;
			}
			else if (!insert_after && neighbors[n].cost+neighbors[n].to_go < T->next->cost + T->next->to_go)
				insert_after = T;
			T = T->next;
		}
		if (T && !insert_after && !T->next)
			// Insert at end of the queue
			insert_after = T;		
		if (insert_after)
		{
			new_node = AS_get_new_node(n);
			if (!new_node)
				continue;
			new_node->next = insert_after->next;
			insert_after->next = new_node;
		}
	}
}

ASNode AS_DataClass::AS_get_new_node(int n)
{
	ASNode   new_node,T;

	// Ignore this neighbor if already tried
	T = tried;
	while (T != NULL)
	{
		// KCK: The following line is only usefull if the user's huristic overestimates -
		// And even then, it only gets slightly better answers for a moderate cost.
		//		if (T->where == neighbors[n].where && neighbors[n].cost+neighbors[n].to_go > T->cost + T->to_go)
		if (T->where == neighbors[n].where)
			return NULL;
		T = T->next;
	}

	if (waste)
	{
		new_node = waste;
		waste = waste->next;
	}
	else
		return NULL;			// We're out of waste nodes. We'll quit looking.
	new_node->cost = neighbors[n].cost;
	new_node->to_go = neighbors[n].to_go;
	new_node->where = neighbors[n].where;
	new_node->next = NULL;
	new_node->path.CopyPath(&location->path);
	if (new_node->path.AddToPath(n,neighbors[n].cost))
		return new_node;
	else
	{
		AS_reattach(new_node);
		return NULL;
	}
}





