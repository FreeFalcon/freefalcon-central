#include "stdhdr.h"
#include "missile.h"

void MissileClass::Accelerometers(void)
{
	if (!ifd)
		return; // JB 010720

   /*--------------------------*/
   /* body axis accelerometers */
   /*--------------------------*/
   ifd->nxcgb =  (ifd->xaero + ifd->xprop)/GRAVITY;
   ifd->nycgb =  (ifd->yaero + ifd->yprop)/GRAVITY;
   ifd->nzcgb = -(ifd->zaero + ifd->zprop)/GRAVITY;

   /*-----------------------*/
   /* stability axis accels */
   /*-----------------------*/
   ifd->nxcgs =  (ifd->xsaero + ifd->xsprop)/GRAVITY;
   ifd->nycgs =  (ifd->ysaero + ifd->ysprop)/GRAVITY;
   ifd->nzcgs = -(ifd->zsaero + ifd->zsprop)/GRAVITY;

   /*------------------*/
   /* wind axis accels */
   /*------------------*/
   ifd->nxcgw =  (ifd->xwaero + ifd->xwprop)/GRAVITY;
   ifd->nycgw =  (ifd->ywaero + ifd->ywprop)/GRAVITY;
   ifd->nzcgw = -(ifd->zwaero + ifd->zwprop)/GRAVITY;
}
