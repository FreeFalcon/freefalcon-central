/******************************************************************************/
/*                                                                            */
/*  Unit Name : timer.cpp                                                     */
/*                                                                            */
/*  Abstract  : Source file for functions implementing the                    */
/*              SIMLIB_TIMER_CLASS.                                           */
/*                                                                            */
/*  Dependencies : Auto-Generated                                             */
/*                                                                            */
/*  Operating System : MS-DOS 6.2, Windows 3.1                                */
/*                                                                            */
/*  Compiler : MSVC V1.5                                                      */
/*                                                                            */
/*  Naming Conventions :                                                      */
/*                                                                            */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Private Functions   : Mixed case with no underscores                  */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Global Variables    : Mixed case with no underscores                  */
/*      Classless Functions : Mixed case with no underscores                  */
/*      Classes             : All upper case seperated by an underscore       */
/*      Defined Constants   : All upper case seperated by an underscore       */
/*      Macros              : All upper case seperated by an underscore       */
/*      Structs/Types       : All upper case seperated by an underscore       */
/*      Private Variables   : All lower case seperated by an underscore       */
/*      Public Variables    : All lower case seperated by an underscore       */
/*      Local Variables     : All lower case seperated by an underscore       */
/*                                                                            */
/*  Development History :                                                     */
/*  Date      Programer           Description                                 */
/*----------------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                               */
/*                                                                            */
/******************************************************************************/
#ifdef _WINDOWS
#include <windows.h>
#endif
#include "stdhdr.h"
#include "timer.h"
#include "error.h"

/*----------------------------------------------------*/
/* Memory Allocation for externals declared elsewhere */
/*----------------------------------------------------*/
SIM_FLOAT SimLibMinorFrameTime = 0.02F;
SIM_FLOAT SimLibMinorFrameRate = 50.0F;
SIM_FLOAT SimLibMajorFrameTime = 0.06F;
SIM_FLOAT SimLibMajorFrameRate = 16.667F;
SIM_FLOAT SimLibTimeOfDay;
SIM_ULONG SimLibElapsedTime;
SIM_UINT SimLibFrameCount = 0;
SIM_INT SimLibMinorPerMajor = 3;
SIMLIB_TIMER_CLASS Timer;


/*-------------------*/
/* Private Functions */
/*-------------------*/

/********************************************************************/
/*                                                                  */
/* Routine: SIM_TIMER_HANDLE SIMLIB_TIMER_CLASS::FindUnusedHandle (void)      */
/*                                                                  */
/* Description:                                                     */
/*    Finds the first available timer handle.                       */
/*                                                                  */
/* Inputs:                                                          */
/*    None                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    Handle of the first available timer or NULL if none           */
/*    available.                                                    */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_TIMER_HANDLE SIMLIB_TIMER_CLASS::FindUnusedHandle(void)
{
	int 
		i;

	i = 0;
	while (_timer[i].id)
	{
		i++;
	}

	if (i == SIM_MAX_TIMERS)
	{
		return NULL;
	}

	return IndexToHandle(i);
}

/*------------------*/
/* Public Functions */
/*------------------*/

