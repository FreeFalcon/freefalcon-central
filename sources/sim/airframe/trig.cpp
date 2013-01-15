/******************************************************************************/
/*                                                                            */
/*  Unit Name : trig.cpp                                                      */
/*                                                                            */
/*  Abstract  : Trigenometry calculations                                     */
/*                                                                            */
/*  Development History :                                                     */
/*  Date      Programer           Description                                 */
/*----------------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                               */
/*   5-Aug-98 DP                  Rewrite to compute nose from velocity       */
/*   5-Aug-98 SCR                 Rewrite from scratch                        */
/*                                                                            */
/******************************************************************************/
#include "stdhdr.h"
#include "airframe.h"
#include "aircrft.h"
#include "Graphics/Include/Matrix.h"

typedef double DTransformMatrix[3][3];

#ifdef _DEBUG
static float epsilon = 0.1F;

float mag(float x, float y, float z) {
	return (float)sqrt(x*x+y*y+z*z);
}

double mag(double x, double y, double z) {
	return (double) sqrt((float)x*(float)x+(float)y*(float)y+(float)z*(float)z);
}

void DebugValidation( Trotation *M )
{
	float row1 = mag( M->M11, M->M12, M->M13 );
	float row2 = mag( M->M21, M->M22, M->M23 );
	float row3 = mag( M->M31, M->M32, M->M33 );

	float col1 = mag( M->M11, M->M21, M->M31 );
	float col2 = mag( M->M12, M->M22, M->M32 );
	float col3 = mag( M->M13, M->M23, M->M33 );

	ShiAssert (fabs(row1 - 1.0F) < epsilon);
	ShiAssert (fabs(row2 - 1.0F) < epsilon);
	ShiAssert (fabs(row3 - 1.0F) < epsilon);
	ShiAssert (fabs(col1 - 1.0F) < epsilon);
	ShiAssert (fabs(col2 - 1.0F) < epsilon);
	ShiAssert (fabs(col3 - 1.0F) < epsilon);
}

void DebugValidation( TransformMatrix &M )
{
	float row1 = mag( M[0][0], M[0][1], M[0][2] );
	float row2 = mag( M[1][0], M[1][1], M[1][2] );
	float row3 = mag( M[2][0], M[2][1], M[2][2] );

	float col1 = mag( M[0][0], M[1][0], M[2][0] );
	float col2 = mag( M[0][1], M[1][1], M[2][1] );
	float col3 = mag( M[0][2], M[1][2], M[2][2] );

	ShiAssert (fabs(row1 - 1.0F) < epsilon);
	ShiAssert (fabs(row2 - 1.0F) < epsilon);
	ShiAssert (fabs(row3 - 1.0F) < epsilon);
	ShiAssert (fabs(col1 - 1.0F) < epsilon);
	ShiAssert (fabs(col2 - 1.0F) < epsilon);
	ShiAssert (fabs(col3 - 1.0F) < epsilon);
}

void DebugValidation( DTransformMatrix &M )
{
	double row1 = mag( M[0][0], M[0][1], M[0][2] );
	double row2 = mag( M[1][0], M[1][1], M[1][2] );
	double row3 = mag( M[2][0], M[2][1], M[2][2] );

	double col1 = mag( M[0][0], M[1][0], M[2][0] );
	double col2 = mag( M[0][1], M[1][1], M[2][1] );
	double col3 = mag( M[0][2], M[1][2], M[2][2] );

	ShiAssert (fabs(row1 - 1.0F) < epsilon);
	ShiAssert (fabs(row2 - 1.0F) < epsilon);
	ShiAssert (fabs(row3 - 1.0F) < epsilon);
	ShiAssert (fabs(col1 - 1.0F) < epsilon);
	ShiAssert (fabs(col2 - 1.0F) < epsilon);
	ShiAssert (fabs(col3 - 1.0F) < epsilon);
}
#else
inline void DebugValidation( Trotation *M ) {};
inline void DebugValidation( TransformMatrix &M ) {};
inline void DebugValidation( DTransformMatrix &M ) {};
#endif

