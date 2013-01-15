/* ----------------------------------------------------------

   LISTS.CPP

   List functions.

   AUTHORS:
     KBR   08/18/94   implemented at spectrum holobyte
     KBR   10/30/94   list allocation blocks
     KBR   05/01/96   ported to win95 (non-specific port) 
     KBR   12/03/96   added thread-safe code
   ---------------------------------------------------------- */
  
#include <stdio.h>
#include <stdlib.h>

#include "lists.h"
#if MEM_ENABLED
#    include "memmgr.h"
#endif

#define ALLOC_UNITS           512
#define ALLOC_SAFETY          20
#define ALLOC_USED_FLAG       0x53554445  /* 'U'S'E'D' */
#define ALLOC_FREE_FLAG       0x52464545  /* 'F'R'E'E' */
#define ALLOC_SWAP_SIZE       64


#ifdef USE_SH_POOLS
	#undef MemFree
	#undef MemFreePtr
	#undef MemMalloc
	#include "Smartheap\Include\smrtheap.h"
	extern MEM_POOL gResmgrMemPool;
	#  define LIST_ALLOC()  MemAllocPtr( gResmgrMemPool, sizeof(LIST), 0 )
	#  define LIST_FREE(a)  MemFreePtr(a)
#else
	#if( USE_LIST_ALLOCATION )
	#  define LIST_ALLOC()  ListAlloc()
	#  define LIST_FREE(a)  ListFree(a)
	#else
	#  define LIST_ALLOC()  MemMalloc( sizeof(LIST), "LIST" )
	#  define LIST_FREE(a)  MemFree(a)
	#endif
#endif


#if( USE_THREAD_SAFE )

   int LIST_MUTEX = 0;         /* If you want to bypass microsoft's very heavy
                                  mutex implementation, do so here. */

#  define WAIT_FOR_LOCK(a)     WaitForSingleObject( a, INFINITE );
#  define RELEASE_LOCK(a)      ReleaseMutex( a );
#  define CREATE_LOCK(a)       { if( !a ) a = CreateMutex( NULL, FALSE, NULL ); \
                                 if( !a ) KEVS_FATAL_ERROR( "Could not get mutex lock." ); }
#else
#  define WAIT_FOR_LOCK(a)     
#  define RELEASE_LOCK(a)      
#  define CREATE_LOCK(a)       
#endif


#if( USE_LIST_ALLOCATION )


/*
 -------------------------------------------------------
            LIST ALLOCATION MEMORY POOLS
 -------------------------------------------------------

  Allocate all list related memory from memory pools

     (... per Roger Fuji's bitchin' & moanin! )


  Allocation Strategy:

  Memory is doled out for LISTs from large blocks of
  memory called 'POOL's.

  The pools are actually ALLOC_UNIT structs.
  The unit of memory that is provided to the caller  
  (and retrieved from a pool) is a LIST_UNIT.        
                                                     
  When a significant number of the LIST_UNITs have   
  been allocated from a pool, a new pool is created. 

  Additionally, LIST_UNITs are provided sequentially.
  eg; unit 1,2,3...  however towards the end of the  
  series, it is quite probable that earlier units    
  have been released.  At this point (when we are    
  doling out indices > ALLOC_SAFETY) we pack the     
  different pools.

  I'm sorry to admit, however, that pack - does not. 
  There is no clean way to pack the memory without   
  substantially vulgarizing the LIST type.

  So...  What I do is,

  a) see if any old pools now have 100% of their     
     units released. if so, free that pool.          

  b) see if any old pools now have more available    
     memory than our current pool, if so swap.       
     >>note: this one may not be a good idea.<<

  d) if this pool still has a lot of available       
     memory, reset our current index to 0.  This     
     will cause us to count up, looking for free     
     units - thus filling in spaces on a first come  
     basis.

  c) if we have to (eg; avail < ALLOC_SAFETY) we 
     allocate another pool.
 -------------------------------------------------------
      ROGERS' LAME O' HACK ADDED ON OCTOBER 3RD      
 -------------------------------------------------------
*/

