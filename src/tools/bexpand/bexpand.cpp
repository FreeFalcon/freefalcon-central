
#include <stdio.h>
#include <string.h>
#include "Brief.h"
#include "CampStr.h"

char FilesToRead[30][30] =	{	"Header.b",
								//"Header2.b",
								"Situate.b",
								"Element.b",
								"Threats.b",
								"THREAT.B",
								"STEERPTH.B",	
								"STEERPT.B",		
								"Loadouth.b",
								"LOADOUT.B",
								"Support.b",		
								"RoE.b",		
								"Emerganc.b",
								"END.B",	
								"Header.db",
							    //"Header2.db",
								"Element.db",
								"FlEvent.db",
								"Flight.db",
								"FOrdnce.db",
								"FOrdEvt.db",	
								"FOrdEnd.db",
								"pilot.db",
								"PElement.db",
								"results.db",
								0 
							};

/*
char FilesToRead[30][30] =	{	"SITUATE.b",				// 4
								0,
								"Loadout.b",			// 5
								"LOADOUTH.B",			// 6
								"NoSquad.b",			// 7
								"OBJECTIV.B",			// 8
								"RoE.b",				// 9
								"SITUATE.B",			// 10
								"SQUAD.B",			
								"STEERPT.B",	
								"STEERPTH.B",		
								"Support.b",		
								"THREAT.B",				// 15
								"Threats.b",
								0 
							};
*/
#define MAX_TOKENS 128

char gTokenList[MAX_TOKENS][30] = { 0 };
char gTokenListValue[MAX_TOKENS][180] = { 0 };
int gTokenListOptions[MAX_TOKENS] = { 0 };
int gTokenListCurOption[MAX_TOKENS] = { 0 };
int	gNumTokens;
char gBaseDir[MAX_PATH];
char gLangDir[MAX_PATH];

extern _TCHAR AirSTypesStr[20][20];
extern _TCHAR GroundSTypesStr[20][20];

_TCHAR	UnitNameLong[128] = "3rd Armored Battalion, 3rd Armored Brigade, 2nd Armored Division";
_TCHAR	DivisionName[128] = "3rd Armored Division";
_TCHAR	UnitNameShort[128] = "1st Armored Battalion";
_TCHAR	SquadronName[128] = "1st Fighter Squadron";
_TCHAR	BridgeName[128] = "Seoul Bridge";
_TCHAR	AirbaseName[128] = "Osan Airbase";
_TCHAR	PortName[128] = "Port of Koksan";
_TCHAR	DepotName[128] = "Cheonyon Depot";
_TCHAR	FeatureName[4][80] = { "Building", "Warehouse", "Fuel Tank", "Runway" };

// ==================================
// Stubs
// ==================================

char DOMAIN_AIR		= 1;
char DOMAIN_LAND	= 2;
char DOMAIN_SEA		= 3;

short NumObjectiveTypes = 30;

// ==================================
// Support functions
// ==================================

FILE* OpenCampFile(char *filename, char *ext, char *mode)
	{
	char	name[MAX_PATH];

	sprintf(name,"%s\\%s.%s",gBaseDir,filename,ext);
	return fopen(name,mode);
	}

void ClearTokenList (void)
	{
	gNumTokens = 0;
	for (int i=0; i<MAX_TOKENS; i++)
		{
		gTokenListValue[i][0] = 0;
		gTokenListCurOption[i] = 0;
		gTokenListOptions[i] = 0;
		}
	}

int FindTokenIndex (char *token)
	{
	for (int i=0; i<gNumTokens; i++)
		{
		if (strcmp (gTokenList[i],token) == 0)
			return i;
		}
	return -1;
	}

void AddToValueList (int index, char *value)
	{
	int		add;
	char	*nval,*cval;

	while (value[0] == ' ')
		value++;

	while (value && value[0] >= '0' && value[0] <= '9')
		{
		nval = strchr(value,' ');
		if (nval)
			{
			nval[0] = 0;
			nval++;
			}

		add = 1;
		cval = gTokenListValue[index];
		while (cval && cval[0] >= '0' && cval[0] <= '9' && add)
			{
			if (atoi(cval) == atoi(value))
				add = 0;
			cval = strchr(cval,' ');
			if (cval)
				cval++;
			}

		if (add)
			{
			strcat(gTokenListValue[index],value);
			strcat(gTokenListValue[index]," ");
			gTokenListOptions[index]++;
			}
		value = nval;
		}
	}

void AddToTokenList (char *token, char *value)
	{
	int i = FindTokenIndex(token);
	if (i >= 0)
		AddToValueList(i,value);
	else
		{
		strcpy(gTokenList[gNumTokens],token);
		AddToValueList(gNumTokens,value);
		gNumTokens++;	
		}
	}

