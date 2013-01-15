// vtempl.h

#pragma once

namespace D3DFrame
{

// Forw decls
/////////////////////////////////////////////////////////////////////////////


// Substitutions
/////////////////////////////////////////////////////////////////////////////


// Constants.
/////////////////////////////////////////////////////////////////////////////

template<class T> inline T Abs( const T A )
{
	return (A>=(T)0) ? A : -A;
}

template<class T> inline T Sgn( const T A )
{
	return (A>0) ? 1 : ((A<0) ? -1 : 0);
}

template<class T> inline T Max( const T A, const T B )
{
	return (A>=B) ? A : B;
}

template<class T> inline T Min( const T A, const T B )
{
	return (A<=B) ? A : B;
}

template<class T> inline T Square( const T A )
{
	return A*A;
}

template<class T> inline T Clamp( const T X, const T Min, const T Max )
{
	return X<Min ? Min : X<Max ? X : Max;
}

template<class T> inline T Align( const T Ptr, INT Alignment )
{
	return (T)(((DWORD)Ptr + Alignment - 1) & ~(Alignment-1));
}

template<class T> inline void Exchange( T& A, T& B )
{
	const T Temp = A;
	A = B;
	B = Temp;
}

template< class T > T Lerp( T& A, T& B, FLOAT Alpha )
{
	return A + Alpha * (B-A);
}

};	// namespace D3DFrame