typedef struct LIST_UNIT
{
   int     check;                   /* Has this unit been provided as a fulfillment ?   */
   void  * ptr_a;                   /* node                                             */
   void  * ptr_b;                   /* roger's lame o' hack                             */
   void  * ptr_c;                   /* next                                             */

} LIST_UNIT;

typedef struct ALLOC_UNIT
{
   LIST_UNIT unit[ ALLOC_UNITS ];   /* The actual table of memory to dole out as LIST * */

   int    index;                    /* Our current location in the table                */
   int    avail;                    /* Number of unit[]'s available from the table      */

   int    sleeping;                 /* Has this block been relegated to hibernation     */

   long   timer;                    /* Always nice to have a timer                      */

   void * min;                      /* Convenience for finding the correct block to     */
   void * max;                      /* perform a free from.                             */

   ALLOC_UNIT * next;               /* Link the allocation blocks                       */
   ALLOC_UNIT * prev;               /* Link the allocation blocks                       */

} ALLOC_UNIT;


PRIVATE
ALLOC_UNIT * 
   GLOBAL_ALLOC_TABLE = NULL;


PRIVATE
void
   ListGlobalPack( void ),
   ListGlobalAlloc( void );

PRIVATE
void
   ListFree( void * unit );

PRIVATE
void *
   ListAlloc( void );

#endif





/* ---------------------------------------------------------------------
   ---------------------------------------------------------------------

   L I S T   A L L O C A T I O N   F U N C T I O N S

   ---------------------------------------------------------------------
   --------------------------------------------------------------------- */


#if( USE_LIST_ALLOCATION )

PUBLIC
void
ListGlobalFree( void )
{
   ALLOC_UNIT * table, *tblptr;

   WAIT_FOR_LOCK( LIST_MUTEX );

   if( !GLOBAL_ALLOC_TABLE )
      return;

   table = GLOBAL_ALLOC_TABLE;

   while( table -> prev )
      table = table -> prev;

   for( ; table ;  ) 
   {
      tblptr = table -> next;
	  #ifdef USE_SH_POOLS
      MemFreePtr( table );
	  #else
      MemFree( table );
	  #endif
      table = tblptr;
   }

   GLOBAL_ALLOC_TABLE = NULL;

   RELEASE_LOCK( LIST_MUTEX );
}


PRIVATE
void 
ListGlobalAlloc( void )
{
   int   i;

   ALLOC_UNIT * new_unit = NULL;
   ALLOC_UNIT * old_unit = NULL;

   WAIT_FOR_LOCK( LIST_MUTEX, INFINITE );

   #ifdef USE_SH_POOLS
   new_unit = (ALLOC_UNIT *)MemAllocPtr( gResmmgrMemPool, sizeof( ALLOC_UNIT ), 0 );
   #else
   new_unit = (ALLOC_UNIT *)MemMalloc( sizeof( ALLOC_UNIT ), "LIST_MEM" );
   #endif

   if( !new_unit )
      KEVS_FATAL_ERROR( "No memory for list pool." );

   if( GLOBAL_ALLOC_TABLE )
   {
      old_unit = GLOBAL_ALLOC_TABLE;

      while( old_unit -> next )
         old_unit = old_unit -> next;
      
      old_unit -> next = new_unit;
      new_unit -> prev = old_unit;
      new_unit -> next = NULL;

      old_unit -> sleeping = TRUE;
   }
   else
   {
      new_unit -> next = NULL;
      new_unit -> prev = NULL;
   }

   new_unit -> sleeping = FALSE;

   new_unit -> avail = ALLOC_UNITS;
   new_unit -> index = 0;
   new_unit -> timer = TIME_COUNT;

   new_unit -> min = &new_unit -> unit[0].ptr_a;
   new_unit -> max = &new_unit -> unit[ALLOC_UNITS].ptr_a;

   for( i=0; i<ALLOC_UNITS; i++ )
      new_unit -> unit[i].check = ALLOC_FREE_FLAG;

   GLOBAL_ALLOC_TABLE = new_unit;

   RELEASE_LOCK( LIST_MUTEX );
}


