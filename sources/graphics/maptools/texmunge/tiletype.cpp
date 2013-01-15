#include <stdio.h>
#include "texmunge.h"


#define COSTCODE	0
#define RIVER		15*16+2
#define ROAD		15*16+3
#define TOWN		15*16+4
#define BURBCODE	2
#define FARMCODE	3
#define FREECODE	4
#define BRIDGECODE	4*16+256*8
#define BRIDGECODE2	4*16+256*9
#define BRIDGECODE3	4*16+256*10
#define EARTCODE3	5
#define FORRCODE	2*16
#define FORROAD		(6*16)+1
#define FORRIVER	(7*16)
#define BHILROAD	(6*16)+2
#define HILLCODE	7
#define BARREN		8
#define HILLROAD	9
#define ORCHID		10
#define DRYFARM		11
#define RICEPAD		11+16
#define RICEPAD2	11+17
#define RIVERCODE	12
#define SIDECODE	13
#define INDUCODE	14
#define BARRHILL	15
#define CITYCODE	16
#define INDUALTCODE	17
#define BURBALTCODE	18
#define FIELCODE	19
#define FIE2CODE	20
#define NONE		999
#define EARTCODE	22
#define BARRCITY	30
#define AIRBASE		4*16
#define BARROAD		6*16
#define DRYRIVER	5*16
#define CANYON		7*16
#define OCEAN2		8*16
#define SWAMP		9*16


WORD DecodeCluster(PIXELCLUSTER c,int row,int col);
WORD BasicSet(WORD tileCode,PIXELCLUSTER c);
WORD BlockSet(int row,int col,int blockSize);
WORD RoadSet(WORD code,PIXELCLUSTER c, unsigned ends);


void NumberToName( WORD k, char *name )
{
	char num[20] = "";
	char landType[20] = "ERR!";

	if (k < 16)
		strcpy (landType, "COST");
	else if (k >= 256*8 && k <= 256*8+16)
		strcpy (landType, "COST");
	else if (k >= BRIDGECODE && k < BRIDGECODE+16)
		strcpy (landType, "BRIG");
	else if (k >= BRIDGECODE2 && k < BRIDGECODE2+16)
		strcpy (landType, "BRIG");
	else if (k >= BRIDGECODE3 && k < BRIDGECODE3+16)
		strcpy (landType, "BRIG");
	else if (k >= HILLCODE*16 && k < HILLCODE*16+16)
		strcpy (landType, "HILL");
	else if (k >= HILLROAD*16 && k < HILLROAD*16+16)
		strcpy (landType, "HROD");
	else if (k >= HILLCODE*16+2048 && k <= HILLCODE*16+2048+9)
		strcpy (landType, "HILL");
	else if (k >= BARRHILL*16 && k <= BARRHILL*16+16)
		strcpy (landType, "BHIL");
	else if (k >= BARRHILL*16+2048 && k <= BARRHILL*16+2048+16)
		strcpy (landType, "BHIL");
	else if (k >= (11+16)*16 && k <= (11+16)*16+16)
		strcpy (landType, "RICE");
	else if (k >= (11+16)*16+2048 && k <= (11+16)*16+2048+16)
		strcpy (landType, "RICE");
	else if (k >= (11+17)*16+2048 && k <= (11+17)*16+16+2048)
		strcpy (landType, "RICE");
	else if (k >= AIRBASE*16 && k <= AIRBASE*16+8)
		strcpy (landType, "BASE");
	else if (k >= RIVERCODE*16 && k < RIVERCODE*16+16)
		strcpy (landType, "RIVR");
	else if (k >= DRYRIVER*16 && k < DRYRIVER*16+16)
		strcpy (landType, "DRIV");
	else if (k >= (7*16)*16 && k < (7*16)*16+16)
		strcpy (landType, "CANY");
	else if (k >= BURBCODE*16 && k < BURBCODE*16+16)
		strcpy (landType, "BURB");
	else if (k >= INDUCODE*16 && k < INDUCODE*16+16)
		strcpy (landType, "INDU");
	else if (k >= INDUCODE*16+2048 && k < INDUCODE*16+2048+16)
		strcpy (landType, "INDU");
	else if (k >= BARRCITY*16 && k < BARRCITY*16+16)
		strcpy (landType, "BCIT");
	else if (k >= BARRCITY*16+2048 && k < BARRCITY*16+2048+16)
		strcpy (landType, "BCIT");
	else if (k >= FARMCODE*16 && k < FARMCODE*16+16)
		strcpy (landType, "FARM");
	else if (k >= BARREN*16 && k <= BARREN*16+9)
		strcpy (landType, "BARR");
	else if (k >= DRYFARM*16 && k <= DRYFARM*16+16)
		strcpy (landType, "DRYF");
	else if (k >= FORRCODE*16 && k < FORRCODE*16+16*5)
		strcpy (landType, "FORR");
	else if (k >= SWAMP*16 && k < SWAMP*16+16)
		strcpy (landType, "SWAM");
	else if (k >= FORRCODE*16+2048 && k <= FORRCODE*16+2048+16)
		strcpy (landType, "FORR");
	else if (k >= SIDECODE*16 && k < SIDECODE*16+16)
		strcpy (landType, "SIDE");
	else if (k >= ORCHID*16 && k < ORCHID*16+16)
		strcpy (landType, "EART");
	else if (k >= EARTCODE3*16 && k < EARTCODE3*16+16)
		strcpy (landType, "EART");
	else if (k >= FREECODE*16 && k < FREECODE*16+16)
		strcpy (landType, "FREE");
	else if (k >= (5*16)*16 && k < (5*16)*16+16)
		strcpy (landType, "DRIV");
	else if (k >= BARROAD*16 && k < BARROAD*16+16)
		strcpy (landType, "BROD");
	else if (k >= OCEAN2*16 && k < OCEAN2*16+16)
		strcpy (landType, "COST");
	else if (k >= (6*16+1)*16 && k < (6*16+1)*16+16)
		strcpy (landType, "FROD");
	else if (k >= (6*16+2)*16 && k < (6*16+2)*16+16)
		strcpy (landType, "BROD");
	else {
		char string[80];
		sprintf( string, "Unrecognized tile code 0x%X", k );
		MessageBox( NULL, string, "Error", MB_OK );
		strcpy (landType, "ERR-");
	}


	sprintf( num, "%03X", k );

	strcpy (name, "H");			//Just use high res tile here.
	strcat (name, landType);
	strcat (name, num);
//	strcat (name, ".bmp");
	strcat (name, ".pcx");
}


