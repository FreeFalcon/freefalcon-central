/***************************************************************************\
    edge.h
    Scott Randolph
    November 25, 1994

    Provides facilities to store and evalute the general equation for
    a line.  Can tell if a point is left of, right of, or on the line.
\***************************************************************************/
#ifndef _EDGE_H_
#define _EDGE_H_

#include <math.h>
#include "grtypes.h"


     
class Edge {
  public:
	Edge()	{};
	~Edge()	{};

  private:
	float	A, B, C;

  public:
  	// Initialize an edge using the exact coefficient values
	void	SetupWithABC( float a, float b, float c ) { 
				A = a;   B = b;   C = c;
			};

  	// Initialize an edge using two points (use instead of a constructor)
	void	SetupWithPoints( float x1, float y1, float x2, float y2 ) { 
				A = y2-y1;   B = x1-x2;   C = -(A*x1 + B*y1);
			};

  	// Initialize an edge using a point and a direction (use instead of a constructor)
	void	SetupWithVector( float x, float y, float dx, float dy ) { 
				A = dy;   B = -dx;   C = -(A*x + B*y);
			};

	// Normalize the coefficients so that vector A,B is a unit vector
	void	Normalize( void )	{ float mag = (float)sqrt(A*A+B*B); A /= mag, B /= mag, C /= mag; };

	// Swap the sense of "LeftOf" and "RightOf" by reorienting the normal vector
	void	Reverse( void )		{ A = -A; B = -B; C = -C; };

	// Returns true if the Edge is strictly horizontal (slope=0).
	BOOL	Horizontal( void )			{ return (B == 0.0f); };

	// Returns true if the Edge is strictly vertical (slope=infinity).
	BOOL	ConstantY( void )			{ return (A == 0.0f); };


	// Assuming the line equation is normalized, returns the distance of the line 
	// from the point.  (looking FROM the first point given to define the line 
	// TO the second point.)
	float	DistanceFrom( float x, float y )	{ return (A*x + B*y + C); };

	// Returns true if the Edge is
	// LeftOf the provided point (looking FROM the first point given to define
	// the line TO the second point.)
	BOOL	LeftOf( float x, float y )	{ return DistanceFrom(x, y) < 0.0f; };

	// Returns true if the Edge is
	// RightOf the provided point (looking FROM the first point given to define
	// the line TO the second point.)
	BOOL	RightOf( float x, float y )	{ return DistanceFrom(x, y) > 0.0f; };


	// Returns the X for the given Y
	float	X( float y )	{	if(A==0.0f) return 0.0f;
    								else return -(B*y + C) / A; };

	// Returns the Y for the given X
	float	Y( float x )	{	if(B==0.0f) return 0.0f;
    								else return -(A*x + C) / B; };


	// Returns the number of the octant in which this line falls (looking FROM the first 
	// point given to define the line TO the second point.)
	int		Quadrant( void )	{	if(A>0.0f)
										// positive Y direction
										if(B>0.0f)
											//negative X direction
											return 1;
										else
											return 0;
									else
										if(B>0.0f)
											//negative X direction
											return 2;
										else
											return 3;
								};
};
 
#endif /* _EDGE_H_ */


