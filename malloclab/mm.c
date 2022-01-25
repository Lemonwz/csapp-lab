/*
 * mm-naive.c - implicit free list
 * 
 * In this approach, free blocks are implicitly linked by the size field
 * in the header or footer of blocks. Therefore, by iterating over all 
 * blocks in the heap, allocator can traverse all free blocks indirectly.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Timalloc",
    /* First member's full name */
    "Comethru",
    /* First member's email address */
    "https://github.com/Lemonwz/csapp-lab/tree/main/malloclab",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* WHFSIZE: single word and header/footer size (4 bytes) */
/* ALIGNMENT: double word size (8 bytes) */
/* CHUNKSIZE: extend heap by this amount (bytes) */
#define WHFSIZE   4
#define ALIGNMENT 8
#define CHUNKSIZE (1<<12)

#define MAX(x,y) ((x)>(y)?(x):(y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0X1)

/* Given block ptr bp, compute address of its header and footer */
/* bp points to the first payload byte */
#define HDRP(bp)  ((char *)(bp) - WHFSIZE)
#define FTRP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)) - ALIGNMENT)

/* Given block ptr bp, compute address of next and previous blocks  */
#define NEXT_BLKP(bp)  ((char *)bp + GET_SIZE(((char *)(bp) - WHFSIZE)))
#define PREV_BLKP(bp)  ((char *)bp - GET_SIZE(((char *)(bp) - ALIGNMENT)))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static char* heap_listp;
static char* prev_listp;

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *first_fit(size_t asize);
static void *next_fit(size_t asize);
static void *best_fit(size_t asize);
static void place(void* bp, size_t asize);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Create the initial empty heap */
    heap_listp = mem_sbrk(4*WHFSIZE);
    if(heap_listp == (void *)-1)
        return -1;
    PUT(heap_listp, 0); /* 填充字 */
    PUT(heap_listp + (1*WHFSIZE), PACK(ALIGNMENT,1)); /* 序言块头部 */
    PUT(heap_listp + (2*WHFSIZE), PACK(ALIGNMENT,1)); /* 序言块尾部 */
    PUT(heap_listp + (3*WHFSIZE), PACK(0,1)); /* 结尾块 */
    heap_listp += (2*WHFSIZE);
    prev_listp = heap_listp;

    /* 将堆扩展chunksize字节并创建初始空闲块*/
    if(extend_heap(CHUNKSIZE/WHFSIZE) == NULL){
        return -1;
    }
    return 0;
}

/*
 * extend heap with a new free block
 */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* 扩展偶数字的空间以满足双字对齐的要求 */
    size = (words % 2) ? (words+1)*(WHFSIZE) : words*(WHFSIZE);
    bp = mem_sbrk(size);
    if(bp == (void *)-1)
        return NULL;
    
    /* 初始化空闲块的头部和尾部 */
    /* 结尾块的头部变成了新空闲块的头部，新空闲块的最后一个字是新的结尾块的头部*/
    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));
    return coalesce(bp);
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if(size == 0)
        return NULL;

    asize = ALIGN(size + SIZE_T_SIZE);
    // bp = first_fit(asize);
    bp = next_fit(asize);
    // bp = best_fit(asize);
    if(bp != NULL){
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WHFSIZE)) == NULL){
        return NULL;
    }
    place(bp, asize);
    return bp;
    
}

/*
 * first fit： search and return the first available free block
 */
static void *first_fit(size_t asize)
{
    void *bp;
    for(bp = heap_listp; GET_SIZE(HDRP(bp))>0; bp = NEXT_BLKP(bp)){
        if(!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp))>=asize){
            return bp;
        }
    }
    return NULL;
}

/*
 * next fit： start search available free block from the last ended 
 */
static void *next_fit(size_t asize)
{
    void *bp;
    for(bp = prev_listp; GET_SIZE(HDRP(bp))>0; bp = NEXT_BLKP(bp)){
        if(!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp))>=asize){
            prev_listp = bp;
            return bp;
        }
    }

    for(bp = heap_listp; bp!=prev_listp; bp = NEXT_BLKP(bp)){
        if(!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp))>=asize){
            prev_listp = bp;
            return bp;
        }
    }
    
    return NULL;
}

