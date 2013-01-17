#include "turbulence.h"
#include "debuggr.h"
#include "renderow.h"
#include "weather.h"

extern unsigned long    vuxGameTime;

extern bool g_bDrawWakeTurbulence;

// static class members
TurbulanceList lTurbulenceList;
unsigned long AircraftTurbulence::lastPurgeTime = 0;

TurbulanceList::~TurbulanceList()
{	// this will clean up any stray objects on exit
	AircraftTurbulence *at;
	while(at = (AircraftTurbulence *)RemHead())
		delete at;

}


class TurbRecordNode : public ANode
{
	friend AircraftTurbulence;

private:
	class TurbKey
	{
	public:
		Tpoint position; // feet
		Tpoint vector;   // normalized vector
		float	time;     // seconds
		float   stength;  // arbitrary strength value
	};

	AircraftTurbulence *owner;

	TurbKey Start, End;

	float RetieveTurbulence(RetrieveTurbulanceParams &rtp);
};


int ClosestApproachLinePoint(Tpoint &A, Tpoint &B, Tpoint &C, float &PercentOfAB) // AB is the line, C is the point
{
	Tpoint AC;
	AC.x = C.x-A.x;
	AC.y = C.y-A.y;
	AC.z = C.z-A.z;
	float lengthAC = sqrt(AC.x*AC.x + AC.y*AC.y + AC.z*AC.z);
	Tpoint ACnorm;
	ACnorm.x = AC.x / (lengthAC + .00000001f); 
	ACnorm.y = AC.y / (lengthAC + .00000001f); 
	ACnorm.z = AC.z / (lengthAC + .00000001f); 

	Tpoint AB;
	AB.x = B.x-A.x;
	AB.y = B.y-A.y;
	AB.z = B.z-A.z;
	float lengthAB = sqrt(AB.x*AB.x + AB.y*AB.y + AB.z*AB.z);
	Tpoint ABnorm;
	ABnorm.x = AB.x / (lengthAB + .00000001f); 
	ABnorm.y = AB.y / (lengthAB + .00000001f); 
	ABnorm.z = AB.z / (lengthAB + .00000001f); 

	float cosAngleCAB = ACnorm.x * ABnorm.x + ACnorm.y * ABnorm.y + ACnorm.z * ABnorm.z;
	float lengthAD	  = cosAngleCAB * lengthAC;

	//D = A + ABnorm * lengthAD;
	PercentOfAB = lengthAD / (lengthAB + .00000001f);
	if(PercentOfAB>=0 && PercentOfAB<=1)
		return 1;
	return 0;
}


AircraftTurbulence::AircraftTurbulence()
{
	counter = -1;
	breakRecord = 0;
	locked = 1;
	lTurbulenceList.Lock();
	lTurbulenceList.AddHead(this);
	lTurbulenceList.Unlock();
	type = WAKE;

}

AircraftTurbulence::~AircraftTurbulence()
{
	lTurbulenceList.Lock();
	Remove();
	lTurbulenceList.Unlock();
	TurbRecordNode *rn;

	while( rn = (TurbRecordNode *)turbRecordList.RemHead() )
	{
		delete rn;
	}

}

void AircraftTurbulence::Release(void)
{
	locked = 0;
}


void AircraftTurbulence::RecordPosition(float Strength, float X, float Y, float Z)
{
	lTurbulenceList.Lock();

	TurbRecordNode *rn;

	rn = (TurbRecordNode *)turbRecordList.GetHead();

	if(counter <= 0 || !rn || breakRecord)
	{
		breakRecord = 0;
		TurbRecordNode *newrn;
		newrn = new TurbRecordNode;
		turbRecordList.AddHead(newrn);

		newrn->End.position.x = X;
		newrn->End.position.y = Y;
		newrn->End.position.z = Z;
		newrn->End.time = vuxGameTime * .001f;
		newrn->End.stength = Strength;
		newrn->owner = this;

		if(rn)
		{
			newrn->Start = rn->End;
		}
		else
		{
			newrn->Start = newrn->End;
		}
		counter = 50;

		// check the last segment, maybe it's dead
		if(	rn = (TurbRecordNode *)turbRecordList.GetTail() )
		{	// purge old nodes
			if(rn->End.time + lifeSpan < vuxGameTime * .001f)
			{
				rn->Remove();
				delete rn;
			}
		}
	}
	else
	{
		rn->End.position.x = X;
		rn->End.position.y = Y;
		rn->End.position.z = Z;
		rn->End.stength = Strength;
		rn->End.time   = vuxGameTime * .001f;
		counter --;
	}

	lTurbulenceList.Unlock();
}

