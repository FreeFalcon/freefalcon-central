
#include "limiters.h"

#ifdef USE_SH_POOLS
MEM_POOL	ThreePointLimiter::pool;
MEM_POOL	ValueLimiter::pool;
MEM_POOL	PercentLimiter::pool;
MEM_POOL	LineLimiter::pool;
MEM_POOL	MinMaxLimiter::pool;
#endif

LimiterMgrClass *gLimiterMgr = NULL;

LimiterMgrClass::LimiterMgrClass(int numdatasets)
{
	int i;

	numDatasets = numdatasets;
	limiterDatasets = new Limiter *[numdatasets * NumLimiterTypes];
	for(i = 0; i< numdatasets * NumLimiterTypes; i++)
	{
		limiterDatasets[i] = NULL;
	}

#ifdef USE_SH_POOLS
	ThreePointLimiter::InitializeStorage();
	ValueLimiter::InitializeStorage();
	PercentLimiter::InitializeStorage();
	LineLimiter::InitializeStorage();
	MinMaxLimiter::InitializeStorage();
#endif
}

LimiterMgrClass::~LimiterMgrClass(void)
{
/*	LimiterLink *next;
	LimiterLink *cur;

	for(int i = 0; i < numDatasets; i++)
	{
		next = limiterDatasets[i];
		while(next)
		{
			cur = next;
			next = cur->next;

			delete cur->limiter;
			cur->limiter = NULL;
			delete cur;
			cur = NULL;
		}

	}*/

	delete [] limiterDatasets;
	limiterDatasets = NULL;

#ifdef USE_SH_POOLS
	ThreePointLimiter::ReleaseStorage();
	ValueLimiter::ReleaseStorage();
	PercentLimiter::ReleaseStorage();
	LineLimiter::ReleaseStorage();
	MinMaxLimiter::ReleaseStorage();
#endif
}

int LimiterMgrClass::ReadLimiters(SimlibFileClass *file, int dataset)
{
//	LimiterLink *next;
//	LimiterLink *cur;
	int limiterType, key;
	int numLimiters, i;
	char buf[160];

	if(!file || dataset < 0 || dataset > numDatasets)
		return FALSE;
/*
	if(limiterDatasets[dataset])
	{
		next = limiterDatasets[dataset];
		while(next)
		{
			cur = next;
			next = cur->next;

			delete cur->limiter;
			cur->limiter = NULL;
			delete cur;
			cur = NULL;
		}
	}*/

	numLimiters = atoi(file->GetNext());
	if(!numLimiters)
		return FALSE;

	for(i = 0; i < numLimiters; i++)
	{
	/*	next = limiterDatasets[dataset];
		limiterDatasets[dataset] = new LimiterLink;
		limiterDatasets[dataset]->next = next;*/

		limiterType = atoi(file->GetNext());
		key = atoi(file->GetNext());

		if(limiterDatasets[dataset * NumLimiterTypes + key] )
		{
			delete limiterDatasets[dataset * numDatasets + key];
			limiterDatasets[dataset * NumLimiterTypes + key]  = NULL;
		}

		switch(limiterType)
		{
		case ltLine:
			//limiterDatasets[dataset]->limiter = new LineLimiter;
			limiterDatasets[dataset * NumLimiterTypes + key] = new LineLimiter;
			break;
		case ltValue:
			//limiterDatasets[dataset]->limiter = new ValueLimiter;
			limiterDatasets[dataset * NumLimiterTypes + key] = new ValueLimiter;
			break;
		case ltPercent:
			//limiterDatasets[dataset]->limiter = new PercentLimiter;
			limiterDatasets[dataset * NumLimiterTypes + key] = new PercentLimiter;
			break;
		case ltThreePt:
			//limiterDatasets[dataset]->limiter = new ThreePointLimiter;
			limiterDatasets[dataset * NumLimiterTypes + key] = new ThreePointLimiter;
			break;
			
		case ltMinMax:
			limiterDatasets[dataset * NumLimiterTypes + key] = new MinMaxLimiter;
			break;			

		}
		
		//limiterDatasets[dataset]->key = key;

		file->ReadLine(buf,160);
/*		if(limiterDatasets[dataset]->limiter)
			limiterDatasets[dataset]->limiter->Setup(buf);*/
		if(limiterDatasets[dataset * NumLimiterTypes + key])
			limiterDatasets[dataset * NumLimiterTypes + key]->Setup(buf);
		
	}

	return TRUE;
}

