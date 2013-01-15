///////////////////////////////////////// FAST MATH FUNCTIONS ///////////////////////////////////////////
// RED - 2007


// Float to Int32
inline	DWORD	F_I32(float x)
{	DWORD	r;
	_asm{ 
			fld		x
			fistp	r
	}
	return r;
}



// Absolute Value
inline	float	F_ABS(float x)
{	
	float r;

	_asm{ 
			fld		x
			fabs	
			fstp	r
	}
	return r;
}