PRIVATE
void *
ListAlloc( void )
{
   LIST_UNIT * lu;

   CREATE_LOCK( LIST_MUTEX );

   WAIT_FOR_LOCK( LIST_MUTEX );

   if( !GLOBAL_ALLOC_TABLE )
      ListGlobalAlloc();

   do
   {
      if ( ( GLOBAL_ALLOC_TABLE -> avail < ALLOC_SAFETY) ||
           ( GLOBAL_ALLOC_TABLE -> index > (ALLOC_UNITS - ALLOC_SAFETY)) )
 
          ListGlobalPack();
      
      lu = (LIST_UNIT *)(&GLOBAL_ALLOC_TABLE -> unit[ GLOBAL_ALLOC_TABLE -> index ]);

      GLOBAL_ALLOC_TABLE -> index++;

   } while( lu -> check != ALLOC_FREE_FLAG );

   lu -> check = ALLOC_USED_FLAG;

   GLOBAL_ALLOC_TABLE -> timer = TIME_COUNT;
   GLOBAL_ALLOC_TABLE -> avail--;

   RELEASE_LOCK( LIST_MUTEX );

   return ( &lu -> ptr_a );
}


PUBLIC
void
ListValidate( void )
{
   ALLOC_UNIT * tmp   = NULL;
   ALLOC_UNIT * table = NULL;

   int i;

   WAIT_FOR_LOCK( LIST_MUTEX );

   if( !GLOBAL_ALLOC_TABLE )
      return;

   for( tmp = GLOBAL_ALLOC_TABLE; tmp; tmp = (ALLOC_UNIT *)table -> prev )
      table = tmp;

   for( tmp = table; tmp; tmp = tmp -> next )
      for( i=0; i<ALLOC_UNITS; i++ )
         if( (tmp -> unit[i].check != ALLOC_FREE_FLAG ) &&
             (tmp -> unit[i].check != ALLOC_USED_FLAG ))
         {
            DBG(PF( "ERROR: Possible overwrite in lists." ));

            DBG(PF( "Unit Address: %x index: %d\n", &tmp -> unit[i] ));
            DBG(PF( "------------------------------------------\n"  ));
            DBG(PF( "node: %x\n", tmp -> unit[i].ptr_a ));
            DBG(PF( "user: %x\n", tmp -> unit[i].ptr_b ));
            DBG(PF( "next: %x\n", tmp -> unit[i].ptr_c ));
         }

   RELEASE_LOCK( LIST_MUTEX );
}


PRIVATE
void
ListFree( void * unit )
{
   int done = FALSE;

   LIST_UNIT * lu;

   ALLOC_UNIT * t   = NULL;
   ALLOC_UNIT * tbl = NULL;

   WAIT_FOR_LOCK( LIST_MUTEX );

   for( t = GLOBAL_ALLOC_TABLE; t; t = (ALLOC_UNIT *)tbl -> prev )
      tbl = t;

   if( !tbl )
   {
      DBG(PF( "Error freeing list structure.\n" ));
      RELEASE_LOCK( LIST_MUTEX );
      return;
   }

   lu = (LIST_UNIT *)( (int) unit - sizeof( int ) );

   if( lu -> check != ALLOC_USED_FLAG )
   {
      ERROR( "Free of a corrupt list node from allocation table." );
      RELEASE_LOCK( LIST_MUTEX );
      return;
   }

   do
   {
      if ( (unit >= tbl -> min) && (unit < tbl -> max) )
      {
         tbl -> avail++;

         lu -> check = ALLOC_FREE_FLAG;

         lu -> ptr_a = NULL;
         lu -> ptr_b = NULL;
         lu -> ptr_c = NULL;

         done = TRUE;

         break;
      }
      else
      {
         tbl = (ALLOC_UNIT *)tbl -> next;
      }

   } while( tbl && !done );

   if ( !done )
      ERROR( "Couldn't find list in allocation table\n" );

   RELEASE_LOCK( LIST_MUTEX );
}


