/***************************************************************************\
	LineList.h
	Scott Randolph
	December 4, 1997

	Keep a list of colored line segments and draw them.
\***************************************************************************/
#ifndef _LINELIST_H_
#define _LINELIST_H_

#include "Renderer\Render3D.h"


extern class LineListClass	TheLineList;


typedef struct LineSegment {
	Tpoint		p1;
	Tpoint		p2;
	DWORD		color;
	LineSegment	*next;
} LineSegment;


class LineListClass {
  public:
	LineListClass()		{ head = NULL; numLines = 0; };
	~LineListClass()	{ ClearAll(); };

	void AddLine( Tpoint *p1, Tpoint *p2, DWORD color );
	void DrawAll( Render3D *renderer );
	void ClearAll( void )	{ while (head) { LineSegment *p=head->next; delete head; head = p; } };

  private:
	LineSegment	*head;
	int			numLines;
};


#endif // _LINELIST_H_
