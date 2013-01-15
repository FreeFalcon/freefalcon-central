#ifndef	RADIX_Included
#define	RADIX_Included

#define	DEFAULT_RADIX 0

typedef struct radix_sort_s
{
	struct radix_sort_s *pNext;
}
radix_sort_t;

#ifdef __cplusplus
extern "C" {
#endif

extern void RadixReset(void);
extern radix_sort_t *RadixSortAscending(radix_sort_t*, int);
extern radix_sort_t *RadixSortDescending(radix_sort_t*, int);

#ifdef __cplusplus
}
#endif

#endif
