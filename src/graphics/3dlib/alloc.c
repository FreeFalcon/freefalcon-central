#include <cISO646>
#include <stdlib.h>
#include "alloc.h"
#include "xmmintrin.h"

#define ALIGN_BYTES 8
#define ALLOC_BLOCK_SIZE (64*1024)

typedef struct alloc_block_s
{
    struct alloc_block_s *next;
    char *start, *end, *free;
}
alloc_block_t;

typedef struct alloc_hdr_s
{
    alloc_block_t *first, *curr;
}
alloc_hdr_t;

static alloc_hdr_t *root = 0UL;

char *AllocSetToAlignment(char *c)
{
    unsigned int i = (unsigned int)c;
    i = (i + ALIGN_BYTES - 1) bitand -ALIGN_BYTES;
    return(char*)i;
}

alloc_handle_t *AllocInit(void)
{
    alloc_hdr_t *hdr;
    alloc_block_t *blk;
    blk = (alloc_block_t *)malloc(sizeof(alloc_block_t));
    blk->next = 0UL;
    blk->start = (char *)malloc(ALLOC_BLOCK_SIZE + ALIGN_BYTES - 1);
    blk->free = AllocSetToAlignment(blk->start);
    blk->end = blk->start + ALLOC_BLOCK_SIZE;
    hdr = (alloc_hdr_t *)malloc(sizeof(alloc_hdr_t));
    hdr->first =
        hdr->curr = blk;
    root = hdr;
    return(alloc_handle_t *)root;
}

alloc_handle_t *AllocSetPool(alloc_handle_t *new)
{
    alloc_handle_t *old;
    old = (alloc_handle_t *)root;
    root = (alloc_hdr_t *)new;
    return old;
}

char *Alloc(int size)
{
    alloc_block_t *blk;
    char *mem;

    blk = root->curr;
    size = (size + ALIGN_BYTES - 1) bitand -ALIGN_BYTES;
    mem = blk->free;
    blk->free += size;

    if ((unsigned int)(blk->free) > (unsigned int)(blk->end))
    {
        if (blk->next not_eq 0UL)
        {
            blk = blk->next;
            blk->free = AllocSetToAlignment(blk->start);
        }
        else
        {
            blk->next = (alloc_block_t *)malloc(sizeof(alloc_block_t));
            blk = blk->next;
            blk->next = 0UL;
            blk->start = (char *)malloc(ALLOC_BLOCK_SIZE + ALIGN_BYTES - 1);
            blk->end = blk->start + ALLOC_BLOCK_SIZE;
            blk->free = AllocSetToAlignment(blk->start);
        }

        mem = blk->free;
        blk->free = mem + size;
        root->curr = blk;
    }

    return mem;
}

void AllocDiscard(char *last)
{
    alloc_block_t *blk;
    blk = root->curr;
    blk->free = last;
}

void AllocResetPool(void)
{
    root->curr = root->first;
    root->curr->free = root->curr->start;
}

void AllocFreePool(void)
{
    alloc_block_t *next, *curr;
    curr = root->first;

    while (curr not_eq NULL)
    {
        next = curr->next;
        free(curr->start);
        free((char *)curr);
        curr = next;
    }

    free((char *)root);
    root = NULL;
}