PRIVATE
void
ListGlobalPack( void )
{
   int done  = FALSE;
   int total = 0;
   

   ALLOC_UNIT * t,
              * tbl = NULL;

   WAIT_FOR_LOCK( LIST_MUTEX );

   for( t = GLOBAL_ALLOC_TABLE; t; t = (ALLOC_UNIT *)t -> prev )
      tbl = t;

   if (!tbl)
   {
      ERROR( "List allocation table empty -- cannot pack." ); 
      GLOBAL_ALLOC_TABLE -> index = 0;
      RELEASE_LOCK( LIST_MUTEX );
      return;
   }

   do
   {
      if( (tbl -> avail == ALLOC_UNITS) &&
          (tbl -> sleeping) )
      {
         if( tbl -> prev )
            tbl -> prev -> next = tbl -> next;
         if( tbl -> next )
            tbl -> next -> prev = tbl -> prev;

         total += (ALLOC_UNITS - tbl -> avail);

         if( tbl -> prev )
            t = tbl -> prev;
         else
            t = tbl -> next;   

         if( GLOBAL_ALLOC_TABLE == tbl ) 
            GLOBAL_ALLOC_TABLE = t;

		  #ifdef USE_SH_POOLS
		  MemFreePtr( tbl );
		  #else
		  MemFree( tbl );
		  #endif
         tbl = t;
      }
      else
      {
         if( tbl -> avail > (GLOBAL_ALLOC_TABLE -> avail + ALLOC_SWAP_SIZE) ) {
            GLOBAL_ALLOC_TABLE = tbl;
         }
         total += (ALLOC_UNITS - tbl -> avail);
         tbl = tbl -> next;
      }

   } while( tbl );

   if( !GLOBAL_ALLOC_TABLE || (GLOBAL_ALLOC_TABLE -> avail < ALLOC_SWAP_SIZE) )
      ListGlobalAlloc();

   GLOBAL_ALLOC_TABLE -> index = 0;

   RELEASE_LOCK( LIST_MUTEX );
}

#endif /* USE_LIST_ALLOCATION */








/* ------------------------------------------------------------------------------------
   ------------------------------------------------------------------------------------

   C O R E   L I S T   F U N C T I O N S

   ------------------------------------------------------------------------------------
   ------------------------------------------------------------------------------------ */


/*
 * append new node to front of list 
 * return pointer to new list head
 * caller should cast returned value to appropriate type
 */


LST_EXPORT LIST *
ListAppend( LIST * list, void * node )
{
   LIST * newnode;

   newnode = (LIST *)LIST_ALLOC();

   newnode -> node = node;
   newnode -> next = list;

   return( newnode );
}                         


/*
 * append new node to end of list
 * caller should cast returned value to appropriate type
 */

LST_EXPORT LIST *
ListAppendEnd( LIST * list, void * node )
{
   LIST * newnode;
   LIST * curr;

   newnode = (LIST *)LIST_ALLOC();

   newnode -> node = node;
   newnode -> next = NULL;

   /* list was null */
   if ( !list ) 
   {
     list = newnode;
   }
   else 
   {
      /* find end of list */
      for( curr=list ; curr -> next != NULL ; curr = curr -> next ) ;

      /* chain in at end */
      curr -> next = newnode;
   }

   return( list );
}

/*
 * append new node after first element, assumes list was non-null
 * caller should cast returned value to appropriate type
 * used to global lists, without disturbing the pointer to 
 * the head of such lists.
 */

LST_EXPORT LIST *
ListAppendSecond( LIST * list, void * node )
{
   LIST * newnode;

   newnode = (LIST *)LIST_ALLOC();

   newnode -> node = node;

   /* chain in after first element */
   newnode -> next = list -> next;

   list -> next = newnode;

   /* return original head unchanged */
   return( list );
}


/*
 * join 2 lists, add l2 at end of l1
 */

LST_EXPORT LIST *
ListCatenate( LIST * l1, LIST * l2 )
{
   LIST *curr;

   if ( !l1 )
      return l2;

   if ( !l2 )
      return l1;

   /* find last element of l1 */
   for( curr = l1; curr -> next != NULL; curr = curr -> next );

   /* catenate */
   curr -> next = l2;

   return l1;
}


/*
 * destroy a list
 * optionally free the data pointed to by node, using supplied destructor fn
 * If destructor is NULL, node data not affected, only list nodes get freed
 */