/************************************************************************/
/*																								*/
/* Routine: TimerOnetimeEventCleanup(TimerInstaninstata *cleanupData)	*/
/*																								*/
/* Description:																			*/
/*		Cleans up the timer class upon execution of one-shot timer events.*/
/*		This must be called before exit from one-shot timer callbacks.    */
/*																								*/
/* Inputs:																					*/
/*    None																					*/
/*																								*/
/* Outputs:																					*/
/*    None																					*/
/*																								*/
/*  Development History :																*/
/*  Date      Programer           Description									*/
/*----------------------------------------------------------------------*/
/*  18-Aug-97 MPS                  Created										*/
/*																								*/
/************************************************************************/
void CountdownEventCleanup(SIMLIB_TIMER_INSTANCE_DATA *cleanupData)
{
	int 
		i;

	SIMLIB_TIMER_CLASS
		*pt;

	F4Assert(cleanupData != NULL);
	F4Assert(cleanupData->_hTimer >= 0 && cleanupData->_hTimer < SIM_MAX_TIMERS);
	F4Assert(cleanupData->_pTimer != NULL);

	pt = cleanupData->_pTimer;
	i = pt->HandleToIndex(cleanupData->_hTimer);

	pt->Cleanup(i);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIMLIB_TIMER_CLASS::SIMLIB_TIMER_CLASS (void)                               */
/*                                                                  */
/* Description:                                                     */
/*    Initializes the timer data structure and creates a window to  */
/*    receive all timer messages.                                   */
/*                                                                  */
/* Inputs:                                                          */
/*    None                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    None                                                          */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*  18-Aug-97 MPS                 Added support for NULL timer      */
/*                                   handles	                       */
/*                                                                  */
/********************************************************************/
SIMLIB_TIMER_CLASS::SIMLIB_TIMER_CLASS (void)
{
	int
		i;

   for (i=0; i<SIM_MAX_TIMERS; i++)
   {
		Cleanup(i);
   }
}

/********************************************************************/
/*                                                                  */
/* Routine: SIMLIB_TIMER_CLASS::~SIMLIB_TIMER_CLASS (void)                              */
/*                                                                  */
/* Description:                                                     */
/*    Timer class destructor.  Stops any running timers             */
/*                                                                  */
/* Inputs:                                                          */
/*    None                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    None                                                          */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*  18-Aug-97 MPS                 Added support for NULL timer      */
/*                                   handles	                       */
/*                                                                  */
/********************************************************************/
SIMLIB_TIMER_CLASS::~SIMLIB_TIMER_CLASS(void)
{
	int
		i;

   for (i=0; i<SIM_MAX_TIMERS; i++)
   {
		StopTimer(IndexToHandle(i));
   }
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_TIMER_HANDLE SIMLIB_TIMER_CLASS::StartTimer                   */
/*             (SIM_INT, SIM_INT)                                   */
/*                                                                  */
/* Description:                                                     */
/*    Defines a timer with period and resolution, then starts it    */
/*    running                                                       */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_INT period - Timer period in msec.                        */
/*    SIM_INT res    - Timer resolution in msec.                    */
/*                                                                  */
/* Outputs:                                                         */
/*    Handle of new timer if created, or NULL on failure            */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*  18-Aug-97 MPS                 Added support for NULL timer      */
/*                                   handles	                       */
/*                                                                  */
/********************************************************************/
//SIM_TIMER_HANDLE SIMLIB_TIMER_CLASS::StartTimer 
//(
//	SIM_INT period,
//	SIM_INT res,
//	LPTIMECALLBACK funcPtr,
//	DWORD userData
//)
SIM_TIMER_HANDLE SIMLIB_TIMER_CLASS::StartTimer 
(
	SIM_INT period,
	SIM_INT res,
	LPTIMECALLBACK funcPtr,
	DWORD userData
)
{
	SIM_TIMER_HANDLE
		hTimer;

	int
		i;

	hTimer = FindUnusedHandle();
   if (hTimer != NULL && period >= res)
   {
		i = HandleToIndex(hTimer);
		Cleanup(i);

		return hTimer;
	}
	else
	{
		return NULL;
	}
}

/********************************************************************/
/*                                                                  */
/* Routine:                                                         */
/*                                                                  */
/* Description:                                                     */
/*    None                                                          */
/*                                                                  */
/* Inputs:                                                          */
/*    None                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    None                                                          */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*  18-Aug-97 MPS                 Added support for NULL timer      */
/*                                   handles	                       */
/*                                                                  */
/********************************************************************/
SIM_INT SIMLIB_TIMER_CLASS::StopTimer (SIM_TIMER_HANDLE hTimer)
{
	SIM_INT
		rVal = 0;

	int
		i;

	if(hTimer != NULL)
	{
		i = HandleToIndex(hTimer);
		
		if (_timer[i].id != NULL)
		{
#ifdef _WINDOWS
	   	if (timeEndPeriod (_timer[i].resolution) == 0)
			{
	      	if (timeKillEvent (_timer[i].id) == 0)
				{
	         	rVal = SIMLIB_OK;
				}
				else
				{
					SimLibErrno = EACCESS;
					rVal = SIMLIB_ERR;
				}
			}
			else
			{
				SimLibErrno = EACCESS;
				rVal = SIMLIB_ERR;
			}
	#endif
		}
	   else
		{
			
			SimLibErrno = ENOTFOUND;
			rVal = SIMLIB_ERR;
		}
		
		Cleanup(i);
	}
	else
	{
		SimLibErrno = EACCESS;
		rVal = SIMLIB_ERR;
	}

	return (rVal);
}

/********************************************************************/
/*                                                                  */
/* Routine:                                                         */
/*                                                                  */
/* Description:                                                     */
/*    None                                                          */
/*                                                                  */
/* Inputs:                                                          */
/*    None                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    None                                                          */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*  18-Aug-97 MPS                 Added support for NULL timer      */
/*                                   handles	                       */
/*                                                                  */
/********************************************************************/
//SIM_TIMER_HANDLE SIMLIB_TIMER_CLASS::StartCountdown 
//(
//	SIM_INT delay,
//	SIM_INT res,
//	LPTIMECALLBACK funcPtr,
//	DWORD userData
//)
SIM_TIMER_HANDLE SIMLIB_TIMER_CLASS::StartCountdown 
(
	SIM_INT delay,
	SIM_INT res,
	LPTIMECALLBACK funcPtr,
	DWORD userData
)
{
	SIM_TIMER_HANDLE 
		hTimer;

	int
		i;

	hTimer = FindUnusedHandle();
   if (hTimer != NULL && delay > res)
   {
		i = HandleToIndex(hTimer);
		Cleanup(i);

#ifdef _WINDOWS
      if (timeBeginPeriod (res) == 0)
      {
			_timer[i].inst._hTimer = hTimer;
			_timer[i].inst._pTimer = this;
			_timer[i].inst._userData = userData;
			
      	_timer[i].id =
				timeSetEvent 
				(
					delay,
					res,
					funcPtr,
					(DWORD)&_timer[i].inst,
					TIME_ONESHOT
				);
        
         if (_timer[i].id == NULL)
         {
				Cleanup(i);
	         return NULL;
	      }
         else
         {
            _timer[i].period = delay;
            _timer[i].resolution = res;
            _timer[i].type = SIM_TIMER_ONESHOT;
				return hTimer;
         }
      }
      else
      {
         return NULL;
      }
#else
		return hTimer;
#endif
   }
   else
	{
		return NULL;
	}
}

/********************************************************************/
/*                                                                  */
/* Routine:                                                         */
/*                                                                  */
/* Description:                                                     */
/*    None                                                          */
/*                                                                  */
/* Inputs:                                                          */
/*    None                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    None                                                          */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_INT SIMLIB_TIMER_CLASS::Wait (SIM_INT delay)
{
    if (delay > 0)    
        return (SIMLIB_OK);
    else
        return (SIMLIB_ERR);
}

