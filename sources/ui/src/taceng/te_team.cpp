///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Tactical Engagement - Robin Heydon
//
// Implements the user interface for the tactical engagement section
// of falcon 4.0
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "vu2.h"
#include "team.h"
#include "division.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "cmap.h"
#include "uicomms.h"
#include "userids.h"
#include "textids.h"
#include "teamdata.h"

void tactical_territory_editor_clear (long ID, short hittype, C_Base *control);
void tactical_territory_editor_restore (long ID, short hittype, C_Base *control);
void tactical_set_drawmode (long ID, short hittype, C_Base *control);
void tactical_set_erasemode (long ID, short hittype, C_Base *control);
void tactical_territory_map_edit (long ID, short hittype, C_Base *control);
void UpdateOccupationMap(void);
void update_team_victory_window();
void SetupOOBWindow();
void UpdateTeamName(long team);
void RebuildTeamLists();
void AddTeam (int teamNum, int defaultStance);

extern C_Handler *gMainHandler;
extern uchar gSelectedTeam;

long gDrawTeam=1;

extern C_Map *gMapMgr;

long TeamBtnIDs[NUM_TEAMS]=
{
	GROUP1_FLAG,
	GROUP2_FLAG,
	GROUP3_FLAG,
	GROUP4_FLAG,
	GROUP5_FLAG,
	GROUP6_FLAG,
	GROUP7_FLAG,
	GROUP8_FLAG,
};

long TeamLineIDs[NUM_TEAMS]=
{
	GROUP1_COLOR,
	GROUP2_COLOR,
	GROUP3_COLOR,
	GROUP4_COLOR,
	GROUP5_COLOR,
	GROUP6_COLOR,
	GROUP7_COLOR,
	GROUP8_COLOR,
};

// JPO - helper routine to map experience to a label
static long Experience2Id(uchar exp)
{
    switch((exp - 60) / 10)
    {
    case 1:
	return CADET_LEVEL;
    case 2:
	return ROOKIE_LEVEL;
    case 3:
	return VETERAN_LEVEL;
    case 4:
	return ACE_LEVEL;
    default:
	return RECRUIT_LEVEL;
    }
}

void Init_Flag_Color_Used()
{
	short i;

	for(i=0;i<TOTAL_FLAGS;i++)
		FlagImageID[i][FLAG_STATUS]=0;

	for(i=0;i<NUM_TEAMS;i++)
		TeamColorUse[i]=0;
}

static uchar GetUnusedFlag()
{
	int i=1;

	while(i < TOTAL_FLAGS)
	{
		if(!FlagImageID[i][FLAG_STATUS])
			return(static_cast<uchar>(i));
		i++;
	}
	return(0);
}

static uchar GetUnusedColor()
{
	int i=1;
	while(i < NUM_TEAMS)
	{
		if(!TeamColorUse[i])
			return(static_cast<uchar>(i));
		i++;
	}
	return(0);
}

void SetupTeamFlags()
{
	C_Window *win=NULL;
	C_Button *EditMap=NULL,*SmallMap=NULL,*BigMap=NULL;
	short i;

	win=gMainHandler->FindWindow(TAC_EDIT_WIN);
	if(win)
		EditMap=(C_Button*)win->FindControl(TEAM_SELECTOR);

	win=gMainHandler->FindWindow(TAC_PUA_MAP);
	if(win)
		SmallMap=(C_Button*)win->FindControl(TEAM_SELECTOR);

	win=gMainHandler->FindWindow(TAC_FULLMAP_WIN);
	if(win)
		BigMap=(C_Button*)win->FindControl(TEAM_SELECTOR);

	for(i=0;i<NUM_TEAMS;i++)
	{
		// Setup flags used
		if(TeamInfo[i] && (TeamInfo[i]->flags & TEAM_ACTIVE))
		{
			if(EditMap)
				EditMap->SetImage(i,FlagImageID[TeamInfo[i]->GetFlag()][SMALL_HORIZ]);
			if(SmallMap)
				SmallMap->SetImage(i,FlagImageID[TeamInfo[i]->GetFlag()][SMALL_HORIZ]);
			if(BigMap)
				BigMap->SetImage(i,FlagImageID[TeamInfo[i]->GetFlag()][SMALL_HORIZ]);
		}
	}
	if(EditMap)
		EditMap->SetState(gSelectedTeam);
	if(SmallMap)
	{
		SmallMap->SetCallback(NULL);
		SmallMap->SetState(FalconLocalSession->GetTeam());
	}
	if(BigMap){
		// sfr: wrong map here
		//SmallMap->SetCallback(NULL);
		BigMap->SetCallback(NULL);
		BigMap->SetState(FalconLocalSession->GetTeam());
	}
	update_team_victory_window ();
}

