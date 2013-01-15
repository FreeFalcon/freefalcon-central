/***************************************************************************\
    TrainingScript.cpp
    Mikal "Wombat778" Shaikh
    06Mar04

	-Script interpreter for training scripts
\***************************************************************************/

#include "Graphics\Include\renderow.h"
#include "stdhdr.h"
#include "soundfx.h"
#include "cpmanager.h"
#include "otwdrive.h"
#include "dispopts.h"
#include "ui95\chandler.h"
#include "falcsnd\psound.h"

#include "sinput.h"		
#include "commands.h"	
#include "trainingscript.h"	


//helper to determine whether to flash
bool doflash(int flashtime)
{
	if (vuxRealTime & flashtime)
		return false;
	return true;

}


//helper to draw text based on orientation

void DoText(float x, float y, char *string, int boxed, int orientation)
{
	switch(orientation)
	{
	case 0:
		OTWDriver.renderer->TextLeft (x,y,string,boxed);							
		break;
	case 1:
		OTWDriver.renderer->TextCenter (x,y,string,boxed);							
		break;
	case 2:
		OTWDriver.renderer->TextRight (x,y,string,boxed);							
		break;
	case 3:
		OTWDriver.renderer->TextLeftVertical (x,y,string,boxed);	//Not sure what the vertical ones do, but leave in there for now						
		break;
	case 4:
		OTWDriver.renderer->TextCenterVertical (x,y,string,boxed);							
		break;
	case 5:
		OTWDriver.renderer->TextRightVertical (x,y,string,boxed);							
		break;
	default:
		OTWDriver.renderer->TextLeft (x,y,string,boxed);							
		break;
	}
}


TrainingScriptClass::TrainingScriptClass()
{			
	startline = NULL;
	endline = NULL;
	capturelist=NULL;
	stacktop=NULL;
	repeatlist=NULL;
	blockallowlist=NULL;
	numfunctions=0;	
	functiontable = new FunctionTableType[MAX_SCRIPTFUNCS];		//Changed from malloc
	RestartScript();
	InitFunctions();
	
}

TrainingScriptClass::~TrainingScriptClass()
{	
	RestartScript();
	ClearAllLines();	
	delete [] functiontable;		//changed from free
}


bool
TrainingScriptClass::CmdPrint(LineType *line)
{
	float waittime = 0.0f;
	

	if (line->numargs!=3)
	{
		nextline=nextline->next;
		return false;
	}			
	
	
	if (firstrun)								//is this the first time that this has been run
	{
		sscanf (line->GetArg(1), "%f", &waittime);		//if yes, set the timer 
		line->localtimer =  (long)((float)waittime * 1000.0f) + vuxRealTime;
		AddRepeatList(line);
		nextline=nextline->next;
		line->localcursorx=cursorx;
		line->localcursory=cursory;
		line->localcolor=color;
		line->localfont=font;
		line->localflash=flash;
		line->localtextboxed=textboxed;
		line->localtextorientation=textorientation;
	}

	if (inrepeatlist && doflash(line->localflash) )
	{
		int tempcolor, tempfont;

		tempfont = OTWDriver.renderer->CurFont();
		tempcolor = OTWDriver.renderer->Color();
		OTWDriver.renderer->SetFont(line->localfont);
		OTWDriver.renderer->SetColor (line->localcolor);

		DoText (line->localcursorx, line->localcursory, line->GetArg(2),line->localtextboxed,line->localtextorientation);							
		
		OTWDriver.renderer->SetFont(tempfont);
		OTWDriver.renderer->SetColor (tempcolor);
	}
	
	
	if (line->localtimer && (vuxRealTime > line->localtimer) )
		DelRepeatList(line);
	

	return true;
	
}

bool
TrainingScriptClass::CmdOval(LineType *line)
{
	float waittime = 0.0f;
	

	if (line->numargs!=3&&line->numargs!=4)
	{
		nextline=nextline->next;
		return false;
	}			
	
	
	if (firstrun)								//is this the first time that this has been run
	{
		sscanf (line->GetArg(1), "%f", &waittime);		//if yes, set the timer 
		line->localtimer =  (long)((float)waittime * 1000.0f) + vuxRealTime;
		AddRepeatList(line);
		nextline=nextline->next;
		line->localcursorx=cursorx;
		line->localcursory=cursory;
		line->localcolor=color;	
		line->localflash=flash;	
	}

	if (inrepeatlist  && doflash(line->localflash))
	{
		int tempcolor=0;
		float xradius=0;
		float yradius=0;

		tempcolor = OTWDriver.renderer->Color();
		OTWDriver.renderer->SetColor (line->localcolor);

		sscanf (line->GetArg(2), "%f", &xradius);		//This is a circle
		if (line->numargs==4)
		{
			sscanf (line->GetArg(3), "%f", &yradius);	//This is an oval
			OTWDriver.renderer->Oval(line->localcursorx, line->localcursory, xradius, yradius);							
		}
		else
			OTWDriver.renderer->Circle(line->localcursorx, line->localcursory, xradius);							

		OTWDriver.renderer->SetColor (tempcolor);
	}
	
	
	if (line->localtimer && (vuxRealTime > line->localtimer) )
		DelRepeatList(line);
	

	return true;
	
}

bool
TrainingScriptClass::CmdLine(LineType *line)
{
	float waittime = 0.0f;
	

	if (line->numargs!=6)
	{											//Wrong number of arguments
		nextline=nextline->next;
		return false;
	}			
	
	
	if (firstrun)								//is this the first time that this has been run
	{
		sscanf (line->GetArg(1), "%f", &waittime);		//if yes, set the timer 
		line->localtimer =  (long)((float)waittime * 1000.0f) + vuxRealTime;
		AddRepeatList(line);
		nextline=nextline->next;
		line->localcursorx=cursorx;
		line->localcursory=cursory;
		line->localcolor=color;	
		line->localflash=flash;
	}

	if (inrepeatlist  && doflash(line->localflash))
	{
		int tempcolor=0;
		float x1=0;
		float x2=0;
		float y1=0;
		float y2=0;

		tempcolor = OTWDriver.renderer->Color();
		OTWDriver.renderer->SetColor (line->localcolor);

		sscanf (line->GetArg(2), "%f", &x1);	
		sscanf (line->GetArg(3), "%f", &y1);	
		sscanf (line->GetArg(4), "%f", &x2);	
		sscanf (line->GetArg(5), "%f", &y2);	
		
		OTWDriver.renderer->Line(x1,y1, x2,y2);							
		
		OTWDriver.renderer->SetColor (tempcolor);
	}
	
	
	if (line->localtimer && (vuxRealTime > line->localtimer) )
		DelRepeatList(line);
	

	return true;
	
}


