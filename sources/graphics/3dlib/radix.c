#include "radix.h"

static radix_sort_t *sortTable[512];

__inline void RadixReset(void )
{
	int i;
	for(i = 255; i >= 0; i--) sortTable[i] = 0UL;
}

__inline radix_sort_t **RadixFindFirst(radix_sort_t **ptr)
{
 	while(0UL == *ptr)
	{
	 	ptr++;
		if((unsigned int)ptr > (unsigned int)(sortTable+255)) return 0UL;
	}

	return(ptr);
}

__inline radix_sort_t **RadixRFindFirst(radix_sort_t **ptr)
{
 	while(0UL == *ptr)
	{
	 	ptr--;
		if((unsigned int)ptr < (unsigned int)sortTable) return 0UL;
	}

	return(ptr);
}

__inline radix_sort_t **RadixFindNext(radix_sort_t **ptr)
{
	do
	{
		ptr++;
		if((unsigned int)ptr > (unsigned int)(sortTable+255)) return 0UL;
	}
 	while(0UL == *ptr);

	return(ptr);
}

__inline radix_sort_t **RadixRFindNext(radix_sort_t **ptr)
{
	do
	{
		ptr--;
		if((unsigned int)ptr < (unsigned int)sortTable) return 0UL;
	}
 	while(0UL == *ptr);

	return(ptr);
}

__inline radix_sort_t *RadixRelink(void)
{
	radix_sort_t **scanh;
	radix_sort_t *head,*tail;

	scanh = RadixFindFirst(sortTable);
	head = scanh[0];
	scanh[0] = 0;
	tail = scanh[256];

	scanh = RadixFindNext(scanh);
	while(scanh != 0UL)
	{
		tail->pNext = scanh[0];
		scanh[0] = 0;
		tail = scanh[256];
		scanh = RadixFindNext(scanh);
	}

	return(head);
}

__inline radix_sort_t *RadixRRelink(void)
{
	radix_sort_t **scanh;
	radix_sort_t *head,*tail;

	scanh = RadixRFindFirst(sortTable+255);
	head = scanh[0];
	scanh[0] = 0;
	tail = scanh[256];

	scanh = RadixRFindNext(scanh);
	while(scanh != 0UL)
	{
		tail->pNext = scanh[0];
		tail = scanh[256];
		scanh[0] = 0;
		scanh = RadixRFindNext(scanh);
	}

	return(head);
}

radix_sort_t *RadixSortAscending(radix_sort_t *iptr, int offset)
{
	radix_sort_t *ptr,*nxt;
	unsigned char *byteval;
	int index;

	ptr = iptr;

	if(ptr->pNext == 0UL) return ptr;

	while(ptr != 0UL)
	{
		byteval = ((unsigned char *)ptr)+offset;
		index = *byteval;
		nxt = ptr->pNext;

		if(0UL == sortTable[index])
			sortTable[index] = ptr;
		else
			sortTable[index+256]->pNext = ptr;

		sortTable[index+256] = ptr;
		ptr->pNext = 0UL;
		ptr = nxt;
	}

	ptr = RadixRelink();

	offset++;

	while(ptr != 0UL )
	{
		byteval = ((unsigned char *)ptr)+offset;
		index = *byteval;
		nxt = ptr->pNext;

		if(0UL == sortTable[index])
			sortTable[index] = ptr;
		else
			sortTable[index+256]->pNext = ptr;

		sortTable[index+256] = ptr;
		ptr->pNext = 0UL;
		ptr = nxt;
	}

	ptr = RadixRelink();

	return(ptr);
}

radix_sort_t *RadixSortDescending(radix_sort_t *iptr, int offset)
{
	radix_sort_t *ptr,*nxt;
	unsigned char *byteval;
	int index;

	ptr = iptr;

	if(ptr->pNext == 0UL) return ptr;

	while(ptr != 0UL)
	{
		byteval = ((unsigned char *)ptr)+offset;
		index = *byteval;
		nxt = ptr->pNext;
		
		if(0UL == sortTable[index])
			sortTable[index] = ptr;
		else
			sortTable[index+256]->pNext = ptr;

		sortTable[index+256] = ptr;
		ptr->pNext = 0UL;
		ptr = nxt;
	}

	ptr = RadixRRelink();
	offset++;

	while(ptr != 0UL)
	{
		byteval = ((unsigned char *)ptr)+offset;
		index = *byteval;
		nxt = ptr->pNext;

		if(0UL == sortTable[index])
			sortTable[index] = ptr;
		else
			sortTable[index+256]->pNext = ptr;

		sortTable[index+256] = ptr;
		ptr->pNext = 0UL;
		ptr = nxt;
	}

	ptr = RadixRRelink();

	offset++;

	while(ptr != 0UL)
	{
		byteval = ((unsigned char *)ptr)+offset;
		index = *byteval;
		nxt = ptr->pNext;

		if(0UL == sortTable[index])
			sortTable[index] = ptr;
		else
			sortTable[index+256]->pNext = ptr;

		sortTable[index+256] = ptr;
		ptr->pNext = 0UL;
		ptr = nxt;
	}

	ptr = RadixRRelink();

	offset++;

	while(ptr != 0UL)
	{
		byteval = ((unsigned char *)ptr)+offset;
		index = *byteval;
		nxt = ptr->pNext;

		if(0UL == sortTable[index])
			sortTable[index] = ptr;
		else
			sortTable[index+256]->pNext = ptr;

		sortTable[index+256] = ptr;
		ptr->pNext = 0UL;
		ptr = nxt;
	}

	ptr = RadixRRelink();

	return(ptr);
}