WORD DecodeCluster(PIXELCLUSTER c,int row,int col)
{
	WORD k = 0;
	unsigned bridge = 0, rivers, roads;

	
	//Coast
/*	if (c.p1 || c.p1A || c.p1B || c.p1C) k |= 1;
	if (c.p2 || c.p2A || c.p2B || c.p2C) k |= 2;
	if (c.p3 || c.p3A || c.p3B || c.p3C) k |= 4;
	if (c.p4 || c.p4A || c.p4B || c.p4C) k |= 8;
		
*/
	k = BasicSet(COSTCODE,c);
	if (k > 0)	
	{		// Check for bridges
		bridge = 0;
		if ((k == 3  && (c.p1 == FREECODE || c.p2 == FREECODE))
	  	 ||  (k == 10 && (c.p2 == FREECODE || c.p4 == FREECODE))
		 ||  (k == 12 && (c.p3 == FREECODE || c.p4 == FREECODE))
		 ||  (k == 5  && (c.p1 == FREECODE || c.p3 == FREECODE)))
			bridge == 1;
	
	 if 	(c.p1 == COSTCODE || c.p2 == COSTCODE || c.p3 == COSTCODE || c.p4 == COSTCODE ||
			 c.p1A == COSTCODE || c.p2A == COSTCODE || c.p3A == COSTCODE || c.p4A == COSTCODE ||
			 c.p1B == COSTCODE || c.p2B == COSTCODE || c.p3B == COSTCODE || c.p4B == COSTCODE ||
			 c.p1C == COSTCODE || c.p2C == COSTCODE || c.p3C == COSTCODE || c.p4C == COSTCODE)
		{if (bridge == 1)
			return k += BRIDGECODE;
		 else
			return k += COSTCODE*16;
		}
	 else
		{if (bridge == 1)
			return k += BRIDGECODE3;
		else
			return k += OCEAN2*16;
		}
	}

		// Check for Docks
//	if ((k == 3 || k == 5 || k == 10 || k == 12) && 
//	(c.p1 == INDUCODE || c.p2 == INDUCODE ||
//	 c.p3 == INDUCODE || c.p4 == INDUCODE))
//		return k += 256*8;

			// Check for River deltas
//	if ((k == 3 || k == 5 || k == 10 || k == 12) && 
//	(c.p1 == RIVERCODE || c.p2 == RIVERCODE ||
//	 c.p3 == RIVERCODE || c.p4 == RIVERCODE))
//		return k += 256*9;

//	if (k < 15)	//Got Coastal or Ocean tile.
//		return k;



	if (c.p1 == 0 && c.p2 == 0 && c.p3 == 0 && c.p4 == 0)
		{c.p1 = DRYFARM;
		c.p2 = DRYFARM;
		c.p3 = DRYFARM;
		c.p4 = DRYFARM;
		}

	k = 0;


	k = RoadSet(ROAD,c,0);
	if (k > 0 && (c.p1 == BARREN || c.p2 == BARREN ||
				  c.p3 == BARREN || c.p4 == BARREN))	//Got Road
		return k += BARROAD*16;

		//Dry rivers
	k = RoadSet(RIVER,c,0);
	if (k > 0 && (c.p1 == BARREN || c.p2 == BARREN ||
				  c.p3 == BARREN || c.p4 == BARREN))	//Got Dry River
		return k += (5*16)*16;

	if (c.p1 == BARRHILL+128 || c.p2 == BARRHILL+128 || c.p3 == BARRHILL+128 || c.p4 == BARRHILL+128)
	{
		k = BlockSet(row,col,3);
		return k += BARRHILL*16 + 256*8;
	}


	// 2X2 blocks
	if (c.p1 == BARRHILL+192 || c.p2 == BARRHILL+192 || c.p3 == BARRHILL+192 || c.p4 == BARRHILL+192)
	{
		k = BlockSet(row,col,2);
		return k += BARRHILL*16 + 256*8 + 9;
	}

	k = BasicSet(BARRHILL,c);
	if (k > 0)	
		return k += BARRHILL*16;
/*	{
		if (k == 15){
			k = BlockSet(row,col,3) + 256*8;
			return k += BARRHILL*16;
		}
		else
			return k += BARRHILL*16;
	} */





	/* Forrest/Hill section */
	k = BasicSet(FORRCODE,c);
	if (k > 0)	//Got Forrest
	{
		rivers = FORRCODE+2;
		roads = FORRCODE+3;
	

	    if ((k == 3 || k == 12 || k == 10 || k == 5) && 
			(c.p1 == RIVER || c.p2 == RIVER ||
			 c.p3 == RIVER || c.p4 == RIVER))
			return k += 4*16 + FORRCODE*16;
		else if ((k == 3 || k == 12 || k == 10 || k == 5) && 
			(c.p1 == ROAD || c.p2 == ROAD ||
			 c.p3 == ROAD || c.p4 == ROAD))
			return k += 4*16+1 + FORRCODE*16;
		else if (k < 15)	// Edge piece
			return k += FORRCODE*16;

		//Forrest Roads
		k = RoadSet(ROAD,c,1);
		if (k > 0 && (c.p1 == FORRCODE || c.p2 == FORRCODE ||
					  c.p3 == FORRCODE || c.p4 == FORRCODE))	//Got Forrest Road
			if ((k == 5 || k == 10 ) && 
				(c.p1 == RIVER || c.p2 == RIVER ||
				c.p3 == RIVER || c.p4 == RIVER))
				return FORRCODE*16 + 4*16 + k/5;
			else if ((k == 5 || k == 10 ) && 
				     (c.p1 == TOWN || c.p2 == TOWN ||
				      c.p3 == TOWN || c.p4 == TOWN))
				return FORRCODE*16 + 4*16 + 6 + k/5;
			else
				return k += roads*16;

		//Forrest rivers
		k = RoadSet(RIVER,c,0);
		if (k > 0 && (c.p1 == FORRCODE || c.p2 == FORRCODE ||
					  c.p3 == FORRCODE || c.p4 == FORRCODE))	//Got Forrest River
			return k += rivers*16;
	
		 k = BlockSet(row,col,2);	// We already know its a forrest block here...
		 return k += FORRCODE*16 + 16 + 9;
		    
	}	/* end Forrest Section */




	k = BasicSet(INDUCODE,c);
	if (k > 0)	
	{
		if (k == 15){
			k = BlockSet(row,col,2);
			return k += INDUCODE*16 + 256*8 + 9;
		}
		else
			return k += INDUCODE*16;
	}

	k = BasicSet(BARRCITY,c);
	if (k > 0)	
	{
		if (k == 15){
			k = BlockSet(row,col,3) + 256*8;
			return k += BARRCITY*16;
		}
		else
			return k += BARRCITY*16;
	}

		//Barren hill Roads
	k = RoadSet(ROAD,c,0);
	if (k > 0 && (c.p1 == BARRHILL || c.p2 == BARRHILL ||
				  c.p3 == BARRHILL || c.p4 == BARRHILL))	//Got Road
		return k += (ROAD*16);





#if 0
	k = BasicSet(MOUNTCODE,c);
	{
	if (k == 15){
		k = BlockSet(row,col,3) + 256*8;
		return k += MOUNTCODE*16;
	}
	else
		return k += MOUNTCODE*16;
}



#endif
	k = BasicSet(SIDECODE,c);
	if (k > 0)	//Got Forrest
	{
		if (k == 15){
			k = BlockSet(row,col,3) + 256*8;
			return k += HILLCODE*16;
		}
		else
			return k += SIDECODE*16;
	}




		//Base
	if (c.p1 >= AIRBASE && c.p1 <= AIRBASE+8)
		k = c.p1;
	else if (c.p2 >= AIRBASE && c.p2 <= AIRBASE+8)
		k = c.p2;
	else if (c.p3 >= AIRBASE && c.p3 <= AIRBASE+8)
		k = c.p3;
	else if (c.p4 >= AIRBASE && c.p4 <= AIRBASE+8)
		k = c.p4;
	if (k > 0)	//Got Base
		{
		k -= AIRBASE;
		return k + AIRBASE*16;
		}
	



	k = BasicSet(BURBCODE,c);
	if (k == 15)
		return k = BURBALTCODE*16+rand()%3;
	if (k > 0)	//Got BURB
		return k += BURBCODE*16;

	
	k = BasicSet(FARMCODE,c);
	if (k > 0)	//Got FARM
		return k += FARMCODE*16;



		// Rice Paddies
	k = BasicSet((11+16),c);
	if (k > 0){	
		if ((k == 15) && ((c.p1 == ((11+16)+128) || c.p2 == ((11+16)+128) || c.p3 == ((11+16)+128) || c.p4 == ((11+16)+128))))   
			k = BlockSet(row,col,3) + 256*8;
		return k += (11+16)*16;
		}



/*MAIN FARMLANDS */
	k = RoadSet(ROAD,c,0);
	if (k > 0 && (c.p1 == DRYFARM || c.p2 == DRYFARM ||
				  c.p3 == DRYFARM || c.p4 == DRYFARM))	//Got Road
					// Check for bridges
		if ((k == 5 || k == 10 ) && 
			(c.p1 == RIVER || c.p2 == RIVER ||
			 c.p3 == RIVER || c.p4 == RIVER))
			return k += BRIDGECODE2;
		else
			return k += FREECODE*16;


	k = RoadSet(RIVER,c,0);
	if (k > 0 && (c.p1 == DRYFARM || c.p2 == DRYFARM ||
				  c.p3 == DRYFARM || c.p4 == DRYFARM))	//Got River
		return k += RIVERCODE*16;


	if (c.p1 == DRYFARM || c.p2 == DRYFARM || c.p3 == DRYFARM || c.p4 == DRYFARM)
	{
		k = BlockSet(row,col,2);
		return k += DRYFARM*16+9;
	}
	
/*END MAIN FARMLANDS */


	if (c.p1 == BARREN || c.p2 == BARREN || c.p3 == BARREN || c.p4 == BARREN)
	{
		k = BlockSet(row,col,3);
		return k += BARREN*16;
	}


			// 2X2 blocks
	if (c.p1 == SWAMP || c.p2 == SWAMP || c.p3 == SWAMP || c.p4 == SWAMP)
	{
		k = BlockSet(row,col,2);
		return k += SWAMP*16 + 9;
	}

	if (c.p1 == ORCHID || c.p2 == ORCHID || c.p3 == ORCHID || c.p4 == ORCHID)   
	{
		k = BlockSet(row,col,3);
		return k += ORCHID*16;
	}

	if (c.p1 == EARTCODE3 || c.p2 == EARTCODE3 || c.p3 == EARTCODE3 || c.p4 == EARTCODE3)   
	{
		k = BlockSet(row,col,3);
		return k += EARTCODE3*16;
	}


		// No Road Rice Paddies
	if (c.p1 == ((11+17)+128) || c.p2 == ((11+17)+128) || c.p3 == ((11+17)+128) || c.p4 == ((11+17)+128))   {
		k = BlockSet(row,col,3) + 256*8;
		return k += (11+17)*16;
		}


	if ((c.p1 == CITYCODE || c.p2 == CITYCODE || c.p3 == CITYCODE || c.p4 == CITYCODE))   //cities
		k = CITYCODE*16+rand()%2;
//	else if ((c.p1 == INDUCODE || c.p2 == INDUCODE || c.p3 == INDUCODE || c.p4 == INDUCODE))   //industrial
//		k = INDUCODE+rand()%3;
//	else if ((c.p1 == FARMCODE || c.p2 == FARMCODE || c.p3 == FARMCODE || c.p4 == FARMCODE))   //farms
//		k = FARMCODE;
	else if ((c.p1 == FIELCODE || c.p2 == FIELCODE || c.p3 == FIELCODE || c.p4 == FIELCODE))   //Fields
		k = FIELCODE*16+rand()%1;
	else if ((c.p1 == EARTCODE || c.p2 == EARTCODE || c.p3 == EARTCODE || c.p4 == EARTCODE))   //Earthcode
		k = EARTCODE*16;

if (k == 0)
		k = c.p1;



	return k;
}