bool
TrainingScriptClass::CmdWaitPrint(LineType *line)
{
	float waittime = 0.0f;

	if (line->numargs!=3)
	{									//Wrong number of arguments
		nextline=nextline->next;
		return false;
	}			
	
	if (firstrun)								//is this the first time that this has been run
	{
		sscanf (line->GetArg(1), "%f", &waittime);		//if yes, set the timer 
		line->localtimer =  (long)((float)waittime * 1000.0f) + vuxRealTime;
	}
	
	if (line->localtimer && (vuxRealTime > line->localtimer) )	
		nextline=nextline->next;		
	else	
	{
		if ( doflash(flash) )
		{
			int tempcolor, tempfont;

			tempfont = OTWDriver.renderer->CurFont();
			tempcolor = OTWDriver.renderer->Color();
			OTWDriver.renderer->SetFont(font);
			OTWDriver.renderer->SetColor (color);

			DoText (cursorx, cursory, line->GetArg(2),textboxed,textorientation);							

			OTWDriver.renderer->SetFont(tempfont);
			OTWDriver.renderer->SetColor (tempcolor);
		}

	}
	

	return true;
	
}


bool
TrainingScriptClass::CmdWait(LineType *line)
{
	float waittime = 0.0f;
	
	if (line->numargs!=2)
	{
		nextline=nextline->next;
		return false;
	}			
	
	if (firstrun)						//is this the first time that this has been run
	{
		sscanf (line->GetArg(1), "%f", &waittime);		//if yes, set the timer 
		line->localtimer =  (long)((float)waittime * 1000.0f) + vuxRealTime;
	}
	
	if (line->localtimer && (vuxRealTime > line->localtimer) )		
	{
		nextline=nextline->next;		
		return true;
	}
		
	return false;
}

bool iscallback(char *arg)
{
	if (isdigit(arg[0]))
		return true;
	return false;
}

bool
TrainingScriptClass::CmdWaitInput(LineType *line)
{
	float waittime = 0.0f;
	
	
	if (line->numargs<3)
	{
		nextline=nextline->next;
		return false;
	}			
	
	if (firstrun)						//is this the first time that this has been run
	{
		sscanf (line->GetArg(1), "%f", &waittime);		//if yes, set the timer 
		line->localtimer =  (long)((float)waittime * 1000.0f) + vuxRealTime;
		capturing=true;
	}
	
	if (line->localtimer && (vuxRealTime > line->localtimer) )				//if no, then if the timer has expired, move to the next line
	{
		nextline=nextline->next;		
		capturing=false;
		return false;						//Timed out, so return false.
	}
	
	for (int i=2;i<line->numargs;i++)
	{

		int callback;		
		callback=0;
		
		if (iscallback(line->GetArg(i)))
		{
			sscanf (line->GetArg(i), "%d", &callback);
			if (IsCaptured(NULL,callback))
			{
				nextline=nextline->next;
				capturing=false;
				ClearCaptureList();
				return true;
			}
		}
		else
		{
			if (IsCaptured(FindFunctionFromString(line->GetArg(i)),NULL))
			{
				nextline=nextline->next;
				capturing=false;
				ClearCaptureList();
				return true;
			}
		}
	}

	return false;
}



bool
TrainingScriptClass::CmdWaitMouse(LineType *line)
{
	float waittime = 0.0f;
	float mousex,mousey, targetdist, targetx, targety;
	
	if (line->numargs!=5)
	{
		nextline=nextline->next;
		return false;
	}			
	
	if (firstrun)						//is this the first time that this has been run
	{
		sscanf (line->GetArg(1), "%f", &waittime);		//if yes, set the timer 
		line->localtimer =  (long)((float)waittime * 1000.0f) + vuxRealTime;
	}
	
	if (line->localtimer && (vuxRealTime > line->localtimer) )				//if no, then if the timer has expired, move to the next line
	{
		nextline=nextline->next;
		return false;						//Timed out, so return false.
	}
	


	sscanf (line->GetArg(2), "%f", &targetx);		
	sscanf (line->GetArg(3), "%f", &targety);
	sscanf (line->GetArg(4), "%f", &targetdist);
	mousex  = -1.0f + ( ( 2.0f * (float) gxPos ) / (float) DisplayOptions.DispWidth);
	mousey  = -1.0f + ( ( 2.0f * (float) gyPos ) / (float) DisplayOptions.DispHeight);


	if (sqrt(((mousex-targetx)*(mousex-targetx))+((mousey-targety)*(mousey-targety)))  <  targetdist ) 
	{
		nextline=nextline->next;
		return true;
	}


	return false;
}

bool
TrainingScriptClass::CmdIfTrue(LineType *line)
{

	if (!nextline->next)
	{
		nextline=nextline->next;
		return false;							//Error in placement
	}			

	if (result)
		nextline=nextline->next;
	else
		nextline=nextline->next->next;

	return result;
}


bool
TrainingScriptClass::CmdIfNotTrue(LineType *line)
{
	if (!nextline->next)
	{
		nextline=nextline->next;
		return false;							//Error in placement
	}			

	if (!result)
		nextline=nextline->next;
	else
		nextline=nextline->next->next;

	return result;
}