/*
 * best fit： search and return the smallest available free block
*/
static void *best_fit(size_t asize)
{
    void *bestbp = NULL;
    size_t bestsize = 1<<30;
    for(char *bp = heap_listp; GET_SIZE(HDRP(bp))>0; bp = NEXT_BLKP(bp)){
        if(!GET_ALLOC(HDRP(bp))){
            size_t freesize = GET_SIZE(HDRP(bp));
            if(freesize >= asize && freesize < bestsize){
                bestsize = freesize;
                bestbp = bp;
            }
        }
    }
    return bestbp;
}

/*
 * place the request block at the beginning of the free block 
 */
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    if((csize-asize) >= 2*ALIGNMENT){
        PUT(HDRP(bp), PACK(asize,1));
        PUT(FTRP(bp), PACK(asize,1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize,0));
        PUT(FTRP(bp), PACK(csize-asize,0));
    } else {
        PUT(HDRP(bp), PACK(csize,1));
        PUT(FTRP(bp), PACK(csize,1));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size,0));
    PUT(FTRP(ptr), PACK(size,0));
    coalesce(ptr);
}

/*
 * coalesce if both current block and next/prev block are free
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t prev_size = GET_SIZE(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    /* case 1: both prev and next blocks are allocted */
    if(prev_alloc && next_alloc)
    {
        return bp;
    }

    /* case 2: prev block is allocated, next block is free */
    if(prev_alloc && !next_alloc)
    {
        PUT(HDRP(bp), PACK(size+next_size,0));
        PUT(FTRP(bp), PACK(size+next_size,0));
    }
    
    /* case 3: next block is allocated, prev block is free */
    if(!prev_alloc && next_alloc)
    {
        PUT(FTRP(bp), PACK(size+prev_size,0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size+prev_size,0));
        bp = PREV_BLKP(bp);
    }

    /* case 4: both prev and next blocks are free */
    if(!prev_alloc && !next_alloc)
    {
        PUT(HDRP(PREV_BLKP(bp)), PACK(size+prev_size+next_size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size+prev_size+next_size,0));
        bp = PREV_BLKP(bp);
    }
    prev_listp = bp;
    return bp;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t newsize)
{
    /*
     * case 1: ptr==NULL, equal to mm_malloc(nsize)
     * case 2: nsize==0, equal to mm_free(ptr)
     * case 3: ptr not null, size nonzero. old block size(osize), new block size(nsize)
     * case 3.1: osize > nsize, the nsize bytes are identified to the first nsize bytes in old block
     * 如果osize-nsize >= 2*ALIGNMENT(16), 则可以分割old block得到空闲块
     * case 3.2: osize < nsize, the first osize bytes are identical, and last nsize-osize bytes are uninitialized
     * 如果old block的下一个块是空闲的且osize+nsize >= nsize, 则可以将old block和next block合并
     * 且合并后需要检查一下合并后的块能不能分割出空闲块
     * case 3.3: malloc a new block and free the old block
     */

    /* case 1: ptr == NULL */
    if(ptr == NULL)
        return mm_malloc(newsize);

    /* case 2: newsize == 0 */
    if(newsize == 0){
        mm_free(ptr);
        return NULL;
    }

    void *oldptr = ptr;
    void *newptr;
    size_t oldsize;
    size_t copySize;

    newsize = ALIGN(newsize + SIZE_T_SIZE);
    oldsize = GET_SIZE(HDRP(oldptr));
    /* newsize == oldsize, 直接返回即可 */
    if(newsize == oldsize)
        return oldptr;
    
    /* case 3.1: 缩小且old block可分割出空闲块 */
    if(newsize < oldsize && oldsize-newsize>=2*ALIGNMENT){
        place(oldptr, newsize);
        return oldptr;
    }
    
    /* case 3.2: 扩大且next block未分配 */
    if(newsize > oldsize && !GET_ALLOC(HDRP(NEXT_BLKP(oldptr)))){
        size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(oldptr)));
        /* 当前块+后一个块的大小能够满足realloc要求，合并 */
        if(oldsize+next_size >= newsize){
            PUT(HDRP(oldptr), PACK(oldsize+next_size,1));
            PUT(FTRP(oldptr), PACK(oldsize+next_size,1));
            /* 检查合并后是否还能分割出空闲块 */
            if(oldsize+next_size-newsize >= 2*ALIGNMENT) {
                place(oldptr, newsize);
            }
            return oldptr;
        }
    }

    /* case 3.3: 重新分配 */
    if((newptr=mm_malloc(newsize))== NULL){
        printf("mm_malloc error in mm_remalloc\n");
        return NULL;
    }
    copySize = oldsize<newsize?oldsize:newsize;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














