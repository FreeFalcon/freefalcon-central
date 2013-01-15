#include "stdhdr.h"
#include "missile.h"

void MissileClass::FlightControlSystem(void)
{
   /*----------------------------*/
   /* gain schedules and filters */
   /*----------------------------*/
    Gains();

   /*--------------*/
   /* control laws */
   /*--------------*/
    Pitch();
    Yaw();
}