bool
TrainingScriptClass::CmdSound(LineType *line)
{
	if (line->numargs=!2)
	{
		nextline=nextline->next;
		return false;
	}
		
	soundid = F4StartStream (line->GetArg(1), 0);
	if (soundid==SND_NO_HANDLE)
		return false;

	nextline=nextline->next;
	return true;
}

bool
TrainingScriptClass::CmdWaitSound(LineType *line)
{	
	
	if (line->numargs=!2)
	{
		nextline=nextline->next;
		return false;
	}
	
	if (firstrun)						//is this the first time that this has been run			
		line->localsoundid = F4StartStream (line->GetArg(1), 0);	
		
	if (!gSoundDriver->IsStreamPlaying(line->localsoundid))
	{
		nextline=nextline->next;
		return true;
	}
	return false;
}

bool
TrainingScriptClass::CmdWaitSoundStop(LineType *line)
{
	float waittime = 0.0f;
	
	if (line->numargs!=2)
	{
		nextline=nextline->next;
		return false;
	}
	
	if (firstrun)						//is this the first time that this has been run
	{
		sscanf (line->GetArg(1), "%f", &waittime);		//if yes, set the timer 
		line->localtimer =  (long)((float)waittime * 1000.0f) + vuxRealTime;
	}
	
	if (line->localtimer && (vuxRealTime > line->localtimer) )		
	{
		nextline=nextline->next;		
		return true;
	}
		
	if (!gSoundDriver->IsStreamPlaying(soundid))
	{
		nextline=nextline->next;
		return true;
	}
	return false;
}

bool
TrainingScriptClass::CmdEndSection(LineType *line)
{
	
	if (stacktop)										//If there is a return stack, then use it
	{
		nextline = PopStack();		
		return true;
	}
	else													//Otherwise, just skip to the next line
	{
		nextline=nextline->next;
		return false;
	}


}

bool
TrainingScriptClass::CmdEndScript(LineType *line)
{
	
	nextline = NULL;									//End the script;
	return true;

}


bool
TrainingScriptClass::CmdBlock(LineType *line)
{
		
	ClearBlockAllowList();
	if (line->numargs==1)
		isblocklist = false;
	else
		isblocklist = true;

	for (int i=1;i<line->numargs;i++)
	{

		int callback;		
		callback=0;
		
		if (iscallback(line->GetArg(i)))
		{
			sscanf (line->GetArg(i), "%d", &callback);
			AddBlockAllowCommand(NULL,callback);			
		}
		else
			AddBlockAllowCommand(FindFunctionFromString(line->GetArg(i)),NULL);
	}			

	nextline=nextline->next;
	return true;
}

bool
TrainingScriptClass::CmdAllow(LineType *line)
{		
	
	ClearBlockAllowList();
	if (line->numargs==1)
		isblocklist = true;
	else
		isblocklist = false;

	for (int i=1;i<line->numargs;i++)
	{

		int callback;		
		callback=0;
		
		if (iscallback(line->GetArg(i)))
		{
			sscanf (line->GetArg(i), "%d", &callback);
			AddBlockAllowCommand(NULL,callback);			
		}
		else
			AddBlockAllowCommand(FindFunctionFromString(line->GetArg(i)), NULL);
	}			
	
	nextline=nextline->next;
	return true;
}


bool
TrainingScriptClass::CmdJumpSection(LineType *line)
{
	LineType *linewalker;

	if (line->numargs=!2)
	{
		nextline=nextline->next;
		return false;
	}

	linewalker=startline;

	while (linewalker)
	{
		if (linewalker->function == -1 && !strcmp(linewalker->GetArg(0), line->GetArg(1)))
		{
			nextline=linewalker;
			return true;
		}
		linewalker=linewalker->next;
	}
	nextline=nextline->next;
	return false;					//Section not found
}

bool
TrainingScriptClass::CmdCallSection(LineType *line)
{
	LineType *linewalker;

	if (line->numargs=!2)
	{
		nextline=nextline->next;
		return false;
	}

	linewalker=startline;
	
	while (linewalker)
	{
		if (linewalker->function == -1 && !strcmp(linewalker->GetArg(0), line->GetArg(1)))
		{
			PushStack(line->next);
			nextline=linewalker;			
			return true;
		}
		linewalker=linewalker->next;
	}
	nextline=nextline->next;		
	return false;					//Section not found
}

bool
TrainingScriptClass::CmdSetCursor(LineType *line)
{
	if (line->numargs=!3)
	{
		nextline=nextline->next;
		return false;
	}
			
	sscanf (line->GetArg(1), "%f", &cursorx);		
	sscanf (line->GetArg(2), "%f", &cursory);		
	nextline=nextline->next;
	return true;	
}

bool
TrainingScriptClass::CmdSetColor(LineType *line)
{
			
	if (line->numargs=!2)
	{
		nextline=nextline->next;
		return false;
	}

	sscanf (line->GetArg(1), "%x", &color);				
	nextline=nextline->next;
	return true;	
}

bool
TrainingScriptClass::CmdSetFont(LineType *line)
{
	if (line->numargs=!2)
	{
		nextline=nextline->next;
		return false;
	}

	sscanf (line->GetArg(1), "%d", &font);				
	nextline=nextline->next;
	return true;	
}

bool
TrainingScriptClass::CmdEnterCritical(LineType *line)
{
	if (line->numargs=!1)
	{
		nextline=nextline->next;
		return false;
	}

	incritical=true;
	nextline=nextline->next;
	return true;	
}

bool
TrainingScriptClass::CmdEndCritical(LineType *line)
{
	if (line->numargs=!1)
	{
		nextline=nextline->next;
		return false;
	}

	incritical=false;
	nextline=nextline->next;
	return true;	
}

bool
TrainingScriptClass::CmdSimCommand(LineType *line)
{
	InputFunctionType theFunc;

	if (line->numargs=!2)
		return false;

	theFunc = FindFunctionFromString(line->GetArg(1));
	if (theFunc)
	{
		int buttonId=UserFunctionTable.GetButtonId(theFunc);
		if(buttonId < 0) 
			theFunc(1, KEY_DOWN, NULL);
		else 		
			theFunc(1, KEY_DOWN, OTWDriver.pCockpitManager->GetButtonPointer(buttonId));		
		nextline=nextline->next;
		return true;
	}
	nextline=nextline->next;
	return false;	
}


