/***************************************************************************\
    matrix.h
    Scott Randolph
    November 30, 1994

    Provides facilities to store and manipulate 4x4 matricies.
\***************************************************************************/
#ifndef _MATRIX_H_
#define _MATRIX_H_

#include "grTypes.h"


/***************************************************************************\
	Reference values provided by this module
\***************************************************************************/
extern const Tpoint		Origin;
extern const Trotation	IMatrix;



/***************************************************************************\
	Functions provided by this module
\***************************************************************************/
void MatrixLoad( Trotation* T,	float a11, float a12, float a13,
	 							float a21, float a22, float a23,
								float a31, float a32, float a33);

void MatrixTranspose( const Trotation* Src, Trotation* Tgt );

void MatrixMult( const Trotation* Mat1, const Trotation* Mat2, Trotation* Transform );

void MatrixMult( const Trotation* Mat1, const float k, Trotation* Transform );

void MatrixMult( const Trotation* M, const Tpoint *P, Tpoint *Tgt );

void MatrixMultTranspose( const Trotation* M, const Tpoint *P, Tpoint *Tgt );

void SetMatrixCPUMode(int nMode);		// 0 - Generic (default), 1- 3DNow, 2- ISSE
	
#endif /* _MATRIX_H_ */