/********************************************************************
                                                                 
 Routine: void AirframeClass::Trigenometry(void)                 
                                                                 
 Description:                                                    
    Do all trig funtions used more than once.  Also calculates   
    wind axis euler angles.                                      
                                                                 
 Inputs (implicit):                                              
    alpha:	angle of attack in degrees (positive means nose above vv)
	beta:	angle of side slipe in degrees (positive means nose LEFT of vv)
	sigma:	velocity vector "yaw" in world space in radians
	gmma:	velocity vector "pitch" in world space in radians
	mu:		velocity vector "roll" in world space in radians
                                                                 
 Outputs (implicit):                                                        
	psi:	nose vector "yaw" in world space in radians
	theta:	nose vector "pitch" in world space in radians
	phi:	nose vector "roll" in world space in radians
	(Cos and Sine of all input and output angles)
	dmx:	"direction cosine matrix" or world space rotation matrix of the body

  Note:
    This is all based on the fact that the columns of a rotation matrix are the
	unit normal basis vectors for a coordinate system.  That is, if we build
	the rotation matrix from direction cosines, then the columns of the matrix
	are unit vectors point forward, right, and up.  Column 1 is "at", column 2
	is "right", and column 3 is "up".
                                                                 
  Development History :                                          
  Date      Programer           Description                      
-----------------------------------------------------------------
  23-Jan-95 LR                  Initial Write                    
                                                                 
********************************************************************/
void AirframeClass::Trigenometry(){

	mlTrig trigAlpha, trigBeta, trig;
	TransformMatrix vv;
	TransformMatrix R;
	ObjectGeometry& pa = platform->platformAngles;

	// Do the trig for all the input angles
	mlSinCos(&trigAlpha, alpha * DTR);
	pa.cosalp = trigAlpha.cos;
	pa.sinalp = trigAlpha.sin;
	
	ShiAssert(!_isnan(trigAlpha.cos));

	mlSinCos(&trigBeta, beta * DTR);
	pa.cosbet = trigBeta.cos;
	pa.sinbet = trigBeta.sin;
	pa.tanbet = (float)tan(beta * DTR);
	
	ShiAssert(!_isnan(trigBeta.cos));

	mlSinCos(&trig, sigma);
	pa.cossig = trig.cos;
	pa.sinsig = trig.sin;

	ShiAssert(!_isnan(trig.cos));
	
	mlSinCos(&trig, mu);
	pa.cosmu  = trig.cos;
	pa.sinmu  = trig.sin;
	
	ShiAssert(!_isnan(trig.cos));

	mlSinCos (&trig, gmma);
	pa.cosgam = trig.cos;
	pa.singam = trig.sin;

	ShiAssert(!_isnan(trig.cos));
	
	// Construct the rotation matrix for the velocity vector
	// We do this to get its basis vectors
	vv[0][0] =  pa.cossig * pa.cosgam;
	vv[0][1] =  pa.sinsig * pa.cosgam;
	vv[0][2] = -pa.singam;
	
	vv[1][0] =  pa.cossig * pa.singam * pa.sinmu - pa.sinsig * pa.cosmu;
	vv[1][1] =  pa.cossig * pa.cosmu + pa.sinsig * pa.singam * pa.sinmu;
	vv[1][2] =  pa.cosgam * pa.sinmu;
	
	vv[2][0] =  pa.sinsig * pa.sinmu + pa.cossig * pa.singam * pa.cosmu;
	vv[2][1] = -pa.cossig * pa.sinmu + pa.sinsig * pa.singam * pa.cosmu;
	vv[2][2] =  pa.cosgam * pa.cosmu;

	DebugValidation( vv );

	// Construct the rotation matrix from velocity vector to nose using
	// alpha and beta
	R[0][0] =  trigBeta.cos * trigAlpha.cos;
	R[0][1] = -trigBeta.sin * trigAlpha.cos;
	R[0][2] = -trigAlpha.sin;
	
	R[1][0] =  trigBeta.sin;
	R[1][1] =  trigBeta.cos;
	R[1][2] =  0.0f;
	
	R[2][0] =  trigBeta.cos * trigAlpha.sin;
	R[2][1] = -trigBeta.sin * trigAlpha.sin;
	R[2][2] =  trigAlpha.cos;

	DebugValidation( R );

	// Construct the nose vector from the velocity vector using rotation built above, 
	// by multiplying the velocity vector times the rotation vector
	platform->dmx[0][0] = R[0][0]*vv[0][0] + R[0][1]*vv[1][0] + R[0][2]*vv[2][0];
	platform->dmx[0][1] = R[0][0]*vv[0][1] + R[0][1]*vv[1][1] + R[0][2]*vv[2][1];
	platform->dmx[0][2] = R[0][0]*vv[0][2] + R[0][1]*vv[1][2] + R[0][2]*vv[2][2];

	platform->dmx[1][0] = R[1][0]*vv[0][0] + R[1][1]*vv[1][0] + R[1][2]*vv[2][0];
	platform->dmx[1][1] = R[1][0]*vv[0][1] + R[1][1]*vv[1][1] + R[1][2]*vv[2][1];
	platform->dmx[1][2] = R[1][0]*vv[0][2] + R[1][1]*vv[1][2] + R[1][2]*vv[2][2];

	// sfr: black triangle fix
	/* original
	platform->dmx[2][0] = R[2][0]*vv[0][0]					  + R[2][2]*vv[2][0];
	platform->dmx[2][1] = R[2][0]*vv[0][1]					  + R[2][2]*vv[2][1];
	platform->dmx[2][2] = R[2][0]*vv[0][2]					  + R[2][2]*vv[2][2];
	*/
	platform->dmx[2][0] = R[2][0]*vv[0][0] + R[2][1]*vv[1][0] + R[2][2]*vv[2][0];
	platform->dmx[2][1] = R[2][0]*vv[0][1] + R[2][1]*vv[1][1] + R[2][2]*vv[2][1];
	platform->dmx[2][2] = R[2][0]*vv[0][2] + R[2][1]*vv[1][2] + R[2][2]*vv[2][2];
	

	DebugValidation( platform->dmx );

#ifdef _DEBUG
	DTransformMatrix	dmx;

	dmx[0][0] = R[0][0]*vv[0][0] + R[0][1]*vv[1][0] + R[0][2]*vv[2][0];
	dmx[0][1] = R[0][0]*vv[0][1] + R[0][1]*vv[1][1] + R[0][2]*vv[2][1];
	dmx[0][2] = R[0][0]*vv[0][2] + R[0][1]*vv[1][2] + R[0][2]*vv[2][2];

	dmx[1][0] = R[1][0]*vv[0][0] + R[1][1]*vv[1][0] + R[1][2]*vv[2][0];
	dmx[1][1] = R[1][0]*vv[0][1] + R[1][1]*vv[1][1] + R[1][2]*vv[2][1];
	dmx[1][2] = R[1][0]*vv[0][2] + R[1][1]*vv[1][2] + R[1][2]*vv[2][2];
						   				
	dmx[2][0] = R[2][0]*vv[0][0]					+ R[2][2]*vv[2][0];
	dmx[2][1] = R[2][0]*vv[0][1]					+ R[2][2]*vv[2][1];
	dmx[2][2] = R[2][0]*vv[0][2]					+ R[2][2]*vv[2][2];

	DebugValidation( dmx );

	//MatrixMult( &R, &vv, &dmx );

	//DebugValidation( dmx );
#endif

	// Construct the nose vector from the velocity vector using rotation built above.
	//MatrixMult( &R, &vv, &N );

	// Derive the body direction angles from the orientation matrix
	psi			= (float)atan2( platform->dmx[0][1], platform->dmx[0][0] );		// yaw
	theta		= (float)-asin( platform->dmx[0][2] );							// pitch
	phi			= (float)atan2( platform->dmx[1][2], platform->dmx[2][2] );		// roll

	// Finally, store the cos and sin of the body angles
	mlSinCos (&trig, psi);
	pa.cospsi = trig.cos;
	pa.sinpsi = trig.sin;

	ShiAssert(!_isnan(trig.cos));

	mlSinCos (&trig, theta);
	pa.costhe = trig.cos;
	pa.sinthe = trig.sin;

	ShiAssert(!_isnan(trig.cos));

	mlSinCos (&trig, phi);
	pa.cosphi = trig.cos;
	pa.sinphi = trig.sin;

	ShiAssert(!_isnan(trig.cos));
}
