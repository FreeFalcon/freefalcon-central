/***************************************************************************\
    Setup.h
    Scott Randolph
    April 2, 1997

    This is a one stop source for the terrain/weather/graphics system
	startup and shutdown sequences.  Just call these functions and you're
	set.
\***************************************************************************/
#ifndef _SETUP_H_
#define	_SETUP_H_

void DeviceIndependentGraphicsSetup( char *theater, char *objects, char* misctex );
void DeviceDependentGraphicsSetup( class DisplayDevice * );
void DeviceDependentGraphicsCleanup( class DisplayDevice * );
void DeviceIndependentGraphicsCleanup( void );

#endif // _SETUP_H_