bool
TrainingScriptClass::CmdWhile(LineType *line)
{
	LineType *linewalker;

	if (line->numargs=!1)
		return false;

	if (!result)
	{
		linewalker=line->next;
	
		while (linewalker)
		{
			if (!strcmp(linewalker->GetArg(0), "EndSection"))
			{				
				nextline=linewalker->next;			
				return false;
			}
			linewalker=linewalker->next;
		}
		nextline=nextline->next;	//No ENDSECTION found.  Just skip the loop
		return false;			
	}
	else
		PushStack(line->previous);
	nextline=nextline->next;
		
	return true;	
}

bool
TrainingScriptClass::CmdWhileNot(LineType *line)
{
	LineType *linewalker;

	if (line->numargs=!1)
		return false;

	if (result)
	{
		linewalker=line->next;
	
		while (linewalker)
		{
			if (!strcmp(linewalker->GetArg(0), "EndSection"))	//3-16-04 changed ENDSECTION to EndSection
			{				
				nextline=linewalker->next;			
				return false;
			}
			linewalker=linewalker->next;
		}
		nextline=nextline->next;	//No ENDSECTION found.  Just skip the loop
		return false;			
	}
	else
		PushStack(line->previous);
	nextline=nextline->next;
		
	return true;	
}


bool
TrainingScriptClass::CmdCallIf(LineType *line)
{
	LineType *linewalker;

	if (line->numargs<2)
		return false;

	if (result)
	{

		linewalker=startline;
	
		while (linewalker)
		{
			if (linewalker->function == -1 && !strcmp(linewalker->GetArg(0), line->GetArg(1)))
			{
				PushStack(line->next);
				nextline=linewalker;			
				return true;
			}
			linewalker=linewalker->next;
		}
		nextline=nextline->next;		
		return false;					//Section not found		
	}
	else if (line->numargs==3)
	{
		linewalker=startline;
	
		while (linewalker)
		{
			if (linewalker->function == -1 && !strcmp(linewalker->GetArg(0), line->GetArg(2)))
			{
				PushStack(line->next);
				nextline=linewalker;			
				return true;
			}
			linewalker=linewalker->next;
		}
		nextline=nextline->next;		
		return false;					//Section not found		
	}
	nextline=nextline->next;

	return true;	
}

bool
TrainingScriptClass::CmdCallIfNot(LineType *line)
{
	LineType *linewalker;

	if (line->numargs<2)
		return false;

	if (!result)
	{

		linewalker=startline;
	
		while (linewalker)
		{
			if (linewalker->function == -1 && !strcmp(linewalker->GetArg(0), line->GetArg(1)))
			{
				PushStack(line->next);
				nextline=linewalker;			
				return true;
			}
			linewalker=linewalker->next;
		}
		nextline=nextline->next;		
		return false;					//Section not found		
	}
	else if (line->numargs==3)
	{
		linewalker=startline;
	
		while (linewalker)
		{
			if (linewalker->function == -1 && !strcmp(linewalker->GetArg(0), line->GetArg(2)))
			{
				PushStack(line->next);
				nextline=linewalker;			
				return true;
			}
			linewalker=linewalker->next;
		}
		nextline=nextline->next;		
		return false;					//Section not found		
	}
	nextline=nextline->next;		
	return true;	
}

bool
TrainingScriptClass::CmdClear(LineType *line)
{
	DelAllRepeatList();
	nextline=nextline->next;
	return true;
}

bool
TrainingScriptClass::CmdClearLast(LineType *line)
{
	int clearline = 0;
	RepeatListType *walker = repeatlist;

	if (line->numargs==2)
	{
		sscanf (line->GetArg(1), "%d", &clearline);		
		int i=1;
		while (walker)
		{
			if (i == clearline) 
			{
				walker->remove=true;
				nextline=nextline->next;
				return true;
			}
			i++;
			walker=walker->next;
		}
		nextline=nextline->next;
		return false;
	}
	else
	{			
		if (repeatlist)
		{
			repeatlist->remove=true;
			nextline=nextline->next;
			return true;
		}
		nextline=nextline->next;
		return false;
	}
}



bool
TrainingScriptClass::CmdSetFlash(LineType *line)
{
	if (line->numargs=!2)
	{
		nextline=nextline->next;
		return false;
	}

	sscanf (line->GetArg(1), "%x", &flash);				
	nextline=nextline->next;
	return true;	
}

bool
TrainingScriptClass::CmdSetCursorCallback(LineType *line)
{
	int i = 0;
	int callback = 0;
	int callbackx=0;
	int callbacky=0;
	
	if (line->numargs=!2){
		nextline=nextline->next;
		return false;
	}

	sscanf (line->GetArg(1), "%d", &callback);

	if (OTWDriver.pCockpitManager->GetActivePanel() && callback)		
		for (i = 0; i < OTWDriver.pCockpitManager->GetActivePanel()->mNumButtonViews; i++)
			if (callback == OTWDriver.pCockpitManager->GetActivePanel()->mpButtonViews[i]->GetCallBackAndXY(&callbackx, &callbacky))
			{
				cursorx=-1.0f + ( ( 2.0f * (float) callbackx ) / (float) DisplayOptions.DispWidth);
				cursory=1.0f - ( ( 2.0f * (float) callbacky ) / (float) DisplayOptions.DispHeight);				

				nextline=nextline->next;
				return true;
			}					
	nextline=nextline->next;
	return false;
}


