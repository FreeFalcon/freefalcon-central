/***************************************************************************\
	Statistics.h
    Scott Randolph
    June 18, 1998

	This module encapsulates some simple statistics gathering which can
	be called from TheSimLoop while the graphics are running.
\***************************************************************************/
#ifndef _STATISTICS_H_
#define _STATISTICS_H_

extern int	log_frame_rate;

void InitializeStatistics(void);
void CloseStatistics(void);
void WriteStatistics(void);
void PrintMemUsage (void);
void WriteMemUsage (void);

#endif // _STATISTICS_H_