LST_EXPORT void 
ListDestroy( LIST * list, PFV destructor )
{
   LIST * prev,
        * curr;

   if ( !list )
      return;

   prev = list;
   curr = list -> next;

   while ( curr )
   {
      if ( destructor )
         (*destructor)(prev -> node);

      prev -> next = NULL;

      LIST_FREE( prev );

      prev = curr;
      curr = curr -> next;
   }

   if( destructor )
      (*destructor)( prev -> node );

   prev -> next = NULL;

   LIST_FREE( prev );

   //ListGlobalPack();
}

     
/*
 * return pointer to nth element in list 
 * return NULL if no such element
 */


LST_EXPORT LIST *
ListNth( LIST * list, int n )
{
   int    i;
   LIST * curr;

   curr = list;

   for( i=0 ; i<n && curr; i++ )
      curr = curr -> next;

   return( curr );
}


/*
 * return Number of entries in list
 */

LST_EXPORT int
ListCount( LIST * list)
{
   LIST * curr;
   int i;

   for( i = 0, curr = list; curr; i++, curr = curr  ->  next ) 
       ;
   
   return( i );
}


/*
 * return index of given list element
 * return -1 if not found
 */

LST_EXPORT int
ListWhere( LIST * list, void * node )
{
   LIST * curr;
   int i;

   for( i = 0, curr = list; curr; i++, curr = curr  ->  next ) 
   {
      if ( curr -> node == node )
         return( i );
   }
   
   /* not found */
   return( -1 );
}


/*
 * remove a node from a list, iff it is found
 * return shortened list
 */

LST_EXPORT LIST *
ListRemove( LIST * list, void * node )
{
   LIST * prev,
        * curr;

   if ( !list )
      return( NULL );

   prev = NULL;
   curr = list;

   while( curr && (curr -> node != node) )
   {
      prev = curr;
      curr = curr -> next;
   }

   /* not found, return list unmodified */
   if ( !curr )
      return( list );

   /* found at head */
   if ( !prev )
      list = list -> next;
   else
      prev -> next = curr -> next;
   
   curr -> next = NULL;

   LIST_FREE( curr );

   return( list );
}


/*
 * return pointer to list node if found, else NULL
 */

LST_EXPORT LIST *
ListFind( LIST * list, void * node )
{
   LIST * curr;

   for( curr = list; curr; curr = curr -> next ) 
   {
      if ( curr -> node == node )
         return curr;
   }
   
   return( NULL );
}

LST_EXPORT LIST *
ListSearch( LIST * list, void * node, PFI func_ptr )
{
   LIST * l;

   for( l =  list; list; list = list -> next )
      if( !((*func_ptr)( list -> node, node )) )
         return( l );

   return( NULL );
}


LST_EXPORT LIST *
ListDup( LIST * list )
{
   LIST * newlist;

   newlist = (LIST *)LIST_ALLOC();

   newlist -> next = NULL;
   newlist -> node = list -> node;
   newlist -> user = list -> user;

   return (newlist);
}


LST_EXPORT LIST *
ListSort( LIST ** list, PFI func_ptr )
{
   LIST **parent_a;
   LIST **parent_b;

   for( parent_a = list; *parent_a; parent_a = &(*parent_a) -> next )
   {
      for( parent_b = &(*parent_a) -> next; *parent_b; parent_b = &(*parent_b) -> next )
      {
         if( func_ptr(*parent_a, *parent_b) > 0 )
         {
            LIST *swap_a, *swap_a_child;
            LIST *swap_b, *swap_b_child;

            swap_a       = *parent_a;
            swap_a_child = (*parent_a) -> next;
            swap_b       = *parent_b;
            swap_b_child = (*parent_b) -> next;

            (*parent_a) -> next = swap_b_child;
            (*parent_a)         = swap_b;

            if( swap_b == swap_a_child )
            {
                (*parent_a) -> next = swap_a;
                parent_b            = &(*parent_a) -> next;
            }
            else {
                (*parent_b) -> next = swap_a_child;
                (*parent_b)         = swap_a;
            }
         }

      }
   }

   return( *list );
}