WORD BasicSet(WORD tileCode,PIXELCLUSTER c)
{
	WORD k = 0;
	
	if (c.p1 == tileCode || c.p1A == tileCode || c.p1B == tileCode || c.p1C == tileCode ||
		c.p1 == (tileCode+128) || c.p1A == (tileCode+128) || c.p1B == (tileCode+128) || c.p1C == (tileCode+128) ||
		c.p1 == (tileCode+192) || c.p1A == (tileCode+192) || c.p1B == (tileCode+192) || c.p1C == (tileCode+192))
			k |= 1;
	if (c.p2 == tileCode || c.p2A == tileCode || c.p2B == tileCode || c.p2C == tileCode ||
		c.p2 == (tileCode+128) || c.p2A == (tileCode+128) || c.p2B == (tileCode+128) || c.p2C == (tileCode+128) ||
		c.p1 == (tileCode+192) || c.p1A == (tileCode+192) || c.p1B == (tileCode+192) || c.p1C == (tileCode+192))
			k |= 2;
	if (c.p3 == tileCode || c.p3A == tileCode || c.p3B == tileCode || c.p3C == tileCode ||
		c.p3 == (tileCode+128) || c.p3A == (tileCode+128) || c.p3B == (tileCode+128) || c.p3C == (tileCode+128) ||
		c.p1 == (tileCode+192) || c.p1A == (tileCode+192) || c.p1B == (tileCode+192) || c.p1C == (tileCode+192))
		k |= 4;
	if (c.p4 == tileCode || c.p4A == tileCode || c.p4B == tileCode || c.p4C == tileCode  || 
		c.p4 == (tileCode+128) || c.p4A == (tileCode+128) || c.p4B == (tileCode+128) || c.p4C == (tileCode+128) ||
		c.p1 == (tileCode+192) || c.p1A == (tileCode+192) || c.p1B == (tileCode+192) || c.p1C == (tileCode+192))
			k |= 8;

	return k;
}


