/*************************************************************************
	$Header: /home/cvsroot/RedCobra/FalcSnd/adpcm.cpp,v 1.1.1.1 2003/09/26 20:20:44 Red Exp $
	
	decode a buffer containing IMA ADPCM into a MS PCM
	
*************************************************************************/	

#include <windows.h>
#include "debuggr.h"
#include "fsound.h"
#include "psound.h"

//  This array is used by IMA_NextStepIndex to determine the next step
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

long CSoundMgr::StreamIMAADPCM(SOUNDSTREAM *Stream,char *dest,long dlen)
{
	DWORD bytesread;
	long bytestoread;

	// Keep buffer filled
	if(Stream->ImaInfo->sreadidx == -1) // read into entire buffer
	{
		bytestoread=min(Stream->ImaInfo->srcsize,Stream->ImaInfo->slen);
		ReadFile(Stream->fp,Stream->ImaInfo->src,bytestoread,&bytesread,NULL);
		Stream->ImaInfo->sreadidx=bytesread;
		Stream->ImaInfo->Status=0;
	}
	else if(Stream->ImaInfo->sreadidx < Stream->ImaInfo->slen)
	{
		if(!(Stream->ImaInfo->Status & SND_STREAM_PART2))
		{
			if((Stream->ImaInfo->sidx % Stream->ImaInfo->srcsize) > (Stream->ImaInfo->srcsize >> 1))
			{
				bytestoread=min(Stream->ImaInfo->slen-Stream->ImaInfo->sreadidx,(Stream->ImaInfo->srcsize >> 1));
				ReadFile(Stream->fp,&Stream->ImaInfo->src[Stream->ImaInfo->sreadidx % Stream->ImaInfo->srcsize],bytestoread,&bytesread,NULL);
				Stream->ImaInfo->sreadidx+=bytesread;
				Stream->ImaInfo->Status ^= SND_STREAM_PART2;
			}
		}
		else 
		{
			if((Stream->ImaInfo->sidx % Stream->ImaInfo->srcsize) < (Stream->ImaInfo->srcsize >> 1))
			{
				bytestoread=min(Stream->ImaInfo->slen-Stream->ImaInfo->sreadidx,(Stream->ImaInfo->srcsize >> 1));
				ReadFile(Stream->fp,&Stream->ImaInfo->src[Stream->ImaInfo->sreadidx % Stream->ImaInfo->srcsize],bytestoread,&bytesread,NULL);
				Stream->ImaInfo->sreadidx+=bytesread;
				Stream->ImaInfo->Status ^= SND_STREAM_PART2;
			}
		}
	}
	// Decode requested length
	if(Stream->ImaInfo->type == SND_WAV_SCHAN)
		return(StreamImaS16(Stream->ImaInfo,dest,dlen));

	return(StreamImaM16(Stream->ImaInfo,dest,dlen));
}

