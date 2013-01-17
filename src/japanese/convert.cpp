// Convert.cpp
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <string.h>
#include <malloc.h>
#include "tacref.h"
//#include "\falcon4\ui\include\tacref.h"

// Prototypes
char  *SkipJunk    ( char *lnptr );
char  *GetALine    ( char *lnptr, char *seps );
long  Text2Long    ( char *txt );
short Text2Short   ( char *txt );
short CheckToken   ( char *line );
void  WriteLong    ( long lnum );
void  WriteShort   ( short snum );
void  WriteText    ( char *txt );
void  DoBegEntity  ();
void  DoBegStat    ();
void  DoBegCat     ();
void  DoBegText    ();
void  DoBegRwr     ();
void  DoEndEntity  ();
void  DoEndStat    ();
void  DoEndCat     ();
void  DoEndText    ();
void  DoEndRwr     ();
void  DoDescript   ();
void  InitVar      ();
void  InitRwrKeep  ();
void  InitDescKeep ();
void  InitCatKeep  ();
void  InitCatInTxtKeep();
void  DoParse      ();
void  CreateBin    ();
//void  Conv2Bin     ();
void  Conv2Bin     (char *ifname,char *ofname);

const MAXTOKEN      = 10;
const MAXFUNC       = 10;
const MAXCATKEEPTXT = 1000;
const MAXCATKEEP    = 150;
const MAXRWRKEEP    = 150;
const MAXDESCKEEP   = 150;
const MAXINTXTKEEP  = 150;

enum {
	_ENDTEXT_=90,
	_ENDCAT_,
	_ENDSTAT_,
	_ENDENTITY_,
	_ENDRWR_
};

// Globals
HDC   gblhdc;
HWND  gblhwnd;
RECT  gblrect;

int   ifh,ofh;
long  flen;
short toktype;
short where;	// current chunk
short txthd;	// 1 means text collection header created, else 0
short ttlentity,ttlstat,ttlcat,ttltxt,ttlrwr;
short ttlcatstrhead;
char  *trdata;	// points 2 current token
char  *startentity,*startstat,*startcat,*starttxt,*startrwr;
char  *startdata,*endingdata;	// start and end of database
char  *nextdata;
char  *tokens[ MAXTOKEN ] = {
	  "BEGIN_ENTITY",
	  "BEGIN_STAT",
	  "BEGIN_CAT",
	  "BEGIN_TEXT",
	  "END_TEXT",
	  "END_CAT",
	  "END_STAT",
	  "END_ENTITY",
	  "BEGIN_RWR",
	  "END_RWR"
};

short tokmap[MAXTOKEN] = {
	  _ENTITY_,
	  _STATS_,
	  _CATEGORY_,
	  _TEXT_,
	  0,
	  0,
	  0,
	  0,
	  _RWR_DATA_,
	  0
};
char *backn = "\n";

void (*tokfunc[MAXFUNC])() = {
	 DoBegEntity,
	 DoBegStat,
	 DoBegCat,
	 DoBegText,
	 DoEndText,
	 DoEndCat,
	 DoEndStat,
	 DoEndEntity,
	 DoBegRwr,
	 DoEndRwr
};
struct Header         *Enthead,*Stathead,*Cathead,*Stringhead;
struct Header         *Rwrhead,*Deschead;
struct Entity         *Entityptr;
struct Category       *Catptr;
struct TextString     *Txtptr,*DescTxtptr;
struct Text4CatString *CatInTxtptr;
struct RWR_Data       *Rwrptr;

char   *CatKeepptr;
char   *CatKeeptxt[MAXCATKEEPTXT];		// for keeping Cat text pointers
char   *CatHeadKeep[MAXCATKEEP];		// for keeping Cat header pointers
char   *CatPtrKeep[MAXCATKEEP];			// for keeping Cat chunk pointers
char   *CatStrKeep[MAXCATKEEP];			// for keeping Cat String header pointers
char   *CatInTxtKeep[MAXCATKEEP][MAXINTXTKEEP];		// 4 keeping Cat Inside Text Str struc ptrs in 1 string header
char   *CatInChildKeep[MAXCATKEEP][MAXINTXTKEEP];	// 4 keeping Cat string data pointers in 1 string header
char   *CatInChildTxt;

