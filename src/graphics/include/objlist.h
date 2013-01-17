/***************************************************************************\
    ObjList.h
    Scott Randolph
    April 22, 1996

	Manage the list of active objects to be drawn each frame for a given
	renderer.
\***************************************************************************/
#ifndef _OBJLIST_H_
#define _OBJLIST_H_

#define _NUM_OBJECT_LISTS_ (5)

#include "DrawObj.h"


// This structure is used in the viewpoint sorting to move objects directly to the list
// they should go to, instead of only allowing them to go up or down one level at a time
struct TransportStr
{
	DrawableObject	*list[_NUM_OBJECT_LISTS_];
	float			bottom[_NUM_OBJECT_LISTS_];
	float			top[_NUM_OBJECT_LISTS_];
};

typedef struct UpdateCallBack {
	void(*fn)(void*, long, const Tpoint*, TransportStr*);
	void				*self;
	struct UpdateCallBack *prev;
	struct UpdateCallBack *next;
} UpdateCallBack;

typedef struct SortCallBack {
	void(*fn)(void*);
	void				*self;
	struct SortCallBack *prev;
	struct SortCallBack *next;
} SortCallBack;

class ObjectDisplayList {
  public:
	ObjectDisplayList();
	~ObjectDisplayList();

	void	Setup( void )	{};
	void	Cleanup( void )	{};

	void	InsertObject( DrawableObject *object );
	void	RemoveObject( DrawableObject *object );

	void	InsertUpdateCallbacks( UpdateCallBack*, SortCallBack*, void *self );
	void	RemoveUpdateCallbacks( UpdateCallBack*, SortCallBack*, void *self );

	void	UpdateMetrics(const Tpoint *pos); // do update without moving around in lists
	void	UpdateMetrics( long listNo, const Tpoint *pos, TransportStr *transList );
	void	SortForViewpoint( void );

	void	ResetTraversal( void )		{ nextToDraw = head; };
	float	GetNextDrawDistance( void )	{ if (nextToDraw) return nextToDraw->distance; else return -1.0f; };
	void	DrawBeyond( float ringDistance, int LOD, RenderOTW *renderer );
	void	DrawBeyond( float ringDistance, Render3D *renderer );

	DrawableObject*	GetNearest( void )			{ return tail; };
	DrawableObject*	GetNext( void )				{ return nextToDraw; };
	DrawableObject* GetNextAndAdvance( void )	{ DrawableObject *p = nextToDraw; if (nextToDraw) nextToDraw = nextToDraw->next;  return p; };
	void InsertionSortLink(DrawableObject **listhead, DrawableObject *listend);
	void QuickSortLink(DrawableObject **head, DrawableObject *end);

	// HANDLE WITH CARE...!!!!!
	// COBRA - RED - This function is used from a list manager to ask the object to kill itself...
	// RemoveMe() would just remove the object from the List, KillMe() would deallocate it too
	void	KillMe() { RemoveTheObject=KillTheObject=true; };
	void	RemoveMe() { RemoveTheObject=true; };

	// This function just preloads all objects in the List withing a range
	// it exits when the list of ojects is fully loaded
	void	PreLoad(class RenderOTW *renderer);

  protected:
	DrawableObject	*head;
	DrawableObject	*tail;
	DrawableObject	*nextToDraw;

	UpdateCallBack	*updateCBlist;
	SortCallBack	*sortCBlist;

	bool	KillTheObject, RemoveTheObject;
};

#endif // _OBJLIST_H_

