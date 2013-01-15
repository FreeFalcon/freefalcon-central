#include "stdhdr.h"
#include "missile.h"
#include "simmath.h"

void MissileClass::Aerodynamics(void)
{
float abeta;         /* Absoloute value of beta */
float signa;         /* Sign of Alpha */
float signb;         /* Sign of beta */
float cx;            /* Raw axial force from table lookup */
float cy;            /* Raw side force from table lookup */
float cz;            /* Raw normal force from table lookup */

   abeta  = (float)fabs(beta);
   if (alpha < 0.0F)
      signa = -1.0F;
   else
      signa = 1.0F;
   if (beta < 0.0F)
      signb = -1.0F;
   else
      signb = 1.0F;

   // JPO -- CTD checks
   ShiAssert(FALSE == F4IsBadReadPtr(aeroData, sizeof *aeroData));
   ShiAssert(FALSE == F4IsBadReadPtr(ifd, sizeof *ifd));

	if (!aeroData || !ifd)
		return; // JB 010803

   /*--------------------------*/
   /* aero coefficient lookups */
   /*--------------------------*/
   cx = Math.TwodInterp (mach, alphat, aeroData->mach, 
                      aeroData->alpha, aeroData->cx, 
                      aeroData->numMach, aeroData->numAlpha,
                      &ifd->lastmach, &ifd->lastalpha);

   cy = Math.TwodInterp (mach, abeta, aeroData->mach, 
                      aeroData->alpha, aeroData->cz, 
                      aeroData->numMach, aeroData->numAlpha,
                      &ifd->lastmach, &ifd->lastalpha) * signb;

   cz = Math.TwodInterp (mach, alphat, aeroData->mach, 
                      aeroData->alpha, aeroData->cz, 
                      aeroData->numMach, aeroData->numAlpha,
                      &ifd->lastmach, &ifd->lastalpha) * signa;

   /*------------------*/
   /* body axis accels */
   /*------------------*/
   ifd->xaero = cx*ifd->qsom;
   ifd->yaero = cy*ifd->qsom;
   ifd->zaero = cz*ifd->qsom;

   /*-----------------------*/
   /* stability axis accels */
   /*-----------------------*/
   ifd->xsaero = ifd->xaero * ifd->geomData.cosalp + ifd->zaero * ifd->geomData.sinalp;
   ifd->ysaero = ifd->yaero;
   ifd->zsaero = ifd->zaero * ifd->geomData.cosalp - ifd->xaero * ifd->geomData.sinalp;

   /*------------------*/
   /* wind axis accels */
   /*------------------*/
   ifd->xwaero =  ifd->xsaero * ifd->geomData.cosbet + ifd->ysaero * ifd->geomData.sinbet;
   ifd->ywaero = -ifd->xsaero * ifd->geomData.sinbet + ifd->ysaero * ifd->geomData.cosbet;
   ifd->zwaero =  ifd->zsaero;
}
