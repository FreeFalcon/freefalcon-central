#include <windows.h>
#include "vu2.h"
#include "acmihash.h"

ACMI_Hash::ACMI_Hash()
{
	TableSize_=0;
	ID_=1;
	Table_=NULL;
}

ACMI_Hash::~ACMI_Hash()
{
	if(TableSize_ || Table_)
		Cleanup();
}

void ACMI_Hash::Setup(unsigned long Size)
{
	unsigned short i;
	TableSize_=Size;

	Table_=new ACMI_HASHROOT[TableSize_];
	for(i=0;i<TableSize_;i++)
		Table_[i].Root_=NULL;
}

void ACMI_Hash::Cleanup()
{
	unsigned long i;
	ACMI_HASHNODE *cur,*prev;

	if(TableSize_ || Table_)
	{
		for(i=0;i<TableSize_;i++)
		{
			cur=Table_[i].Root_;
			while(cur)
			{
				prev=cur;
				cur=cur->Next;
				delete prev;
			}
		}
		delete Table_;
		Table_=NULL;
		TableSize_=0;
	}
}

long ACMI_Hash::Find(VU_ID ID)
{
	unsigned long idx;
	ACMI_HASHNODE *cur;

	if(!TableSize_ || !Table_) return(NULL);

	idx=(ID.creator_ | ID.num_) % TableSize_;
	cur=Table_[idx].Root_;
	while(cur)
	{
		if(cur->ID == ID)
		{
			return(cur->Index);
		}
		cur=cur->Next;
	}
	return(0);
}

ACMI_HASHNODE *ACMI_Hash::Get(VU_ID ID)
{
	unsigned long idx;
	ACMI_HASHNODE *cur;

	if(!TableSize_ || !Table_) return(NULL);

	idx=(ID.creator_ | ID.num_) % TableSize_;
	cur=Table_[idx].Root_;
	while(cur)
	{
		if(cur->ID == ID)
		{
			return(cur);
		}
		cur=cur->Next;
	}
	return(NULL);
}

long ACMI_Hash::Add(VU_ID ID,char *lbl,long color)
{
	unsigned long idx;
	ACMI_HASHNODE *cur,*newhash;

	if(!TableSize_ || !Table_) return(0);

	cur=Get(ID);
	if(cur)
	{
		if(lbl)
		{
			strncpy(cur->label,lbl,15);
			cur->label[15]=0;
			cur->color=color;
		}
		return(cur->Index);
	}

	newhash=new ACMI_HASHNODE;
	newhash->ID=ID;
	if(lbl)
	{
		strncpy(newhash->label,lbl,15);
		newhash->label[15]=0;
	}
	else
		memset(newhash->label,0,16);
	newhash->color=color;
	newhash->Index=ID_++;
	newhash->Next=NULL;

	idx=(ID.creator_ | ID.num_) % TableSize_;

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
	return(newhash->Index);
}

void ACMI_Hash::Remove(VU_ID ID)
{
	unsigned long idx;
	ACMI_HASHNODE *cur,*prev;

	if(!TableSize_ || !Table_) return;

	idx=(ID.creator_ | ID.num_) % TableSize_;

	if(!Table_[idx].Root_) return;

	Table_[idx].Root_;
	if(Table_[idx].Root_->ID == ID)
	{
		prev=Table_[idx].Root_;
		Table_[idx].Root_=Table_[idx].Root_->Next;
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
				delete prev;
				return;
			}
			cur=cur->Next;
		}
	}
}

long ACMI_Hash::GetFirst(ACMI_HASHNODE **current,unsigned long *curidx)
{
	ACMI_HASHNODE *cur;

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
			return(cur->Index);
	}
	*current=NULL;
	return(-1);
}

long ACMI_Hash::GetNext(ACMI_HASHNODE **current,unsigned long *curidx)
{
	ACMI_HASHNODE *cur;

	if(!*current)
		return(-1);

	cur=(*current)->Next;
	while(!cur && *curidx < (TableSize_-1))
	{
		(*curidx)++;
		cur=Table_[*curidx].Root_;
	}
	*current=cur;
	if(cur)
		return(cur->Index);
	return(-1);
}
