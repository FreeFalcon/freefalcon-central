#ifndef _WINDOWS_
#include <windows.h>
#endif

class SimBaseClass;
class HSIClass;

class NAVClass
{
private:

	float m_courseDeviation;
	float m_desiredCourse;
	float m_distanceToWayPoint;
	float m_bearingToWayPoint;
	BOOL m_toTrue;
	SimBaseClass* m_ownship;

	float ConvertToNav(float);

public:
	NAVClass(SimBaseClass*);
	int Exec(void);
	void Display(HSIClass*);
	void SetDesiredCourse(float l_desiredCourse){m_desiredCourse = l_desiredCourse;}
};

