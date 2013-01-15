#include <windows.h>
#include <stdio.h>
#include "vu2.h"
#include "falcsess.h"
#include "uicomms.h"
#include "stats.h"

void EncryptBuffer(uchar startkey,uchar *buffer,long length);
void DecryptBuffer(uchar startkey,uchar *buffer,long length);

PlayerStats::PlayerStats()
{
	Root_=NULL;
	memset(SaveName_,0,MAX_PATH);
}

PlayerStats::~PlayerStats()
{
	StatList *cur,*last;

	cur=Root_;
	Root_=NULL;
	while(cur)
	{
		last=cur;
		cur=cur->Next;
		delete last;
	}
}

void PlayerStats::LoadStats()
{
	StatList *cur,*newrec,*last;
	StatRec rec;
	FILE *fp;

	// dispose of previous Stats
	cur=Root_;
	while(cur)
	{
		last=cur;
		cur=cur->Next;
		delete last;
	}
	Root_=NULL;

	fp=fopen(SaveName_,"rb");
	if(fp)
	{
		while(fread(&rec,sizeof(StatRec),1,fp))
		{
			DecryptBuffer(0x25,(uchar*)&rec,sizeof(StatRec));
			if(!rec.CheckSum) // No tampering... keep it
			{
				if(rec.aa_kills || rec.ag_kills || rec.as_kills || rec.an_kills || rec.rating || rec.missions)
				{
					newrec=new StatList;
					newrec->Next=NULL;
					memcpy(&newrec->data,&rec,sizeof(StatRec));
					if(!Root_)
						Root_=newrec;
					else
					{
						cur=Root_;
						while(cur->Next)
							cur=cur->Next;
						cur->Next=newrec;
					}
				}
			}
		}
		fclose(fp);
	}
	if(!Find(-1,-1,-1))
	{
		cur=Root_;
		while(cur)
		{
			if(cur->data.missions)
				cur->data.rating = (char)min(100.0f,max(0,((float)cur->data.rating + 0.5f) * 25.0f));
			cur=cur->Next;
		}
		AddStat(-1,-1,-1,-1,-1,-1,-1,-1,-1);
	}
}

void PlayerStats::SaveStats()
{
	StatRec rec;
	StatList *cur;
	FILE *fp;

	if(!Root_)
		return;

	fp=fopen(SaveName_,"wb");
	if(fp)
	{
		cur=Root_;
		while(cur)
		{
			memcpy(&rec,&cur->data,sizeof(StatRec));
			EncryptBuffer(0x25,(uchar*)&rec,sizeof(StatRec));
			fwrite(&rec,sizeof(StatRec),1,fp);
			cur=cur->Next;
		}
		fclose(fp);
	}
}

void PlayerStats::AddStat(long IP,long Date,long Rev,short aa,short ag,short an,short as,short missions,char rating)
{
	StatList *newrec,*cur;

	if(!aa && ! ag && !an && !as && !missions && !rating)
		return;

	cur=Find(IP,Date,Rev);
	if(cur)
	{
		if( cur->data.aa_kills == aa && 
			cur->data.ag_kills == ag &&
			cur->data.an_kills == an &&
			cur->data.as_kills == as &&
			cur->data.missions == missions &&
			cur->data.rating == rating ) // No reason to save if nothing changed
				return;
		if(cur->data.IP == IP && cur->data.Date == Date && cur->data.Rev == Rev)
		{
			cur->data.aa_kills=aa;
			cur->data.ag_kills=ag;
			cur->data.an_kills=an;
			cur->data.as_kills=as;
			cur->data.missions=missions;
			cur->data.rating=rating;
			SaveStats();
			return;
		}
	}
	newrec=new StatList;
	newrec->Next=NULL;

	newrec->data.IP=IP;
	newrec->data.Date=Date;
	newrec->data.Rev=Rev;
	newrec->data.aa_kills=aa;
	newrec->data.ag_kills=ag;
	newrec->data.an_kills=an;
	newrec->data.as_kills=as;
	newrec->data.missions=missions;
	newrec->data.rating=rating;
	newrec->data.CheckSum=0;

	if(!Root_)
		Root_=newrec;
	else
	{
		cur=Root_;
		while(cur->Next)
			cur=cur->Next;
		cur->Next=newrec;
	}
	SaveStats();
}

StatList *PlayerStats::Find(long IP,long Date,long Rev)
{
	StatList *cur,*Found;

	Found=NULL;
	cur=Root_;
	while(cur)
	{
		if(cur->data.IP == IP && cur->data.Date == Date && cur->data.Rev <= Rev)
		{
			Found=cur;
		}
		cur=cur->Next;
	}
	return(Found);
}