long CSoundMgr::MemStreamIMAADPCM(SOUNDSTREAM *Stream,char *dest,long dlen)
{
	// Decode requested length
	if(Stream->ImaInfo->type == SND_WAV_SCHAN)
		return(StreamImaS16(Stream->ImaInfo,dest,dlen));

	return(StreamImaM16(Stream->ImaInfo,dest,dlen));
}
//------------------------------------------------------------------------
//function StreamImaM16
//decodes ima adpcm into mono 16-bit pcm
long CSoundMgr::StreamImaM16(IMA_STREAM *Info, char *dBuff, long dlen)
{
	short	stepSize;
	IMA_BLOCK	*header;
	long		didx;

	short	encSample;

	didx=0;

	//step through each byte of IMA ADPCM and decode it to PCM
	while (Info->sidx < Info->slen && didx < dlen && Info->didx < Info->dlen)
	{			 
		if(!Info->blockLength)
		{
			Info->blockLength 	= min(Info->slen, SND_ADPCM_MBLOCK_ALIGN);
			Info->blockLength    -= sizeof(IMA_BLOCK) * SND_WAV_MCHAN;
			
			//get the block header
			header				 = (IMA_BLOCK *)&Info->src[Info->sidx % Info->srcsize];
			Info->sidx			+= sizeof(IMA_BLOCK);
			Info->predSampleL	 = header->iSamp0;
			Info->stepIndexL	 = (short)header->bStepTableIndex;
			if (!IMA_ValidStepIndex(Info->stepIndexL))
			{
				MonoPrint("S16:  invalid left step index\n");
				return 0;
			}
				
			//write out the first sample
			*(short *)&dBuff[didx] = (short)Info->predSampleL;
			didx += sizeof(short);
			Info->didx++;

			Info->count=0;
		}
		while (Info->blockLength && didx < dlen && Info->didx < Info->dlen)
		{
			if(!Info->count)
			{
				Info->leftSamples 	= Info->src[Info->sidx % Info->srcsize];
				Info->sidx++;

				Info->blockLength--;

				Info->count=2;
			}

			while(Info->count && didx < dlen && Info->didx < Info->dlen)
			{
				encSample	= (short)(Info->leftSamples & 0x0F);
				stepSize	= step[Info->stepIndexL];
				Info->predSampleL  = IMA_SampleDecode(encSample, Info->predSampleL, stepSize);
				Info->stepIndexL	= IMA_NextStepIndex(encSample, Info->stepIndexL);

				*(short *)&dBuff[didx] = (short)Info->predSampleL;
				didx += sizeof(short);
				Info->didx++;

				Info->leftSamples >>= 4;
				Info->count--;
			}
		}			
	}

	//return the number of bytes written
	return (long)(didx);
}
//end of function ImaDecodeM16
//-----------------------------------------------------------------------
//function StreamImaS16
//decodes ima adpcm into stereo 16-bit pcm
// This function is used for streaming IMA_ADPCM stereo...
//
//
//
long CSoundMgr::StreamImaS16(IMA_STREAM *Info, char *dBuff, long dlen)
{
	short		stepSize;
	IMA_BLOCK	*header;
	long		didx;

	short	encSampleL;
	short	encSampleR;

	didx=0;

	//step through each byte of IMA ADPCM and decode it to PCM
	while (Info->sidx < Info->slen && didx < dlen && Info->didx < Info->dlen)
	{
		if(!Info->blockLength)
		{
			//data should always be block aligned
			if (Info->slen < SND_ADPCM_SBLOCK_ALIGN)
			{
				MonoPrint("S16:  buffer length is less than the block alignment\n");
				return 0;
			}
				 
			Info->blockLength  = SND_ADPCM_SBLOCK_ALIGN;
			Info->blockLength -= sizeof(IMA_BLOCK) * SND_WAV_SCHAN;
			
			//get the left header
			header				 = (IMA_BLOCK *)&Info->src[Info->sidx % Info->srcsize];
			Info->sidx			+= sizeof(IMA_BLOCK);
			Info->predSampleL	 = header->iSamp0;
			Info->stepIndexL	 = (short)header->bStepTableIndex;
			if (!IMA_ValidStepIndex(Info->stepIndexL))
			{
				MonoPrint("S16:  invalid left step index\n");
				return 0;
			}
			
			//get the right header
			header				 = (IMA_BLOCK *)&Info->src[Info->sidx % Info->srcsize];
			Info->sidx			+= sizeof(IMA_BLOCK);
			Info->predSampleR	 = header->iSamp0;
			Info->stepIndexR	 = (short)header->bStepTableIndex;
			if (!IMA_ValidStepIndex(Info->stepIndexR))
			{
				MonoPrint("S16:  invlid right step index\n");
				return 0;
			}
				
			//write out the first sample
			*(long *)&dBuff[didx] = MAKELONG(Info->predSampleL, Info->predSampleR);
			didx += sizeof(long);
			Info->didx++;
			
			//the first long contains 4 left samples the second long
			//contains 4 right samples.  Will process the source in 8-byte
			//chunks to make it eay to interleave the output correctly
			if ((Info->blockLength%8) != 0)
			{
				MonoPrint("S16:  buffer length is not divisible by 8\n");
				return 0;
			}
			Info->count=0;
		}
		while (Info->blockLength && didx < dlen && Info->didx < Info->dlen)
		{
			if(!Info->count)
			{
				Info->blockLength    -= 8;

				Info->leftSamples 	 = *(long *)&Info->src[Info->sidx % Info->srcsize];
				Info->sidx	  		+= sizeof(long);
				Info->rightSamples 	 = *(long *)&Info->src[Info->sidx % Info->srcsize];
				Info->sidx			+= sizeof(long);
			
				Info->count = 8;
			}
			while(Info->count && didx < dlen && Info->didx < Info->dlen)
			{
				//left channel
				encSampleL			= (short)(Info->leftSamples & 0x0F);
				stepSize			= step[Info->stepIndexL];
				Info->predSampleL	= IMA_SampleDecode(encSampleL, Info->predSampleL, stepSize);
				Info->stepIndexL	= IMA_NextStepIndex(encSampleL, Info->stepIndexL);

				//right channel
				encSampleR 			= (short)(Info->rightSamples & 0x0F);
				stepSize			= step[Info->stepIndexR];
				Info->predSampleR	= IMA_SampleDecode(encSampleR, Info->predSampleR, stepSize);
				Info->stepIndexR 	= IMA_NextStepIndex(encSampleR, Info->stepIndexR);

				//write out the sample
				*(long *)&dBuff[didx] = MAKELONG(Info->predSampleL, Info->predSampleR);
				didx += sizeof(long);
				Info->didx++;

				//shift the next input ssample into the low-order 4 bits
				Info->leftSamples 	>>= 4;
				Info->rightSamples	>>= 4;
				Info->count--;
			}
		}		
	}
	return (long)(didx);
}
//end of function StreamImaS16
//-----------------------------------------------------------------------
//function ImaDecodeS16
//decodes ima adpcm into stereo 16-bit pcm
long CSoundMgr::ImaDecodeS16(char *sBuff, char *dBuff, long bufferLength)
{
	short	blockHeaderSize;
	int		blockAlignment;
	int		blockLength;
	char	*dBuffStart;
	long	leftSamples;
	long	rightSamples;
	short	stepSize;
	short	i;
	IMA_BLOCK	header;

	short	predSampleL;
	short	stepIndexL;
	short	encSampleL;

	short	predSampleR;
	short	stepIndexR;
	short	encSampleR;

	//put some commonly used info in more accessible variables and
	//init some variables
	blockHeaderSize = sizeof(IMA_BLOCK) * SND_WAV_SCHAN;
	blockAlignment 	= SND_ADPCM_SBLOCK_ALIGN;
	dBuffStart		= dBuff;

	//step through each byte of IMA ADPCM and decode it to PCM
	while (bufferLength)
		{
		//data should always be block aligned
		if (bufferLength < blockAlignment)
			{
			MonoPrint("S16:  buffer length is less than the block alignment\n");
			return 0;
			}
			 
		blockLength 	= blockAlignment;
		bufferLength   -= blockLength;
		blockLength    -= blockHeaderSize;
		
		//get the left header
		header		= *(IMA_BLOCK *)sBuff;
		sBuff		= sBuff + sizeof(IMA_BLOCK);
		predSampleL = header.iSamp0;
		stepIndexL	= (short)header.bStepTableIndex;
		if (!IMA_ValidStepIndex(stepIndexL))
			{
			MonoPrint("S16:  invalid left step index\n");
			return 0;
			}
		
		//get the right header
		header		= *(IMA_BLOCK *)sBuff;
		sBuff		= sBuff + sizeof(IMA_BLOCK);
		predSampleR	= header.iSamp0;
		stepIndexR	= (short)header.bStepTableIndex; 
		if (!IMA_ValidStepIndex(stepIndexR))
			{
			MonoPrint("S16:  invlid right step index\n");
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
			MonoPrint("S16:  buffer length is not divisible by 8\n");
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
				encSampleL	= (short)(leftSamples & 0x0F);
				stepSize	= step[stepIndexL];
				predSampleL = IMA_SampleDecode(encSampleL, predSampleL, stepSize);
				stepIndexL	= IMA_NextStepIndex(encSampleL, stepIndexL);

				//right channel
				encSampleR 	= (short)(rightSamples & 0x0F);
				stepSize	= step[stepIndexR];
				predSampleR = IMA_SampleDecode(encSampleR, predSampleR, stepSize);
				stepIndexR 	= IMA_NextStepIndex(encSampleR, stepIndexR);

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
long CSoundMgr::ImaDecodeM16(char *sBuff, char	*dBuff,	long bufferLength)
{
	short	blockHeaderSize;
	int		blockAlignment;
	int		blockLength;
	char	*dBuffStart;
	long	sample;
	short	stepSize;
	IMA_BLOCK	header;

	short	predSample;
	short	stepIndex;
	short	encSample;

	//put some commonly used info in more accessible variables and
	//init some variables
	blockHeaderSize = sizeof(IMA_BLOCK) * SND_WAV_MCHAN;
	blockAlignment 	= SND_ADPCM_MBLOCK_ALIGN;
	dBuffStart		= dBuff;

	//step through each byte of IMA ADPCM and decode it to PCM
	while (bufferLength >= blockHeaderSize)
		{			 
		blockLength 	= (UINT)min(bufferLength, blockAlignment);
		bufferLength   -= blockLength;
		blockLength    -= blockHeaderSize;
		
		//get the block header
		header		= *(IMA_BLOCK *)sBuff;
		sBuff		= sBuff + sizeof(IMA_BLOCK);
		predSample  = header.iSamp0;
		stepIndex 	= (short)header.bStepTableIndex;
		if (!IMA_ValidStepIndex(stepIndex))
			{
			MonoPrint("M16: invalid step index\n");
			return 0;
			}
			
		//write out the first sample
		*(short *)dBuff = (short)predSample;
		dBuff			= dBuff + sizeof(short);
		
		while (blockLength--)
			{
			sample	 	= *sBuff++;
			
			//sample 1
			encSample	= (short)(sample & 0x0F);
			stepSize	= step[stepIndex];
			predSample  = IMA_SampleDecode(encSample, predSample, stepSize);
			stepIndex	= IMA_NextStepIndex(encSample, stepIndex);

			*(short *)dBuff = (short)predSample;
			dBuff			= dBuff + sizeof(short);

			//sample 2
			encSample	= (short)(sample >> 4);
			stepSize	= step[stepIndex];
			predSample	= IMA_SampleDecode(encSample, predSample, stepSize);
			stepIndex 	= IMA_NextStepIndex(encSample, stepIndex);

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
//  short IMA_SampleDecode
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

short CSoundMgr::IMA_SampleDecode(short nEncodedSample,short nPredictedSample, short nStepSize)
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
//  short IMA_NextStepIndex
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

short CSoundMgr::IMA_NextStepIndex(short nEncodedSample, short nStepIndex)
{
    //
    //  compute new stepsize step
    //
    nStepIndex = (short)(nStepIndex + next_step[nEncodedSample]); // clamped to 0 <= nStepIndex <= 88

    if (nStepIndex < 0)
        nStepIndex = 0;
    else if (nStepIndex > 88)
        nStepIndex = 88;

    return (nStepIndex);
}


//------------------------------------------------------------------------
//  
//  BOOL IMA_ValidStepIndex
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

BOOL CSoundMgr::IMA_ValidStepIndex(short nStepIndex)
{

    if( nStepIndex >= 0 && nStepIndex <= 88 )
        return TRUE;
    else
        return FALSE;
}

//------------------------------------------------------------------------
