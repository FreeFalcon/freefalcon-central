#include <windows.h>
#include <tchar.h>

#ifndef uchar
typedef unsigned char uchar;
#endif

#include "campweap.h"
#include "cstores.h"

void StoresList::Cleanup()
{
	int i;

	for(i=0;i<_ALL_;i++)
		RemoveAll(i);
	for(i=0;i<_ALL_;i++)
		Stores_[0]=NULL;
	current=NULL;
	ListID_=_ALL_;
	GetType_=_ALL_;
}

STORESLIST *StoresList::Create(long ID,_TCHAR *Name,long Type,long wgt,long fuel,float df,short stock)
{
	STORESLIST *store;
	int i;

	if(Find(ID))
		return(current);

	store=new STORESLIST;
	if(store == NULL)
		return(NULL);

	store->ID=ID;
	_tcscpy(store->Name,Name);
	store->Type=Type;
	store->Weight=wgt;
	store->Fuel=fuel;
	store->DragFactor=df;
	store->Stock=stock;
	for(i=0;i<HARDPOINT_MAX;i++)
		store->HardPoint[i]=0;
	store->Next=NULL;

	return(store);
}

void StoresList::AddHardPoint(long ID,long hp,short count)
{
	STORESLIST *cur;

	cur=Find(ID);

	if(cur == NULL || hp >= HARDPOINT_MAX)
		return;

	cur->HardPoint[hp]=count;
}

void StoresList::Add(STORESLIST *store,STORESLIST **list)
{
	STORESLIST *cur;

	if(store == NULL) return;

	if(*list == NULL)
		*list=store;
	else
	{
		cur=*list;
		while(cur->Next)
			cur=cur->Next;
		cur->Next=store;
	}
}

void StoresList::RemoveAll(STORESLIST **list)
{
	STORESLIST *cur,*prev;

	cur=*list;
	while(cur)
	{
		prev=cur;
		cur=cur->Next;
		delete prev;
	}
	*list=NULL;
}

void StoresList::Remove(long ID,STORESLIST **top)
{
	STORESLIST *cur,*delme;

	if(*top == NULL) return;

	cur=*top;
	if((*top)->ID == ID)
	{
		*top=cur->Next;
		delete cur;
	}
	else
	{
		while(cur->Next)
		{
			if(cur->Next->ID == ID)
			{
				delme=cur->Next;
				cur->Next=cur->Next->Next;
				delete delme;
				return;
			}
			cur=cur->Next;
		}
	}
}

void StoresList::Sort(long ID)
{
	STORESLIST *list;
	STORESLIST *sort1,*sort2;
	STORESLIST temp;
	int i;

	if(ID >= _ALL_) return;

	list=Stores_[ID];

	if(list == NULL)
		return;

	sort2=list->Next;

	while(sort2)
	{
		sort1=list;
		while(sort1 != sort2)
		{
			if(stricmp(sort2->Name,sort1->Name) < 0)
			{
				temp.ID=sort1->ID;
				_tcscpy(temp.Name,sort1->Name);
				temp.Type=sort1->Type;
				temp.Weight=sort1->Weight;
				temp.Fuel=sort1->Fuel;
				temp.DragFactor=sort1->DragFactor;
				temp.Stock=sort1->Stock;
				for(i=0;i<HARDPOINT_MAX;i++)
					temp.HardPoint[i]=sort1->HardPoint[i];

				sort1->ID=sort2->ID;
				_tcscpy(sort1->Name,sort2->Name);
				sort1->Type=sort2->Type;
				sort1->Weight=sort2->Weight;
				sort1->Fuel=sort2->Fuel;
				sort1->DragFactor=sort2->DragFactor;
				sort1->Stock=sort2->Stock;
				for(i=0;i<HARDPOINT_MAX;i++)
					sort1->HardPoint[i]=sort2->HardPoint[i];
			
				sort2->ID=temp.ID;
				_tcscpy(sort2->Name,temp.Name);
				sort2->Type=temp.Type;
				sort2->Weight=temp.Weight;
				sort2->Fuel=temp.Fuel;
				sort2->DragFactor=temp.DragFactor;
				sort2->Stock=temp.Stock;
				for(i=0;i<HARDPOINT_MAX;i++)
					sort2->HardPoint[i]=temp.HardPoint[i];
			}
			sort1=sort1->Next;
		}
		sort2=sort2->Next;
	}
}