int FindCurrentValue (char *token)
	{
	int		v,i = FindTokenIndex(token);
	char	*vptr;

	if (i >= 0)
		{
		v = gTokenListCurOption[i];
		vptr = gTokenListValue[i];
		while (v)
			{
			vptr = strchr(vptr,' ');
			if (vptr)
				vptr++;
			v--;
			}
		return atoi(vptr);
		}
	return 0;
	}

int DoIfToken (char *token)
	{
	char	*vptr = strchr(token,' ');
	int		value;

	if (vptr)
		{
		*vptr = 0;
		vptr++;
		while (vptr[0] == ' ')
			vptr++;
		}

	value = FindCurrentValue(token);
	if (!vptr)
		return value;
	else
		{
		while (vptr && vptr[0] >= '0' && vptr[0] <= '9')
			{
			if (atoi(vptr) == value)
				return 1;
			if (vptr = strchr(vptr,' '))
				vptr++;
			}
		}
	return 0;
	}

int IncrementPassOptions (void)
	{
	int tnum = 0;

	while (tnum < gNumTokens)
		{
		gTokenListCurOption[tnum]++;
		if (gTokenListCurOption[tnum] >= gTokenListOptions[tnum])
			{
			gTokenListCurOption[tnum] = 0;
			tnum++;
			}
		else
			return 1;
		}
	return 0;
	}

void AddStringToBrief (_TCHAR *hdr, _TCHAR *str, _TCHAR *brief)
	{
	if (!str || !str[0] || !hdr || !hdr[0])
		return;
	_tcscat(brief,"(");
	_tcscat(brief,hdr);
	_tcscat(brief,")");
	_tcscat(brief,str);
	}
	
void AddIndexedStringToBrief (int sid, _TCHAR *brief)
	{
	_TCHAR		wstring[MAX_STRLEN_PER_TOKEN];
	_TCHAR		idstr[MAX_STRLEN_PER_TOKEN];

	sprintf(idstr,"%d",sid);
	ReadIndexedString(sid, wstring, MAX_STRLEN_PER_TOKEN);
	AddStringToBrief(idstr,wstring,brief);
	}

void AddNumberToBrief (int sid, _TCHAR *brief)
	{
	int			dig,div=10000;
	int			start=0;

	while (div > 0)
		{
		dig = sid/div;
		if (start || dig > 0)
			{
			AddIndexedStringToBrief(0+dig,brief);
			start++;
			sid -= dig*div;
			}
		div /= 10;
		}
	if (!start)
		AddIndexedStringToBrief(0,brief);
	}

void AddTimeToBrief (CampaignTime time, _TCHAR *brief)
	{
	_TCHAR		tstring[MAX_STRLEN_PER_TOKEN];

	GetTimeString(time, tstring);
	AddStringToBrief("TIME",tstring,brief);
	}

void AddLocationToBrief (char type, _TCHAR *brief)
	{
	_TCHAR		wdstr[41],wtmp[41],name[41],format[80],dist[10];
	_TCHAR		hdr[30];

	switch(type)
		{
		case 'N':
		case 'n':
			strcpy(hdr,"NEAREST_LOCATION");
			break;
		case 'T':
		case 't':
			strcpy(hdr,"THE_LOCATION");
			break;
		case 'G':
		case 'g':
		default:
			strcpy(hdr,"GENERAL_LOCATION");
			break;
		case 'S':
		case 's':
			strcpy(hdr,"SPECIFIC_LOCATION");
			break;
		}

	strcpy(name,"Seoul");
	int h = rand()%9;
	if (h < 8 || type == 'T' || type == 't')
		{
		ReadIndexedString(30+h,wdstr,40);
		switch (type)
			{
			case 'N':
			case 'n':
				// Say 'direction of name'
				ReadIndexedString(53, format, MAX_STRLEN_PER_TOKEN);
				ConstructOrderedSentence(wtmp, format, wdstr, name);
				break;
			case 'T':
			case 't':
				// Say 'name'
				_stprintf(wtmp,name);
				break;
			case 'g':
			case 's':
				// Say 'x nm direction of name'
				_stprintf(dist,"5");
				ReadIndexedString(52, format, MAX_STRLEN_PER_TOKEN);
				ConstructOrderedSentence(wtmp, format, dist, wdstr, name);
				break;
			default:
				// Say 'x km direction of name'
				_stprintf(dist,"5");
				ReadIndexedString(51, format, MAX_STRLEN_PER_TOKEN);
				ConstructOrderedSentence(wtmp, format, dist, wdstr, name);
				break;
			}
		}
	else if (rand()%2)
		{
		if  (type > 'a' && type < 'z')
			{
			// Say 'over x'
			ReadIndexedString(56, format, MAX_STRLEN_PER_TOKEN);
			}
		else
			{
			// Say 'within x'
			ReadIndexedString(55, format, MAX_STRLEN_PER_TOKEN);
			}
		ConstructOrderedSentence(wtmp, format, name);
		}
	else
		{
		// Just say 'near x'
		ReadIndexedString(54, format, MAX_STRLEN_PER_TOKEN);
		ConstructOrderedSentence(wtmp, format, name);
		}
	AddStringToBrief(hdr,wtmp,brief);
	}

