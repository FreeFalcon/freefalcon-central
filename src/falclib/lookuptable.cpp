#include "lookuptable.h"
#include "token.h"

LookupTable::LookupTable()
{
	pairs=0;
}

LookupTable::~LookupTable()
{
}

float LookupTable::Lookup(float In)
	{
		if(In < table[0].input)
		{
			return(table[0].output);
		}

		int l,l1;
		for(l = 0 ; l < (pairs - 1) ; l++)
		{
			l1 = l + 1;
			if(In < table[l1].input)
			{
#define RESCALE(in,inmin,inmax,outmin,outmax) ( ((float)(in) - (inmin)) * ((outmax) - (outmin)) / ((inmax) - (inmin)) + (outmin))
				return(RESCALE(In, table[l].input,table[l1].input, table[l].output, table[l1].output));
			}
		}

		// assume we fell thru
		return(table[pairs-1].output);
	}



TwoDimensionTable::TwoDimensionTable()
{
	int l;

	for(l=0; l<2; l++)
	{
		axis[l].breakPoint = 0;
		axis[l].breakPointCount = 0;
	}

	data = 0;
}

TwoDimensionTable::~TwoDimensionTable()
{
	int l;

	for(l=0; l<2; l++)
	{
		if(axis[l].breakPoint)
			delete [] axis[l].breakPoint;
	}

	if(data)
		delete [] data;
}

void TwoDimensionTable::Parse(char *inputStr)
{
	int l;

	SetTokenString(inputStr);

	for(l=0; l<2; l++)
	{
		axis[l].breakPointCount = TokenI(0);
		if(axis[l].breakPointCount == 0)
			return;
	}

	for(l=0; l<2; l++)
	{
		int t;

		axis[l].breakPoint = new float[axis[l].breakPointCount];

		for(t=0; t<axis[l].breakPointCount; t++)
		{
			axis[l].breakPoint[t] = TokenF(0);
		}
	}

	int dataSize = axis[0].breakPointCount * axis[1].breakPointCount;
	data = new float[dataSize];

	for(l=0; l<dataSize; l++)
	{
		data[l] = TokenF(0);
	}
}

float TwoDimensionTable::Lookup(float a, float b)
{
	float arg[2];

	if(!data) return 0;

	arg[0] = a;
	arg[1] = b;

	int index1[2], index2[2];
	float fraction[2];

	int l;

	for(l=0; l<2; l++)
	{
		if(arg[l] <= axis[l].breakPoint[0])
		{
			index1[l] = index2[l] = 0;
			fraction[l] = 0;
		}
		else
		{
			if(arg[l] >= axis[l].breakPoint[axis[l].breakPointCount - 1])
			{
				index1[l] = index2[l] = axis[l].breakPointCount - 1;
				fraction[l] = 0;
			}
			else
			{
				int t;

				for(t=0; t<axis[l].breakPointCount-1; t++)
				{
					if( arg[l] >  axis[l].breakPoint[t] && 
						arg[l] <= axis[l].breakPoint[t+1] )
					{
						index1[l]   = t;
						index2[l]   = t+1;
						fraction[l] = RESCALE(arg[l],axis[l].breakPoint[t],axis[l].breakPoint[t],0,1);
					}
				}
			}
		}
	}

	float d,e;

	d = RESCALE(fraction[0], 0, 1, Data(index1[0],index1[1]), Data(index2[0],index1[1]) );
	e = RESCALE(fraction[0], 0, 1, Data(index1[0],index2[1]), Data(index2[0],index2[1]) );

	return RESCALE(fraction[1], 0, 1, d, e );


}