bool
TrainingScriptClass::CmdSetCursorDial(LineType *line)
{
	int i = 0;
	int callback = 0;
	int callbackx=0;
	int callbacky=0;
	
	if (line->numargs=!2){
		nextline=nextline->next;
		return false;
	}

	sscanf (line->GetArg(1), "%d", &callback);

	if (OTWDriver.pCockpitManager->GetActivePanel() && callback)		
		for (i = 0; i < OTWDriver.pCockpitManager->GetActivePanel()->mNumObjects; i++ ) 
		{			
			if (callback == OTWDriver.pCockpitManager->GetActivePanel()->mpObjects[i]->mCallbackSlot)
			{
				callbackx=OTWDriver.pCockpitManager->GetActivePanel()->mpObjects[i]->mDestRect.left+
					      ((OTWDriver.pCockpitManager->GetActivePanel()->mpObjects[i]->mDestRect.right-
						  OTWDriver.pCockpitManager->GetActivePanel()->mpObjects[i]->mDestRect.left)/2.0f);
				callbacky=OTWDriver.pCockpitManager->GetActivePanel()->mpObjects[i]->mDestRect.top+
						  ((OTWDriver.pCockpitManager->GetActivePanel()->mpObjects[i]->mDestRect.bottom-
						  OTWDriver.pCockpitManager->GetActivePanel()->mpObjects[i]->mDestRect.top)/2.0f);	
				cursorx=-1.0f + ( ( 2.0f * (float) callbackx ) / (float) DisplayOptions.DispWidth);
				cursory=1.0f - ( ( 2.0f * (float) callbacky ) / (float) DisplayOptions.DispHeight);				

				nextline=nextline->next;
				return true;
			}			
		}
	nextline=nextline->next;
	return false;
}

bool
TrainingScriptClass::CmdWaitCallbackVisible(LineType *line)
{
	float waittime = 0.0f;
	int i = 0;
	int callback = 0;
	int callbackx=0;
	int callbacky=0;

	if (line->numargs!=3)
	{									//Wrong number of arguments
		nextline=nextline->next;
		return false;
	}			
	
	if (firstrun)								//is this the first time that this has been run
	{
		sscanf (line->GetArg(1), "%f", &waittime);		//if yes, set the timer 
		line->localtimer =  (long)((float)waittime * 1000.0f) + vuxRealTime;
	}
	
	if (line->localtimer && (vuxRealTime > line->localtimer) )	
		nextline=nextline->next;		
	else	
	{
		sscanf (line->GetArg(2), "%d", &callback);

		if (OTWDriver.pCockpitManager->GetActivePanel() && callback)		
		for(i = 0 ; i < OTWDriver.pCockpitManager->GetActivePanel()->mNumButtonViews; i++) 			
			if (callback == OTWDriver.pCockpitManager->GetActivePanel()->mpButtonViews[i]->GetCallBackAndXY(&callbackx, &callbacky))
			{
				nextline=nextline->next;
				return true;
			}								
	}
	

	return false;
}


bool
TrainingScriptClass::CmdWaitDialVisible(LineType *line)
{
	float waittime = 0.0f;
	int i = 0;
	int callback = 0;
	int callbackx=0;
	int callbacky=0;

	if (line->numargs!=3)
	{									//Wrong number of arguments
		nextline=nextline->next;
		return false;
	}			
	
	if (firstrun)								//is this the first time that this has been run
	{
		sscanf (line->GetArg(1), "%f", &waittime);		//if yes, set the timer 
		line->localtimer =  (long)((float)waittime * 1000.0f) + vuxRealTime;
	}
	
	if (line->localtimer && (vuxRealTime > line->localtimer) )	
		nextline=nextline->next;		
	else	
	{
		sscanf (line->GetArg(2), "%d", &callback);

		if (OTWDriver.pCockpitManager->GetActivePanel() && callback)		
			for (i = 0; i < OTWDriver.pCockpitManager->GetActivePanel()->mNumObjects; i++ ) 						
				if (callback == OTWDriver.pCockpitManager->GetActivePanel()->mpObjects[i]->mCallbackSlot)
				{					
					nextline=nextline->next;
					return true;
				}						

	}	
	return false;
}





bool
TrainingScriptClass::CmdSetTextBoxed(LineType *line)
{
	if (line->numargs=!2)
	{
		nextline=nextline->next;
		return false;
	}

	sscanf (line->GetArg(1), "%x", &textboxed);				
	nextline=nextline->next;
	return true;	
}

bool
TrainingScriptClass::CmdMoveCursor(LineType *line)
{
	float dx = 0;
	float dy = 0;
	if (line->numargs=!3)
	{
		nextline=nextline->next;
		return false;
	}
			
	sscanf (line->GetArg(1), "%f", &dx);		
	sscanf (line->GetArg(2), "%f", &dy);
	cursorx+=dx;
	cursory+=dy;
	nextline=nextline->next;
	return true;	
}

bool
TrainingScriptClass::CmdSetTextOrientation(LineType *line)
{	
	if (line->numargs=!2)
	{
		nextline=nextline->next;
		return false;
	}
			
	sscanf (line->GetArg(1), "%d", &textorientation);			
	nextline=nextline->next;
	return true;	
}

bool
TrainingScriptClass::CmdSetViewCallback(LineType *line)
{
	int i = 0;
	int callback = 0;
	int numpanels = 0;
	int callbackx, callbacky; //these are only here because getcallbackandXY needs these.
	
	if (line->numargs=!2){
		nextline=nextline->next;
		return false;
	}

	sscanf (line->GetArg(1), "%d", &callback);

	numpanels = OTWDriver.pCockpitManager->GetNumPanels();

	if (callback)
		for (int panelcounter = 0; panelcounter < numpanels; panelcounter++ ) 		
			for (i = 0; i < OTWDriver.pCockpitManager->GetPanel(panelcounter)->mNumButtonViews; i++ ) 						
				if (callback == OTWDriver.pCockpitManager->GetPanel(panelcounter)->mpButtonViews[i]->GetCallBackAndXY(&callbackx, &callbacky))
				{		
					nextline=nextline->next;
					OTWDriver.pCockpitManager->SetActivePanel(OTWDriver.pCockpitManager->GetPanel(panelcounter)->mIdNum);
					return true;
				}						
	nextline=nextline->next;
	return false;
}

