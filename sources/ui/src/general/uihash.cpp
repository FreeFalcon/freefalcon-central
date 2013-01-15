#include <windows.h>
#include "uihash.h"

UI_Hash::UI_Hash()
{
	TableSize_=0;
	Table_=NULL;

	Callback_=NULL;
}

UI_Hash::~UI_Hash()
{
	if(TableSize_ || Table_)
		Cleanup();
}

void UI_Hash::Setup(unsigned long Size)
{
	unsigned short i;
	TableSize_=Size;

	Table_=new UI_HASHROOT[TableSize_];
	for(i=0;i<TableSize_;i++)
		Table_[i].Root_=NULL;
}

void UI_Hash::Cleanup()
{
	unsigned long i;
	UI_HASHNODE *cur,*prev;

	if(TableSize_ || Table_)
	{
		for(i=0;i<TableSize_;i++)
		{
			cur=Table_[i].Root_;
			while(cur)
			{
				prev=cur;
				cur=cur->Next;
				if(Callback_)
					(*Callback_)(prev->Record);
				else
					delete prev->Record;
				delete prev;
			}
		}
		delete Table_;
		Table_=NULL;
		TableSize_=0;
	}
}

void *UI_Hash::Find(unsigned long ID)
{
	unsigned long idx;
	UI_HASHNODE *cur;

	if(!TableSize_ || !Table_) return(NULL);

	idx=ID % TableSize_;
	cur=Table_[idx].Root_;
	while(cur)
	{
		if(cur->ID == ID)
		{
			return(cur->Record);
		}
		cur=cur->Next;
	}
	return(NULL);
}

void UI_Hash::Add(unsigned long ID,void *rec)
{
	unsigned long idx;
	UI_HASHNODE *cur,*newhash;

	if(!TableSize_ || !Table_ || !rec) return;

	if(Find(ID)) return;

	newhash=new UI_HASHNODE;
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

void UI_Hash::Remove(unsigned long ID)
{
	unsigned long idx;
	UI_HASHNODE *cur,*prev;

	if(!TableSize_ || !Table_) return;

	idx=ID % TableSize_;

	if(!Table_[idx].Root_) return;

	Table_[idx].Root_;
	if(Table_[idx].Root_->ID == ID)
	{
		prev=Table_[idx].Root_;
		Table_[idx].Root_=Table_[idx].Root_->Next;
		if(Callback_)
			(*Callback_)(prev->Record);
		else
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
				if(Callback_)
					(*Callback_)(prev->Record);
				else
					delete prev->Record;
				delete prev;
				return;
			}
			cur=cur->Next;
		}
	}
}

void *UI_Hash::GetFirst(UI_HASHNODE **current,unsigned long *curidx)
{
	UI_HASHNODE *cur;

	*curidx=0;
	*current=NULL;

	cur=Table_[*curidx].Root_;
	while(!cur && *curidx < (TableSize_-1))
	{
		(*curidx)++;
		cur=Table_[*curidx].Root_;
	}
	if(*curidx < TableSize_)
	{
		*current=cur;
		if(cur)
			return(cur->Record);
	}
	*current=NULL;
	return(NULL);
}

void *UI_Hash::GetNext(UI_HASHNODE **current,unsigned long *curidx)
{
	UI_HASHNODE *cur;

	if(!*current)
		return(NULL);

	cur=(*current)->Next;
	while(!cur && *curidx < (TableSize_-1))
	{
		(*curidx)++;
		cur=Table_[*curidx].Root_;
	}
	*current=cur;
	if(cur)
		return(cur->Record);
	return(NULL);
}
