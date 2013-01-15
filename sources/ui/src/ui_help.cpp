#include <windows.h>
#include "fsound.h"
#include "FalcSnd\psound.h"
#include "chandler.h"
#include "userids.h"
#include "textids.h"

extern int MainLastGroup,CampaignLastGroup,HelpLoaded,TacLastGroup;
extern C_Handler *gMainHandler;
extern C_Parser *gMainParser;

void CloseWindowCB(long ID,short hittype,C_Base *control);

static int TestLast=SND_NO_HANDLE;

static WAVEFORMATEX m_def=
{
	WAVE_FORMAT_PCM,
	1,
	22050,
	44100,
	2,
	16,
	0,
};

static WAVEFORMATEX s_def=
{
	WAVE_FORMAT_PCM,
	2,
	22050,
	88200,
	4,
	16,
	0,
};

void TestSoundCB(long,short hittype,C_Base *ctrl)
{
	C_ListBox *lbox;
	long itemID;
	SOUND_RES *snd;

	if(hittype != C_TYPE_SELECT)
		return;

	lbox=(C_ListBox*)ctrl;
	if(lbox)
	{
		itemID=lbox->GetTextID();
		if(itemID)
		{
			snd=gSoundMgr->GetSound(itemID);
			if(snd)
			{
				gSoundMgr->PlaySound(snd);
			}
		}
	}

}

void UI_Help_Guide_CB(long,short hittype,C_Base *ctrl)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if(ctrl->GetGroup())
	{
		gMainHandler->EnableWindowGroup(ctrl->GetGroup());
	}
	else
	{
		if(MainLastGroup == 4000) // Campaign
		{
			if(ctrl->GetUserNumber(CampaignLastGroup))
			{
				gMainHandler->EnableWindowGroup(ctrl->GetUserNumber(CampaignLastGroup));
			}
		}
		else if(MainLastGroup == 3000) // Campaign
		{
			if(ctrl->GetUserNumber(TacLastGroup))
			{
				gMainHandler->EnableWindowGroup(ctrl->GetUserNumber(TacLastGroup));
			}
		}
	}
}

void HookupHelpGuideWindows(long ID)
{
	C_Window *win;
	C_Button *btn;
	C_ListBox *lbox;

	win=gMainHandler->FindWindow(ID);
	if(win)
	{
		btn=(C_Button*)win->FindControl(CLOSE_WINDOW);
		if(btn)
			btn->SetCallback(CloseWindowCB);

		lbox=(C_ListBox*)win->FindControl(TEST_GENERAL_ID);
		if(lbox)
			lbox->SetCallback(TestSoundCB);

		lbox=(C_ListBox*)win->FindControl(TEST_MUSIC_ID);
		if(lbox)
			lbox->SetCallback(TestSoundCB);

		lbox=(C_ListBox*)win->FindControl(TEST_IA_STREAM_ID);
		if(lbox)
			lbox->SetCallback(TestSoundCB);

		lbox=(C_ListBox*)win->FindControl(TEST_DF_STREAM_ID);
		if(lbox)
			lbox->SetCallback(TestSoundCB);

		lbox=(C_ListBox*)win->FindControl(TEST_CP_STREAM_ID);
		if(lbox)
			lbox->SetCallback(TestSoundCB);

		lbox=(C_ListBox*)win->FindControl(TEST_CP2_STREAM_ID);
		if(lbox)
			lbox->SetCallback(TestSoundCB);
	}
}

void LoadHelpGuideWindows()
{
	long ID;

	if(HelpLoaded) return;

	if( _LOAD_ART_RESOURCES_)
		gMainParser->LoadImageList("help_res.lst");
	else
		gMainParser->LoadImageList("help_art.lst");
	gMainParser->LoadSoundList("help_snd.lst");
	gMainParser->LoadWindowList("help_scf.lst");  // Modified by M.N. - add art/art1024 by LoadWindowList

	ID=gMainParser->GetFirstWindowLoaded();
	while(ID)
	{
		HookupHelpGuideWindows(ID);
		ID=gMainParser->GetNextWindowLoaded();
	}
	HelpLoaded++;
}