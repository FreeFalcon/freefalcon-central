#include <windows.h>
#include "stdio.h"
#include "f4vu.h"
#include "cphoneb.h"
#include "vutypes.h"
#include "f4comms.h"
#include "comdata.h"

PhoneBook::PhoneBook(){
	Root_=NULL;
	Current_=NULL;
}

PhoneBook::~PhoneBook(){
	if(Root_){
		Cleanup();
	}
}

void PhoneBook::Setup(){
}

void PhoneBook::Cleanup(){
	RemoveAll();
}

PHONEBOOK *PhoneBook::FindID(long ID){
	PHONEBOOK *cur;

	cur=Root_;
	while(cur){
		if(cur->ID == ID){
			return(cur);
		}
		cur=cur->Next;
	}
	return(NULL);
}

void PhoneBook::Load(char *filename){
	long count,i;
	FILE *ifp;

	ifp=fopen(filename,"rb");
	if(!ifp){
		return;
	}

	// read number of entries
	fread(&count,sizeof(long),1,ifp);
	// read each entry
	for(i=0;i<count;i++){
		_TCHAR url[MAX_URL_SIZE+1];
		// read description
		fread(url,MAX_URL_SIZE,1,ifp);
		url[MAX_URL_SIZE]= '\0';
		// read ports
		unsigned short lp, rp;
		fread(&lp, sizeof(short), 1, ifp);
		fread(&rp, sizeof(short), 1, ifp);

		// add new entry to our list
		Add(url, lp, rp);
	}
	fclose(ifp);
}

void PhoneBook::Save(char *filename){
	PHONEBOOK *cur;
	long count;
	FILE *ofp;

	// count number of entries
	count=0;
	cur=Root_;
	while(cur){
		count++;
		cur=cur->Next;
	}

	// open file for writing
	ofp=fopen(filename,"wb");
	if(!ofp){
		return;
	}

	// write count
	fwrite(&count,sizeof(long),1,ofp);

	// go through list saving each
	cur=Root_;
	while(cur){
		// save description
		fwrite(cur->url,MAX_URL_SIZE,1,ofp);
		// save ports
		fwrite(&cur->localPort, sizeof(short), 1, ofp);
		fwrite(&cur->remotePort, sizeof(short), 1, ofp);

		cur=cur->Next;
	}

	fclose(ofp);
}

void PhoneBook::Add(_TCHAR *desc, unsigned short lp, unsigned short rp){
	PHONEBOOK *newph,*cur;

	long tmpID=1;

	while(FindID(tmpID)){
		tmpID++;
	}

	newph=new PHONEBOOK;
	newph->ID=tmpID;
	_tcsncpy(newph->url,desc,MAX_URL_SIZE-1);
	newph->url[MAX_URL_SIZE]=0;
	newph->Next=NULL;

	// tail insert
	if (Root_ == NULL){
		Root_=newph;
	}
	// add tail
	else{
		cur=Root_;
		while(cur->Next){
			cur=cur->Next;
		}
		cur->Next=newph;
	}
}

void PhoneBook::Remove(long ID)
{
	PHONEBOOK *cur,*prev;

	if(!Root_) return;

	if(Root_->ID == ID){
		cur=Root_;
		Root_=Root_->Next;
		if(Current_ == cur)
			Current_=Root_;
		delete cur;
	}
	else {
		prev=Root_;
		cur=prev->Next;
		while(cur){
			if(cur->ID == ID){
				prev->Next=cur->Next;
				if(Current_ == cur){
					Current_=Current_->Next;
				}
				delete cur;
				return;
			}
			prev=cur;
			cur=cur->Next;
		}
	}
}

void PhoneBook::RemoveAll(){
	PHONEBOOK *cur,*prev;
	cur=Root_;
	while(cur) {
		prev=cur;
		cur=cur->Next;
		delete prev;
	}
	Root_=NULL;
	Current_=NULL;
}
