/***************************************************************************\
    matrix.cpp
    Scott Randolph
    November 30, 1994

    Provides facilities to store and manipulate 4x4 matricies.
\***************************************************************************/
#include "matrix.h"

// OW
#include "..\..\amd\inc\amatrix.h"
#include "..\..\amd\inc\atrans.h"

void MatrixMult_Generic( const Trotation* S1, const Trotation* S2, Trotation* T );
void MatrixMult_3DNow( const Trotation* S1, const Trotation* S2, Trotation* T );

void MatrixMult_Generic( const Trotation* M, const float k, Trotation* T );
void MatrixMult_3DNow( const Trotation* M, const float k, Trotation* T );

void MatrixMultTranspose_Generic( const Trotation* M, const Tpoint *P, Tpoint *Tgt );
void MatrixMultTranspose_3DNow( const Trotation* M, const Tpoint *P, Tpoint *Tgt );

void (*pMatrixMult1)(const Trotation* Mat1, const Trotation* Mat2, Trotation* Transform) = MatrixMult_Generic;
void (*pMatrixMult2)(const Trotation* Mat1, const float k, Trotation* Transform) = MatrixMult_Generic;
void (*pMatrixMultTranspose)(const Trotation* M, const Tpoint *P, Tpoint *Tgt) = MatrixMultTranspose_Generic;

// Handy constants to have available
const Tpoint	Origin	= { 0.0f, 0.0f, 0.0f };
const Trotation	IMatrix = { 1.0f, 0.0f, 0.0f,
							0.0f, 1.0f, 0.0f,
							0.0f, 0.0f, 1.0f };

/***************************************************************************\
	Initialize the contents of a matrix provided by the caller
\***************************************************************************/
void MatrixLoad( Trotation* T,	float a11, float a12, float a13,
	 							float a21, float a22, float a23,
								float a31, float a32, float a33)
{
	T->M11 = a11,	T->M12 = a12,	T->M13 = a13;
	T->M21 = a21,	T->M22 = a22,	T->M23 = a23;
	T->M31 = a31,	T->M32 = a32,	T->M33 = a33;
}						    


/***************************************************************************\
	Swap the rows and columns of the matrix
\***************************************************************************/
void MatrixTranspose( const Trotation* Src, Trotation* T )
{
	T->M11 = Src->M11,	T->M12 = Src->M21,	T->M13 = Src->M31;
	T->M21 = Src->M12,	T->M22 = Src->M22,	T->M23 = Src->M32;
	T->M31 = Src->M13,	T->M32 = Src->M23,	T->M33 = Src->M33;
}
						   
/***************************************************************************\
	Multiply the two provided matricies and store the result in the target
\***************************************************************************/
void MatrixMult( const Trotation* S1, const Trotation* S2, Trotation* T )
{
	pMatrixMult1(S1, S2, T);
}


/***************************************************************************\
	Multiply the two provided matricies and store the result in the target
\***************************************************************************/
void MatrixMult( const Trotation* M, const float k, Trotation* T )
{
	pMatrixMult2(M, k, T);
}


/***************************************************************************\
	Multiply the matrix with the point and store the result in the target
\***************************************************************************/
void MatrixMult( const Trotation* M, const Tpoint *P, Tpoint *Tgt )
{
	Tgt->x = M->M11*P->x + M->M12*P->y + M->M13*P->z;
	Tgt->y = M->M21*P->x + M->M22*P->y + M->M23*P->z;
	Tgt->z = M->M31*P->x + M->M32*P->y + M->M33*P->z;
}


/***************************************************************************\
	Multiply the transpose of the matrix with the point and store the 
	result in the target
\***************************************************************************/
void MatrixMultTranspose( const Trotation* M, const Tpoint *P, Tpoint *Tgt )
{
	pMatrixMultTranspose(M, P, Tgt);
}


// OW

// Generic Versions
////////////////////////////////

