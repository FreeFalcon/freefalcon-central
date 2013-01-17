#include "caution.h"
#include "debuggr.h"

//-------------------------------------------------
// CautionClass::CautionClass
//-------------------------------------------------

CautionClass::CautionClass() {

	int	i;

	for (i = 0; i < NumVectors; i++) {
		mpBitVector[i]	= 0x0;
	}
}

//-------------------------------------------------
// CautionClass::IsFlagSet
//-------------------------------------------------

BOOL CautionClass::IsFlagSet() {

	int i;
	unsigned int flag = 0;


	for(i = 0; i < NumVectors; i++) {
		flag 	|= mpBitVector[i];
	}

	return (flag != 0);
}

//-------------------------------------------------
// CautionClass::ClearFlag
//-------------------------------------------------

void CautionClass::ClearFlag() {
MonoPrint("remove call\n");
}

//-------------------------------------------------
// CautionClass::SetCaution
//-------------------------------------------------

void CautionClass::SetCaution(type_CSubSystem subsystem) {

	SetCaution((int) subsystem);
}

//-------------------------------------------------
// CautionClass::SetCaution
//-------------------------------------------------

void CautionClass::SetCaution(type_TWSubSystem subsystem) {
	
	SetCaution((int) subsystem);
}

//-------------------------------------------------
// CautionClass::ClearCaution
//-------------------------------------------------

void CautionClass::ClearCaution(type_CSubSystem subsystem) {

	ClearCaution((int) subsystem);
}

//-------------------------------------------------
// CautionClass::ClearCaution
//-------------------------------------------------

void CautionClass::ClearCaution(type_TWSubSystem subsystem) {

	ClearCaution((int) subsystem);
}

//-------------------------------------------------
// CautionClass::GetCaution
//-------------------------------------------------

BOOL CautionClass::GetCaution(type_CSubSystem subsystem) {
	return GetCaution((int) subsystem);
}

//-------------------------------------------------
// CautionClass::GetCaution
//-------------------------------------------------

BOOL CautionClass::GetCaution(type_TWSubSystem subsystem) {
	return GetCaution((int) subsystem);
}

//-------------------------------------------------
// CautionClass::SetCaution
//-------------------------------------------------

void CautionClass::SetCaution(int subsystem) {

	int		vectorNum	= 0;
	int		bitNum;

	vectorNum	= subsystem / BITS_PER_VECTOR;
	bitNum		= subsystem - vectorNum * BITS_PER_VECTOR;

	mpBitVector[vectorNum] |= 0x01 << bitNum;

}

//-------------------------------------------------
// CautionClass::ClearCaution
//-------------------------------------------------

void CautionClass::ClearCaution(int subsystem) {
	
	int		vectorNum	= 0;
	int		bitNum;

	vectorNum	= subsystem / BITS_PER_VECTOR;
	bitNum		= subsystem - vectorNum * BITS_PER_VECTOR;

	mpBitVector[vectorNum] &= ~(0x01 << bitNum);

}

//-------------------------------------------------
// CautionClass::GetCaution
//-------------------------------------------------

BOOL CautionClass::GetCaution(int subsystem) {

	int		vectorNum	= 0;
	int		bitNum;

	vectorNum	= subsystem / BITS_PER_VECTOR;
	bitNum		= subsystem - vectorNum * BITS_PER_VECTOR;

	return ((mpBitVector[vectorNum] & (0x01 << bitNum)) != 0);
}

