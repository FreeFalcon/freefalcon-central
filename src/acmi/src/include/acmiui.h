#ifndef _ACMIUI_H
#define _ACMIUI_H

#include "ui\include\userids.h"
 
// MLR 12/22/2003 - Defined, because the code had bug WRT these values no being updated everywhere.
// in seconds
// MB 
// test comment
#define ACMI_TRAILS_SHORT	15
#define ACMI_TRAILS_MEDIUM	30
#define ACMI_TRAILS_LONG	60
#define ACMI_TRAILS_MAX		120


void LoadACMIWindows();
void HookupControls(long ID);

#endif