/***************************************************************************\
	Multiply the two provided matricies and store the result in the target
\***************************************************************************/
void MatrixMult_Generic( const Trotation* S1, const Trotation* S2, Trotation* T )
{
	T->M11 = S1->M11*S2->M11 + S1->M12*S2->M21 + S1->M13*S2->M31;
	T->M12 = S1->M11*S2->M12 + S1->M12*S2->M22 + S1->M13*S2->M32;
	T->M13 = S1->M11*S2->M13 + S1->M12*S2->M23 + S1->M13*S2->M33;

	T->M21 = S1->M21*S2->M11 + S1->M22*S2->M21 + S1->M23*S2->M31;
	T->M22 = S1->M21*S2->M12 + S1->M22*S2->M22 + S1->M23*S2->M32;
	T->M23 = S1->M21*S2->M13 + S1->M22*S2->M23 + S1->M23*S2->M33;
									 				   
	T->M31 = S1->M31*S2->M11 + S1->M32*S2->M21 + S1->M33*S2->M31;
	T->M32 = S1->M31*S2->M12 + S1->M32*S2->M22 + S1->M33*S2->M32;
	T->M33 = S1->M31*S2->M13 + S1->M32*S2->M23 + S1->M33*S2->M33;
}


/***************************************************************************\
	Multiply the two provided matricies and store the result in the target
\***************************************************************************/
void MatrixMult_Generic( const Trotation* M, const float k, Trotation* T )
{
	T->M11 = M->M11*k;	T->M12 = M->M12*k;	T->M13 = M->M13*k;
	T->M21 = M->M21*k;	T->M22 = M->M22*k;	T->M23 = M->M23*k;
	T->M31 = M->M31*k;	T->M32 = M->M32*k;	T->M33 = M->M33*k;
}

/***************************************************************************\
	Multiply the transpose of the matrix with the point and store the 
	result in the target
\***************************************************************************/
void MatrixMultTranspose_Generic( const Trotation* M, const Tpoint *P, Tpoint *Tgt )
{
	Tgt->x = M->M11*P->x + M->M21*P->y + M->M31*P->z;
	Tgt->y = M->M12*P->x + M->M22*P->y + M->M32*P->z;
	Tgt->z = M->M13*P->x + M->M23*P->y + M->M33*P->z;
}

// 3DNow Versions
////////////////////////////////

/***************************************************************************\
	Multiply the two provided matricies and store the result in the target
\***************************************************************************/
void MatrixMult_3DNow( const Trotation* S1, const Trotation* S2, Trotation* T )
{
	_mul_m3x3((float *) T, (float *) S1, (float *) S2);
}

/***************************************************************************\
	Multiply the two provided matricies and store the result in the target
\***************************************************************************/
void MatrixMult_3DNow( const Trotation* M, const float k, Trotation* T )
{
	_mul_m3x3s((float *) T, (float *) M, k);
}

/***************************************************************************\
	Multiply the transpose of the matrix with the point and store the 
	result in the target
\***************************************************************************/
void MatrixMultTranspose_3DNow( const Trotation* M, const Tpoint *P, Tpoint *Tgt )
{
	_trans_v1x3((float *) Tgt, (float *) M, (float *) P);	
}

// Support Stuff
////////////////////////////////

void SetMatrixCPUMode(int nMode)		// 0 - Generic (default), 1- 3DNow, 2- ISSE
{
	switch(nMode)
	{
		case 0:
		{
			pMatrixMult1 = MatrixMult_Generic;
			pMatrixMult2 = MatrixMult_Generic;
			pMatrixMultTranspose = MatrixMultTranspose_Generic;
			break;
		}

		case 1:
		{
			pMatrixMult1 = MatrixMult_3DNow;
			pMatrixMult2 = MatrixMult_3DNow;
			pMatrixMultTranspose = MatrixMultTranspose_3DNow;
			break;
		}

		case 2:
		{
			// pMatrixMult1 = MatrixMult_ISSE;
			// pMatrixMult2 = MatrixMult_ISSE;
			// pMatrixMultTranspose = MatrixMultTranspose_ISSE;
			// break;
		}

		default: ShiAssert(false);
	}
}
