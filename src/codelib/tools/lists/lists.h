/* ----------------------------------------------------
    LISTS.H
   ----------------------------------------------------
  
	Written by Kevin Ray	(c) 1994 Spectrum Holobyte.
   ---------------------------------------------------- */

#if !defined(__LISTS_H_INCLUDED)
#  define __LISTS_H_INCLUDED  1

#include "omni.h"	                      /* Get our configuration */



/*  -----------------------------------
         Configuration Options 
    -----------------------------------  */

#ifndef USE_LIST_ALLOCATIONS              /* should the list allocs be globbed into a pre-alloc'd table ? */
#  define   USE_LIST_ALLOCATIONS    YES
#endif

#ifndef USE_THREAD_SAFE                   /* should the list library be multithread approved ? */
#  define   USE_THREAD_SAFE         YES
#endif


/*  ---------------------------------- 
            Build as a DLL ?
    ----------------------------------  */

#ifdef _DLL_VERSION
#   define LST_EXPORT  CFUNC __declspec (dllexport)
#else
#   define LST_EXPORT  CFUNC
#endif



/*  ---------------------------------- 
       For convenient type coercion 
    ----------------------------------  */

#define LIST_APPEND(a,b)         ListAppend( (LIST*)(a), (void*)(b) )
#define LIST_APPEND_END(a,b)     ListAppendEnd( (LIST*)(a), (void*)(b) )
#define LIST_APPEND_SECOND(a,b)  ListAppendSecond( (LIST*)(a),(void*)(b) )
#define LIST_CATENATE(a,b)       ListCatenate( (LIST*)(a),(LIST*)(b) )
#define LIST_DESTROY(a,b)        ListDestroy( (LIST*)(a), b )
#define LIST_NTH(a,b)            ListNth( (LIST*)(a), (int)(b) )
#define LIST_COUNT(a)            ListCount( (LIST*)(a) )
#define LIST_WHERE(a,b)          ListWhere( (LIST*)(a), (void*)(b) )
#define LIST_REMOVE(a,b)         ListRemove( (LIST*)(a), (void*)(b) )
#define LIST_FIND(a,b)           ListFind( (LIST*)(a), (void*)(b)  )
#define LIST_SEARCH(a,b,c)       ListSearch( (LIST*)(a), (void*)(b), c )
#define LIST_DUP(a)              ListDup( (LIST*)(a) )
#define LIST_SORT(a, fn)         ListSort( (LIST**)(a), fn )



/*  -------------------------------------------------- 
       All singly linked lists have this structure
    --------------------------------------------------  */

typedef struct LIST
{
    void * node;          /* pointer to node data */
    void * user;          /* pointer to user data */

    struct LIST * next;   /* next list node */

} LIST;


#if( USE_LIST_ALLOCATIONS )

   LST_EXPORT void 
      ListValidate( void );

   LST_EXPORT void
      ListGlobalFree( void );

#endif /* USE_LIST_ALLOCATIONS */



LST_EXPORT LIST *
   ListAppend( LIST * list, void * node );

LST_EXPORT LIST *
   ListAppendEnd( LIST * list, void * node );

LST_EXPORT LIST *
   ListAppendSecond( LIST * list, void * node );

LST_EXPORT LIST *
   ListCatenate( LIST * l1, LIST * l2 );

LST_EXPORT LIST *
   ListNth( LIST * list, int n );

LST_EXPORT int 
   ListCount( LIST * list );

LST_EXPORT LIST *
   ListRemove( LIST * list, void * node );

LST_EXPORT LIST *
   ListFind( LIST * list, void * node );

LST_EXPORT void 
   ListDestroy( LIST * list, PFV destructor );

LST_EXPORT int
   ListWhere( LIST * list, void * node );

LST_EXPORT LIST *
   ListSearch( LIST * list, void * node, PFI func_ptr );

LST_EXPORT LIST *
   ListDup( LIST * list );

LST_EXPORT LIST *
   ListSort( LIST ** list, PFI func_ptr );

#endif /* __LISTS_H_INCLUDED */