char   *RwrHeadKeep[MAXRWRKEEP];	// ptrs 2 RWR heads in 1 entity
char   *RwrPtrKeep [MAXRWRKEEP];	// ptrs 2 RWR data ptrs in 1 entity
char   *DescHeadKeep;				// ptr 2 Description header in 1 ent
char   *DescPtrKeep [MAXDESCKEEP];	// ptrs 2 Description data ptrs in 1 ent
char   *DescChildKeep[MAXDESCKEEP];	// ptrs 2 Desc child texts in 1 entity
char   *DescChildTxt;
long   curtxtidx;
long   currwridx;
long   curdesidx;
long   curcatidx;
long   curintxtidx;
long   catintxtidxkeep[MAXCATKEEP];

LRESULT CALLBACK WinProc (HWND, UINT,WPARAM,LPARAM);

// Skip junks in the beginning of the file which start with double slash
char *SkipJunk( char *lnptr )
{
	char *dblslh = "//";
	char *newlnptr,*token;

	token    = strtok( lnptr, backn );	// get 1st token
	newlnptr = token;
	/* While there are tokens in "string" */
	while ( ( token ) && (!strncmp( token,dblslh,2 )) ) {
		/* Get next token: */
		token = strtok( NULL, backn );
		if ( token ) newlnptr = token;
	}
	// put back linefeed since it's replaced
	//len = strlen( token );
	//token[len] = 0x0A;
	return( newlnptr );
}

// Write a long number
void WriteLong( long lnum )
{
	long numwr;

	numwr = _write( ofh, &lnum, sizeof(long) );
}

// Write a short number
void WriteShort( short snum )
{
	long numwr;

	numwr = _write( ofh, &snum, sizeof(short) );
}

// Write a text string
void WriteText( char *txt )
{
	long numwr,len;

	len   = strlen( txt );
	numwr = _write( ofh, txt, len );
}

// Text 2 Long
long Text2Long( char *txt )
{
	long lnum;

	lnum = atol( txt );
	return( lnum );
}

// Text 2 Short
short Text2Short( char *txt )
{
	short snum;

	snum = atoi( txt);
	return( snum );
}

// Get a line to check
// returns a line ptr, else NULL
char *GetALine( char *lnptr, char *seps )
{
	char *newlnptr;

	newlnptr = strtok( lnptr,seps );
	nextdata = newlnptr+strlen( newlnptr )+1;
	return( newlnptr );
}

// Check tokens
// returns token type if it is one, else -1
short CheckToken( char *line )
{
	short i,tokentype;
	long  len;

	tokentype   = -1;
	len         = strlen( line );
	line[len-1] = '\0';	// replace a 0xD
	for ( i = 0; i < MAXTOKEN; i++ ) {
		if ( !strcmp( tokens[i],line ) ) {	// got it
			tokentype = i;
			break;
		}
	}
	line[len-1] = 0xD;	// put it back

	return( tokentype );
}

// Process Begin Entity token
void DoBegEntity()
{
	long  lGroupID,lSubGroupID,lEntityID;
	short lModelID,len,i;
	char  *tptr;

	where = _ENTITY_;
	if ( !Enthead )   Enthead   = new Header;
	if ( !Entityptr ) Entityptr = new Entity;
	Enthead->type = tokmap[toktype];
	trdata   = nextdata;
	trdata   = GetALine( trdata,backn );	// GroupID
	lGroupID = Text2Long( trdata );
	Entityptr->GroupID = lGroupID;

	trdata      = nextdata;
	trdata      = GetALine( trdata,backn );	// SubGroupID
	lSubGroupID = Text2Long( trdata );
	Entityptr->SubGroupID = lSubGroupID;

	trdata    = nextdata;
	trdata    = GetALine( trdata,backn );	// EntityID
	lEntityID = Text2Long( trdata );
	Entityptr->EntityID = lEntityID;

	ttlentity += ( 3*sizeof(long) );

	trdata   = nextdata;
	trdata   = GetALine( trdata,backn );	// ModelID
	lModelID = Text2Short( trdata );
	Entityptr->ModelID = lModelID;

	ttlentity += ( sizeof(short) );

	trdata = nextdata;
	trdata = GetALine( trdata,backn );	// EntityName
	len    = strlen( trdata );
	tptr   = (char *)&Entityptr->Name[0];
	for ( i=0;i < len-1;i++ ) {
		*tptr = trdata[i];
		tptr++;
	}
	*tptr = '\0';
	ttlentity += 32;	// fix length
}