struct RetrieveTurbulanceParams
{
	Tpoint pos;
	Tpoint fwd, up, right;

	float wakeEffect, yawEffect, pitchEffect, rollEffect;
};

float AircraftTurbulence::GetTurbulence(float X, float Y, float Z, float Yaw, float Pitch, float Roll, float &WakeEffect, float &YawEffect, float &PitchEffect, float &RollEffect)
{
	lTurbulenceList.Lock();
	float str = 0;

	AircraftTurbulence *at;
	RetrieveTurbulanceParams rtp;

	rtp.pitchEffect = 0;
	rtp.rollEffect  = 0;
	rtp.yawEffect   = 0;
	rtp.wakeEffect  = 0;
	rtp.pos.x = 0;
	rtp.pos.y = 0;
	rtp.pos.z = 0;

	rtp.pos.x = X;
	rtp.pos.y = Y;
	rtp.pos.z = Z;

	float costhe,sinthe,cospsi,sinpsi, sinphi, cosphi;

	costhe = (float)cos (Pitch);
	sinthe = (float)sin (Pitch);
	cospsi = (float)cos (Yaw);
	sinpsi = (float)sin (Yaw);
	cosphi = (float)cos (Roll);
	sinphi = (float)sin (Roll);

	rtp.fwd.x =  cospsi*costhe;
	rtp.fwd.y = -sinpsi*cosphi + cospsi*sinthe*sinphi;
	rtp.fwd.z =  sinpsi*sinphi + cospsi*sinthe*cosphi;

	rtp.right.x =  sinpsi*costhe;
	rtp.right.y =  cospsi*cosphi + sinpsi*sinthe*sinphi;
	rtp.right.z = -cospsi*sinphi + sinpsi*sinthe*cosphi;

	rtp.up.x = -sinthe;
	rtp.up.y =  costhe*sinphi;
	rtp.up.z =  costhe*cosphi;

	if( lastPurgeTime > vuxGameTime)
	{
		lastPurgeTime = 0;
	}

	if( lastPurgeTime + 1000 < vuxGameTime)
	{	// purge every second
		lastPurgeTime = vuxGameTime;
		// purge released and empty objects
		at = (AircraftTurbulence *)lTurbulenceList.GetHead();
		while(at)
		{
			AircraftTurbulence *at2 = (AircraftTurbulence *)at->GetSucc();
		
			if(!at->locked)
			{
				if(!at->turbRecordList.GetHead())
				{
					delete at;
				}
			}

			at = at2;
		}
	}

	at = (AircraftTurbulence *)lTurbulenceList.GetHead();
	while(at)
	{
		str += at->RetieveTurbulence(rtp);

		at = (AircraftTurbulence *)at->GetSucc();
	}

	WakeEffect  = rtp.wakeEffect;
	YawEffect   = rtp.yawEffect;
	PitchEffect = rtp.pitchEffect;
	RollEffect  = rtp.rollEffect;

	//MonoPrint("Returning Turb %f\n",str);

	lTurbulenceList.Unlock();
	return(str);
}

float AircraftTurbulence::RetieveTurbulence(RetrieveTurbulanceParams &rtp)
{
	TurbRecordNode *rn;
	float retval = 0;

	rn = (TurbRecordNode *)turbRecordList.GetHead();
	while(rn)
	{
		TurbRecordNode *rn2 = (TurbRecordNode *)rn->GetSucc();

		if(rn->End.time + lifeSpan < vuxGameTime * .001f)
		{
			rn->Remove();
			delete rn;
		}
		else
		{
			float s = rn->RetieveTurbulence(rtp);

			if(s>retval)
				retval = s;
		}

		rn = rn2;
	}

	return retval;
}

