#ifndef	ALLOC_Included
#define	ALLOC_Included

#define	DEFAULT_ALLOC 0

typedef struct { int dummy; } alloc_handle_t;

#ifdef __cplusplus
extern "C" {
#endif

extern alloc_handle_t *AllocInit(void);
extern alloc_handle_t *AllocSetPool(alloc_handle_t*);
extern char *Alloc(int);
extern void AllocDiscard(char*);
extern void AllocResetPool(void);
extern void AllocFreePool(void);

#ifdef __cplusplus
}
#endif

#endif