void ReadComments (FILE* fh)
	{
	int					c;

	c = fgetc(fh);
	while (c == '\n')
		c = fgetc(fh);
	while (c == '/' && !feof(fh))
		{
		c = fgetc(fh);
		while (c != '\n' && !feof(fh))
			c = fgetc(fh);
		while (c == '\n')
			c = fgetc(fh);
		}
	ungetc(c,fh);
	}

char* ReadToken (FILE *fp, char name[], int len)
	{
	char buffer[256];
	char *sptr;

	fgets(buffer,256,fp);
	strncpy(name,buffer,len);
	if (name[len-1])
		name[len-1] = 0;
	sptr = strchr(name,'\n');
	if (sptr)
		*sptr = '\0';
	return name;
	}

void ConstructOrderedSentence (_TCHAR *string, _TCHAR *format, ... )
	{
	int			done=0,count=0,index=0;
	va_list		params;
	_TCHAR		argstring[MAX_STRLEN_PER_TOKEN],addchar[2];

	string[0] = 0;
	while (format[index])
		{
		if (format[index] == '#')
			{
			// read and add the numbered argument
			index++;
			count = format[index] - '0';	// arg #
			va_start( params, format );     // Initialize variable arguments.
			while (count >= 0)
				{
				sprintf(argstring,va_arg( params, _TCHAR*));
				count--;
				}
			va_end( params );				// Reset variable arguments.
			_tcscat(string,argstring);
			}
		else
			{
			// Add the character
			addchar[0] = format[index];
			addchar[1] = 0;
			_tcscat(string,addchar);
			}
		index++;
		}
	}

// ==================================
// Name functions
// ==================================

short	*NameIndex=NULL;
short	NameEntries=0;
char	NameFile[MAX_PATH];

void LoadNames (char* filename)
	{
	FILE	*fp;

	if ((fp = OpenCampFile(filename,"idx","rb")) == NULL)
		return;
	sprintf(NameFile,filename);
	fread(&NameEntries,sizeof(short),1,fp);
	NameIndex = new short[NameEntries];
	fread(NameIndex,sizeof(short),NameEntries,fp);
	fclose(fp);
	}

void FreeNames (void)
	{
	if (NameIndex)
		{
		delete [] NameIndex;
		NameIndex = NULL;
		}
	}

_TCHAR* ReadNameString (int sid, _TCHAR *wstr, unsigned int len)
	{
	FILE	*fp;
	short	size,rlen;

	size = NameIndex[sid+1]-NameIndex[sid];
	rlen = size / sizeof(_TCHAR);
	if (rlen >= len)
		rlen = len - 1;

	if ((fp = OpenCampFile(NameFile,"wch","rb")) == NULL)
		return NULL;
	fseek(fp,NameIndex[sid],0);
	fread(wstr,sizeof(_TCHAR),rlen,fp);
	wstr[rlen] = 0;
	fclose(fp);
	return wstr;
	}

void BuildExampleNames (void)
	{
	_TCHAR	temp1[10],temp2[20],temp3[10];
	_TCHAR	bat[40],brig[40],div[40],squad[40];

	ReadIndexedString(615, bat, 39);
	ReadIndexedString(614, brig, 39);
	ReadIndexedString(613, div, 39);
	ReadIndexedString(610, squad, 39);
	GetNumberName((rand()%4),temp1);
	GetNumberName((rand()%4),temp2);
	GetNumberName((rand()%4),temp3);

	_sntprintf(UnitNameLong,128,"%s %s %s, %s %s %s, %s %s %s",temp1,GroundSTypesStr[1],bat,temp2,GroundSTypesStr[1],brig,temp3,GroundSTypesStr[1],div);
	_sntprintf(DivisionName,128,"%s %s %s",temp3,GroundSTypesStr[1],div);
	_sntprintf(UnitNameShort,128,"%s %s %s",temp1,GroundSTypesStr[1],bat);
	_sntprintf(SquadronName,128,"%s %s %s",temp1,AirSTypesStr[1],squad);
	ReadNameString(500,BridgeName,128);
	ReadNameString(480,AirbaseName,128);
	ReadNameString(650,PortName,128);
	ReadNameString(475,DepotName,128);
//	FeatureName[80][4] = { "Building", "Warehouse", "Fuel Tank", "Runway" };
	}

// ==================================
// Primary functions
// ==================================