void SetupTeamColors()
{ // Set ATO & OOB & Camp Map
	C_Window *win;
	C_Line   *line;

	gDrawTeam=gSelectedTeam;

	win=gMainHandler->FindWindow(TAC_EDIT_WIN);
	if(win)
	{
		line=(C_Line*)win->FindControl(TEAM_COLOR);
		if(line)
		{
			if (TeamInfo[gSelectedTeam])
			{
				line->SetColor(TeamColorList[TeamInfo[gSelectedTeam]->GetColor()]);
			}
			line->Refresh();
		}
	}

	win=gMainHandler->FindWindow(TAC_PUA_MAP);
	if(win)
	{
		line=(C_Line*)win->FindControl(TEAM_COLOR);
		if(line)
		{
			if (TeamInfo[gSelectedTeam])
			{
				line->SetColor(TeamColorList[TeamInfo[gSelectedTeam]->GetColor()]);
			}
			line->Refresh();
		}
	}

	win=gMainHandler->FindWindow(TAC_FULLMAP_WIN);
	if(win)
	{
		line=(C_Line*)win->FindControl(TEAM_COLOR);
		if(line)
		{
			if (TeamInfo[gSelectedTeam])
			{
				line->SetColor(TeamColorList[TeamInfo[gSelectedTeam]->GetColor()]);
			}
			line->Refresh();
		}
	}
	SetupOOBWindow();
	update_team_victory_window ();
}

void SetupTeamNames()
{ // Set ATO & OOB & Camp Map
	RebuildTeamLists();
	SetupOOBWindow();
	update_team_victory_window ();
}

void SetupTeamData(void)
{
	SetupTeamFlags();
	SetupTeamColors();
	SetupTeamNames();
	SetupOOBWindow();
	update_team_victory_window ();
}

void UpdateBigMapColors(long team)
{
	long idx,j;
	if(TeamInfo[team])
		idx=TeamInfo[team]->GetColor();
	else
		idx=0;
	for(j=0;j<8;j++)
	{
		gMapMgr->SetAirIcons(team,j,TeamFlightColorIconIDs[idx][j][0],TeamFlightColorIconIDs[idx][j][1]);
	}
	gMapMgr->SetArmyIcons(team,TeamColorIconIDs[idx][0],TeamColorIconIDs[idx][1]);
	gMapMgr->SetNavyIcons(team,TeamColorIconIDs[idx][0],TeamColorIconIDs[idx][1]);
	gMapMgr->SetObjectiveIcons(team,TeamColorIconIDs[idx][0],TeamColorIconIDs[idx][1]);
	gMapMgr->RemapTeamColors(team);
}

