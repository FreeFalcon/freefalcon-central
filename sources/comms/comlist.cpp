/** @file ComList.cpp
* Implementation of Comms list, taking out GlobalListHead 
* @author sfr
*/

#include "ComList.h"

// global variable
ComList GlobalComList;

// C functions
void comListAdd(ComIP *comIP){
	GlobalComList.addCom(comIP);
}

void comListRemove(ComIP *comIP){
	GlobalComList.removeCom(comIP);
}

ComIP *comListGetFirstP(int protocol){
	return GlobalComList.iterBegin(protocol);
}

ComIP *comListGetNextP(int protocol){
	return GlobalComList.iterGetNext(protocol);
}

ComIP *comListFindProtocolRport(int protocol, unsigned short rport){
	return GlobalComList.findProtRport(protocol, rport);
}

ComIP *comListFindProtocolId(int protocol, unsigned long id){
	return GlobalComList.findProtId(protocol, id);
}

ComIP *comListFindDangling(int protocol){
	return GlobalComList.findProtDangling(protocol);
}


// class functions
ComList::ComList(){
	// do nothing
}

ComList::~ComList(){
	comList.clear();
}

void ComList::addCom(ComIP *com){
	// add tail
	comList.push_back(com);
}


void ComList::removeCom(ComIP *com){
	comList.remove(com);
}


// iter functions
ComIP *ComList::iterBegin(int protocol){
	for (
		iterator = comList.begin();
		iterator != comList.end();
		++iterator
	){
		// match
		ComIP *com = *iterator;
		if (
			(protocol == -1) || // matches all
			(ComAPIGetProtocol((ComAPIHandle)(com)) == protocol)
		){
			return *iterator;
		}
	}
	return NULL;
}

ComIP *ComList::iterGetNext(int protocol){
	// end of list
	if (iterator == comList.end()){
		return NULL;
	}
	// find next
	// we begin incrementing the old position
	for (++iterator; iterator != comList.end(); ++iterator ){
		ComIP *com = *iterator;
		if (
			(protocol == -1) ||
			(ComAPIGetProtocol((ComAPIHandle)(com)) == protocol)
		){
			return *iterator;
		}
	}
	return NULL;
}

// search functions
ComIP *ComList::findProtRport(int protocol, unsigned short port){
	for (ComIP *comIP = iterBegin(protocol); comIP != NULL; comIP = iterGetNext(protocol)){
		unsigned short pport = port;
		unsigned short cport = ComAPIGetRecvPort((ComAPIHandle)comIP);
		if (pport == cport){
			return comIP;
		}
	}
	// not found
	return NULL;
}

ComIP *ComList::findProtId(int protocol, unsigned long id){
	for (ComIP *comIP = iterBegin(protocol); comIP != NULL; comIP = iterGetNext(protocol)){
		if (
			(ComAPIGetPeerId((ComAPIHandle)comIP) == id)
		){
			return comIP;
		}
	}	
	return NULL;
}



ComIP *ComList::findProtDangling(int protocol){
	for (ComIP *comIP = iterBegin(protocol); comIP != NULL; comIP = iterGetNext(protocol)){
		if (
			(ComAPIGetPeerIP((ComAPIHandle)comIP) == CAPI_DANGLING_IP)
		){
			return comIP;
		}
	}	
	return NULL;
}
