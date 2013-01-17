#include "falclib.h"
#include "chandler.h"
#include "userids.h"
#include "PlayerOp.h"
#include "sim\include\stdhdr.h"
#include "ui_setup.h"
#include "falclib\include\fsound.h"
#include "falclib\include\soundfx.h"
#include <tchar.h>
#include "cmusic.h"
#include "f4find.h"

#include "falcsnd\VoiceManager.h"
#include "falcsnd\conv.h"
#include "falclib\include\soundgroups.h"
#include "falcsnd\psound.h"

extern C_Handler *gMainHandler;
extern C_Music *gMusic;
extern VoiceManager *VM;
extern int noUIcomms;
void PlayRandomMessage(int channel);

#define RESCALE(in,inmin,inmax,outmin,outmax) (int)( ((float)(in) - (inmin)) * ((outmax) - (outmin)) / ((inmax) - (inmin)) + (outmin))


/////////////
/////SoundTab
/////////////

void SoundFlagsCB(long ID,short hittype,C_Base *control); // MLR 12/13/2003 -
void SoundExtVolCB(long ID,short hittype,C_Base *control); // MLR 12/13/2003 - 

void InitButton(long ID,C_Window *win)
{
	C_Button *btn;

	if(win)
	{
		btn=(C_Button*)win->FindControl(ID);
		if(btn)
		{
			btn->SetState(0);
			switch(btn->GetUserNumber(1))
			{
				case 1: // Voice Manager
					if(VM)
					{
						VM->VMSilenceChannel(btn->GetUserNumber(0));
						VM->VMResetVoice(btn->GetUserNumber(0));
						F4SetStreamVolume(VM->VoiceHandle(btn->GetUserNumber(0)),PlayerOptions.GroupVol[btn->GetUserNumber(2)]);
					}
					break;
				case 2: // UI Sound effects
					gSoundMgr->StopSound(btn->GetUserNumber(0));
					gSoundMgr->SetVolume(PlayerOptions.GroupVol[btn->GetUserNumber(2)]);
					break;
				case 4: // Sim Sounds
					F4StopSound(btn->GetUserNumber(0));
					F4SetVolume(btn->GetUserNumber(0),PlayerOptions.GroupVol[btn->GetUserNumber(2)]);
					break;
			}
			btn->Refresh();
		}
	}
}

void InitSlider(long ID,C_Window *win)
{
	double pos;
	double range;
	C_Slider *sldr;

	if(win)
	{
		sldr=(C_Slider*)win->FindControl(ID);
		if(sldr)
		{
			range = sldr->GetSliderMax()-sldr->GetSliderMin();
			
			pos = PlayerOptions.GroupVol[sldr->GetUserNumber(2)];
			pos = (pos < SND_RNG)? SND_RNG:( (pos > 0) ? 0 : pos);
			pos = range - sqrt(static_cast<float>(pos) * (-1.0F))*range/SQRT_SND_RNG;

			sldr->Refresh();
			sldr->SetSliderPos(static_cast<long>(pos));
			sldr->Refresh();
		}
	}
}