void AnalyseFile (char *filename)
	{
	int		done = 0,curr_stack=0;
	FILE	*fp;
	char	default_value_string[5] = "0 1";
	char	*sptr,*vptr,token[256],tmp[5];
	char	if_token[MAX_STACK][256];

	fp = OpenCampFile(filename,"","r");
	while (!done)
		{
		ReadComments(fp);
		ReadToken(fp,token,120);
		if (!token[0])
			continue;

		if (strncmp(token,"#IF",3)==0)
			{
			sptr = token+4;
			vptr = strchr(sptr,' ');
			if (!vptr)
				{
				strcpy(tmp,default_value_string);
				vptr = tmp;
				}
			else
				{
				*vptr = 0;
				vptr++;
				}
			curr_stack++;
			// Don't repeat package missions with flight missions
			if (strcmp(token,"#IF_PACKAGE_MISSION_EQ")==0)
				{
				sptr = strchr(token,'E');
				sptr += 2;
//				AddToTokenList("MISSION_EQ",vptr);
				}
			// Ignore player specific stuff - it's just color changes
			if (strncmp(token,"#IF_PLAYER",10))
				{
				AddToTokenList(sptr,vptr);
				strcpy(if_token[curr_stack],sptr);
				}
			}
		else if (strcmp(token,"#ELSE")==0)
			{
			int i = FindTokenIndex(if_token[curr_stack]);
			if (i >= 0)
				AddToValueList(i,"0");
			}
		else if (strcmp(token,"#ENDIF")==0)
			{
			if (curr_stack>0)
				curr_stack--;
			}
		else if (strcmp(token,"#ENDSCRIPT")==0)
			{
			done = 1;
			continue;
			}

		// Check for hidden evaluators..
		if (strncmp(token,"MISSION_DESCRIPTION",19)==0)
			AddToTokenList("MISSION_EQ","1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33");
//		else if (strncmp(token,"PACKAGE_MISSION_DESCRIPTION",27)==0)
//			AddToTokenList("PACKAGE_MISSION_EQ","1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33");
		else if (strncmp(token,"PACKAGE_MISSION_DESCRIPTION",27)==0)
			AddToTokenList("MISSION_EQ","1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33");
		else if (strncmp(token,"CONTEXT_STR",11)==0)
			AddToTokenList("CONTEXT_EQ","1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45");
//		else if (strcmp(token,"PACKAGE_SUCCESS")==0 || strcmp(token,"FLIGHT_SUCCESS")==0 || strcmp(token,"LONG_MISSION_SUCCESS")==0)
//			AddToTokenList("MISSION_SUCCESS","1 2 3 4 5");
		}
	fclose(fp);
	}

