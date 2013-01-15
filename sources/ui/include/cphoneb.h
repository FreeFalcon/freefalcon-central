#ifndef _PHONE_BOOK_H_
#define _PHONE_BOOK_H_

#include <tchar.h>
#include "comdata.h"

typedef struct PhoneListStr PHONEBOOK;

#define MAX_URL_SIZE (100)

struct PhoneListStr {
	long ID;
	_TCHAR url[MAX_URL_SIZE+1];
	unsigned short localPort;
	unsigned short remotePort;
	PHONEBOOK *Next;
};

class PhoneBook
{
	private:
		PHONEBOOK *Root_;
		PHONEBOOK *Current_;


	public:

		// creates an empty list
		PhoneBook();
		// destroys list
		~PhoneBook();

		void Setup();
		void Cleanup();

		// load list from file
		void Load(char *filename);
		// save list to file
		void Save(char *filename);
		// add an entry to list
		void Add(_TCHAR *desc, unsigned short localPort, unsigned short remotePort);
		// remove an entry from list
		void Remove(long ID);
		// remove all entries from list
		void RemoveAll();
		// returns an entry
		PHONEBOOK *FindID(long ID);
		// get first entry
		PHONEBOOK *GetFirst()   { Current_=Root_; return(GetCurrent()); }
		// get next entry
		PHONEBOOK *GetNext()    { if(Current_) { Current_=Current_->Next; return(GetCurrent()); } return(NULL); }
		// get current entry
		PHONEBOOK *GetCurrent() { if(Current_) return(Current_); return(NULL); }
};

#endif