void InitSoundSetup()
{
	C_Window *win;

	if (gSoundDriver)		// NULL if -nosound
	{
		PlayerOptions.GroupVol[MASTER_SOUND_GROUP]=gSoundDriver->GetMasterVolume();
	}
	win=gMainHandler->FindWindow(SETUP_WIN);
	if(win)
	{
		InitButton(ENGINE_SND,win);
		InitButton(SIDEWINDER_SND,win);
		InitButton(RWR_SND,win);
		InitButton(COCKPIT_SND,win);
		InitButton(COM1_SND,win);
		InitButton(COM2_SND,win);
		InitButton(SOUNDFX_SND,win);
		InitButton(UISOUNDFX_SND,win);

		InitSlider(ENGINE_VOLUME,win);
		InitSlider(SIDEWINDER_VOLUME,win);
		InitSlider(RWR_VOLUME,win);
		InitSlider(COCKPIT_VOLUME,win);
		InitSlider(COM1_VOLUME,win);
		InitSlider(COM2_VOLUME,win);
		InitSlider(SOUNDFX_VOLUME,win);
		InitSlider(UISOUNDFX_VOLUME,win);
		InitSlider(MUSIC_VOLUME,win);
		InitSlider(MASTER_VOLUME,win);



		/*
		UI_ENABLE_3D_SOUNDS							=70214,
		UI_ENABLE_DOPPLER_EFFECT					=70215,
		UI_ENABLE_DISTANCE_EFFECT					=70216,
		UI_ENABLE_INTERNAL_SOUND					=70217
		*/

		C_Button *button;
		int l;

		for(l=0;l<4;l++)
		{
			button=(C_Button *)win->FindControl(UI_ENABLE_3D_SOUNDS + l);
			if(button != NULL)
			{
				if(PlayerOptions.SoundFlags & 1<<l)
					button->SetState(C_STATE_1);
				else
					button->SetState(C_STATE_0);
				button->SetCallback(SoundFlagsCB);
				button->Refresh();
			}
		}

		/*
		EXTERNAL_SOUND_VOLUME						=70218
		*/

		C_Slider *sldr;

		sldr=(C_Slider*)win->FindControl(EXTERNAL_SOUND_VOLUME);
		if(sldr)
		{
//			if(PlayerOptions.SoundFlags<-10000)
//				PlayerOptions.SoundFlags=-10000;

//			if(PlayerOptions.SoundFlags>10000)
//				PlayerOptions.SoundFlags=10000;

//			if(PlayerOptions.SoundFlags<-5000)
//				PlayerOptions.SoundFlags=-5000;

//			if(PlayerOptions.SoundFlags>1000)
//				PlayerOptions.SoundFlags=1000;

			int smin,smax,pos;

			smax = sldr->GetSliderMax();
			smin = sldr->GetSliderMin();

//			pos=RESCALE(PlayerOptions.SoundExtAttenuation,-10000,1000,smin,smax);
			pos=RESCALE(PlayerOptions.SoundExtAttenuation,-5000,5000,smin,smax);

			sldr->SetSliderPos(pos);


//			sldr->SetSliderRange(-1000,1000);
			sldr->SetCallback(SoundExtVolCB);
			sldr->Refresh();

		}

		gMusic->SetVolume(PlayerOptions.GroupVol[MUSIC_SOUND_GROUP]);
	}
};

void TestButtonCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if(!control)
		return;

	switch(control->GetUserNumber(1))
	{
		case 1: // Voice Manager
			if(!VM)
				break;
			if(control->GetState())
			{
				VM->VMHearChannel(control->GetUserNumber(0));
				PlayRandomMessage(control->GetUserNumber(0));
			}
			else
			{
				VM->VMSilenceChannel(control->GetUserNumber(0));
				VM->VMResetVoice(control->GetUserNumber(0));
			}
			break;
		case 4: // Sim Sounds
		    {
			int idx = control->GetUserNumber(0);
			idx -= 100; // 100 based
			int handle = SFX_DEF && idx < NumSFX ? SFX_DEF[idx].handle : SND_NO_HANDLE;
			if(control->GetState())
				F4LoopSound(handle);
			else
				F4StopSound(handle);
		    }
			break;
		case 2: // UI Sound effects
			if(control->GetState())
				gSoundMgr->LoopSound(control->GetUserNumber(0));
			else
				gSoundMgr->StopSound(control->GetUserNumber(0));
			break;
	}
}