void DoToken (char *token, char *brief_string)
	{
	char		*sptr;
	int			i,value;

	// special tokens
	if (strcmp(token,"#EOL")==0)
		{
		_tcscat(brief_string,"\n");
		return;
		}
	else if (strcmp(token,"#SPACE")==0)
		{
		_tcscat(brief_string," ");
		return;
		}
	else if (strncmp(token,"#TAB",4)==0)
		{
		sptr = token+4;
		i = atoi(sptr);
		_tcscat(brief_string," ");
		return;
		}
/*	if (strncmp(token,"#INC",4)==0)
		{
		sptr = token+5;
		if (strcmp(sptr,"PILOT")==0)
			{
			if (mec->curr_pilot)
				mec->curr_pilot = mec->FindPilotDataFromAC(fn, mec->curr_pilot->aircraft_slot+1);
			else
				mec->curr_pilot = mec->FindPilotDataFromAC(fn, 0);
			}
		if (strcmp(sptr,"WEAPON")==0)
			mec->curr_weapon++;
		if (strcmp(sptr,"DATA")==0)
			mec->curr_data++;
		}
*/

	// Text string ids
	if (atoi(token) > 0)
		AddIndexedStringToBrief(atoi(token),brief_string);

	// Add all our script tokens here
	else if (strcmp(token,"FLIGHT_NUM")==0)
		AddNumberToBrief(1234,brief_string);	
	else if (strcmp(token,"FLIGHT_NAME")==0)
		AddIndexedStringToBrief(2000+rand()%150,brief_string);
	else if (strcmp(token,"PLANE_NAME")==0)
		{
		AddIndexedStringToBrief(2000+rand()%150,brief_string);
		AddNumberToBrief(rand()%4,brief_string);
		}
	else if (strcmp(token,"MISSION_NAME")==0)
		{
		value = FindCurrentValue("MISSION_EQ");
		AddIndexedStringToBrief(300+value,brief_string);
		}
	else if (strncmp(token,"MISSION_DESCRIPTION",19)==0)
		{
		value = FindCurrentValue("MISSION_EQ");
		sptr = token + 19;
		i = sptr[0] - '1';
		AddIndexedStringToBrief(400+i*50+value,brief_string);
		}
	else if (strncmp(token,"PACKAGE_MISSION_DESCRIPTION",27)==0)
		{
//		value = FindCurrentValue("PACKAGE_MISSION_EQ");
		value = FindCurrentValue("MISSION_EQ");
		sptr = token + 27;
		i = sptr[0] - '1';
		AddIndexedStringToBrief(900+i*50+value,brief_string);
		}
	else if (strcmp(token,"REQUESTING_UNIT")==0 || strcmp(token,"TARGET_NAME_UNIT")==0)
		{
		AddStringToBrief("BATTALION_NAME_LONG",UnitNameLong,brief_string);
		}
	else if (strcmp(token,"PACKAGE_TARGET_NAME_UNIT")==0)
		{
		AddStringToBrief("DIVISION_NAME",DivisionName,brief_string);
		}
	else if (strncmp(token,"PACKAGE_TARGET_NAME",19)==0)
		{
		i = rand()%6;
		if (!i)
			AddStringToBrief("OBJECTIVE_NAME",AirbaseName,brief_string);
		else if (i == 1)
			AddStringToBrief("OBJECTIVE_NAME",BridgeName,brief_string);
		else if (i == 2)
			AddStringToBrief("OBJECTIVE_NAME",PortName,brief_string);
		else
			AddStringToBrief("OBJECTIVE_NAME",DepotName,brief_string);
		}
	else if (strcmp(token,"ENTITY_NAME")==0)
		{
		i = rand()%2;
		if (i)
			AddStringToBrief("SQUADRON_NAME",SquadronName,brief_string);
		else
			AddStringToBrief("SQUADRON_NAME",UnitNameShort,brief_string);
		}
	else if (strcmp(token,"TARGET_VEHICLE_NAME")==0 || strcmp(token,"REQUESTING_UNIT_VEHICLE")==0 || strcmp(token,"INTERCEPTOR_NAME")==0 || strcmp(token,"AIRCRAFT_TYPE")==0)
		{
		AddStringToBrief("VEHICLE_NAME","F-16", brief_string);
		}
	else if (strcmp(token,"TARGET_OWNER")==0)
		{
		AddIndexedStringToBrief(40+rand()%6+1,brief_string);
		}
	else if (strcmp(token,"PACKAGE_TARGET_BUILDING")==0 || strcmp(token,"TARGET_BUILDING")==0)
		{
		i = rand()%4;
		if (!i)
			AddStringToBrief("FEATURE_NAME",FeatureName[0],brief_string);
		else if (i == 1)
			AddStringToBrief("FEATURE_NAME",FeatureName[1],brief_string);
		else if (i == 2)
			AddStringToBrief("FEATURE_NAME",FeatureName[2],brief_string);
		else
			AddStringToBrief("FEATURE_NAME",FeatureName[3],brief_string);
		}
	else if (strcmp(token,"REQUESTING_UNIT_DEST")==0 || strcmp(token,"TARGET_NAME")==0)
		{
		i = rand()%4;
		if (!i)
			AddStringToBrief("OBJECTIVE_NAME",AirbaseName,brief_string);
		else if (i == 1)
			AddStringToBrief("OBJECTIVE_NAME",BridgeName,brief_string);
		else if (i == 2)
			AddStringToBrief("OBJECTIVE_NAME",PortName,brief_string);
		else
			AddStringToBrief("OBJECTIVE_NAME",DepotName,brief_string);
		}
/*	else if (strcmp(token,"AWACS_NAME")==0)
		{
		mec->curr_data = 1;
		}
	else if (strcmp(token,"JSTAR_NAME")==0)
		{
		mec->curr_data = 1;
		}
	else if (strcmp(token,"TANKER_NAME")==0)
		{
		mec->curr_data = 1;
		}
*/
	else if (strncmp(token,"CONTEXT_STR",11)==0)
		{
		value = FindCurrentValue("CONTEXT_EQ");
		sptr = token + 11;
		i = sptr[0] - '1';
		AddIndexedStringToBrief(700+i*50+value,brief_string);
		}
	else if (strncmp(token,"RESULT_STR",10)==0)
		{
		_TCHAR		temp[128],format[128],t1[80],t2[80],hdr[30];
		int			success = FindCurrentValue("PACKAGE_SUCCESS_EQ");
		char		*sptr;
	
		value = FindCurrentValue("CONTEXT_EQ");

		sptr = strchr(token,' ');
		if (sptr && strchr(sptr,'U'))
			sprintf(t1,"(BATTALION_NAME)%s",UnitNameShort);
		else if (sptr && strchr(sptr,'O'))
			sprintf(t1,"(OBJECTIVE_NAME)%s",BridgeName);
		if (sptr && strchr(sptr,'R'))
			sprintf(t2,"(BATTALION_NAME)%s",UnitNameShort);

		switch (success)
			{
			case 0:
				ReadIndexedString(1200+value,format,127);
				ConstructOrderedSentence(temp,format,t1,t2);
				sprintf(hdr,"%d",1200+value);
				AddStringToBrief(hdr,temp,brief_string);
				break;
			case 1:
				ReadIndexedString(1300+value,format,127);
				ConstructOrderedSentence(temp,format,t1,t2);
				sprintf(hdr,"%d",1300+value);
				AddStringToBrief(hdr,temp,brief_string);
				break;
			case 2:
				ReadIndexedString(1400+value,format,127);
				ConstructOrderedSentence(temp,format,t1,t2);
				sprintf(hdr,"%d",1400+value);
				AddStringToBrief(hdr,temp,brief_string);
				break;
			case 3:
				ReadIndexedString(1500+value,format,127);
				ConstructOrderedSentence(temp,format,t1,t2);
				sprintf(hdr,"%d",1500+value);
				AddStringToBrief(hdr,temp,brief_string);
				break;
			default:
				AddIndexedStringToBrief(1100,brief_string);
				break;
			}
		}
	else if (strcmp(token,"NUM_AIRCRAFT")==0)
		{
		AddIndexedStringToBrief(rand()%4,brief_string);
		}
	else if (strcmp(token,"TIME_ON_TARGET")==0 || strcmp(token,"TIME_ON_STATION_LABEL")==0 || strcmp(token,"PATROL_TIME")==0)
		{
		AddTimeToBrief(1234567,brief_string);
		}
	else if (strcmp(token,"ACTUAL_TIME_ON_TARGET")==0)
		{
		if (!rand()%3)
			AddIndexedStringToBrief(239,brief_string);
		else
			{
			AddTimeToBrief(123456,brief_string);
			if (!rand()%3)
				{
				AddIndexedStringToBrief(117,brief_string);
				AddNumberToBrief(20,brief_string);
				AddIndexedStringToBrief(240,brief_string);
				AddIndexedStringToBrief(118,brief_string);
				}
			else if (!rand()%3)
				{
				AddIndexedStringToBrief(117,brief_string);
				AddNumberToBrief(20,brief_string);
				AddIndexedStringToBrief(241,brief_string);
				AddIndexedStringToBrief(118,brief_string);
				}
			}
		}
	else if (strcmp(token,"ALTERNATE_STRIP_NAME")==0)
		{
		AddStringToBrief(token,AirbaseName,brief_string);
		}
	else if (strcmp(token,"ENEMY_NAME")==0)
		{
		if (rand()%5)
			AddIndexedStringToBrief(40+1+rand()%7,brief_string);
		else
			AddIndexedStringToBrief(113,brief_string);
		}
	else if (strncmp(token,"GENERAL_LOCATION",16)==0 || strncmp(token,"SPECIFIC_LOCATION",17)==0 || strncmp(token,"NEAREST_LOCATION",16)==0 || strncmp(token,"THE_LOCATION",12)==0)
		{
		AddLocationToBrief(tolower(token[0]), brief_string);
		}

	//
	// Mission evaluation tokens here
	//
	else if (strcmp(token,"PACKAGE_SUCCESS")==0 || strcmp(token,"FLIGHT_SUCCESS")==0)
		{
		value = FindCurrentValue("MISSION_SUCCESS");
		AddIndexedStringToBrief(20+value,brief_string);
//		AddIndexedStringToBrief(20+rand()%5,brief_string);
		}
	else if (strcmp(token,"PILOT_RATING")==0)
		AddIndexedStringToBrief(10+rand()%5,brief_string);
	else if (strcmp(token,"PILOT_NAME")==0)
		{
		_TCHAR	temp[128];
		ReadIndexedString(2301,temp, 127);
		AddStringToBrief(token,temp,brief_string);
		}
	else if (strcmp(token,"PILOT_STATUS")==0)
		AddIndexedStringToBrief(95+rand()%4,brief_string);
	else if (strcmp(token,"AA_KILLS")==0)
		AddNumberToBrief(2,brief_string);
	else if (strcmp(token,"AG_KILLS")==0)
		AddNumberToBrief(1,brief_string);
	else if (strcmp(token,"FRIENDLY_LOSSES")==0)
		AddIndexedStringToBrief(3,brief_string);
	else if (strcmp(token,"LONG_MISSION_SUCCESS")==0)
		{
		_TCHAR		tstring[80],wstring[80];
		value = FindCurrentValue("MISSION_SUCCESS");
		AddIndexedStringToBrief(25+value,brief_string);
		ReadIndexedString(1000+rand()%5, wstring, 80);
		_stprintf(tstring,wstring,1);
		AddStringToBrief(token,tstring,brief_string);
		}
	else if (strcmp(token,"PLANE_STATUS")==0)
		AddIndexedStringToBrief(90+rand()%4,brief_string);
	else if (strcmp(token,"SHOW_THREATS")==0)
		{
		if (rand()%2)
			{
			AddStringToBrief("VEHICLE_NAME","SA-2",brief_string);
			AddIndexedStringToBrief(235,brief_string);
			AddLocationToBrief('s', brief_string);
			AddStringToBrief("","\n",brief_string);
			}
		else
			{
			AddStringToBrief("VEHICLE_NAME","ZSU-23",brief_string);
			AddIndexedStringToBrief(236,brief_string);
			AddLocationToBrief('s', brief_string);
			AddStringToBrief("","\n",brief_string);
			}
		}
	else if (strcmp(token,"THREAT_VEHICLE_NAME")==0)
		{
		AddStringToBrief("VEHICLE_NAME","T-72",brief_string);
		}
	else if (strcmp(token,"WEAPON_LOAD")==0)
		AddNumberToBrief(rand()%3,brief_string);
	else if (strcmp(token,"WEAPON_NAME")==0)
		AddStringToBrief(token,"Aim-9", brief_string);
	else if (strcmp(token,"WEAPON_FIRED")==0)
		AddNumberToBrief(2, brief_string);
	else if (strcmp(token,"WEAPON_HIT")==0)
		AddNumberToBrief(1, brief_string);
	else if (strcmp(token,"WEAPON_MISSED")==0)
		AddNumberToBrief(1, brief_string);
	else if (strcmp(token,"WEAPON_HIT_RATIO")==0)
		AddNumberToBrief(50,brief_string);
	else if (strcmp(token,"SHOW_EVENT")==0)
		{
		_TCHAR	format[128],temp[128],name[40],time[40];

		ReadIndexedString(41,name,127);
		GetTimeString(123456, time);

		AddStringToBrief("","\n",brief_string);
		ReadIndexedString(1700,format,127);
		ConstructOrderedSentence(temp,format,name,"F-16","Viper","1",time);
		AddStringToBrief("1700",temp,brief_string);
		AddStringToBrief("","\n",brief_string);
		ReadIndexedString(1701,format,127);
		ConstructOrderedSentence(temp,format,name,"F-16","Viper","1",time);
		AddStringToBrief("1701",temp,brief_string);
		AddStringToBrief("","\n",brief_string);
		ReadIndexedString(1702,format,127);
		ConstructOrderedSentence(temp,format,"Viper","1",time);
		AddStringToBrief("1702",temp,brief_string);
		AddStringToBrief("","\n",brief_string);
		ReadIndexedString(1703,format,127);
		ConstructOrderedSentence(temp,format,"Viper","1",name,"F-16",time);
		AddStringToBrief("1703",temp,brief_string);
		AddStringToBrief("","\n",brief_string);
		ReadIndexedString(1708,format,127);
		ConstructOrderedSentence(temp,format,"Viper","1",name,"F-16",time);
		AddStringToBrief("1708",temp,brief_string);
		AddStringToBrief("","\n",brief_string);
		ReadIndexedString(1710,format,127);
		ConstructOrderedSentence(temp,format,"Viper","1",time);
		AddStringToBrief("1710",temp,brief_string);
		AddStringToBrief("","\n",brief_string);
		}
	else if (strcmp(token,"WAYPOINT_NUM")==0)
		AddNumberToBrief(1, brief_string);
	else if (strcmp(token,"WAYPOINT_ACTION")==0)
		AddIndexedStringToBrief(350+rand()%20, brief_string);
	else if (strcmp(token,"WAYPOINT_TIME")==0)
		AddTimeToBrief(12345678, brief_string);
	else if (strcmp(token,"WAYPOINT_HEADING")==0)
		AddNumberToBrief(45,brief_string);
	else if (strcmp(token,"WAYPOINT_SPEED")==0)
		AddNumberToBrief(450,brief_string);
	else if (strcmp(token,"WAYPOINT_ALT")==0)
		AddNumberToBrief(10000,brief_string);
	else if (strcmp(token,"WAYPOINT_CLIMB")==0)
		{
//		AddIndexedStringToBrief(1600,brief_string);
		AddIndexedStringToBrief(1650,brief_string);
		}
	else if (strcmp(token,"WAYPOINT_DESC")==0)
		AddIndexedStringToBrief(1650+rand()%20, brief_string);
	else if (strcmp(token,"ENEMY_SQUADRONS")==0)
		{
		AddStringToBrief("SQUADRON_NAME",SquadronName, brief_string);
		AddStringToBrief("SQUADRON_NAME"," (F-16)	-- 100%% ", brief_string);
		AddIndexedStringToBrief(164, brief_string);
		AddStringToBrief("","\n", brief_string);
		}
	else if (strcmp(token,"ENTITY_ELEMENT_NAME")==0)
		AddStringToBrief("VEHICLE_NAME","Mig-29",brief_string);
	else if (strcmp(token,"ENTITY_OPERATIONAL")==0)
		AddNumberToBrief(100,brief_string);
	else if (strcmp(token,"BEST_FEATURES")==0)
		AddStringToBrief("FEATURE_NAME",FeatureName[0],brief_string);
	else if (strcmp(token,"POTENTIAL_TARGETS")==0)
		{
		AddStringToBrief("OBJECTIVE_NAME",BridgeName, brief_string);
		AddStringToBrief("","  100%% ", brief_string);
		AddIndexedStringToBrief(165, brief_string);
		AddStringToBrief("","\n", brief_string);
		}
	}