// NOTE: Automatically fixes duplicate flags & colors
void SetupTeamListValues()
{
	C_Window *win;
	C_Line *line;
	C_Button *btn;
	long i;
	long btnidx;

	win=gMainHandler->FindWindow(TAC_TEAM_WIN);
	if(win)
	{
		Init_Flag_Color_Used();
		btnidx=0;
		for(i=0;i<NUM_TEAMS;i++)
		{
			if(TeamInfo[i] && (TeamInfo[i]->flags & TEAM_ACTIVE))
			{
				if(FlagImageID[TeamInfo[i]->GetFlag()][FLAG_STATUS])
					TeamInfo[i]->SetFlag(GetUnusedFlag());
				FlagImageID[TeamInfo[i]->GetFlag()][FLAG_STATUS]=1;
				btn=(C_Button*)win->FindControl(TeamBtnIDs[btnidx]);
				if(btn)
				{
					if(i == gSelectedTeam)
						btn->SetState(1);
					else
						btn->SetState(0);

					btn->SetImage(0,FlagImageID[TeamInfo[i]->GetFlag()][BIG_VERT_DARK]);
					btn->SetImage(1,FlagImageID[TeamInfo[i]->GetFlag()][BIG_VERT]);
					btn->SetFlagBitOff(C_BIT_INVISIBLE);
					btn->Refresh();
					btn->SetUserNumber(0,i);
				}

				if(TeamColorUse[TeamInfo[i]->GetColor()])
					TeamInfo[i]->SetColor(GetUnusedColor());

				TeamColorUse[TeamInfo[i]->GetColor()]=1;
				line=(C_Line*)win->FindControl(TeamLineIDs[btnidx]);
				if(line)
				{
					line->SetColor(TeamColorList[TeamInfo[i]->GetColor()]);
					line->SetFlagBitOff(C_BIT_INVISIBLE);
					line->Refresh();
				}
				btnidx++;
			}
		}
		while(btnidx < NUM_TEAMS)
		{
			btn=(C_Button*)win->FindControl(TeamBtnIDs[btnidx]);
			if(btn)
			{
				btn->Refresh();
				btn->SetFlagBitOn(C_BIT_INVISIBLE);
				btn->SetUserNumber(0,0);
			}
			line=(C_Line*)win->FindControl(TeamLineIDs[btnidx]);
			if(line)
			{
				line->Refresh();
				line->SetFlagBitOn(C_BIT_INVISIBLE);
			}
			btnidx++;
		}
		UpdateOccupationMap();
	}
	update_team_victory_window ();
}

void SetupCurrentTeamValues(long team)
{
	C_Window *win;
	C_EditBox *ebox;
	C_ListBox *lbox;
	C_Box *box;
	C_Button *btn;

	if(team >= NUM_TEAMS) // Out of range
		return;

	if(!TeamInfo[team]) // Team undefined
		return;

	win=gMainHandler->FindWindow(TAC_TEAM_WIN);
	if(win)
	{
		btn=(C_Button*)win->FindControl(CURRENT_FLAG);
		if(btn)
		{
			btn->SetImage(0,FlagImageID[TeamInfo[team]->GetFlag()][BIG_HORIZ]);
			btn->Refresh();
		}
		box=(C_Box*)win->FindControl(CURRENT_COLOR);
		if(box)
		{
			box->SetColor(TeamColorList[TeamInfo[team]->GetColor()]);
			box->Refresh();
		}
		lbox=(C_ListBox*)win->FindControl(PILOT_SKILL);
		if(lbox)
		{
			lbox->SetValue(Experience2Id(TeamInfo[team]->airExperience));
			lbox->Refresh();
		}
		lbox=(C_ListBox*)win->FindControl(SAM_SKILL);
		if(lbox)
		{
			lbox->SetValue(Experience2Id(TeamInfo[team]->airDefenseExperience));			
			lbox->Refresh();
		}
		ebox=(C_EditBox*)win->FindControl(CURRENT_NAME);
		if(ebox)
		{
			ebox->SetText(TeamInfo[team]->GetName());
			ebox->Refresh();
		}
		ebox=(C_EditBox*)win->FindControl(MISSION_STATEMENT);
		if(ebox)
		{
			ebox->SetText(TeamInfo[team]->GetMotto());
			ebox->Refresh();
		}
	}
}

void MakeNewTeamCB(long,short hittype,C_Base *)
{
	long i;
	int	tid;
	_TCHAR buffer[30];

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	i=1;
	while(TeamInfo[i] && i < NUM_TEAMS)
		i++;

	if(i < NUM_TEAMS)
	{
		gSelectedTeam=static_cast<uchar>(i);
		gDrawTeam=i;
		AddTeam(gSelectedTeam,War);
		if(TeamInfo[gSelectedTeam])
		{
			TeamInfo[gSelectedTeam]->flags |= TEAM_ACTIVE;
			TeamInfo[gSelectedTeam]->SetFlag(GetUnusedFlag());
			FlagImageID[TeamInfo[gSelectedTeam]->GetFlag()][FLAG_STATUS]=1;
			TeamInfo[gSelectedTeam]->SetColor(GetUnusedColor());
			TeamColorUse[TeamInfo[gSelectedTeam]->GetColor()]=1;
			_stprintf(buffer,"%s #%1ld",gStringMgr->GetString(TXT_TEAM),gSelectedTeam);
			TeamInfo[gSelectedTeam]->SetName(buffer);
			_stprintf(buffer," ");
			TeamInfo[gSelectedTeam]->SetMotto(buffer);
			// Set this team at war with everyone
			for (tid=0; tid<NUM_TEAMS; tid++)
				{
				if (tid == gSelectedTeam)
					TeamInfo[gSelectedTeam]->stance[tid] = Allied;
				else
					TeamInfo[gSelectedTeam]->stance[tid] = War;
				}
		}
		UpdateBigMapColors(gSelectedTeam);
		SetupTeamData();
		SetupCurrentTeamValues(gSelectedTeam);
		SetupTeamListValues();
		UpdateOccupationMap();
		update_team_victory_window ();
	}
	else
	{// Error Message No more teams available
	}
}