// Process End Entity token
void DoEndEntity()
{
	short i,j;

	where         = _ENDENTITY_;
	ttlentity     += (2*sizeof(short));
	Enthead->size = ttlentity;
	// Write it here
	CreateBin();
	if ( Enthead )   delete Enthead; Enthead = NULL;
	if ( Entityptr ) delete Entityptr; Entityptr = NULL;
	if ( Stathead)   delete Stathead; Stathead = NULL;
	for ( i = 0; i < MAXRWRKEEP; i++ ) {
		delete RwrHeadKeep[i];
		delete RwrPtrKeep[i];
	}
	for ( i = 0; i < MAXDESCKEEP; i++ ) {
		delete DescPtrKeep[i];
		delete DescChildKeep[i];
	}
	delete DescHeadKeep; DescHeadKeep = NULL;

	for ( i = 0; i < MAXCATKEEP; i++ ) {
		delete CatHeadKeep[i];
		delete CatPtrKeep[i];
		delete CatStrKeep[i];
	}
	
	for ( j = 0; j < MAXCATKEEP; j++ ) {
		for ( i = 0; i < MAXINTXTKEEP; i++ ) {
			delete CatInTxtKeep[j][i];
			delete CatInChildKeep[j][i];
		}
	}

	ttlentity = ttlstat = ttlcat = ttltxt = ttlrwr = 0;
	currwridx = 0;	// reset idx
	InitRwrKeep ();	// reset ptrs
	curdesidx = 0;
	InitDescKeep();
	curcatidx = 0;
	ttlcatstrhead = 0;
	curintxtidx   = 0;
	InitCatKeep ();
	InitCatInTxtKeep();
}

// Process Begin Stat token
void DoBegStat()
{
	where     = _STATS_;
	Stathead  = new Header;
	ttlentity += (2*sizeof(short));	// for the header
	Stathead->type = tokmap[toktype];
}

// Process End Stat token
void DoEndStat()
{
	where          = _ENDSTAT_;
	Stathead->size = ttlstat;
	ttlstat = 0;
}

// Process Begin Cat token
void DoBegCat()
{
	short len,i;
	char  *tptr;

	where   = _CATEGORY_;
	Cathead = new Header;
	CatHeadKeep[curcatidx] = (char *)Cathead;
	Catptr  = new Category;
	CatPtrKeep[curcatidx]  = (char *)Catptr;
	ttlentity += (2*sizeof(short));	// for the header
	ttlstat   += (2*sizeof(short));	// for the header
	Cathead->type = tokmap[toktype];

	trdata = nextdata;
	trdata = GetALine( trdata,backn );	// CatText
	len    = strlen( trdata );
	tptr   = (char *)&Catptr->Name[0];
	for ( i=0;i < len-1;i++ ) {
		*tptr = trdata[i];
		tptr++;
	}
	*tptr = '\0';
	ttlentity += 40;	// fix length
	ttlstat += 40;
	ttlcat += 40;
}

// Process End Cat token
void DoEndCat()
{
	long i;

	where  = _ENDCAT_;
	Cathead->size = ttlcat;
	ttlcat = 0;
	if ( Cathead ) delete Cathead; Cathead = NULL;
	if ( Catptr )  delete Catptr;  Catptr  = NULL;
	for (i = 0; i < curtxtidx; i++) {
		delete CatKeeptxt[i];
		CatKeeptxt[i] = NULL;
	}
}

