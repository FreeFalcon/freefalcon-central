#include <ddraw.h>
#include <d3d.h>
#include <d3dxcore.h>
#include <d3dxmath.h>

#include "../include/grtypes.h"
#include "../include/polylib.h"


void	AssignPmatrixToD3DXMATRIX(D3DXMATRIX *d, Pmatrix *s);
void	AssignD3DXMATRIXToPmatrix(Pmatrix *d, D3DXMATRIX *s );


// The Scripts Pointer Array
extern	bool	(*DXScriptArray[])(D3DVECTOR *pos, ObjectInstance*, DWORD*);
