#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "hash.h"

C_Hash::C_Hash()
{
	flags_=0;
	TableSize_=0;
	Table_=NULL;

	curidx_=0;
	Current_=NULL;
}

C_Hash::~C_Hash()
{
	if(TableSize_ || Table_)
		Cleanup();
}

void C_Hash::Setup(long Size)
{
	int i;
	TableSize_=Size;

	Table_=new C_HASHROOT[TableSize_];
	for(i=0;i<TableSize_;i++)
		Table_[i].Root_=NULL;
}

void C_Hash::Cleanup()
{
	long i;
	C_HASHNODE *cur,*prev;

	if(TableSize_ || Table_)
	{
		for(i=0;i<TableSize_;i++)
		{
			cur=Table_[i].Root_;
			while(cur)
			{
				prev=cur;
				cur=cur->Next;
				if(flags_ & HSH_REMOVE)
					delete prev->Record;
				delete prev;
			}
		}
		delete Table_;
		Table_=NULL;
		TableSize_=0;
	}
}

void *C_Hash::Find(long ID)
{
	long idx;
	C_HASHNODE *cur;

	if(!TableSize_ || !Table_ || ID < 0) return(NULL);

	idx=ID % TableSize_;
	cur=Table_[idx].Root_;
	while(cur)
	{
		if(cur->ID == ID)
			return(cur->Record);
		cur=cur->Next;
	}
	return(NULL);
}

void C_Hash::Add(long ID,void *rec)
{
	long idx;
	C_HASHNODE *cur,*newhash;

	if(!TableSize_ || !Table_ || !rec || ID < 0) return;

	if(Find(ID)) return;

	newhash=new C_HASHNODE;
	newhash->ID=ID;
	newhash->Record=rec;
	newhash->Next=NULL;

	idx=ID % TableSize_;

	if(!Table_[idx].Root_)
	{
		Table_[idx].Root_=newhash;
	}
	else
	{
		cur=Table_[idx].Root_;
		while(cur->Next)
			cur=cur->Next;
		cur->Next=newhash;
	}
}

long C_Hash::AddText(char *string)
{
	long idx,ID,i;
	C_HASHNODE *cur,*newhash;
	char *data;

	if(!TableSize_ || !Table_ || !string) return(-1);

	ID=0;
	idx=strlen(string);
	for(i=0;i<idx;i++)
		ID += string[i];

	idx=ID % TableSize_;

	cur=Table_[idx].Root_;
	while(cur)
	{
		if(strcmp(string,(char *)cur->Record) == 0)
			return(cur->ID);
		cur=cur->Next;
	}

	i=strlen(string);
	newhash=new C_HASHNODE;
	data=new char[i+1];
	strncpy(data,string,i);
	data[i]=0;
	newhash->Record=data;
	newhash->Next=NULL;

	ID=idx << 16;
	if(!Table_[idx].Root_)
	{
		Table_[idx].Root_=newhash;
		newhash->ID=idx << 16;
	}
	else
	{
		ID++;
		cur=Table_[idx].Root_;
		while(cur->Next)
		{
			ID++;
			cur=cur->Next;
		}
		cur->Next=newhash;
		newhash->ID=ID;
	}
	return(newhash->ID);
}

long C_Hash::AddTextID(long TextID,char *string)
{
	long idx,ID,i;
	C_HASHNODE *cur,*newhash;
	char *data;

	if(!TableSize_ || !Table_ || !string) return(-1);

	ID=0;
	idx=strlen(string);
	for(i=0;i<idx;i++)
		ID += string[i];

	idx=ID % TableSize_;

	cur=Table_[idx].Root_;
	while(cur)
	{
		if(strcmp(string,(char *)cur->Record) == 0)
			return(cur->ID);
		cur=cur->Next;
	}

	i=strlen(string);
	newhash=new C_HASHNODE;
	data=new char[i+1];
	strncpy(data,string,i);
	data[i]=0;
	newhash->ID=TextID;
	newhash->Record=data;
	newhash->Next=NULL;

	ID=idx << 16;
	if(!Table_[idx].Root_)
	{
		Table_[idx].Root_=newhash;
	}
	else
	{
		ID++;
		cur=Table_[idx].Root_;
		while(cur->Next)
		{
			ID++;
			cur=cur->Next;
		}
		cur->Next=newhash;
	}
	return(ID);
}

char *C_Hash::FindText(long ID)
{
	long idx,i;
	C_HASHNODE *cur;

	if(!TableSize_ || !Table_ || (ID < 0)) return(NULL);

	idx=ID >> 16;
	i=ID & 0x0000ffff;
	cur=Table_[idx].Root_;
	for(i=0;i<(ID & 0x0000ffff) && cur;i++)
		cur=cur->Next;

	if(cur)
		return((char *)cur->Record);
	return(NULL);
}

long C_Hash::FindTextID(char *string)
{
	long idx,i;
	long ID;
	C_HASHNODE *cur;

	if(!TableSize_ || !Table_ || !string) return(-1);

	ID=0;
	idx=strlen(string);
	for(i=0;i<idx;i++)
		ID += string[i];

	idx=ID % TableSize_;

	cur=Table_[idx].Root_;
	while(cur)
	{
		if(strcmp(string,(char *)cur->Record) == 0)
			return(cur->ID);
		cur=cur->Next;
	}
	return(-1);
}

long C_Hash::FindTextID(long ID)
{
	long idx,i;
	C_HASHNODE *cur;

	if(!TableSize_ || !Table_ || (ID < 0)) return(NULL);

	idx=ID >> 16;
	i=ID & 0x0000ffff;
	cur=Table_[idx].Root_;
	for(i=0;i<(ID & 0x0000ffff) && cur;i++)
		cur=cur->Next;

	if(cur)
		return(cur->ID);
	return(-1);
}

void C_Hash::Remove(long ID)
{
	long idx;
	C_HASHNODE *cur,*prev;

	if(!TableSize_ || !Table_ || (ID < 0)) return;

	idx=ID % TableSize_;

	if(!Table_[idx].Root_) return;

	if(Table_[idx].Root_->ID == ID)
	{
		prev=Table_[idx].Root_;
		Table_[idx].Root_=Table_[idx].Root_->Next;
		if(flags_ & HSH_REMOVE)
			delete prev->Record;
		delete prev;
	}
	else
	{
		cur=Table_[idx].Root_;
		while(cur->Next)
		{
			if(cur->Next->ID == ID)
			{
				prev=cur->Next;
				cur->Next=cur->Next->Next;
				if(flags_ & HSH_REMOVE)
					delete prev->Record;
				delete prev;
				return;
			}
			cur=cur->Next;
		}
	}
}
void *C_Hash::GetFirst()
{
	C_HASHNODE *cur;

	curidx_=0;
	Current_=NULL;

	cur=Table_[curidx_].Root_;
	while(!cur && curidx_ < (TableSize_-1))
	{
		curidx_++;
		cur=Table_[curidx_].Root_;
	}
	Current_=cur;
	if(cur)
		return(cur->Record);
	return(NULL);
}

void *C_Hash::GetNext()
{
	C_HASHNODE *cur;

	if(!Current_)
		return(NULL);

	cur=Current_->Next;
	while(!cur && curidx_ < (TableSize_-1))
	{
		curidx_++;
		cur=Table_[curidx_].Root_;
	}
	Current_=cur;
	if(cur)
		return(cur->Record);
	return(NULL);
}