static void RemoveTeamCB(long,short hittype,C_Base *)
{
	short i,TeamCount,new_owner;
	CampBaseClass	*entity;
	VuListIterator	eit(AllCampList);

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	TeamCount=0;
	for(i=0;i<NUM_TEAMS;i++)
	{
		if(TeamInfo[i] && (TeamInfo[i]->flags & TEAM_ACTIVE))
			TeamCount++;
	}
	if(TeamCount < 2)
	{
		// Error, you need at least 1 team
		return;
	}

	if(TeamInfo[gSelectedTeam])
	{
		FlagImageID[TeamInfo[gSelectedTeam]->GetFlag()][FLAG_STATUS]=0;
		TeamColorUse[TeamInfo[gSelectedTeam]->GetColor()]=0;
		RemoveTeam(gSelectedTeam);
		UpdateBigMapColors(gSelectedTeam);

		TeamCount=gSelectedTeam;
		i=0;
		while(!i)
		{
			gSelectedTeam--;
			if(gSelectedTeam < 1)
				gSelectedTeam=NUM_TEAMS-1;
			if(TeamInfo[gSelectedTeam] || gSelectedTeam == TeamCount)
				i=1;
		}

		if (FalconLocalSession->GetTeam() == gSelectedTeam)
			{
			FalconLocalSession->SetCountry(255);
			for (i=0; i<NUM_TEAMS; i++)
				{
				if (TeamInfo[i])
					FalconLocalSession->SetCountry(static_cast<uchar>(i));
				}
			}

		SetupTeamData();
		gDrawTeam=i;
		SetupCurrentTeamValues(gSelectedTeam);
		SetupTeamListValues();
		update_team_victory_window ();
	}

	// Pick a team to give all our stuff to.
	for (i=7; i<NUM_TEAMS && !TeamInfo[i]; i--);
		new_owner = i;

	entity = (CampBaseClass*) eit.GetFirst();
	while (entity)
		{
		entity->SetOwner(static_cast<uchar>(new_owner));
		if (entity->IsObjective())
			((Objective)entity)->SetObjectiveOldown(static_cast<uchar>(new_owner));
		entity = (CampBaseClass*) eit.GetNext();
		}
}

static void ChoosePrevFlag(long,short hittype,C_Base *)
{
	short oldFlag,newflag,done;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	oldFlag=static_cast<short>(TeamInfo[gSelectedTeam]->GetFlag());
	newflag=oldFlag;

	done=0;
	while(!done)
	{
		newflag--;
		if(newflag < 1)
			newflag+=TOTAL_FLAGS;
		if(!FlagImageID[newflag][FLAG_STATUS])
			done=1;
	}
	FlagImageID[newflag][FLAG_STATUS]=1;
	FlagImageID[oldFlag][FLAG_STATUS]=0;
	TeamInfo[gSelectedTeam]->SetFlag(static_cast<uchar>(newflag));
	SetupCurrentTeamValues(gSelectedTeam);
	SetupTeamListValues();
	SetupTeamFlags();
}

static void ChooseNextFlag(long,short hittype,C_Base *)
{
	short oldFlag,newflag,done;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	oldFlag=static_cast<short>(TeamInfo[gSelectedTeam]->GetFlag());
	FlagImageID[oldFlag][FLAG_STATUS]=0;
	newflag=oldFlag;

	done=0;
	while(!done)
	{
		newflag++;
		if(newflag >= TOTAL_FLAGS)
			newflag=1;
		if(!FlagImageID[newflag][FLAG_STATUS])
			done=1;
	}
	FlagImageID[newflag][FLAG_STATUS]=1;
	TeamInfo[gSelectedTeam]->SetFlag(static_cast<uchar>(newflag));
	SetupCurrentTeamValues(gSelectedTeam);
	SetupTeamListValues();
	SetupTeamFlags();
}

