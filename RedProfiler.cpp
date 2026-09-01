#include	"RedProfiler.h"
#include <intrin.h>   // Artscout - 2026 (x64): __rdtsc
#include	"string.h"
#include	"stdio.h"
#include	"timemgr.h"

RedProfItemType	RedProfiles[MAX_PROFILES];
RedDisplayType	RedResults[MAX_PROFILES];
static	bool	TimeBase;
static	DWORD	LastTime=0;
static	__int64	ReferenceProfile;
static	char	ProfileMessage[MAX_MESSAGES][128];



void	ProfileMex(int Slot, const char *Message)
{
	int	a;

	for(a=0; a<MAX_MESSAGES; a++){
		if(!ProfileMessage[a][0]){	strcpy(ProfileMessage[a], Message); return; }
	}
}



int		SeekProfile(char *name)
{
	int	a,c=MAX_PROFILES-1;

	for(a=0; a<MAX_PROFILES; a++){
		if(!strcmp(RedProfiles[a].ProName,name)) return(a);
		if(RedProfiles[a].Type==EMPTY_PROF) c=a;
	}
	if(a==MAX_PROFILES) {
		strcpy(RedProfiles[c].ProName,name);
		RedProfiles[c].Average=0;
	}
	return(c);
}


void	StartProfile(char *name)
{	__int64	x;
	int	a;

	x = (__int64)__rdtsc();   // Artscout - 2026 (x64): RDTSC via intrinsic

	a=SeekProfile(name);
	if(!RedProfiles[a].Level){
		RedProfiles[a].Start=x;
		RedProfiles[a].Type=TIME_PROF;						// Time Profile as default
		RedProfiles[a].TimeOut=TIME_OUT;					// Default time on Display
	}
	RedProfiles[a].Level++;									// Stackable Profile Item
}


void	StartMainProfile(char *name)
{	__int64	x;
	int	a;

	x = (__int64)__rdtsc();   // Artscout - 2026 (x64): RDTSC via intrinsic

	a=SeekProfile(name);
	if(!RedProfiles[a].Level){
		RedProfiles[a].Start=x;
		RedProfiles[a].Type=MAIN_PROF;						// Time Profile as default
		RedProfiles[a].TimeOut=TIME_OUT;					// Default time on Display
	}
	RedProfiles[a].Level++;									// Stackable Profile Item
}



void	ProfileCount(char *name)
{	
	int	a;
	a=SeekProfile(name);
	if(RedProfiles[a].Type!=COUNT_PROF){
		RedProfiles[a].Average=0;
		RedProfiles[a].Type=COUNT_PROF;
	}
	RedProfiles[a].Average++;
	RedProfiles[a].TimeOut=TIME_OUT;					// Default time on Display
}




void	StopProfile(char *name)
{	__int64	x;

	int	a;
	x = (__int64)__rdtsc();   // Artscout - 2026 (x64): RDTSC via intrinsic

	a=SeekProfile(name);
	if(RedProfiles[a].Level==1){									// Stackable Profile Item
		if(RedProfiles[a].Type==TIME_PROF || RedProfiles[a].Type==MAIN_PROF){
			RedProfiles[a].Stop=x;
			RedProfiles[a].Total+=(RedProfiles[a].Stop-RedProfiles[a].Start);
		}
	} 
	
	if(RedProfiles[a].Level) RedProfiles[a].Level--;

	RedProfiles[a].TimeOut=TIME_OUT;					// Default time on Display

}

void	ReportValue(char *name, __int64 Value)
{	int	a;

	a=SeekProfile(name);
	RedProfiles[a].Type=VALUE_PROF;
	RedProfiles[a].Average=Value;
	RedProfiles[a].TimeOut=TIME_OUT;					// Default time on Display
}



void	ProfileBuildList(void)
{
	int	a,i;
	ReferenceProfile=0;
	for(a=0; a<MAX_PROFILES; a++){
		if(RedProfiles[a].Type!=EMPTY_PROF){
			for(i=0; i<MAX_PROFILES; i++) {
				if(!strcmp(RedResults[i].ProName,RedProfiles[a].ProName)) break;
				if(RedResults[i].Type==EMPTY_PROF) break;
			}
			if(i==MAX_PROFILES) i--;
			RedResults[i].Type=RedProfiles[a].Type;
			if(RedResults[i].Type==TIME_PROF || RedResults[i].Type==MAIN_PROF){
				RedProfiles[a].Average=(RedProfiles[a].Average-RedProfiles[a].Average/10)+RedProfiles[a].Total;
				if(RedProfiles[a].Type==MAIN_PROF) ReferenceProfile=RedProfiles[a].Average>>16;
				RedResults[i].Result=RedProfiles[a].Average>>16;
			} else
				RedResults[i].Result=RedProfiles[a].Average;
			strcpy(RedResults[i].ProName,RedProfiles[a].ProName);
			RedProfiles[a].Type=EMPTY_PROF;
			RedResults[i].TimeOut=TIME_OUT;

		}
		RedProfiles[a].Total=0;
		RedProfiles[a].Level=0;
	}

	DWORD TimeNow=TheTimeManager.GetClockTime();
	TimeBase=false;
	if((TimeNow-LastTime)>1000) {
		TimeBase=true;
		LastTime=TimeNow;
	}
}


void	FixMainProfile(void)
{
}


bool	ReportProfileNr(int  Num, char *dest )
{
	if(!ReferenceProfile){
		if(RedResults[Num].Type==EMPTY_PROF) return(false);
		if(RedResults[Num].Type==TIME_PROF) sprintf(dest, "%s : %4.2f", RedResults[Num].ProName, ((float)RedResults[Num].Result)/100);
		if(RedResults[Num].Type==COUNT_PROF) sprintf(dest, "%s : %8d", RedResults[Num].ProName, RedResults[Num].Result);
		if(RedResults[Num].Type==VALUE_PROF) sprintf(dest, "%s : %8d", RedResults[Num].ProName, RedResults[Num].Result);
	} else {
		if(RedResults[Num].Type==EMPTY_PROF) return(false);
		if(RedResults[Num].Type==TIME_PROF || RedResults[Num].Type==MAIN_PROF) sprintf(dest, "%s : %4.1f %% - %4.1f", RedResults[Num].ProName, ((float)RedResults[Num].Result/(float)ReferenceProfile)*100.0f, ((float)RedResults[Num].Result)/100.0f);
		if(RedResults[Num].Type==COUNT_PROF) sprintf(dest, "%s : %8d", RedResults[Num].ProName, RedResults[Num].Result);
		if(RedResults[Num].Type==VALUE_PROF) sprintf(dest, "%s : %8d", RedResults[Num].ProName, RedResults[Num].Result);
	}
	if(TimeBase){
		if(RedResults[Num].TimeOut) RedResults[Num].TimeOut--;
		if(!RedResults[Num].TimeOut) {
			RedResults[Num].Type=EMPTY_PROF;
			memcpy(&RedResults[Num],&RedResults[Num+1],sizeof(RedDisplayType)*(MAX_PROFILES-Num-1));
		}
	}
	return(true);
}

void	ReportMessageNr(int  Num, char *dest )
{
	strcpy(dest, ProfileMessage[Num]);
	ProfileMessage[Num][0]=0x00;
}