void DoFile (char *filename, FILE *op)
	{
	int		done=0,curr_stack=0,stack_active[MAX_STACK] = { 1 };
	FILE	*fp;
	char	brief_string[1024],token[256];

	do
		{
		brief_string[0] = 0;
		done = 0;
		stack_active[0] = 1;
		fp = OpenCampFile(filename,"","r");
		while (fp && !done)
			{
			ReadComments(fp);
			ReadToken(fp,token,120);
			if (!token[0])
				continue;

			// Handle standard tokens
			if (strncmp(token,"#IF",3)==0)
				{
				// Don't repeat package missions with flight missions
				if (strncmp(token,"#IF_PACKAGE_MISSION_EQ",22)==0)
					{
					char *sptr = strchr(token,'E');
					sptr -= 2;
					sptr[0] = '#';
					sptr[1] = 'I';
					sptr[2] = 'F';
					sprintf(token,sptr);
					}
				curr_stack++;
				if (!stack_active[curr_stack-1])
					stack_active[curr_stack] = 0;
				else
					stack_active[curr_stack] = DoIfToken(token+4);
				continue;
				}
			else if (strcmp(token,"#ELSE")==0)
				{
				if (curr_stack>0 && stack_active[curr_stack-1])
					stack_active[curr_stack] = !stack_active[curr_stack];
				continue;
				}
			else if (strcmp(token,"#ENDIF")==0)
				{
				if (!curr_stack)
					printf("<Brief reading Error - unmatched #ENDIF>\n");
				else
					curr_stack--;
				continue;
				}
			else if (strcmp(token,"#ENDSCRIPT")==0)
				{
				done = 1;
				continue;
				}

			// Check for section activity
			if (stack_active[curr_stack])
				DoToken(token, brief_string);

			}
		if (strlen(brief_string) > 1)
			{
			fprintf(op,"\n");
//			printf("\n");
			for (int i=0; i<gNumTokens; i++)
				{
				fprintf(op,"%s = %d\n",gTokenList[i],FindCurrentValue(gTokenList[i]));
//				printf("%s = %d\n",gTokenList[i],FindCurrentValue(gTokenList[i]));
				}
//			printf(brief_string);
			fprintf(op,brief_string);
			}
		fclose(fp);
		}
		while (IncrementPassOptions());
	}

