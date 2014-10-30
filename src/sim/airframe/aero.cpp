/******************************************************************************/
/*                                                                            */
/*  Unit Name : aero.cpp                                                      */
/*                                                                            */
/*  Abstract  : Finds aerodynamic forces                                      */
/*                                                                            */
/*  Dependencies : Auto-Generated                                             */
/*                                                                            */
/*  Operating System : MS-DOS 6.2,                                            */
/*                                                                            */
/*  Compiler : WATCOM C/C++ V10                                               */
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
#include "stdhdr.h"
#include "AirFrame.h"
#include "simbase.h"
#include "aircrft.h"
#include "limiters.h"
#include "Graphics/Include/tmap.h"
#include "otwdrive.h"
#include "dofsnswitches.h"

/********************************************************************/
/*                                                                  */
/* Routine: void  AirframeClass::Aerodynamics (void)               */
/*                                                                  */
/* Description:                                                     */
/*    Look up current aero forces based in alpha/beta and convert   */
/*    to G loads in the three A/C axes                              */
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
extern bool g_bNewFm;
extern bool g_bTurb;
extern int gameCompressionRatio;//TJL 05/30/04

void AirframeClass::Aerodynamics(void)
{
    float dragAlpha;
    float tempAlpha;//, flapFactor = 0.0F;
    float lift, drag, cdStores;
    float cl1, cl2;
    float cd1, cd2;
    int i;
    Limiter *limiter;

    ShiAssert(aeroData);

    if ( not aeroData)
        return;

    // make the tef's and lef's useful

    if (auxaeroData->hasTef)
    {
        if (platform->IsComplex())
        {
            if (auxaeroData->hasFlapperons)
                tefFactor = platform->GetDOFValue(COMP_LT_FLAP) + platform->GetDOFValue(COMP_RT_FLAP);
            else
                tefFactor = platform->GetDOFValue(COMP_LT_TEF) + platform->GetDOFValue(COMP_RT_TEF);
        }
        else
        {
            if (auxaeroData->hasFlapperons)
                tefFactor = platform->GetDOFValue(SIMP_LT_AILERON) + platform->GetDOFValue(SIMP_RT_AILERON);
            else
                tefFactor = platform->GetDOFValue(SIMP_LT_TEF) + platform->GetDOFValue(SIMP_RT_TEF);
        }

        if (auxaeroData->flap2Nozzle)
        {
            nozzlePos = tefFactor / 2.0f; // save the nozzle position
        }

        tefFactor /= (DTR * auxaeroData->tefMaxAngle * 2);
#if 0

        if (g_bNewFm)
            tefFactor = (((AircraftClass *)platform)->GetDOFValue(2) + ((AircraftClass *)platform)->GetDOFValue(3)) * RTD / 20.0F;
        else
            tefFactor = (((AircraftClass *)platform)->GetDOFValue(2) + ((AircraftClass *)platform)->GetDOFValue(3)) * RTD / 10.0F;

#endif
    }
    else
    {
        tefFactor = 0; // fix this up oneday
    }


    if (auxaeroData->hasLef)
    {
        if (platform->IsComplex())
            lefFactor = platform->GetDOFValue(COMP_RT_LEF) + platform->GetDOFValue(COMP_LT_LEF);
        else
            lefFactor = platform->GetDOFValue(SIMP_RT_LEF) + platform->GetDOFValue(SIMP_LT_LEF);

        lefFactor /= (2 * DTR * auxaeroData->lefMaxAngle);
#if 0
        lefFactor = ((AircraftClass *)platform)->GetDOFValue(9) * RTD / 10.0f;
#endif

    }
    else lefFactor = 0;

    // JPO - I think the original is correct... after thinking about it a bit.
#if 0

    if (g_bNewFm)
        tempAlpha = alpha + tefFactor + lefFactor;//me123 changed - lef to + lef
    else
#endif
        tempAlpha = alpha + tefFactor - lefFactor;

    //TJL 03/13/04 Turb ideas 05/30/04 Disable turb with time compression
    float addTurb = 0.0f;
    float turb = 0.0f;

    if (g_bTurb and (gameCompressionRatio == 1))
    {
        addTurb = Turbulence(turb);
    }




    /*-----------------------*/
    /* get aero coefficients */
    /*-----------------------*/
    cl = Math.TwodInterp(mach, tempAlpha, aeroData->mach, aeroData->alpha,
                         aeroData->clift, aeroData->numMach,
                         aeroData->numAlpha, &curMachBreak, &curAlphaBreak) *
         aeroData->clFactor;
    //TJL 03/13/04 Turb ideas
    cl = cl + addTurb;

    cl *= (1 + tefFactor * auxaeroData->CLtefFactor);


    cy = Math.TwodInterp(mach, alpha, aeroData->mach, aeroData->alpha,
                         aeroData->cy, aeroData->numMach,
                         aeroData->numAlpha, &curMachBreak, &curAlphaBreak) *
         aeroData->cyFactor;

    // If simplified, use a lower alpha to find drag. This results in
    // less speed bleed in the corners
    if (IsSet(Simplified))
    {
        if (fabs(beta) * 0.6F > tempAlpha)
            dragAlpha = 0.75F * (float)fabs(beta) * 0.6F;
        else
            dragAlpha = 0.75F * tempAlpha;
    }
    else
    {
        if (fabs(beta) * 0.6F > tempAlpha)
            dragAlpha = (float)fabs(beta) * 0.6F;
        else
            dragAlpha = tempAlpha;
    }

    cd = Math.TwodInterp(mach, dragAlpha, aeroData->mach, aeroData->alpha,
                         aeroData->cdrag, aeroData->numMach,
                         aeroData->numAlpha, &curMachBreak, &curAlphaBreak) *
         aeroData->cdFactor;

    //TJL 03/13/04 Turb ideas
    cd = cd + (addTurb * 0.5f);

    cd *= (1 + tefFactor * auxaeroData->CDtefFactor + lefFactor * auxaeroData->CDlefFactor);


    if (dragChute == DRAGC_DEPLOYED)
        cd += auxaeroData->dragChuteCd;

    /*------------------------*/
    /* Local lift curve slope */
    /*------------------------*/
    i = 0;
    cl1 = Math.TwodInterp(mach, tempAlpha - 2.0F, aeroData->mach, aeroData->alpha,
                          aeroData->clift, aeroData->numMach,
                          aeroData->numAlpha, &curMachBreak, &i);

    cd1 = Math.TwodInterp(mach, dragAlpha - 2.0F, aeroData->mach, aeroData->alpha,
                          aeroData->cdrag, aeroData->numMach,
                          aeroData->numAlpha, &curMachBreak, &i);

    cl2 = Math.TwodInterp(mach, tempAlpha + 2.0F, aeroData->mach, aeroData->alpha,
                          aeroData->clift, aeroData->numMach,
                          aeroData->numAlpha, &curMachBreak, &i);

    cd2 = Math.TwodInterp(mach, dragAlpha + 2.0F, aeroData->mach, aeroData->alpha,
                          aeroData->cdrag, aeroData->numMach,
                          aeroData->numAlpha, &curMachBreak, &i);

    if (cl2 - cl1 not_eq 0.0F)
    {
        clalpha = (cl2 - cl1) * 0.25F  * (1 + tefFactor * auxaeroData->CLtefFactor);
        cnalpha = ((cl2 - cl1) * platform->platformAngles.cosalp +
                   (cd2 - cd1) * platform->platformAngles.sinalp) * 0.25F * (1 + tefFactor * auxaeroData->CDtefFactor);
    }

    F4Assert( not _isnan(cnalpha));

    //F4Assert ( not IsSet(Trimming) and cnalpha not_eq 0.0F);
    //F4Assert ( not IsSet(Trimming) and clalpha not_eq 0.0F);

    /*------------------*/
    /* lift curve slope */
    /*------------------*/
    cl1 = Math.TwodInterp(mach, 0.0F, aeroData->mach, aeroData->alpha,
                          aeroData->clift, aeroData->numMach,
                          aeroData->numAlpha, &curMachBreak, &i);

    cl2 = Math.TwodInterp(mach, 10.0F, aeroData->mach, aeroData->alpha,
                          aeroData->clift, aeroData->numMach,
                          aeroData->numAlpha, &curMachBreak, &i);

    clalph = (cl2 - cl1) * 0.1F  * (1 + tefFactor * auxaeroData->CLtefFactor);
    clalph0 = clalph;
    clift0  = cl1  * (1 + tefFactor * auxaeroData->CLtefFactor);

    /*---------------*/
    /* lift and drag */
    /*---------------*/
    cdStores = 0;
    limiter = gLimiterMgr->GetLimiter(StoresDrag, vehicleIndex);

    if (limiter)
        cdStores = limiter->Limit(mach) * dragIndex;


    //Ground Effect
    BIG_SCALAR pz = platform->ZPos();

    if ( not IsSet(IsDigital) and pz > -groundZ - 200.0F)
    {
        float span, factor;

        span = GetAeroData(AeroDataSet::Span); // 0.1066 correct for F-16, close enough for everyone else

        if (fabs(groundZ - pz) < span * 0.2F)
        {
            cl *= 1.13F;
            clalph0 = clalph *= 1.13F;
            clift0 *= 1.13F;
            clalpha *= 1.13F;
            cnalpha *= 1.13F;
        }
        else if (fabs(groundZ - pz) < span)
        {
            factor = (1.13F - ((float)fabs(groundZ - pz - span * 0.2F) / (span * 0.8F)) * 0.13F);

            cl *= factor;
            clalph0 = clalph *= factor;
            clift0 *= factor;
            clalpha *= factor;
            cnalpha *= factor;
        }
    }

    //TJL 09/05/04 Stall Model
    //Equation to determine stall: VI = 17.16 * SQRT ((W/S)/CL)
    if (auxaeroData->criticalAOA > 0.0f and IsSet(InAir) and platform->IsPlayer())
    {
        float stallSpeed = 0.0f;

        if (alpha > 10.0f)
        {
            stallSpeed = 17.16f * sqrt((weight / area) / fabsf(cl));
        }
        else
        {
            stallSpeed = 0.0f;
        }

        //Stall Horn
        if ((vcas - stallSpeed) < 3.0f or (auxaeroData->criticalAOA - alpha) < 3.0f)
        {
            SetFlag(LowSpdHorn);
            platform->SoundPos.Sfx(auxaeroData->sndLowSpeed);
        }
        else
        {
            ClearFlag(LowSpdHorn);
        }

        if (stallMode == FlatSpin or vt == 0.0f)
        {
            lift = 0.0f;
        }

        else if (vcas < stallSpeed or alpha > auxaeroData->criticalAOA)
        {
            float pscmd = 0.0f;
            lift = min(0.0f, cl * 0.5f) * (vcas / stallSpeed);

            if (platform->platformAngles.sinphi > 0.0F)
            {
                pscmd = platform->platformAngles.sinphi * 80.0F * DTR;
            }
            else
            {
                pscmd = platform->platformAngles.sinphi * 80.0F * DTR;
            }

            RollIt(pscmd, SimLibMinorFrameTime);
        }
        else
        {
            lift = cl * qsom;
        }
    }
    //MPS Old stuff
    else
    {
        if (stallMode == FlatSpin or vt == 0.0F)
        {
            lift = 0.0F;
        }
        else
        {
            lift  =  cl * qsom;
        }
    }


    cd += auxaeroData->CDSPDBFactor * dbrake +
          auxaeroData->CDLDGFactor * gearPos +
          cdStores;
    drag  = cd * qsom;

    /*------------------*/
    /* body axis accels */
    /*------------------*/
    xaero = -drag * platform->platformAngles.cosalp +
            lift * platform->platformAngles.sinalp;
    yaero =  cy * qsom * (beta - (float)fabs(beta) * yshape * 0.5F);
    zaero = -lift * platform->platformAngles.cosalp -
            drag * platform->platformAngles.sinalp;

    ShiAssert( not _isnan(platform->platformAngles.sinalp));
    ShiAssert( not _isnan(platform->platformAngles.cosalp));
    ShiAssert( not _isnan(xaero));
    ShiAssert( not _isnan(zaero));

    /*-----------------------*/
    /* stability axis accels */
    /*-----------------------*/
    xsaero = -drag;
    ysaero = yaero;
    zsaero = -lift;

    /*------------------*/
    /* wind axis accels */
    /*------------------*/
    xwaero =  xsaero * platform->platformAngles.cosbet +
              ysaero * platform->platformAngles.sinbet;
    ywaero = -xsaero * platform->platformAngles.sinbet +
             ysaero * platform->platformAngles.cosbet;
    zwaero =  zsaero;
}
