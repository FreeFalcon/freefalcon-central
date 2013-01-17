#ifndef COMLIST_H
#define COMLIST_H

/**
* sfr
* ComList is a list of Com handlers. This is being used in place of GlobalList
* All functions exported must be C named.
*/

// data types here
#include "capipriv.h"


#ifdef __cplusplus
extern "C" {
#endif
// C part of the API
// so we can keep C code happy
void comListAdd(ComIP*);
void comListRemove(ComIP*);

// iteration functions P (at end of function name) here means protocol
// prepare to iterate
//ComIP *comListGetFirst();
// get next element
//ComIP *comListGetNext();
// prepare to iterate using protocol
ComIP *comListGetFirstP(int protocol);
// get next element using protocol
ComIP *comListGetNextP(int protocol);


// search functions
// find a given Com by the receive port, using hos order
ComIP *comListFindProtocolRport(int protocol, unsigned short port);
/** finds a com handle of the given protocol and id (host order) */
ComIP* comListFindProtocolId(int protocol, unsigned long id);
/** Finds a com handle to deal with a message which does not fit into any other comm in list */
ComIP *comListFindDangling(int protocol);


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
// C++
#include <list>

class ComList {
private:
	std::list<ComIP*> comList;
	std::list<ComIP*>::iterator iterator; // used to iterate on this list
	
public:
	ComList();
	~ComList();

	// add a com to the list
	void addCom(ComIP *);
	// remove a com from list
	void removeCom(ComIP *);
	// used to iterate, return NULL when no more elements
	// iter begin must always begin iteration
	// if protoco == -1, always match
	ComIP *iterBegin(int protocol);
	ComIP *iterGetNext(int protocol);
	// search functions return NULL if not found
	ComIP *findProtRport(int protocol, unsigned short port);
	ComIP *findProtId(int protocol, unsigned long id);
	// get default comms for given protocol
	ComIP *findProtDangling(int protocol);
};

// we make this global, just like GlobalListHead
// defined in ComList.cpp
extern ComList GlobalComList;

// for some weird reason, CAPI hton and ntoh functions are not working here
// lets do our own
//unsigned short comList_htons(unsigned short);
//unsigned long comList_htonl(unsigned long);

#endif // cplusplus
#endif