static void ChoosePrevColor(long,short hittype,C_Base *)
{
	short oldcolor,newcolor,done;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	oldcolor=static_cast<short>(TeamInfo[gSelectedTeam]->GetColor());
	TeamColorUse[oldcolor]=0;
	newcolor=oldcolor;

	done=0;
	while(!done)
	{
		newcolor--;
		if(newcolor < 1)
			newcolor=NUM_TEAMS-1;
		if(!TeamColorUse[newcolor])
			done=1;
	}
	TeamColorUse[newcolor]=1;
	TeamInfo[gSelectedTeam]->SetColor(static_cast<uchar>(newcolor));
	UpdateBigMapColors(gSelectedTeam);
	UpdateOccupationMap();
	SetupCurrentTeamValues(gSelectedTeam);
	SetupTeamListValues();
}

static void ChooseNextColor(long,short hittype,C_Base *)
{
	short oldcolor,newcolor,done;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	oldcolor=static_cast<short>(TeamInfo[gSelectedTeam]->GetColor());
	TeamColorUse[oldcolor]=0;
	newcolor=oldcolor;

	done=0;
	while(!done)
	{
		newcolor++;
		if(newcolor >= NUM_TEAMS)
			newcolor=1;
		if(!TeamColorUse[newcolor])
			done=1;
	}
	TeamColorUse[newcolor]=1;
	TeamInfo[gSelectedTeam]->SetColor(static_cast<uchar>(newcolor));
	UpdateBigMapColors(gSelectedTeam);
	UpdateOccupationMap();
	SetupCurrentTeamValues(gSelectedTeam);
	SetupTeamListValues();
	SetupTeamColors();
}

void ChooseTeamCB(long,short hittype,C_Base *base)
{
	C_Window *win;
	C_Button *btn;
	C_Line   *line;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	gSelectedTeam=static_cast<uchar>(base->GetUserNumber(0));
	if(gSelectedTeam < 1 || gSelectedTeam >= NUM_TEAMS)
		gSelectedTeam=1;

	gDrawTeam=gSelectedTeam;
	SetupCurrentTeamValues(gSelectedTeam);
	SetupTeamListValues();
	// Set team # on Map Windows (TEAM_SELECTOR)

	FalconLocalSession->SetCountry(gSelectedTeam);

	win=gMainHandler->FindWindow(TAC_EDIT_WIN);
	if(win)
	{
		btn=(C_Button*)win->FindControl(TEAM_SELECTOR);
		if(btn)
		{
			btn->SetState(gSelectedTeam);
			btn->Refresh();
		}
		line=(C_Line*)win->FindControl(TEAM_COLOR);
		if(line)
		{
			line->SetColor(TeamColorList[TeamInfo[gSelectedTeam]->GetColor()]);
			line->Refresh();
		}
	}

	win=gMainHandler->FindWindow(TAC_PUA_MAP);
	if(win)
	{
		btn=(C_Button*)win->FindControl(TEAM_SELECTOR);
		if(btn)
		{
			btn->SetState(gSelectedTeam);
			btn->Refresh();
		}
		line=(C_Line*)win->FindControl(TEAM_COLOR);
		if(line)
		{
			line->SetColor(TeamColorList[TeamInfo[gSelectedTeam]->GetColor()]);
			line->Refresh();
		}
	}

	win=gMainHandler->FindWindow(TAC_FULLMAP_WIN);
	if(win)
	{
		btn=(C_Button*)win->FindControl(TEAM_SELECTOR);
		if(btn)
		{
			btn->SetState(gSelectedTeam);
			btn->Refresh();
		}
		line=(C_Line*)win->FindControl(TEAM_COLOR);
		if(line)
		{
			line->SetColor(TeamColorList[TeamInfo[gSelectedTeam]->GetColor()]);
			line->Refresh();
		}
	}
}

static void SetTeamNameCB(long,short hittype,C_Base *base)
{
	C_EditBox *ebox;

	if(hittype != DIK_RETURN && hittype)
		return;

	ebox=(C_EditBox*)base;
	if(ebox && TeamInfo[gSelectedTeam])
		TeamInfo[gSelectedTeam]->SetName(ebox->GetText());
	UpdateTeamName(gSelectedTeam);
	SetupTeamNames();
}