// Process Begin Text token
void DoBegText()
{
	short len,lcount;
	short i;
	char  *tptr;
	char  *endmark = "END_TEXT";
	char  *newlnptr;
	char  *token;
	long  lXrefGroup,lXrefSubGroup,lEntityID;

	curtxtidx  = 0;	// currently Not Used
	if ( where == _CATEGORY_ ) {	//
		Stringhead = new Header;
		CatStrKeep[curcatidx] = (char *)Stringhead;
		Stringhead->type      = tokmap[toktype];
		ttlentity  += (2*sizeof(short));
		ttlstat    += (2*sizeof(short));
		ttlcat     += (2*sizeof(short));

	trdata = nextdata;
	token  = strtok( trdata, backn );	// get 1st token
	lcount = 0;
	/* While there are tokens in "string" */
	while ( token ) {
		if (strncmp( token,endmark,strlen(endmark) )) {
			/* Not Endmark, Get next token: */
			//if ( !strncmp(token,"Propulsion: 2 Gas",17)) { // just checking
			//	len = 0;
			//}
			lcount++;	// count how many text strings are inside this block of TEXT
			CatInTxtptr = new Text4CatString;
			CatInTxtKeep[curcatidx][curintxtidx] = (char *)CatInTxtptr;

			len  = strlen( token );
			if ( len == 1 ) len++;
			CatInTxtptr->length = len;
			CatInChildTxt       = new char[len];
			CatInChildKeep[curcatidx][curintxtidx] = CatInChildTxt;
			tptr = CatInChildTxt;
			for ( i=0;i < len-1;i++ ) {
				*tptr = token[i];
				tptr++;
			}
			*tptr         = '\0';
			ttlentity     += len;
			ttlstat       += len;
			ttlcat        += len;
			ttlcatstrhead += len;
			//
			token = strtok( NULL, backn );
			if ( token ) newlnptr = token;
			if ( (strncmp(token,"END_TEXT",8)) ) {
				// more text string in this block
				curintxtidx++;
			}
		}
		else {	// End mark found, look ahead
			// get the 3 xrefs
			trdata     = token+strlen(token)+1;
			trdata     = GetALine( trdata,backn );	// XrefGroup
			lXrefGroup = Text2Long( trdata );
			CatInTxtptr->GroupID = lXrefGroup;

			trdata        = nextdata;
			trdata        = GetALine( trdata,backn );	// XrefSubGroup
			lXrefSubGroup = Text2Long( trdata );
			CatInTxtptr->SubGroupID = lXrefSubGroup;

			trdata    = nextdata;
			trdata    = GetALine( trdata,backn );	// XrefVehicle or EntityID
			lEntityID = Text2Long( trdata );
			CatInTxtptr->EntityID = lEntityID;

			ttlentity     += (lcount* (sizeof(short)+(3*sizeof(long))) );	// 11/23/98
			ttlstat       += (lcount* (sizeof(short)+(3*sizeof(long))) );
			ttlcat        += (lcount* (sizeof(short)+(3*sizeof(long))) );
			ttlcatstrhead += (lcount* (sizeof(short)+(3*sizeof(long))) );
			lcount        = 0;	// reset

			tptr = nextdata;
			if ( !(strncmp(tptr,"END_CAT",7)) ) { // no more text block, the end of a Cat
				trdata = tptr;
				curintxtidx++;
				break;
			}
			else {
				token = tptr;
				token = strtok( token, backn );
				token = strtok( NULL, backn );	// points 2 text
				curintxtidx++;
			}
		}
	}
	// Look ahead if there is more Cat
	token = trdata;
	token = strtok( token, backn );
	token = strtok( NULL, backn );
	if ( !(strncmp(token,"BEGIN_CAT",9)) ) {
		trdata           = token;	// more Cat
		nextdata         = token;
		Cathead->size    = ttlcat;	// save previous Cat chunk size
		ttlcat           = 0;	// reset 4 next cat
		Stringhead->size = ttlcatstrhead;
		ttlcatstrhead    = 0;
		catintxtidxkeep[curcatidx] = curintxtidx;
		curintxtidx      = 0;	// NEED 2 save this first
		curcatidx++;
		return;
	}

	// Advance to Description text
	/* While there are tokens in "string" */
	catintxtidxkeep[curcatidx] = curintxtidx;	// 4 the last Cat
	Cathead->size    = ttlcat;	// save previous Cat chunk size
	ttlcat           = 0;
	Stringhead->size = ttlcatstrhead;
	ttlcatstrhead    = 0;
	curcatidx++;	// 4 loop sake
	while ( token ) {
		if (strncmp( token,"BEGIN_TEXT",10 )) {
			/* Not Description Text yet, Get next token */
			token = strtok( NULL, backn );
			if ( token ) newlnptr = token;
		}
		else { // Beginning of Description text found
			Stathead->size = ttlstat;
			trdata         = token;
			nextdata       = token;
			where          = _ENDSTAT_;
			// Do the description here
			DoDescript();
			break;
		}
	}


	// put back linefeed since it's replaced
	len = strlen( token );	// does nothing, just 4 debugging
	//token[len] = 0x0A;

	} // ENDIF Category
}

