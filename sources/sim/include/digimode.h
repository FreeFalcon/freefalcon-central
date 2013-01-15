#ifndef _DIGI_MODES_H
#define _DIGI_MODES_H

/*--------------------------------------------------------*/
/* Define Modes or States in decreasing order of priority */
/*                                                        */
/* DEFENSIVE_MODES is the number of the lowest priority   */
/*                defensive mode.                         */
/*                                                        */
/* LAST_VALID is the number of the lowest priority mode   */
/*            of any kind.                                */
/*--------------------------------------------------------*/

/*---------------*/
/* Sensor States */
/*---------------*/
#define NO_INFO            0
#define BEARING_ONLY       1
#define RANGE_AND_BEARING  2
#define EXACT_POSITION     3

/*-------------*/
/* Sensor ID's */
/*-------------*/
#define NO_ID              0
#define ID_IFF             1
#define EID                2
#define VID                3

#endif /* _DIGI_MODES_H */

