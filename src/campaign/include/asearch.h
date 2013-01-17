#ifndef ASEARCH_H
#define ASEARCH_H

// A-Search is an A* Search algorythm which can be used to find the best-cost
// path between two nodes in a directed graph or a grid map (specialized directed
// graph, essentially.

#define AS_THREAD_SAFE	1

#ifdef AS_THREAD_SAFE
#include "F4Thread.h"						// Insert your thread package here
#endif 

// ==========================================
// ASearch flags
// ==========================================

#define RETURN_EMPTY_ON_FAIL		0x00	// Return an empty path if search failed
#define	RETURN_PARTIAL_ON_FAIL		0x01	// Return a partial path if we failed to complete before timeout
#define RETURN_PARTIAL_ON_MAX		0x02	// Return a partial path if we exceed our path length

// ==========================================
// A* Data types and defines
// ==========================================

#define MAX_SEARCH		2000				// How many locations were willing to scan
#define MAX_DISTANCE	96					// Longest path we'll look for
#define MAX_NEIGHBORS	8					// How many neighbors we can have
#define PATH_BITS		4					// How many bits needed per direction (0-8 requires 4 bits)
											// 8 should be divisible by this (i.e: 2,4 or 8)
#define PATH_MASK		0xF					// binary number, PATH_BITS wide, all 1s.

#define PATH_ARRAY  ((MAX_DISTANCE*PATH_BITS)+7)/8		// # of array entries
#define PATH_DIV	(8/PATH_BITS)

#define SMALL_PATH_MAX_DISTANCE		8		// Maximum length of our special smaller version of the path class

typedef float costtype;

typedef unsigned char uchar;

// ==========================================
// A* classes
// ==========================================

// ==========
// Path Class
// ==========

class SmallPathClass;
class MaxPathClass;

class BasePathClass
	{
	friend class SmallPathClass;
	friend class PathClass;
	private:
		costtype	cost;					// Cost of full path
		uchar		length;					// Length of the path
		uchar		max_length;				// Longest this path can get
		uchar		current_location;
		uchar		*path;					// The actual path bits

	public:
		BasePathClass (void);
		//sfr: added rem
		//BasePathClass (uchar **stream);
		BasePathClass (uchar **stream, long *rem);
		~BasePathClass ();
		int Save (uchar **stream);
		int SaveSize (void);

		int GetNextDirection (void);		// Get the next direction in the path
		int GetPreviousDirection (int num);	// Get the numth direction before current direction
		int GetDirection (int num);			// Get the numth direction in the path
		int GetCurrentPosition (void)			{ return current_location; }
		int GetLength (void)					{ return length; }
		int GetMaxLength (void)					{ return max_length; }
		costtype GetCost (void)					{ return cost; }

		void SetPathPosition (unsigned char num){ current_location = num; }
		void SetDirection (int num, int d);
		void SetNextDirection (int d);
		void SetCost (costtype c)				{ cost = c; }
		void StepPath (void);				// Move to the next direction
		int AddToPath (int d, costtype c);
		void ClearPath (void);
		int CopyPath (BasePathClass* from_path);// Copy from from_path
	};

class PathClass : public BasePathClass
	{
	private:
		uchar		path_pool[((MAX_DISTANCE*PATH_BITS)+7)/8];		// The actual path bits
	public:
		PathClass (void);
		int SaveSize (void);
	};

class SmallPathClass : public BasePathClass
	{
	private:
		uchar		path_pool[((SMALL_PATH_MAX_DISTANCE*PATH_BITS)+7)/8];		// The actual path bits
	public:
		SmallPathClass (void);
		// sfr: added serialization functions functions
		SmallPathClass(uchar **stream, long *rem);
		int Save(uchar **stream);
		int SaveSize (void);
	};

typedef BasePathClass* Path;

// =============
// AS_Node Class
// =============

class AS_NodeClass
	{
	private:
	public:
		AS_NodeClass*	next;						// used by queue
		costtype		cost;						// How much it cost to get here
		costtype		to_go;						// Minimum guess as to how much further
		void*			where;						// Coordinate or Object we're at
		PathClass		path;						// How we got there
	public:
//		AS_NodeClass (void);
//		~AS_NodeClass (void);
	};
typedef AS_NodeClass* ASNode;

// =============
// AS_Data Class
// =============

class AS_DataClass
	{
	private:
		ASNode			queue;						// The queue of locations to search
		ASNode			tried;						// The list of nodes visited
		ASNode			waste;						// Unused nodes
		ASNode			location;					// Where we are right now
		ASNode			node_ptr;					// Array of all our nodes
		AS_NodeClass	neighbors[MAX_NEIGHBORS];	// The list of current neighbors
#ifdef AS_THREAD_SAFE
		F4CSECTIONHANDLE* cs;						// Critical section for this search class
#endif
	public:
		AS_DataClass (void);
		~AS_DataClass (void);

		// This is the key function
		// returns:		 1 = Successfull search
		//				 0 = Found partial path
		//				-1 = Unable to find a path
		int ASSearch (Path p, void* origin, void* target, void (*ex)(AS_DataClass* asd, void* o, void* t), int flags, int max_search, costtype max_cost);

		// This needs to be called per potential direction by the extend function
		void ASFillNode (int node, costtype *cost, costtype *to_go, char dir, void* where);

	private:
		void AS_dispose_queue (ASNode N);
		void AS_attach_queues(void);
		void AS_reattach(ASNode n);
		void AS_merge (int count);
		ASNode AS_get_new_node(int n);
	};
typedef AS_DataClass* ASData;

// ======================================
// The one and only ASearch class
// ======================================

extern	ASData			ASD;

#endif
