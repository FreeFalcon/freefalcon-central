/*************************************************************************
	$Header: /home/cvsroot/RedCobra/FalcSnd/imadpcm.cpp,v 1.1.1.1 2003/09/26 20:20:44 Red Exp $
	
	decode a buffer containing IMA ADPCM into a MS PCM
	
*************************************************************************/	

#include <windows.h>
#include <stdio.h>

#include "mssw.h"
#include "msima.h"
#include "exit.h"
#include "sound.h"


//  This array is used by imaadpcmNextStepIndex to determine the next step
//  index to use.  The step index is an index to the step[] array, below.
//
const short next_step[16] =
{
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

//  This array contains the array of step sizes used to encode the ADPCM
//  samples.  The step index in each ADPCM block is an index to this array.
//
const short step[89] =
{
        7,     8,     9,    10,    11,    12,    13,
       14,    16,    17,    19,    21,    23,    25,
       28,    31,    34,    37,    41,    45,    50,
       55,    60,    66,    73,    80,    88,    97,
      107,   118,   130,   143,   157,   173,   190,
      209,   230,   253,   279,   307,   337,   371,
      408,   449,   494,   544,   598,   658,   724,
      796,   876,   963,  1060,  1166,  1282,  1411,
     1552,  1707,  1878,  2066,  2272,  2499,  2749,
     3024,  3327,  3660,  4026,  4428,  4871,  5358,
     5894,  6484,  7132,  7845,  8630,  9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350,
    22385, 24623, 27086, 29794, 32767
};

short  	imaadpcmSampleDecode(short nEncodedSample,short nPredictedSample, short nStepSize);
short 	imaadpcmNextStepIndex(short nEncodedSample, short nStepIndex);
BOOL 	imaadpcmValidStepIndex(short nStepIndex);
//-----------------------------------------------------------------------
//function ImaDecodeS16
//decodes ima adpcm into stereo 16-bit pcm
long ImaDecodeS16(S8 *sBuff, S8	*dBuff,	S32	bufferLength)
{
	short	blockHeaderSize;
	UINT	blockAlignment;
	UINT	blockLength;
	S8		*dBuffStart;
	long	leftSamples;
	long	rightSamples;
	short	stepSize;
	short	i;
	ImaBlockHeader_t	header;

	short	predSampleL;
	short	stepIndexL;
	short	encSampleL;

	short	predSampleR;
	short	stepIndexR;
	short	encSampleR;

	//put some commonly used info in more accessible variables and
	//init some variables
	blockHeaderSize = sizeof(ImaBlockHeader_t) * SND_WAV_SCHAN;
	blockAlignment 	= SND_ADPCM_SBLOCK_ALIGN;
	dBuffStart		= dBuff;

	//step through each byte of IMA ADPCM and decode it to PCM
	while (bufferLength)
		{
		//data should always be block aligned
		if (bufferLength < blockAlignment)
			{
			BOF_PRINT(("S16:  buffer length is less than the block alignment\n"));
			return 0;
			}
			 
		blockLength 	= blockAlignment;
		bufferLength   -= blockLength;
		blockLength    -= blockHeaderSize;
		
		//get the left header
		header		= *(ImaBlockHeader_t *)sBuff;
		sBuff		= sBuff + sizeof(ImaBlockHeader_t);
		predSampleL = header.iSamp0;
		stepIndexL	= (short)header.bStepTableIndex;
		if (!imaadpcmValidStepIndex(stepIndexL))
			{
			BOF_PRINT(("S16:  invalid left step index\n"));
			return 0;
			}
		
		//get the right header
		header		= *(ImaBlockHeader_t *)sBuff;
		sBuff		= sBuff + sizeof(ImaBlockHeader_t);
		predSampleR	= header.iSamp0;
		stepIndexR	= (short)header.bStepTableIndex; 
		if (!imaadpcmValidStepIndex(stepIndexR))
			{
			BOF_PRINT(("S16:  invlid right step index\n"));
			return 0;
			}
			
		//write out the first sample
		*(long *)dBuff = MAKELONG(predSampleL, predSampleR);
		dBuff			= dBuff + sizeof(long);
		
		//the first long contains 4 left samples the second long
		//contains 4 right samples.  Will process the source in 8-byte
		//chunks to make it eay to interleave the output correctly
		if ((blockLength%8) != 0)
			{
			BOF_PRINT(("S16:  buffer length is not divisible by 8\n"));
			return 0;
			}
		while (0 != blockLength)
			{
			blockLength    -= 8;

			leftSamples 	= *(long *)sBuff;
			sBuff	  		= sBuff + sizeof(long);
			rightSamples 	= *(long *)sBuff;
			sBuff			= sBuff + sizeof(long);
			
			for (i=8; i>0; i--)
				{
				//left channel
				encSampleL	= (leftSamples & 0x0F);
				stepSize	= step[stepIndexL];
				predSampleL = imaadpcmSampleDecode(encSampleL, predSampleL, stepSize);
				stepIndexL	= imaadpcmNextStepIndex(encSampleL, stepIndexL);

				//right channel
				encSampleR 	= (rightSamples & 0x0F);
				stepSize	= step[stepIndexR];
				predSampleR = imaadpcmSampleDecode(encSampleR, predSampleR, stepSize);
				stepIndexR 	= imaadpcmNextStepIndex(encSampleR, stepIndexR);

				//write out the sample
				*(long *)dBuff = MAKELONG(predSampleL, predSampleR);
				dBuff			= dBuff + sizeof(long);

				//shift the next input ssample into the low-order 4 bits
				leftSamples 	>>= 4;
				rightSamples	>>= 4;
				} //loop of i=8 decrement to 0 	
			} //0 != blockLength				
		} //while 0 != bufferLength

	//return the number of bytes written
	return (long)(dBuff - dBuffStart);

}
//end of function ImaDecodeS16
//------------------------------------------------------------------------
//function ImaDecodeM16
//decodes ima adpcm into mono 16-bit pcm
long ImaDecodeM16(char *sBuff, char	*dBuff,	long bufferLength)
{
	short	blockHeaderSize;
	UINT	blockAlignment;
	UINT	blockLength;
	char	*dBuffStart;
	long	sample;
	short	stepSize;
	ImaBlockHeader_t	header;

	short	predSample;
	short	stepIndex;
	short	encSample;

	//put some commonly used info in more accessible variables and
	//init some variables
	blockHeaderSize = sizeof(ImaBlockHeader_t) * SND_WAV_MCHAN;
	blockAlignment 	= SND_ADPCM_MBLOCK_ALIGN;
	dBuffStart		= dBuff;

	//step through each byte of IMA ADPCM and decode it to PCM
	while (bufferLength >= blockHeaderSize)
		{			 
		blockLength 	= (UINT)min(bufferLength, blockAlignment);
		bufferLength   -= blockLength;
		blockLength    -= blockHeaderSize;
		
		//get the block header
		header		= *(ImaBlockHeader_t *)sBuff;
		sBuff		= sBuff + sizeof(ImaBlockHeader_t);
		predSample  = header.iSamp0;
		stepIndex 	= (short)header.bStepTableIndex;
		if (!imaadpcmValidStepIndex(stepIndex))
			{
			BOF_PRINT(("M16: invalid step index\n"));
			return 0;
			}
			
		//write out the first sample
		*(short *)dBuff = (short)predSample;
		dBuff			= dBuff + sizeof(short);
		
		while (blockLength--)
			{
			sample	 	= *sBuff++;
			
			//sample 1
			encSample	= (sample & (char)0x0F);
			stepSize	= step[stepIndex];
			predSample  = imaadpcmSampleDecode(encSample, predSample, stepSize);
			stepIndex	= imaadpcmNextStepIndex(encSample, stepIndex);

			*(short *)dBuff = (short)predSample;
			dBuff			= dBuff + sizeof(short);

			//sample 2
			encSample	= (sample >> 4);
			stepSize	= step[stepIndex];
			predSample	= imaadpcmSampleDecode(encSample, predSample, stepSize);
			stepIndex 	= imaadpcmNextStepIndex(encSample, stepIndex);

			*(short *)dBuff	= (short)predSample;
			dBuff			= dBuff + sizeof(short);
			} //0 != blockLength				
		} //while bufferLength >= blockHeaderSize

	//return the number of bytes written
	return (long)(dBuff - dBuffStart);

}
//end of function ImaDecodeM16
//------------------------------------------------------------------------
//  
//  short imaadpcmSampleDecode
//  
//  Description:
//      This routine decodes a single ADPCM sample. 
//  
//  Arguments:
//      short nEncodedSample:  The sample to be decoded.
//      short nPredictedSample:  The predicted value of the sample (in PCM).
//      short nStepSize:  The quantization step size used to encode the sample.
//  
//  Return (short):  The decoded PCM sample.
//  
//------------------------------------------------------------------------

short imaadpcmSampleDecode(short nEncodedSample,short nPredictedSample, short nStepSize)
{
    LONG            lDifference;
    LONG            lNewSample;

    //
    //  calculate difference:
    //
    //      lDifference = (nEncodedSample + 1/2) * nStepSize / 4
    //
    lDifference = nStepSize>>3;

    if (nEncodedSample & 4) 
        lDifference += nStepSize;

    if (nEncodedSample & 2) 
        lDifference += nStepSize>>1;

    if (nEncodedSample & 1) 
        lDifference += nStepSize>>2;

    //
    //  If the 'sign bit' of the encoded nibble is set, then the
    //  difference is negative...
    //
    if (nEncodedSample & 8)
        lDifference = -lDifference;

    //
    //  adjust predicted sample based on calculated difference
    //
    lNewSample = nPredictedSample + lDifference;

    //
    //  check for overflow and clamp if necessary to a 16 signed sample.
    //  Note that this is optimized for the most common case, when we
    //  don't have to clamp.
    //
    if( (long)(short)lNewSample == lNewSample )
    {
        return (short)lNewSample;
    }

    //
    //  Clamp.
    //
    if( lNewSample < -32768 )
        return (short)-32768;
    else
        return (short)32767;
}
//------------------------------------------------------------------------
//  
//  short imaadpcmNextStepIndex
//  
//  Description:
//      This routine calculates the step index value to use for the next
//      encode, based on the current value of the step index and the current
//      encoded sample.  
//  
//  Arguments:
//      short nEncodedSample:  The current encoded ADPCM sample.
//      short nStepIndex:  The step index value used to encode nEncodedSample.
//  
//  Return (short):  The step index to use for the next sample.
//  
//------------------------------------------------------------------------

short imaadpcmNextStepIndex(short nEncodedSample, short nStepIndex)
{
    //
    //  compute new stepsize step
    //
    nStepIndex += next_step[nEncodedSample];

    if (nStepIndex < 0)
        nStepIndex = 0;
    else if (nStepIndex > 88)
        nStepIndex = 88;

    return (nStepIndex);
}


//------------------------------------------------------------------------
//  
//  BOOL imaadpcmValidStepIndex
//  
//  Description:
//      This routine checks the step index value to make sure that it is
//      within the legal range.
//  
//  Arguments:
//      
//      short nStepIndex:  The step index value.
//  
//  Return (BOOL):  TRUE if the step index is valid; FALSE otherwise.
//  
//------------------------------------------------------------------------

BOOL imaadpcmValidStepIndex(short nStepIndex)
{

    if( nStepIndex >= 0 && nStepIndex <= 88 )
        return TRUE;
    else
        return FALSE;
}

//------------------------------------------------------------------------
