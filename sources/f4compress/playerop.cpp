#include "PlayerOp.h"

PlayerOptionsClass PlayerOptions;

PlayerOptionsClass::PlayerOptionsClass (void)
{
	Initialize();
}

void PlayerOptionsClass::Initialize(void)
{
}

int PlayerOptionsClass::LoadOptions(_TCHAR* filename)
{
	return TRUE;
}

void PlayerOptionsClass::ApplyOptions (void)
{
}

int PlayerOptionsClass::SaveOptions(_TCHAR* filename)
{
	return TRUE;
}

int PlayerOptionsClass::InCompliance(RulesStruct *rules)
{
	return TRUE;
}
void PlayerOptionsClass::ComplyWRules(RulesStruct *rules)
{
}

int CheckNumberPlayers(void)
{
	return 1;
}