float TurbRecordNode::RetieveTurbulence(RetrieveTurbulanceParams &rtp)
{
	float fraction;

	if(ClosestApproachLinePoint(Start.position,End.position,rtp.pos,fraction))
	{
		Tpoint linePoint;
		Tpoint delta;
		linePoint.x = Start.position.x + (End.position.x - Start.position.x) * fraction;
		linePoint.y = Start.position.y + (End.position.y - Start.position.y) * fraction;
		linePoint.z = Start.position.z + (End.position.z - Start.position.z) * fraction;
		delta.x = linePoint.x - rtp.pos.x;
		delta.y = linePoint.y - rtp.pos.y;
		delta.z = linePoint.z - rtp.pos.z;
		float distance = sqrt(delta.x*delta.x + delta.y*delta.y + delta.z*delta.z) + 1;
		float age      = (vuxGameTime * .001F) - (Start.time    + (End.time    - Start.time) * fraction);
		float radius = owner->growthRate * age + 1 + owner->startRadius;
		
		if(distance > radius) return 0;

		float strength = Start.stength + (End.stength - Start.stength) * fraction;
		age = age/owner->lifeSpan; // make 0..1
		age *= age * age; // this will make the fade out less linear
		if(age>1) 
			age = 1;

		float rad = distance / radius;
		rad *= rad * rad; // this will make the interpolation from funnel center to a/c position less linear

		strength *= (1 - rad) * (1 - age );
		float dirMult = 1;

		if(strength)
		{
			switch(owner->type)
			{
			case AircraftTurbulence::WAKE:
				rtp.wakeEffect += strength;
				break;
			case AircraftTurbulence::RVORTEX:
				//dirMult = -1;
			case AircraftTurbulence::LVORTEX:
				Tpoint segNormal;
				segNormal.x = End.position.x - Start.position.x;
				segNormal.y = End.position.y - Start.position.y;
				segNormal.z = End.position.z - Start.position.z;
/*
//				segNormal.Normalize();
				float l = sqrt(segNormal.x*segNormal.x + segNormal.y*segNormal.y + segNormal.z*segNormal.z);
				if (l > 0.000001f) 
				{
						segNormal.x /= l;
						segNormal.y /= l;
						segNormal.z /= l;
				}

				// only doing roll for now;
				float rcos = segNormal % rtp.fwd;
*/
				if (owner->type == AircraftTurbulence::RVORTEX)
					dirMult = 1.0f;
				else
					dirMult = -1.0f;
				rtp.rollEffect += strength * dirMult;
				break;
			}
		}

		return strength;
	}
	return 0;
}


void AircraftTurbulence::Draw( class RenderOTW *renderer ) // debug useage
{
	if(!g_bDrawWakeTurbulence)
		return;
	lTurbulenceList.Lock();	
	Tpoint pos1, pos2;

	AircraftTurbulence *n;

	n = (AircraftTurbulence *)lTurbulenceList.GetHead();

	while(n)
	{	
		TurbRecordNode *rn = (TurbRecordNode *)n->turbRecordList.GetHead();

		while(rn)
		{
			renderer->SetColor(0xff0000ff);
			renderer->Render3DLine(&rn->Start.position, &rn->End.position);

			renderer->SetColor(0xffff00ff);
			pos1.x = pos2.x = rn->End.position.x;
			pos1.y = pos2.y = rn->End.position.y;
			pos1.z = pos2.z = rn->End.position.z;

			float size = rn->owner->growthRate * ((vuxGameTime * .001f) - rn->End.time) + 1 + rn->owner->startRadius;

			pos1.x-= size;
			pos2.x+= size;

			renderer->Render3DLine(&pos1, &pos2);

			pos1.x = pos2.x = rn->End.position.x;
			pos1.y = pos2.y = rn->End.position.y;
			pos1.z = pos2.z = rn->End.position.z;

			pos1.y-= size;
			pos2.y+= size;

			renderer->Render3DLine(&pos1, &pos2);

			pos1.x = pos2.x = rn->End.position.x;
			pos1.y = pos2.y = rn->End.position.y;
			pos1.z = pos2.z = rn->End.position.z;

			pos1.z-= size;
			pos2.z+= size;

			renderer->Render3DLine(&pos1, &pos2);

			rn = (TurbRecordNode *)rn->GetSucc();
		}
		n = (AircraftTurbulence *)n->GetSucc();
	}	
	lTurbulenceList.Unlock();
}

// note to self
// vortex, get dot between player a/c and segment 0' = +roll  90' = pitch  180' = -roll