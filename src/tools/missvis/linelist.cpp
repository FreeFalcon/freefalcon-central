/***************************************************************************\
	LineList.cpp
	Scott Randolph
	December 4, 1997

	Keep a list of colored line segments and draw them.
\***************************************************************************/
#include "LineList.h"


LineListClass	TheLineList;


void LineListClass::AddLine( Tpoint *p1, Tpoint *p2, DWORD color )
{
	LineSegment *p = new LineSegment;

	p->p1 = *p1;
	p->p2 = *p2;
	p->color = color;
	p->next = head;

	head = p;
}


void LineListClass::DrawAll( Render3D *renderer )
{
	LineSegment *p = head;

	while (p) {
		renderer->SetColor( p->color );
		renderer->Render3DLine( &p->p1, &p->p2 );
		p = p->next;
	}
}