static void SetTeamStatementCB(long,short hittype,C_Base *base)
{
	C_EditBox *ebox;

	if(hittype != DIK_RETURN)
		return;

	ebox=(C_EditBox*)base;
	if(ebox && TeamInfo[gSelectedTeam])
		TeamInfo[gSelectedTeam]->SetMotto(ebox->GetText());
}

// JPO - callback to set experience
static void SetTeamExperience(long ID,short hittype,C_Base *base)
{
	C_ListBox *lbox;

	if(hittype != DIK_RETURN)
		return;

	lbox=(C_ListBox*)base;
	int value = 0;
	if(lbox && TeamInfo[gSelectedTeam]) {
	    switch(lbox->GetTextID())
	    {
	    case CADET_LEVEL:
		value = 1;
		break;
	    case ROOKIE_LEVEL:
		value = 2;
		break;
	    case VETERAN_LEVEL:
		value = 3;
		break;
	    case ACE_LEVEL:
		value = 4;
		break;
	    default:
		value = 0;
		break;
	    }
	}
	if (ID == PILOT_SKILL)
		TeamInfo[gSelectedTeam]->airExperience = 60 + 10 * value;
	else if (ID == SAM_SKILL)
		TeamInfo[gSelectedTeam]->airDefenseExperience = 60 + 10 * value;

}

void Hookup_Team_Win(C_Window *win)
{
	C_Base *base;

	if(win)
	{
		if(win->GetID() != TAC_TEAM_WIN)
			return;

		base=win->FindControl(TAC_MAP_TE);
		if(base)
			base->SetCallback(tactical_territory_map_edit);

		base=win->FindControl(DRAW_TERRITORY);
		if(base)
			base->SetCallback(tactical_set_drawmode);

		base=win->FindControl(ERASE_TERRITORY);
		if(base)
			base->SetCallback(tactical_set_erasemode);

		base=win->FindControl(CLEAR_TERRITORY);
		if(base)
			base->SetCallback(tactical_territory_editor_clear);

		base=win->FindControl(UNDO_TERRITORY);
		if(base)
			base->SetCallback(tactical_territory_editor_restore);

		base=win->FindControl(GROUP1_FLAG);
		if(base)
			base->SetCallback(ChooseTeamCB);

		base=win->FindControl(GROUP2_FLAG);
		if(base)
			base->SetCallback(ChooseTeamCB);

		base=win->FindControl(GROUP3_FLAG);
		if(base)
			base->SetCallback(ChooseTeamCB);

		base=win->FindControl(GROUP4_FLAG);
		if(base)
			base->SetCallback(ChooseTeamCB);

		base=win->FindControl(GROUP5_FLAG);
		if(base)
			base->SetCallback(ChooseTeamCB);

		base=win->FindControl(GROUP6_FLAG);
		if(base)
			base->SetCallback(ChooseTeamCB);

		base=win->FindControl(GROUP7_FLAG);
		if(base)
			base->SetCallback(ChooseTeamCB);

		base=win->FindControl(GROUP8_FLAG);
		if(base)
			base->SetCallback(ChooseTeamCB);

		base=win->FindControl(TAC_NEW);
		if(base)
			base->SetCallback(MakeNewTeamCB);

		base=win->FindControl(TAC_DELETE_TEAM);
		if(base)
			base->SetCallback(RemoveTeamCB);

		base=win->FindControl(CURRENT_NAME);
		if(base)
			base->SetCallback(SetTeamNameCB);

		base=win->FindControl(MISSION_STATEMENT);
		if(base)
			base->SetCallback(SetTeamStatementCB);

		base=win->FindControl(PREV_FLAG);
		if(base)
			base->SetCallback(ChoosePrevFlag);

		base=win->FindControl(NEXT_FLAG);
		if(base)
			base->SetCallback(ChooseNextFlag);

		base=win->FindControl(PREV_COLOR);
		if(base)
			base->SetCallback(ChoosePrevColor);

		base=win->FindControl(NEXT_COLOR);
		if(base)
			base->SetCallback(ChooseNextColor);

		base=win->FindControl(PILOT_SKILL);
		if(base)
			base->SetCallback(SetTeamExperience);

		base=win->FindControl(SAM_SKILL);
		if(base)
			base->SetCallback(SetTeamExperience);
	}
}