bool
TrainingScriptClass::CmdSetViewDial(LineType *line)
{
	int i = 0;
	int callback = 0;
	int numpanels = 0;
	
	if (line->numargs=!2){
		nextline=nextline->next;
		return false;
	}

	sscanf (line->GetArg(1), "%d", &callback);

	numpanels = OTWDriver.pCockpitManager->GetNumPanels();

	if (callback)
		for (int panelcounter = 0; panelcounter < numpanels; panelcounter++ ) 		
			for (i = 0; i < OTWDriver.pCockpitManager->GetPanel(panelcounter)->mNumObjects; i++ ) 						
				if (callback == OTWDriver.pCockpitManager->GetPanel(panelcounter)->mpObjects[i]->mCallbackSlot)
				{		
					nextline=nextline->next;
					OTWDriver.pCockpitManager->SetActivePanel(OTWDriver.pCockpitManager->GetPanel(panelcounter)->mIdNum);
					return true;
				}						
	nextline=nextline->next;
	return false;
}

bool
TrainingScriptClass::CmdSetPanTilt(LineType *line)
{
	
	float temppan, temptilt;

	if (line->numargs=!3){
		nextline=nextline->next;
		return false;
	}
	
	sscanf(line->GetArg(1), "%f", &temppan);
	sscanf(line->GetArg(2), "%f", &temptilt);
	OTWDriver.SetCameraPanTilt(temppan, temptilt);
	nextline=nextline->next;
	return false;
}

bool
TrainingScriptClass::CmdMovePanTilt(LineType *line)
{
	
	float temppan, temptilt, movepan, movetilt;

	if (line->numargs=!3){
		nextline=nextline->next;
		return false;
	}

	OTWDriver.GetCameraPanTilt(&temppan, &temptilt);
	sscanf(line->GetArg(1), "%f", &movepan);
	sscanf(line->GetArg(2), "%f", &movetilt);
	OTWDriver.SetCameraPanTilt(temppan+movepan, temptilt+movetilt);
	nextline=nextline->next;
	return false;
}

bool
TrainingScriptClass::CmdSetCursor3D(LineType *line)
{

	Tpoint newpoint;
	ThreeDVertex screenpoint;

	if (line->numargs=!4){
		nextline=nextline->next;
		return false;
	}

	sscanf(line->GetArg(1), "%f", &newpoint.x);
	sscanf(line->GetArg(2), "%f", &newpoint.y);
	sscanf(line->GetArg(3), "%f", &newpoint.z);

	OTWDriver.renderer->TransformCameraCentricPoint(&newpoint, &screenpoint);

	if (screenpoint.csZ<0)
	{
		cursorx =  (screenpoint.x - OTWDriver.renderer->shiftX) / OTWDriver.renderer->scaleX;
		cursory =  (screenpoint.y - OTWDriver.renderer->shiftY) / OTWDriver.renderer->scaleY;
	}

	nextline=nextline->next;
	return false;
}


bool
TrainingScriptClass::RunLine(LineType *line)
{
	if (line->function != -1) 
		result = (this->*functiontable[line->function].function)(line);
	else
		nextline=nextline->next;

	return true;
}


bool
TrainingScriptClass::RunScript()
{   

	DisplayMatrix dm;	
	OTWDriver.renderer->SaveDisplayMatrix(&dm);
	OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
	OTWDriver.renderer->CenterOriginInViewport();
		
	firstrun=false;
	
	RunRepeatList();
	CleanupRepeatList();

	if (nextline)
	{
		do
		{
	
			LineType *currentline = nextline;
		
			if (lastline==nextline) 
				firstrun=false;
			else 
				firstrun=true;
			RunLine(currentline);
			lastline=currentline;
		} while (incritical);
	}
	else
	{
		OTWDriver.renderer->RestoreDisplayMatrix(&dm);
		return false;
	}
	OTWDriver.renderer->RestoreDisplayMatrix(&dm);
	return true;
}
		
bool
TrainingScriptClass::RestartScript()
{   
	ClearStack();
	ClearRepeatList();
	ClearCaptureList();
	ClearBlockAllowList();
	numlines = 0;
	lastline = NULL;
	nextline = startline;		
	timer = 0;
	result = false;
	color = 0xff0000ff;
	cursorx = 0.0f;
	cursory = 0.0f;
	soundid = SND_NO_HANDLE;
	font = 2;
	flash = 0;
	textboxed = 0;
	textorientation = 0;
	repeatlist=NULL;
	blockallowlist=NULL;
	isblocklist=true;
	inrepeatlist=false;
	firstrun=false;
	incritical=false;

	capturelist=NULL;
	capturing=false;
	
	return true;
}

void 
TrainingScriptClass::PushStack(LineType *returnline)
{
	StackType *newstackitem;

	newstackitem = new StackType;
	newstackitem->line=returnline;
	newstackitem->next=stacktop;
	stacktop=newstackitem;
}

LineType *
TrainingScriptClass::PopStack()
{
	StackType *oldstackitem;
	LineType *returnline;

	if (stacktop)
	{	oldstackitem = stacktop;
		stacktop = oldstackitem->next;
		returnline=oldstackitem->line;
		delete oldstackitem;
		return returnline;
	}
	return NULL;
}

void
TrainingScriptClass::ClearStack()
{
	StackType *walker;
	StackType *nextwalker;

	walker=stacktop;

	while (walker)
	{
		nextwalker=walker->next;
		delete walker;
		walker=nextwalker;
	}	
	stacktop = NULL;

}


bool
LineType::AddArg(char *argtext)
{
	ArgType *newarg;

	if (!strcmp(argtext,""))		//If this is a null string, return 
		return false; 

	newarg = new ArgType;

	newarg->arglength=strlen(argtext);	
	newarg->arg=new char[newarg->arglength+1];		//Changed from malloc

	strcpy(newarg->arg,argtext);
	
	if (endarg)
		endarg->next=newarg;
	if (!startarg)
		startarg=newarg;
	endarg=newarg;

	numargs++;
	return true;

}