// Process description text
void DoDescript()
{
	short lZoomAdjust,lVertical,lHoriz,lMissile;
	short len,i;
	char  *newlnptr;
	char  *token,*tptr;
	short ttldesctxt;

	Deschead       = new Header;
	Deschead->type = _DESCRIPTION_;
	DescHeadKeep   = (char *)Deschead;
	ttlentity      += ( 2*sizeof(short) );
	ttldesctxt     = 0;

	token = trdata;
	token = strtok( NULL, backn );
	/* While there are tokens in "string" */
	while ( token ) {
		if (strncmp( token,"END_TEXT",8 )) {
			/* Not End Text yet, Copy token & Get next token */
			DescTxtptr = new TextString;
			DescPtrKeep[curdesidx] = (char *)DescTxtptr;
			ttlentity  += ( sizeof(short) );

			len = strlen( token );
			if ( len == 1 ) len++;
			DescTxtptr->length = len;
			DescChildTxt       = new char[len];
			DescChildKeep[curdesidx] = DescChildTxt;
			tptr = DescChildTxt;
			for ( i=0;i < len-1;i++ ) {
				*tptr = token[i];
				tptr++;
			}
			*tptr      = '\0';
			ttlentity  += len;
			ttldesctxt += len+sizeof(short);
			curdesidx++;

			token = strtok( NULL, backn );
			if ( token ) newlnptr = token;
		}
		else { // End of Text block found
			// Get some values here
			trdata      = token+strlen(token)+1;
			trdata      = GetALine( trdata,backn );	// ZoomAdjust
			lZoomAdjust = Text2Short( trdata );
			Entityptr->ZoomAdjust = lZoomAdjust;

			trdata    = nextdata;
			trdata    = GetALine( trdata,backn );	// Vert Offset
			lVertical = Text2Short( trdata );
			Entityptr->VerticalOffset = lVertical;

			trdata = nextdata;
			trdata = GetALine( trdata,backn );	// Horiz Offset
			lHoriz = Text2Short( trdata );
			Entityptr->HorizontalOffset = lHoriz;

			trdata   = nextdata;
			trdata   = GetALine( trdata,backn );	// MissileFlag
			lMissile = Text2Short( trdata );
			Entityptr->MissileFlag = lMissile;

			ttlentity += ( 4*sizeof(short) );
			trdata    = nextdata;
			trdata    = GetALine( trdata,backn );	// Photo fname
			len       = strlen( trdata );
			tptr      = (char *)&Entityptr->PhotoFile[0];
			for ( i=0;i < len-1;i++ ) {
				*tptr = trdata[i];
				tptr++;
			}
			*tptr          = '\0';
			ttlentity      += 32;	// fix length
			Deschead->size = ttldesctxt;
			break;
		}
	}
}

// Process End Text token
void DoEndText()
{
	where = _ENDTEXT_;
	Stringhead->size = ttltxt;
	if ( Stringhead ) delete Stringhead; Stringhead = NULL;
	if ( Txtptr )     delete Txtptr; Txtptr = NULL;
}

// Process Begin Rwr token
void DoBegRwr()
{
	short lSearchSymbol,lLockSymbol;
	short len,i;
	long  lSearchTone,lLockTone;
	char  *tptr;

	where   = _RWR_DATA_;
	Rwrhead = new Header;
	RwrHeadKeep[currwridx] = (char *)Rwrhead;
	Rwrptr  = new RWR_Data;
	RwrPtrKeep[currwridx]  = (char *)Rwrptr;

	Rwrhead->type = tokmap[toktype];
	trdata = nextdata;
	trdata = GetALine( trdata,backn );	// RWRNAME
	len    = strlen( trdata );
	tptr   = (char *)&Rwrptr->Name[0];
	for ( i=0;i < len-1;i++ ) {
		*tptr = trdata[i];
		tptr++;
	}
	*tptr     = '\0';
	ttlentity += 32;	// fix length
	ttlrwr    += 32;

	trdata = nextdata;
	trdata = GetALine( trdata,backn );	// SearchSymbol
	lSearchSymbol       = Text2Short( trdata );
	Rwrptr->SearchState = lSearchSymbol;
	ttlentity += (sizeof(short));
	ttlrwr    += (sizeof(short));

	trdata = nextdata;
	trdata = GetALine( trdata,backn );	// SearchTone
	lSearchTone        = Text2Long( trdata );
	Rwrptr->SearchTone = lSearchTone;
	ttlentity += (sizeof(long));
	ttlrwr    += (sizeof(long));

	trdata = nextdata;
	trdata = GetALine( trdata,backn );	// LockSymbol
	lLockSymbol       = Text2Short( trdata );
	Rwrptr->LockState = lLockSymbol;
	ttlentity += (sizeof(short));
	ttlrwr    += (sizeof(short));

	trdata = nextdata;
	trdata = GetALine( trdata,backn );	// LockTone
	lLockTone        = Text2Long( trdata );
	Rwrptr->LockTone = lLockTone;
	ttlentity += (sizeof(long));
	ttlrwr    += (sizeof(long));
}

// Process End Rwr token
void DoEndRwr()
{
	where  = _ENDRWR_;
	Rwrhead->size = ttlrwr;
	currwridx++;	// ready 4 next memory
	ttlrwr = 0;	// current RWR total size
}

