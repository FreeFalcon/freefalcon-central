#include "stdhdr.h"
#include "missile.h"
#include "drawable.h"

void MissileClass::Flight(void)
{
   if (inputData->displayType != DisplayHTS && display)
   {
      display->DisplayExit();
      display = NULL;
   }
   Atmosphere();
   FlightControlSystem();
   Aerodynamics();
   Engine();
   Accelerometers();
   EquationsOfMotion();
}     