char *
LineType::GetArg(int argnum)
{
	ArgType *argwalker;
	

	argwalker=startarg;
	int i=0;
	while (argwalker)
	{
		if (argnum==i)
			return argwalker->arg;
		argwalker=argwalker->next;
		i++;
	}
	return "";		//This should NEVER happen.
}



bool
TrainingScriptClass::AddLine(char *linetext)
{
	LineType *newline;
	int charcounter = 0;	
	bool inquote = false;
	char templine[MAX_LINE_LENGTH];

	if ( !linetext||!linetext[0] )
		return false;

	
	newline = new LineType;
	newline->previous=endline;
	

	if (endline)
	{
		newline->previous=endline;
		endline->next=newline;
	}	
	if (!startline)
		startline=newline;
	endline=newline;

	numlines++;
	
	int finished = false;

	for (int i=0;linetext[i]!='\0' && !finished;i++)
	{
		switch (linetext[i])
		{
		case '"': 
			{
				inquote=!inquote;
				break;
			}
		case '\n':
			{				
				templine[charcounter]='\0';
				charcounter=0;
				newline->AddArg(templine);								
				finished=true;
				break;
			}
		case '/':
			{
				if (linetext[i+1]=='/')					//This should mean that only lines with // should be a comment, not just one slash
				{
					templine[charcounter]='\0';
					charcounter=0;
					newline->AddArg(templine);								
					finished=true;
					break;				
				}				
				else
				{			
					templine[charcounter]=linetext[i];
					charcounter++;
					break;
				}
			}
		case '\\':
			{
				if (linetext[i+1]=='n')
				{
					templine[charcounter]='\n';
					charcounter++;
					i++;				//Make sure we are skipping the 'n' character
					break;
				}
				else
				{
					templine[charcounter]=linetext[i];
					charcounter++;
					break;
				}				
			}

		default:
			{
				if ((linetext[i]!=' ')||inquote)
				{
					templine[charcounter]=linetext[i];
					charcounter++;
				}
				else
				{
					templine[charcounter]='\0';
					charcounter=0;
					newline->AddArg(templine);					
				}
			}
		}
	}

	//If the first argument is a function, find the function number.  The first argument is still the text of the function, but the number will have a much reduced performance impact
	newline->function=FindFunction(newline->GetArg(0));

	return true;

}

bool TrainingScriptClass::AddFunction(char *funcname, ScriptFunctionType function)
{
	if (numfunctions<MAX_SCRIPTFUNCS)
	{
		strncpy(functiontable[numfunctions].funcname,funcname,SCRIPTFUNC_SIZE);
		functiontable[numfunctions].function = function;
		numfunctions++;
		return true;
	}
	return false;
}

void TrainingScriptClass::InitFunctions()
{
		//The list of functions that we recognize.
		AddFunction("Print",CmdPrint);
		AddFunction("WaitPrint", CmdWaitPrint);
		AddFunction("WaitInput", CmdWaitInput);
		AddFunction("Wait",CmdWait);
		AddFunction("WaitMouse", CmdWaitMouse);
		AddFunction("WaitSoundStop",CmdWaitSoundStop);
		AddFunction("WaitSound",CmdWaitSound);
		AddFunction("Sound", CmdSound);
		AddFunction("EndSection",CmdEndSection);
		AddFunction("EndScript",CmdEndScript);
		AddFunction("Block",CmdBlock);
		AddFunction("Allow",CmdAllow);
		AddFunction("If",CmdIfTrue);
		AddFunction("IfNot",CmdIfNotTrue);
		AddFunction("Jump",CmdJumpSection);
		AddFunction("Call",CmdCallSection);
		AddFunction("SetCursor",CmdSetCursor);
		AddFunction("SetColor",CmdSetColor);
		AddFunction("SetFont",CmdSetFont);
		AddFunction("Oval",CmdOval);
		AddFunction("Line",CmdLine);
		AddFunction("EnterCritical",CmdEnterCritical);
		AddFunction("EndCritical",CmdEndCritical);
		AddFunction("SimCommand",CmdSimCommand);
		AddFunction("While",CmdWhile);
		AddFunction("WhileNot",CmdWhileNot);
		AddFunction("CallIf",CmdCallIf);
		AddFunction("CallIfNot",CmdCallIfNot);
		AddFunction("Clear",CmdClear);
		AddFunction("ClearLast",CmdClearLast);
		AddFunction("SetFlash",CmdSetFlash);
		AddFunction("SetCursorCallback",CmdSetCursorCallback);
		AddFunction("SetCursorDial",CmdSetCursorDial);
		AddFunction("WaitCallbackVisible",CmdWaitCallbackVisible);
		AddFunction("WaitDialVisible",CmdWaitDialVisible);
		AddFunction("SetTextBoxed",CmdSetTextBoxed);
		AddFunction("MoveCursor",CmdMoveCursor);
		AddFunction("SetTextOrientation",CmdSetTextOrientation);
		AddFunction("SetViewCallback",CmdSetViewCallback);
		AddFunction("SetViewDial",CmdSetViewDial);
		AddFunction("SetPanTilt",CmdSetPanTilt);
		AddFunction("MovePanTilt",CmdMovePanTilt);
		AddFunction("SetCursor3D",CmdSetCursor3D);
}

int 
TrainingScriptClass::FindFunction(char *funcname)
{
	for (int i=0;i<numfunctions;i++)
		if (!strcmp(functiontable[i].funcname,funcname))
			return i;
	return -1;
}


void 
TrainingScriptClass::ClearAllLines()
{
	LineType *linewalker;
	LineType *nextlinewalker;

	linewalker=startline;

	while (linewalker)
	{
		nextlinewalker=linewalker->next;
		delete linewalker;
		linewalker=nextlinewalker;
	}
	numlines=0;
	startline=endline=lastline=nextline=NULL;
}