// Parse the tacref database file
void DoParse()
{
	char Prsdone;

	Prsdone = 0;
	while ( !Prsdone ) {
		trdata  = GetALine( trdata,backn );
		toktype = CheckToken( trdata );
		if ( toktype != -1 ) {
			(tokfunc[toktype])();
		}
		trdata = nextdata;
		if ( trdata >= endingdata ) {
			Prsdone = 1;
		}
	}
}

void InitVar()
{
	long i;

	ttlentity  = ttlstat = ttlcat = ttltxt = ttlrwr = 0;
	Enthead    = NULL;
	Entityptr  = NULL;
	Stathead   = NULL;
	Cathead    = NULL;
	Catptr     = NULL;
	Stringhead = NULL;
	Txtptr     = NULL;
	Rwrhead    = NULL;
	Rwrptr     = NULL;

	currwridx     = 0;
	InitRwrKeep ();
	curdesidx     = 0;
	DescHeadKeep  = NULL;
	InitDescKeep();
	curcatidx     = 0;
	ttlcatstrhead = 0;
	curintxtidx   = 0;
	InitCatKeep ();
	InitCatInTxtKeep();

	for (i = 0; i< MAXCATKEEPTXT; i++) {
		CatKeeptxt[i] = NULL;
	}
}

void InitRwrKeep()
{
	short i;

	for ( i = 0; i < MAXRWRKEEP; i++ ) {
		RwrHeadKeep[i] = NULL;
		RwrPtrKeep[i]  = NULL;
	}
}

void InitDescKeep()
{
	short i;

	for ( i = 0; i < MAXDESCKEEP; i++ ) {
		DescPtrKeep[i]   = NULL;
		DescChildKeep[i] = NULL;
	}
}

void InitCatKeep()
{
	short i;

	for ( i = 0; i < MAXCATKEEP; i++ ) {
		CatHeadKeep[i]     = NULL;
		CatPtrKeep[i]      = NULL;
		CatStrKeep[i]      = NULL;
		catintxtidxkeep[i] = 0;
	}
}

void InitCatInTxtKeep()
{
	short i,j;

	for ( j = 0; j < MAXCATKEEP; j++ ) {
		for ( i = 0; i < MAXINTXTKEEP; i++ ) {
			CatInTxtKeep[j][i]   = NULL;
			CatInChildKeep[j][i] = NULL;
		}
	}
}

