// Turbulence Code
// Version: Beta 1
// Author: TJL
// Initial Create: 03/13/04
// Revision Date:
//
#include "stdhdr.h"
#include "AirFrame.h"
#include "simbase.h"
#include "aircrft.h"
#include "limiters.h"
#include "Graphics/Include/tmap.h"
#include "otwdrive.h"
#include "dofsnswitches.h"
#include "fakerand.h"
#include "drawable.h"
#include "Graphics/Include/Display.h"
#include "cmpglobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "TerrTex.h"
#include "hud.h"
#include "cmpclass.h"
#include "weather.h"

extern int TimeOfDayGeneral();
float turbPower;



/* Function is called from aero.cpp */

float AirframeClass::Turbulence(float turb)
{

    DWORD time;
    int groundType;
    int weather = 0;
    int cloudTurb = 0;
    float wind = 0.0f;
    float temp = 0.0f;
    float dt;
    float windHeading = 0.0f;
    float alt = 0.0f;
    float roll = 0.0f;

    if ( not TheHud)
        return 0.0f;
    else
        alt = TheHud->hat;

    dt = (vuxGameTime - platform->LastUpdateTime()) / 1000.0F;

    //Determine Time of Day
    time = TheCampaign.GetMinutesSinceMidnight();

    //Determine Weather Condition
    weather = realWeather->weatherCondition; //Sunny (1), Fair, Poor, Inclement
    wind = realWeather->windSpeed;
    temp = ((WeatherClass*)realWeather)->temperature;
    windHeading = realWeather->windHeading;

    // Here we determine the surface type and assign a turb score
    groundType = OTWDriver.GetGroundType(x, y);

    //Display
    //MonoPrint("Ground Type %d\n",groundType);

    /*
    COVERAGE_NODATA = 0;
    COVERAGE_WATER = 1;
    COVERAGE_RIVER = 2;
    COVERAGE_SWAMP = 3;
    COVERAGE_PLAINS = 4;
    COVERAGE_BRUSH = 5;
    COVERAGE_THINFOREST = 6;
    COVERAGE_THICKFOREST = 7;
    COVERAGE_ROCKY = 8;
    COVERAGE_URBAN = 9;
    COVERAGE_ROAD = 10;
    COVERAGE_RAIL = 11;
    COVERAGE_BRIDGE = 12;
    COVERAGE_RUNWAY = 13;
    COVERAGE_STATION = 14;
    COVERAGE_OBJECT = 15; // JB carrier

    */

    //REWRITE
    //Determine Weather

    //Sunny: mech, heat convection, calm twilight and evening
    if (weather == 1)
    {
        if (time > 540 and time < 990)//0900 - 1630
        {
            if ((groundType == COVERAGE_WATER or groundType == COVERAGE_RIVER or
                 groundType == COVERAGE_SWAMP) and (alt > -5000.0f))
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 5) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    if (time > 540 and time < 720)//9-12
                    {
                        turb = PRANDFloat() * ((rand() % 10) * 0.05f);
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                    else
                    {
                        turb = PRANDFloat() * ((rand() % 20) * 0.05f);
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                }
                else
                {
                    turb = 0.0f;
                }
            }

            else if (groundType == COVERAGE_URBAN and (alt > -5000.0f and alt < -300.0f))
            {
                if (alt > -1500.0f and alt < -300.0f)
                {
                    if (turbOn == 0)
                    {
                        turbOn = ((rand() % 8) * 1000);
                    }

                    if ((SimLibElapsedTime - turbTimer) >= turbOn)
                    {
                        if (time > 540 and time < 720)//9-12
                        {
                            turb = PRANDFloat() * ((rand() % 60) * 0.05f); //3.0 Max
                            turbTimer = SimLibElapsedTime;
                            turbOn = 0;
                        }
                        else
                        {
                            turb = PRANDFloat() * ((rand() % 100) * 0.05f); //5.0 Max
                            turbTimer = SimLibElapsedTime;
                            turbOn = 0;
                        }
                    }
                }
                else if (alt <= -1500.0f and alt > -5000.0f)
                {
                    if (turbOn == 0)
                    {
                        turbOn = ((rand() % 8) * 1000);
                    }

                    if ((SimLibElapsedTime - turbTimer) >= turbOn)
                    {
                        turb = PRANDFloat() * ((rand() % 50) * 0.05f); //2.5 Max
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                }

                else
                {
                    turb = 0.0f;
                }
            }
            //all other terrain
            else if (alt > -1500.0f and alt < -300.0f)
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 3) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    if ((rand() % 10) == 1) //Random big bump
                    {
                        turb = (PRANDFloat() * 3.0f);//3.0 MAX
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                    else
                    {
                        turb = PRANDFloat() * ((rand() % 20) * 0.05f); //0.5 MAX
                        //turb = PRANDFloat() * turbPower;
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                }
            }
            else if (alt <= -1500.0f and alt > -5000.0f)
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 8) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    turb = PRANDFloat() * ((rand() % 6) * 0.05f); //0.3 max
                    turbTimer = SimLibElapsedTime;
                    turbOn = 0;
                }
            }

            else if (alt > -300.0f and alt <= -10.0f)//Mechanical turb
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 3) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    if (wind >= 8.0f)
                    {
                        turb = (PRANDFloat() * ((rand() % 100) * (wind * 0.001f))); //3.0 MAX
                        //roll = PRANDFloat() * (rand()%100*0.05);
                        roll = turb * 1.5f;
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                    else
                    {
                        turb = PRANDFloat() * ((rand() % 16) * 0.05f); //0.8 MAX
                        //roll = PRANDFloat() * (rand()%100*0.05);
                        roll = turb;
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                }
            }
            //Tropopause boudary area between Troposphere and Stratosphere
            //strong winds, moderate to severe turbulence
            else if (platform->ZPos() <= -35250.0f and platform->ZPos() >= -36000.0f)
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 2) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    turb = PRANDFloat() * ((rand() % 40) * 0.05f); //2 max
                    turbTimer = SimLibElapsedTime;
                    turbOn = 0;
                }
            }

        }
        else //general atmospherics 16:31 - 08:59
        {
            if (alt > -300.0f and alt <= -10.0f)//Mechanical turb
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 3) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    if (wind >= 8.0f) //Random big bump
                    {
                        turb = (PRANDFloat() * ((rand() % 100) * (wind * 0.001f))); //3.0 MAX
                        //roll = PRANDFloat() * (rand()%100*0.05);
                        roll = turb;
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                    else
                    {
                        turb = PRANDFloat() * ((rand() % 16) * 0.05f); //0.8 MAX
                        //roll = PRANDFloat() * (rand()%100*0.05);
                        roll = turb;
                        //turb = PRANDFloat() * turbPower;
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                }
            }


            else if (alt > -1500.0f and alt < -300.0f)
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 5) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    if ((rand() % 30) == 1) //Random bump
                    {
                        turb = (PRANDFloat() * 1.0f);//1.0 MAX
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                    else
                    {
                        turb = PRANDFloat() * ((rand() % 20) * 0.05f); //0.5 MAX
                        //turb = PRANDFloat() * turbPower;
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                }
            }
            else if (alt <= -1500.0f and alt > -5000.0f)
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 8) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    turb = PRANDFloat() * ((rand() % 6) * 0.05f); //0.3 max
                    turbTimer = SimLibElapsedTime;
                    turbOn = 0;
                }
            }

            //Tropopause boudary area between Troposphere and Stratosphere
            //strong winds, moderate to severe turbulence
            else if (platform->ZPos() <= -35250.0f and platform->ZPos() >= -36000.0f)
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 2) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    turb = PRANDFloat() * ((rand() % 40) * 0.05f); //2 max
                    turbTimer = SimLibElapsedTime;
                    turbOn = 0;
                }
            }

            else
            {
                turb = 0.0f;
            }


        }

    }//End Sunny

    //Fair: mech, heat convection, cloud convection
    else if (weather == 2)
    {
        //Clouds
        float cloudRadius, cloudDist, xx, yy, zz;
        // cloudRadius = 4000.0F * 2.0f;
        cloudRadius = 4000.0F * 2.3f * 2.0f; // Cobra - FRB - need larger radius due to z-fighting fix
        int drawCell = DRAWABLECELL;
        int numCells = NUMCELLS;

        //Are we near a cloud?
        if (cloudTurb == 0)
        {
            for (int row = drawCell; row < numCells - 2; row++)
            {
                for (int col = drawCell; col < numCells - 2; col++)
                {

                    xx = (realWeather->weatherCellArray[row][col].cloudPosX + realWeather->weatherShiftX) - x;

                    yy = (realWeather->weatherCellArray[row][col].cloudPosY + realWeather->weatherShiftY) - y;

                    zz = ((WeatherClass*)realWeather)->cumulusZ - z;
                    cloudDist = sqrt(xx * xx + yy * yy);

                    if (cloudDist < cloudRadius)
                    {
                        cloudTurb = 1;
                        break;
                    }

                    cloudTurb = 0;
                }

                if (cloudTurb == 1)
                    break;

            }
        }

        //Turb actions
        if (cloudTurb == 1 and (zz > -3500.0f and zz < 2500.0f))
        {
            turb = PRANDFloat() * ((rand() % 20) * 0.05f);
            cloudTurb = 0;
        }
        else if (time > 540 and time < 990)//0900 - 1630
        {
            if ((groundType == COVERAGE_WATER or groundType == COVERAGE_RIVER or
                 groundType == COVERAGE_SWAMP) and (alt > -5000.0f))
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 5) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    if (time > 540 and time < 720)//9-12
                    {
                        turb = PRANDFloat() * ((rand() % 10) * 0.05f);
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                    else
                    {
                        turb = PRANDFloat() * ((rand() % 20) * 0.05f);
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                }
                else
                {
                    turb = 0.0f;
                }
            }

            else if (groundType == COVERAGE_URBAN and (alt > -5000.0f and alt < -300.0f))
            {
                if (alt > -1500.0f and alt < -300.0f)
                {
                    if (turbOn == 0)
                    {
                        turbOn = ((rand() % 8) * 1000);
                    }

                    if ((SimLibElapsedTime - turbTimer) >= turbOn)
                    {
                        if (time > 540 and time < 720)//9-12
                        {
                            turb = PRANDFloat() * ((rand() % 60) * 0.05f); //3.0 Max
                            turbTimer = SimLibElapsedTime;
                            turbOn = 0;
                        }
                        else
                        {
                            turb = PRANDFloat() * ((rand() % 100) * 0.05f); //5.0 Max
                            turbTimer = SimLibElapsedTime;
                            turbOn = 0;
                        }
                    }
                }
                else if (alt <= -1500.0f and alt > -5000.0f)
                {
                    if (turbOn == 0)
                    {
                        turbOn = ((rand() % 8) * 1000);
                    }

                    if ((SimLibElapsedTime - turbTimer) >= turbOn)
                    {
                        turb = PRANDFloat() * ((rand() % 50) * 0.05f); //2.5 Max
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                }

                else
                {
                    turb = 0.0f;
                }
            }
            //all other terrain
            else if (alt > -1500.0f and alt < -300.0f)
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 3) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    if ((rand() % 10) == 1) //Random big bump
                    {
                        turb = (PRANDFloat() * 3.0f);//3.0 MAX
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                    else
                    {
                        turb = PRANDFloat() * ((rand() % 20) * 0.05f); //0.5 MAX
                        //turb = PRANDFloat() * turbPower;
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                }
            }
            else if (alt <= -1500.0f and alt > -5000.0f)
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 8) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    turb = PRANDFloat() * ((rand() % 6) * 0.05f); //0.3 max
                    turbTimer = SimLibElapsedTime;
                    turbOn = 0;
                }
            }

            else if (alt > -300.0f and alt <= -10.0f)//Mechanical turb
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 3) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    if (wind >= 8.0f)
                    {
                        turb = (PRANDFloat() * ((rand() % 100) * (wind * 0.001f))); //3.0 MAX
                        //roll = PRANDFloat() * (rand()%100*0.05);
                        roll = turb * 1.5f;
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                    else
                    {
                        turb = PRANDFloat() * ((rand() % 16) * 0.05f); //0.8 MAX
                        //roll = PRANDFloat() * (rand()%100*0.05);
                        roll = turb;
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                }
            }
            //Tropopause boudary area between Troposphere and Stratosphere
            //strong winds, moderate to severe turbulence
            else if (platform->ZPos() <= -35250.0f and platform->ZPos() >= -36000.0f)
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 2) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    turb = PRANDFloat() * ((rand() % 40) * 0.05f); //2 max
                    turbTimer = SimLibElapsedTime;
                    turbOn = 0;
                }
            }

        }
        else //general atmospherics 16:31 - 08:59
        {
            if (alt > -300.0f and alt <= -10.0f)//Mechanical turb
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 3) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    if (wind >= 8.0f) //Random big bump
                    {
                        turb = (PRANDFloat() * ((rand() % 100) * (wind * 0.001f))); //3.0 MAX
                        //roll = PRANDFloat() * (rand()%100*0.05);
                        roll = turb;
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                    else
                    {
                        turb = PRANDFloat() * ((rand() % 16) * 0.05f); //0.8 MAX
                        //roll = PRANDFloat() * (rand()%100*0.05);
                        roll = turb;
                        //turb = PRANDFloat() * turbPower;
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                }
            }


            else if (alt > -1500.0f and alt < -300.0f)
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 5) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    if ((rand() % 10) == 1) //Random bump
                    {
                        turb = (PRANDFloat() * 1.0f);//1.0 MAX
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                    else
                    {
                        turb = PRANDFloat() * ((rand() % 20) * 0.05f); //0.5 MAX
                        //turb = PRANDFloat() * turbPower;
                        turbTimer = SimLibElapsedTime;
                        turbOn = 0;
                    }
                }
            }
            else if (alt <= -1500.0f and alt > -5000.0f)
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 8) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    turb = PRANDFloat() * ((rand() % 6) * 0.05f); //0.3 max
                    turbTimer = SimLibElapsedTime;
                    turbOn = 0;
                }
            }

            //Tropopause boudary area between Troposphere and Stratosphere
            //strong winds, moderate to severe turbulence
            else if (platform->ZPos() <= -35250.0f and platform->ZPos() >= -36000.0f)
            {
                if (turbOn == 0)
                {
                    turbOn = ((rand() % 2) * 1000);
                }

                if ((SimLibElapsedTime - turbTimer) >= turbOn)
                {
                    turb = PRANDFloat() * ((rand() % 40) * 0.05f); //2 max
                    turbTimer = SimLibElapsedTime;
                    turbOn = 0;
                }
            }

            else
            {
                turb = 0.0f;
            }


        }
    }//End Fair

    //Poor: mech, convection, shear
    /*Cobra: For now this applies to both poor and inclement
    Do below, in, and above weather.  Since this weather is theater wide and all day/night
    we won't do time checks.
    */
    else if (weather >= 3)
    {
        //General Atmospherics: mech, convection, shear
        //under
        if (platform->ZPos() > realWeather->stratusZ)
        {
            if (turbOn == 0)
            {
                turbOn = ((rand() % 8) * 1000);
            }

            if ((SimLibElapsedTime - turbTimer) >= turbOn)
            {
                if (weather == 3)//9-12
                {
                    turb = PRANDFloat() * ((rand() % 60) * 0.05f); //3.0 Max
                    turbTimer = SimLibElapsedTime;
                    turbOn = 0;
                }
                else
                {
                    turb = PRANDFloat() * ((rand() % 100) * 0.05f); //5.0 Max
                    turbTimer = SimLibElapsedTime;
                    turbOn = 0;
                }
            }

        }//end under

        //in
        else if (platform->ZPos() <= realWeather->stratusZ and 
                 platform->ZPos() > (realWeather->stratusZ - realWeather->stratusDepth))
        {
            turb = PRANDFloat() * ((rand() % 20) * 0.05f);

        }//End in


    }//End Poor

    /*else //Inclement
    {
     //General Atmospherics: mech, convection, shear (greater intensity)

    }//End Inclement*/




    //END REWRITE


    pstab += Math.FLTust(roll, 5.0f, dt, oldRoll1);
    //MonoPrint("TurbOn %d\n",turbOn);
    turb = Math.FLTust(turb, 1.0f, dt, oldTurb1);

    return turb;
}