int	LimiterMgrClass::HasLimiter(int key, int dataset)
{
/*	LimiterLink *cur = limiterDatasets[dataset];

	while(cur)
	{
		if(cur->key == key)
			return TRUE;

		cur = cur->next;
	}

	return FALSE;*/

	return limiterDatasets[dataset * NumLimiterTypes + key] ? 1 : 0;
}

Limiter *LimiterMgrClass::GetLimiter(int key, int dataset)
{
/*	LimiterLink *cur = limiterDatasets[dataset];

	while(cur)
	{
		if(cur->key == key)
			return cur->limiter;

		cur = cur->next;
	}

	return NULL;*/
	return limiterDatasets[dataset * NumLimiterTypes + key];
}


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

Limiter::Limiter(LimiterType ltype) : type(ltype)
{

}

float Limiter::Limit(float x)
{
	return x;
}


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

LineLimiter::LineLimiter(void):Limiter(ltLine)
{
	m = 0.0F;
	b = 0.0F;
	upperX = 0.0F;
	lowerX = 0.0F;
}

void LineLimiter::Setup(char *string)
{
	float x1,y1,x2,y2;
	if(sscanf(string, "%f %f %f %f",&x1,&y1,&x2,&y2) == 4)
		Setup(x1,y1,x2,y2);	
}

void LineLimiter::Setup(float x1,float y1, float x2, float y2)
{
	m = (y2 - y1)/(x2 - x1);
	b = y1 - m * x1;
	upperX = max(x1,x2);
	lowerX = min(x1,x2);
}

float LineLimiter::Limit(float x)
{
	if( x > upperX )
		return m*upperX + b;
	else if ( x < lowerX)
		return m*lowerX + b;
	else
		return m*x + b;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

ThreePointLimiter::ThreePointLimiter(void):Limiter(ltThreePt)
{
	m1 = 0.0F;
	b1 = 0.0F;
	m2 = 0.0F;
	b2 = 0.0F;
	upperX = 0.0F;
	middleX = 0.0F;
	lowerX = 0.0F;
}


void ThreePointLimiter::Setup(char *string)
{
	float x[3],y[3];
	if(sscanf(string, "%f %f %f %f %f %f",&x[0],&y[0],&x[1],&y[1],&x[2],&y[2]) == 6)
	{
		Setup(x,y);	
	}
}

void ThreePointLimiter::Setup(float *x,float *y)
{
	float tempX, tempY;
	int		i,j;

	for(i = 0; i < 2; i++)
	{
		for(j = 2; j > i; j--)
		{
			if(x[j-1] > x[j])
			{
				tempX = x[j];
				tempY = y[j];
				x[j] = x[j-1];
				y[j] = y[j-1];
				x[j-1] = tempX;
				y[j-1] = tempY;
			}
		}
	}

	upperX = x[2];
	middleX = x[1];
	lowerX = x[0];
	
	m1 = (y[1] - y[0])/(x[1] - x[0]);
	b1 = y[0] - m1 * x[0];

	m2 = (y[1] - y[2])/(x[1] - x[2]);
	b2 = y[1] - m2 * x[1];
}

float ThreePointLimiter::Limit(float x)
{
	if( x > upperX )
		return m2*upperX + b2;
	else if ( x < lowerX)
		return m1*lowerX + b1;
	else if(x>middleX)
		return m2*x + b2;
	else
		return m1*x + b1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

ValueLimiter::ValueLimiter(void):Limiter(ltValue)
{
	value = 0.0F;
}

void ValueLimiter::Setup(char *string)
{
	float x;

	if(sscanf(string, "%f",&x) == 1)
		Setup(x);	
}

void ValueLimiter::Setup(float newValue)
{
	value = newValue;
}

//float ValueLimiter::Limit(float x)
float ValueLimiter::Limit(float)
{
	return value;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

MinMaxLimiter::MinMaxLimiter(void):Limiter(ltMinMax)
{
	minimum = 0.0F;
	maximum = 0.0F;
}

void MinMaxLimiter::Setup(char *string)
{
	float min, max;

	if(sscanf(string, "%f %f",&min, &max) == 2)
		Setup(min, max);	
}

void MinMaxLimiter::Setup(float Min, float Max)
{
	minimum = Min;
	maximum = Max;
}

float MinMaxLimiter::Limit(float x)
{
	return min(maximum, max(x, minimum));
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

PercentLimiter::PercentLimiter(void):Limiter(ltValue)
{
	percent = 1.0F;
}

void PercentLimiter::Setup(char *string)
{
	float x;

	if(sscanf(string, "%f",&x) == 1)
		Setup(x);	
}

void PercentLimiter::Setup(float newPercent)
{
	percent = newPercent;
}

float PercentLimiter::Limit(float x)
{
	return x*percent;
}