// Write out An Entity chunk
void CreateBin()
{
	long  numwr;
	long  lnum;
	short snum,i,j,qty,k;
	char  *tcptr;
	char  tbuff[80];

// Entity Record
	numwr = 0;
	snum  = Enthead->type;
	numwr += _write( ofh, &snum, sizeof(short) );
	snum  = Enthead->size;
	numwr += _write( ofh, &snum, sizeof(short) );
	lnum  = Entityptr->GroupID;
	numwr += _write( ofh, &lnum, sizeof(long) );
	lnum  = Entityptr->SubGroupID;
	numwr += _write( ofh, &lnum, sizeof(long) );
	lnum  = Entityptr->EntityID;
	numwr += _write( ofh, &lnum, sizeof(long) );
	snum  = Entityptr->ModelID;
	numwr += _write( ofh, &snum, sizeof(short) );

	snum  = Entityptr->ZoomAdjust;
	numwr += _write( ofh, &snum, sizeof(short) );
	snum  = Entityptr->VerticalOffset;
	numwr += _write( ofh, &snum, sizeof(short) );
	snum  = Entityptr->HorizontalOffset;
	numwr += _write( ofh, &snum, sizeof(short) );
	snum  = Entityptr->MissileFlag;
	numwr += _write( ofh, &snum, sizeof(short) );

	for ( k = 0; k < 80; k++ ) tbuff[k] = '\0';
	tcptr = Entityptr->Name;
	strcpy( tbuff, tcptr );
	tcptr = tbuff;
	numwr += _write( ofh, tcptr,32 );	// fix length
	tcptr = Entityptr->PhotoFile;
	for ( k = 0; k < 80; k++ ) tbuff[k] = '\0';
	strcpy( tbuff, tcptr );
	tcptr = tbuff;
	numwr += _write( ofh, tcptr, 32 );	// fix length

// Statistics Record (only a header)
	snum  = ((struct Header *)Stathead)->type;
	numwr += _write( ofh, &snum, sizeof(short) );
	snum  = ((struct Header *)Stathead)->size;
	numwr += _write( ofh, &snum, sizeof(short) );
//
	// Category Record
	for ( i = 0; i < curcatidx; i++ ) {
		snum  = ((struct Header *)CatHeadKeep[i])->type;
		numwr += _write( ofh, &snum, sizeof(short) );
		snum  = ((struct Header *)CatHeadKeep[i])->size;
		numwr += _write( ofh, &snum, sizeof(short) );
		numwr = 0;
		tcptr = ((struct Category *)CatPtrKeep[i])->Name;
		for ( k = 0; k < 80; k++ ) tbuff[k] = '\0';
		strcpy( tbuff, tcptr );
		tcptr = tbuff;
		numwr += _write( ofh, tcptr, 40 );	// fix length

		// Category text
		snum  = ((struct Header *)CatStrKeep[i])->type;
		numwr += _write( ofh, &snum, sizeof(short) );
		snum  = ((struct Header *)CatStrKeep[i])->size;
		numwr += _write( ofh, &snum, sizeof(short) );
		qty   = (short) catintxtidxkeep[i];
		for ( j = 0; j < qty ; j++ ) {
			lnum  = ((struct Text4CatString *)CatInTxtKeep[i][j])->GroupID;
			numwr += _write( ofh, &lnum, sizeof(long) );
			lnum  = ((struct Text4CatString *)CatInTxtKeep[i][j])->SubGroupID;
			numwr += _write( ofh, &lnum, sizeof(long) );
			lnum  = ((struct Text4CatString *)CatInTxtKeep[i][j])->EntityID;
			numwr += _write( ofh, &lnum, sizeof(long) );


			snum  = ((struct Text4CatString *)CatInTxtKeep[i][j])->length;
			numwr += _write( ofh, &snum, sizeof(short) );
			tcptr = CatInChildKeep[i][j];
			if ( *tcptr == 0x0d ) {
				*tcptr = 0x0;
				numwr  += _write( ofh, tcptr, 2 );
			}
			else {
				numwr  += _write( ofh, tcptr, strlen(tcptr)+1 );
			}

		}
		if ( numwr != ((struct Header *)CatHeadKeep[i])->size)
		{	// just checking
			numwr += 10;
			numwr -= 10;
		}
	}

// Description Record (header... no data)
	snum  = ((struct Header *)DescHeadKeep)->type;
	numwr += _write( ofh, &snum, sizeof(short) );
	snum  = ((struct Header *)DescHeadKeep)->size;
	numwr += _write( ofh, &snum, sizeof(short) );

	// Description text
	for ( i = 0; i < curdesidx; i++ ) {
		// Description text
		snum  = ((struct TextString *)DescPtrKeep[i])->length;
		numwr += _write( ofh, &snum, sizeof(short) );
		tcptr = DescChildKeep[i];
		if ( *tcptr == 0x0d ) {
			*tcptr  = 0x0;
			numwr   += _write( ofh, tcptr, 2 );
		}
		else {
			numwr   += _write( ofh, tcptr, strlen(tcptr)+1 );
		}
	}
//
		snum  =_RWR_DATA_;
		numwr += _write( ofh, &snum, sizeof(short) );
		snum  = currwridx * sizeof(struct RWR_Data);	// can be 0 if no RWR data
		numwr += _write( ofh, &snum, sizeof(short) );
	// RWR Record
	for ( i = 0; i < currwridx; i++ ) {
		snum  = ((struct RWR_Data *)RwrPtrKeep[i])->SearchState;
		numwr += _write( ofh, &snum, sizeof(short) );
		snum  = ((struct RWR_Data *)RwrPtrKeep[i])->LockState;
		numwr += _write( ofh, &snum, sizeof(short) );
		lnum  = ((struct RWR_Data *)RwrPtrKeep[i])->SearchTone;
		numwr += _write( ofh, &lnum, sizeof(long) );
		lnum  = ((struct RWR_Data *)RwrPtrKeep[i])->LockTone;
		numwr += _write( ofh, &lnum, sizeof(long) );

		tcptr = ((struct RWR_Data *)RwrPtrKeep[i])->Name;
		for ( k = 0; k < 80; k++ ) tbuff[k] = '\0';
		strcpy( tbuff, tcptr );
		tcptr = tbuff;
		numwr += _write( ofh, tcptr, 32 );	// fix length
	}

	numwr = numwr;	// just checking
}