LineType::LineType()
{
	function=-1;
	startarg=NULL;
	endarg=NULL;
	numargs=0;
	next=NULL;
	previous=NULL;

	
	localtimer=0;
	localcolor=0xff0000ff;
	localcursorx=0;
	localcursory=0;
	localfont=2;
	localsoundid=SND_NO_HANDLE;
	localflash = 0;
	localtextboxed = 0;
	localtextorientation = 0;

}

LineType::~LineType()
{
	ArgType *argwalker;
	ArgType *nextargwalker;

	argwalker=startarg;

	while (argwalker)
	{
		nextargwalker=argwalker->next;
		delete argwalker;
		argwalker=nextargwalker;
	}
	numargs=0;	
}


ArgType::ArgType()
{
	arg=NULL;
	next=NULL;
	arglength=NULL;
}

ArgType::~ArgType()
{
	delete [] arg;			//changed from free
}



void 
TrainingScriptClass::AddRepeatList(LineType *line)
{
	RepeatListType *temp;

	temp = new RepeatListType;
	temp->line=line;
	temp->next=repeatlist;
	repeatlist=temp;
	temp->remove=false;
}

void 
TrainingScriptClass::ClearRepeatList()
{
	RepeatListType *walker;
	RepeatListType *nextwalker;

	walker=repeatlist;

	while (walker)
	{
		nextwalker=walker->next;
		delete walker;
		walker=nextwalker;
	}	
	repeatlist = NULL;
}

bool 
TrainingScriptClass::DelRepeatList(LineType *line)
{
	RepeatListType *walker = repeatlist;

	while (walker)
	{
		if (walker->line==line)	
			walker->remove=true;		
		walker=walker->next;			

	}	
	return true;

}

bool 
TrainingScriptClass::DelAllRepeatList()
{
	RepeatListType *walker = repeatlist;

	while (walker)
	{		
		walker->remove=true;		
		walker=walker->next;			

	}	
	return true;

}

void
TrainingScriptClass::CleanupRepeatList()
{
	RepeatListType *walker = repeatlist;
	RepeatListType *prevwalker =NULL ;	

	while (walker)
	{
		if (walker->remove)
		{
			if (prevwalker)
				prevwalker->next=walker->next;
			else 
				repeatlist=walker->next;
			prevwalker=walker;
			walker=walker->next;	
			delete prevwalker;	
			prevwalker=NULL;
		}
		else
		{
			prevwalker=walker;
			walker=walker->next;	
		}

	}	
			 

}


void 
TrainingScriptClass::RunRepeatList()
{
	RepeatListType *walker = repeatlist;

	while (walker)
	{
		inrepeatlist=true;
		RunLine(walker->line);
		inrepeatlist=false;
		walker=walker->next;
	}		
}

bool
TrainingScriptClass::CaptureCommand(InputFunctionType theFunc, int callback)
{	
	CaptureListType *temp;

	if (theFunc||callback)
	{

		temp = new CaptureListType;
		temp->theFunc=theFunc;
		temp->callback=callback;
		temp->next=capturelist;
		capturelist=temp;		
		return true;
	}
	return false;
}

bool
TrainingScriptClass::IsCaptured(InputFunctionType theFunc, int callback)
{	
	CaptureListType *walker;
	
	
	walker = capturelist;

	while(walker)
	{
		if (theFunc && walker->theFunc==theFunc)
			return true;
		if (callback && walker->callback==callback)
			return true;
		walker=walker->next;

	}
	return false;
}

void 
TrainingScriptClass::ClearCaptureList()
{
	CaptureListType *walker=capturelist;
	CaptureListType *nextwalker=NULL;
	
	while (walker)
	{		
		nextwalker=walker->next;
		delete walker;
		walker=nextwalker;
	}

	capturelist = NULL;
}
		
bool
TrainingScriptClass::AddBlockAllowCommand(InputFunctionType theFunc, int callback)		//Can be used to either allow or disallow a command based on the type of list it is
{	
	CaptureListType *temp;

	if (theFunc||callback)
	{
		temp = new CaptureListType;
		temp->theFunc=theFunc;
		temp->callback=callback;
		temp->next=blockallowlist;
		blockallowlist=temp;		
		return true;
	}
	return false;
}

//Check if the command is blocked or allowed
bool
TrainingScriptClass::IsBlocked(InputFunctionType theFunc, int callback)
{	
	CaptureListType *walker;
	
	walker = blockallowlist;	

	if (isblocklist)				//is this a blocklist or an allow list
	{
		while(walker)				//This is a blocklist	
		{
			if (theFunc && walker->theFunc==theFunc)
				return true;
			if (callback && walker->callback==callback)
				return true;
			walker=walker->next;
		}
		return false;
	}
	else
	{
		while(walker)					//This is an allow list
		{
			if (theFunc && walker->theFunc==theFunc)
				return false;
			if (callback && walker->callback==callback)
				return false;
			walker=walker->next;
		}
		return true;

	}	
}

void 
TrainingScriptClass::ClearBlockAllowList()
{
	CaptureListType *walker=blockallowlist;
	CaptureListType *nextwalker=NULL;
	
	while (walker)
	{		
		nextwalker=walker->next;
		delete walker;
		walker=nextwalker;
	}

	blockallowlist = NULL;
}

CaptureListType::CaptureListType()
{
	theFunc=NULL;
	callback=0;
	next=NULL;
}


bool
TrainingScriptClass::LoadScript(char *name)
{    
    FILE*			ScriptFile;
	char			templine[MAX_LINE_LENGTH];
	int				i = 0;

	ClearAllLines();
	RestartScript();
	
    ScriptFile = fopen(name,"r");
  
    F4Assert(ScriptFile);			//Error: Couldn't open file
	
	if (ScriptFile) {
		while (!feof(ScriptFile)) 
		{			
			fgets(templine, MAX_LINE_LENGTH, ScriptFile);
			AddLine(templine);
		}

		fclose(ScriptFile);
		nextline=startline;
		return true;
	}
	return false;
}