void SoundSliderCB(long,short hittype,C_Base *control)
{
	double pos,range;
	int volume;
	
	if(hittype != C_TYPE_MOUSEMOVE)
		return;

	range = ((C_Slider *)control)->GetSliderMax()-((C_Slider *)control)->GetSliderMin();
	pos =   (1.0F - ((C_Slider *)control)->GetSliderPos() / range);
	
	volume = (int)(pos * pos * (SND_RNG));
	PlayerOptions.GroupVol[control->GetUserNumber(2)]=volume;

	int idx,handle;

	switch(control->GetUserNumber(1))
	{
		case 1: // Voice Manager
			if(VM)
				F4SetStreamVolume(VM->VoiceHandle(control->GetUserNumber(0)),volume);
			break;
		case 2: // UI Sound effects
			gSoundMgr->SetVolume(volume);
			break;
		case 3: // Music
			gMusic->SetVolume(volume);
			break;
		case 4: // Sim Sounds
			idx = control->GetUserNumber(0);
			idx -= 100; // 100 based
			handle = SFX_DEF && idx < NumSFX ? SFX_DEF[idx].handle : SND_NO_HANDLE;
//			F4SetVolume(control->GetUserNumber(0),volume);
			F4SetVolume(handle,volume);
			break;
		case 5: // Master
			if (gSoundDriver)
			{
				gSoundDriver->SetMasterVolume(volume);
			}
			break;
	}
}

void PlayVoicesCB(long,short,C_Base *control)
{
	if(VM)
	{	
		C_Button *button;

		button = (C_Button *)control->Parent_->FindControl(COM2_SND);
		if(button && button->GetState())
		{
			PlayRandomMessage(button->GetUserNumber(0));
		}
	
		button = (C_Button *)control->Parent_->FindControl(COM1_SND);
		if(button && button->GetState())
		{
			PlayRandomMessage(button->GetUserNumber(0));
		}
	}
}

// M.N.
void TogglePlayerVoiceCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;
	
	if(((C_Button *)control)->GetState())
		PlayerOptions.PlayerRadioVoice = true;
	else
		PlayerOptions.PlayerRadioVoice = false;
}

void ToggleUICommsCB(long, short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if(((C_Button *)control)->GetState())
	{
		PlayerOptions.UIComms = true;
		noUIcomms = false;
	}
	else
	{
		PlayerOptions.UIComms = false;
		noUIcomms = true;
	}
}

void SoundFlagsCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if(!control)
		return;

	int ID=control->GetID();
	switch(ID)
	{
		/*
		UI_ENABLE_3D_SOUNDS							=70214,
		UI_ENABLE_DOPPLER_EFFECT					=70215,
		UI_ENABLE_DISTANCE_EFFECT					=70216,
		UI_ENABLE_INTERNAL_SOUND					=70217
		*/

		case UI_ENABLE_3D_SOUNDS: 
		case UI_ENABLE_DOPPLER_EFFECT:
		case UI_ENABLE_DISTANCE_EFFECT:
		case UI_ENABLE_INTERNAL_SOUND:
			ID-=UI_ENABLE_3D_SOUNDS;
			int flagbit = 1<<ID;
			if(!VM)
				break;
			if(control->GetState())
			{
				PlayerOptions.SoundFlags |= flagbit;
			}
			else
			{
				PlayerOptions.SoundFlags &= (~flagbit);
			}
			break;
	}
}

void SoundExtVolCB(long,short hittype,C_Base *control)
{
	
	if(hittype != C_TYPE_MOUSEMOVE)
		return;

	int smin,smax,pos;

	smax = ((C_Slider *)control)->GetSliderMax();
	smin = ((C_Slider *)control)->GetSliderMin();
	pos  = ((C_Slider *)control)->GetSliderPos();




//	PlayerOptions.SoundExtAttenuation = RESCALE(pos,smin,smax,-10000,10000);
	PlayerOptions.SoundExtAttenuation = RESCALE(pos,smin,smax,-5000,5000);
/*
	FILE *fp;
	if(fp=fopen("slider.txt","a+"))
	{
		fprintf(fp,"Value %d\n",RESCALE(pos,smin,smax,-10000,10000));
		fclose(fp);
	}
*/
}