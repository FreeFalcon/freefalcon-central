#if _MSC_VER < 1200
#error You need VC6 or higher
#endif // _MSC_VER < 1200

#pragma once

// ATL
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

// Smart ptr stuff
#include <comdef.h>

// DirectX
#define D3D_OVERLOADS

#include <ddraw.h>
#include <d3d.h>
#include <d3dxcore.h>
#include <d3dxmath.h>

// STL
#include <vector>
#include <string>
#include <map>

// Misc
#include <io.h>
#include <math.h>
#include <stdio.h>

#include "smart.h"