//void Conv2Bin()
void Conv2Bin(char *ifname,char *ofname)
{
//	char *ifname = "D:\\falcon4\\TOOLS\\tacref\\tacrefdb.txt";
//	char *ofname = "D:\\falcon4\\TOOLS\\tacref\\tacrefdb.bin";
	long bytesread;

	ifh = _open( ifname, _O_BINARY|_O_RDONLY );
	if( ifh == -1 ) {
		printf("Error: can't open input file (%s)\n",ifname);
		return;	// Error
	}
	//DrawText( gblhdc, "File Opened for Reading!", -1, &gblrect, DT_SINGLELINE);

	ofh = _open( ofname, _O_TRUNC|_O_CREAT|_O_BINARY|_O_WRONLY, _S_IREAD|_S_IWRITE );
	if( ofh == -1 ) {
		if ( ifh ) _close ( ifh );
		printf("Can't create output file (%s)\n",ofname);
		return;
	}

	// Get file length
	flen = _filelength( ifh );
	/* Allocate space for dumping database file  */
	trdata = NULL;
	trdata = (char *) malloc( flen );
	if( !trdata ) {
		// Error, printf( "Insufficient memory !\n" );
		if ( ifh ) _close ( ifh );
		if ( ofh ) _close ( ofh );
		return;
	}

	// reads in the database file
	if (( bytesread = (long) _read( ifh, trdata, flen )) <= 0 ) {
		// Error, perror( "Problem reading file" );
		if ( ifh )    _close ( ifh );
		if ( ofh )    _close ( ofh );
		if ( trdata ) { free( trdata ); trdata = NULL; }
		return;
	}

	startdata   = trdata;
	endingdata  = startdata+flen;

	trdata      = SkipJunk( trdata );
	startentity = trdata;
	InitVar();
	// Conversion
	DoParse();

	_close( ifh );
    _close( ofh );
	DrawText( gblhdc, "Both files are Closed !            ", -1, &gblrect, DT_SINGLELINE);

    free( startdata );
}

int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hPrev, PSTR szCmdLine, int iCmdShow)
{
/*
	static     char szAppName[] = "convert";
	HWND       hwnd;
	MSG        msg;
	WNDCLASSEX wndclass;

	wndclass.cbSize        = sizeof(wndclass);
	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = WinProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInst;
	wndclass.hIcon         = LoadIcon (NULL,IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL,IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szAppName;
	wndclass.hIconSm       = LoadIcon(NULL,IDI_APPLICATION);

	RegisterClassEx(&wndclass);

	hwnd = CreateWindow(szAppName,
					   "Convert",
					   WS_OVERLAPPEDWINDOW,
					   CW_USEDEFAULT,
					   CW_USEDEFAULT,
					   CW_USEDEFAULT,
					   CW_USEDEFAULT,
					   NULL,
					   NULL,
					   hInst,
					   NULL);

	ShowWindow(hwnd,iCmdShow);
	UpdateWindow(hwnd);

// Reads in tac ref database file
*/
	char infile[MAX_PATH],outfile[MAX_PATH];
	char *token;
	long Error=0;
	
	token=strtok(szCmdLine," ");
	if(token)
	{
		strcpy(infile,token);
		token=strtok(NULL," ");
		if(token)
			strcpy(outfile,token);
		else
			Error=1;
	}
	else
		Error=1;

	printf("TACREF Database converter to binary tool - by Billy Sutyono\n");
	if(Error)
	{
		printf("Usage:  txt2bin   <input>  <output>\n\n");
		exit(0);
	}
//	Conv2Bin();
	Conv2Bin(infile,outfile);
/*
	while(GetMessage(&msg,NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return(msg.lParam);
*/
	return(0);
}
/*
LRESULT CALLBACK WinProc (HWND hwnd, UINT message,WPARAM wParam,LPARAM lParam)
{
	HDC         hdc;
	PAINTSTRUCT ps;
	RECT        rect;
	LRESULT     retval=1;

	switch(message)
	{
		case WM_CREATE:
			 return(0);
		case WM_SYSKEYDOWN:
		case WM_CHAR:
			 return(0);

		case WM_PAINT:
			 hdc=BeginPaint(hwnd,&ps);
			 GetClientRect(hwnd,&rect);

			 gblhdc  = hdc;
			 gblhwnd = hwnd;
			 gblrect = rect;

			 DrawText(hdc, "Begin reading file", -1, &rect, DT_SINGLELINE);
  
			 EndPaint(hwnd,&ps);
			 return(0);
		case WM_LBUTTONUP:

			 return(0);

		case WM_DESTROY:
			 PostQuitMessage(0);
			 return(0);
	}
	return(DefWindowProc(hwnd,message,wParam,lParam));
}
*/