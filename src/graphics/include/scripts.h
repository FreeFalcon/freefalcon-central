/***************************************************************************\
    Scripts.h
    Scott Randolph
    April 4, 1998

    Provides custom code for use by specific BSPlib objects.
\***************************************************************************/
#ifndef _SCRIPTS_H_
#define _SCRIPTS_H_

typedef void (*ScriptFunctionPtr)(void);

extern ScriptFunctionPtr	ScriptArray[];
extern int					ScriptArrayLength;

#endif // _SCRIPTS_H_