WORD BlockSet(int row,int col,int blockSize)
{	
	int index;

	row = row % blockSize;
	col = col % blockSize;

	index = (row)*blockSize + (col);

	return index+1;

}

WORD RoadSet(WORD code,PIXELCLUSTER c, unsigned ends)
{
	WORD k = 0;

	if (((c.p1 == code || c.p2 == code) && (c.p1C == code || c.p2A == code)) 
		|| (c.p1A == code && c.p1C == code) || (c.p1 == code && c.p1B == code))
		k |= 1;
	if (((c.p2 == code || c.p4 == code) && (c.p2C == code || c.p4C == code)) 
		|| (c.p2B == code && c.p2 == code) || (c.p4 == code && c.p4B == code))
		k |= 2;
	if (((c.p3 == code || c.p4 == code) && (c.p3C == code || c.p4A == code)) 
		|| (c.p3 == code && c.p3B == code) || (c.p3A == code && c.p3C == code))
		k |= 4;
	if (((c.p1 == code || c.p3 == code) && (c.p1A == code || c.p3A == code))
		|| (c.p1A == code && c.p1C == code) || (c.p3A == code && c.p3C == code))
		k |= 8;

	if (k > 0 && ends == 0){
	  // Eliminate end pieces...
		if (k == 1)
			k = 5;
		if (k == 2)
			k = 10;
		if (k == 4)
			k = 5;
		if (k == 8)
			k = 10;
		}	

	return k;
}