// =====================
// Main
// =====================

int main( int argc, char **argv )
	{
	char	*args;
	int		i = 0;
	FILE	*op;
	char	filename[MAX_PATH];

	if (argc >= 2)
		{
		for(int i=1; i<argc; i++)
			{
			args = argv[i];
			switch ( args[1] )
				{
				case 'd':
					sprintf(gBaseDir,args + 2);
					break;
				case 'l':
					sprintf(gLangDir,args + 2);
					sprintf(gBaseDir,"D:\\falcon4\\%s\\campaign\\save",gLangDir);
					break;
				default:
					break;
				}
			}
		}
	else
		{
		sprintf(gBaseDir,"D:\\falcon4\\campaign\\save");
		sprintf(gLangDir,"output");
		printf("Working directory: %s",gBaseDir);
//		scanf("%s",baseDir);
		}

	ReadIndex("strings");
	LoadNames("Korea");
	BuildExampleNames();
	sprintf(filename,"%s\\%s.txt",gBaseDir,gLangDir);
	op = fopen(filename,"w");

	while (FilesToRead[i][0])
		{
		ClearTokenList();
		fprintf(op,"\n==============================\n");
		fprintf(op,"FILENAME: %s\n",FilesToRead[i]);
		fprintf(op,"==============================\n");
		printf("\n==============================\n");
		printf("FILENAME: %s\n",FilesToRead[i]);
		printf("==============================\n");
		AnalyseFile(FilesToRead[i]);
		DoFile(FilesToRead[i],op);
		i++;
		}

	FreeIndex();
	FreeNames();
	fclose(op);
	printf("Done! - press <return> to exit\n");
	getchar();

	return 1;
	}

