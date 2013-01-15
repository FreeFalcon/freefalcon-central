/*
** Name: FAKERAND.H
** Description:
**		When randomness doesn't need to be completely random -- just
**		a modicum of random -- then these inline functions should be
**		quick
*/

#ifndef _FAKERAND_
#define _FAKERAND_

// fakerand wasn't good enough for doing something that
// needed to be cyclicly randomiized like the tracer fire.
#define	NRANDPOS ((float)( (float)rand()/(float)RAND_MAX ))
#define NRAND	 ( 1.0f - 2.0f * NRANDPOS )

// edg: I Dunno.  Maybe there's a bug in this or the compiler is
// doing something weird, but I find that in many cases it seems like
// the counter doesn't get incremented or something -- ie the same thing
// always happens.  So, for now, if a module wants to use the lookup
// rands it should define USE_FAKERAND prior to including this file

#ifdef USE_FAKERAND
// counter for cycling thru tables
static unsigned int randCtr1 = 0;
static unsigned int randCtr2 = 0;
static unsigned int randCtr3 = 0;
static unsigned int randCtr4 = 0;
static unsigned int randCtr5 = 0;

// random int from 0 to 4
static int randIntTab5[16] =
{
	0, 1, 0, 2,
	4, 0, 2, 0,
	3, 0, 4, 0,
	4, 1, 0, 3,
};

inline int PRANDInt5( void )
{
	randCtr1++;
	return randIntTab5[ (randCtr1 & 0x0000000f) ];
}


// random int from 0 to 2
static int randIntTab3[8] =
{
	0, 1, 0, 2,
	0, 0, 1, 0,
};
inline int PRANDInt3( void )
{
	randCtr2++;
	return randIntTab3[ (randCtr2 & 0x00000007) ];
}

// rand floats from -1.0 to 1.0
static float randFloatTab[16] =
{
	0.7f, -0.6f, -0.9f, 0.8f,
	0.1f, 0.5f, -0.3f, -0.6f,
	1.0f, 0.2f, -0.8f, 0.3f,
	-1.0f, 0.8f, -0.4f, 0.3f
};

inline float PRANDFloat( void )
{
	randCtr3++;
	return randFloatTab[ (randCtr3 & 0x0000000f) ];
}

// rand floats from 0.0 to 1.0
static float randFloatTabPos[16] =
{
	0.7f, 0.6f, 0.1f, 0.8f,
	0.1f, 0.5f, 0.3f, 0.6f,
	1.0f, 0.2f, 0.8f, 0.1f,
	0.0f, 0.2f, 0.9f, 0.4f
};

inline float PRANDFloatPos( void )
{
	randCtr4++;
	return randFloatTabPos[ (randCtr4 & 0x0000000f) ];
}

// random int from 0 to 5
static int randIntTab6[16] =
{
	0, 1, 5, 2,
	4, 3, 2, 0,
	3, 2, 4, 5,
	4, 1, 0, 3,
};

inline int PRANDInt6( void )
{
	randCtr5++;
	return randIntTab6[ (randCtr5 & 0x0000000f) ];
}

#else  // "real" randomness for above functions

// COBRA - RED - The Real Superfast Random Function is this one
inline	long GenerateFastRandom( void )
{
	static long LastRandom;
	long	FastRandom;														// The Random Variable
	_asm {	
			push	edx
			push	eax
			RDTSC
			add	DWORD PTR FastRandom,edx
			xor	DWORD PTR FastRandom,eax
			mov	eax, DWORD PTR LastRandom
			add	DWORD PTR FastRandom,eax
			add	DWORD PTR LastRandom,edx
			pop	eax
			pop	edx
	}
	return(FastRandom);
}


inline int PRANDInt5( void )
{
//	return	FloatToInt32(NRANDPOS * 4.9999f);
	int	x= (int)GenerateFastRandom()&0x07;
	return((x<5)?x:x-5);
}

inline int PRANDInt3( void )
{
//	return	FloatToInt32(NRANDPOS * 2.9999f);
	int	x= (int)GenerateFastRandom()&0x03;
	return((x<3)?x:x-3);
}


inline float PRANDFloat( void )
{
//	return NRAND;
	float	x=	(float)(GenerateFastRandom()&0xffff);
	return( 1 - 2 * x / 65535.0f);
}

inline float PRANDFloatPos( void )
{
//	return NRANDPOS;
	float	x=	(float)(GenerateFastRandom()&0xffff);
	return( x / 65535.0f);
}

inline int PRANDInt6( void )
{
//	return	FloatToInt32( (float)(NRANDPOS * 5.9999f) );
	int	x= (int)GenerateFastRandom()&0x07;
	return((x<6)?x:x-6);
}

#endif


#endif
