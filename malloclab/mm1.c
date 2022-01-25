/*
 * mm1.c - explicit free lists + segregated free lists(segreated fits)
 * 
 * In this approach, organize the heap as a explicit doubly linked free list,
 * there is a pred/succ pointer in each free block to find the prev/next free
 * block. Segregated free list means maintain multiple free lists and blocks
 * in each free list have similar size (size class). 
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm1.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
// team_t team1 = {
//     /* Team name */
//     "Timalloc",
//     /* First member's full name */
//     "Comethru",
//     /* First member's email address */
//     "https://github.com/Lemonwz/csapp-lab/tree/main/malloclab",
//     /* Second member's full name (leave blank if none) */
//     "",
//     /* Second member's email address (leave blank if none) */
//     ""
// };

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* word size and double word size */
#define WSIZE 4
#define DSIZE 8

/* initial and normal heap extend size */
#define INIT_CHUNKSIZE (1<<6)
#define NORMAL_CHUNKSIZE (1<<12)

/* size of free list array */
#define LIST_SIZE 32

/* max and min helper function */
#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

/* Read size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0X1)

/* Given block ptr bp, compute address of its header and footer */
/* bp points to the first payload byte */
#define HDRP(bp)  ((char *)(bp) - WSIZE)
#define FTRP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks  */
#define NEXT_BLKP(bp)  ((char *)bp + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)bp - GET_SIZE(((char *)(bp) - DSIZE)))

/* Set pointer p points to ptr */
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

/* Given free block ptr fbp, compute address of its pred and succ ptr */
#define PRED_PTR(fbp) ((char *)(fbp))
#define SUCC_PTR(fbp) ((char *)(fbp) + WSIZE)

/* Given free block ptr fbp, compute address of pred and succ free blocks */
#define PRED_FB(fbp) (*(char **)(fbp))
#define SUCC_FB(fbp) (*(char **)(SUCC_PTR(fbp)))

/* 
 * {1},        {2-3},       {4-7},       {8-15}
 * {16-31},    {32-63},     {64-127},    {128-255},   {256-511}
 * {512-1023}, {1024-2047}, {2048-4095}, {4096-8191}, {8192-16383} ......
*/
void *segregated_free_list[LIST_SIZE];

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *first_fit(size_t asize);
static void *place(void* fbp, size_t asize);
static void insert_node(void *fbp, size_t size);
static void delete_node(void *fbp);

/* 
 * mm_init - initialize the malloc package.
 */
int mm1_init(void)
{
    /* initialize segregated free list */
    int listidx;
    for(listidx=0; listidx<LIST_SIZE; ++listidx) {
        segregated_free_list[listidx] = NULL;
    }

    /* c0-reate the initial empty heap */
    char *heap;
    if((heap=mem_sbrk(4*WSIZE)) == (void *)-1){
        return -1;
    }

    PUT(heap, 0);
    PUT(heap+(1*WSIZE), PACK(DSIZE,1));
    PUT(heap+(2*WSIZE), PACK(DSIZE,1));
    PUT(heap+(3*WSIZE), PACK(0,1));

    /* 将堆扩展chunksize字节并创建初始空闲块*/
    if(extend_heap(INIT_CHUNKSIZE/WSIZE) == NULL){
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

    size = (words % 2) ? (words+1)*(WSIZE) : words*(WSIZE);
    if((bp=mem_sbrk(size)) == (void *)-1){
        return NULL;
    }
    
    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));

    insert_node(bp, size);
    return coalesce(bp);
}

/*
 * 
 */
void mm1_free (void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size,0));
    PUT(FTRP(ptr), PACK(size,0));

    insert_node(ptr, size);
    coalesce(ptr);
}

/*
 * coalesce if both current block and next/prev block are free
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t prev_size = GET_SIZE(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc) {
        return bp;
    }

    if(!prev_alloc && next_alloc) {
        delete_node(bp);
        delete_node(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size+prev_size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size+prev_size, 0));
        bp = PREV_BLKP(bp);
    }

    if(prev_alloc && !next_alloc) {
        delete_node(bp);
        delete_node(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size+next_size, 0));
        PUT(FTRP(bp), PACK(size+next_size, 0));
    }

    if(!prev_alloc && !next_alloc) {
        delete_node(bp);
        delete_node(PREV_BLKP(bp));
        delete_node(NEXT_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size+prev_size+next_size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size+prev_size+next_size, 0));
        bp = PREV_BLKP(bp);
    }

    insert_node(bp, GET_SIZE(HDRP(bp)));
    return bp;
}

/*
 * 
 */
void *mm1_malloc (size_t size)
{
    size_t asize;
    size_t extendsize;
    void *bp;

    if(size == 0) {
        return NULL;
    }

    asize = ALIGN(size + SIZE_T_SIZE);
    bp = first_fit(asize);
    if(bp == NULL) {
        extendsize = MAX(asize, NORMAL_CHUNKSIZE);
        if((bp=extend_heap(extendsize/WSIZE)) == NULL) {
            return NULL;
        }
    }

    bp = place(bp, asize);
    return bp;
}

/*
 * first fit： find and return the first available free block
 */
static void *first_fit(size_t asize)
{
    void *fbp;
    int listidx;

    listidx = (1<<5) - __builtin_clz(asize) - 1;
    while(listidx < LIST_SIZE) {
        fbp = segregated_free_list[listidx];
        while((fbp != NULL) && (asize > GET_SIZE(HDRP(fbp)))) {
            fbp = PRED_FB(fbp);
        }
        if(fbp != NULL) {
            return fbp;
        }
        listidx++;
    }

    return NULL;
}

/*
 * 
 */
static void *place(void* fbp, size_t asize)
{
    size_t bsize = GET_SIZE(HDRP(fbp));
    size_t rsize = bsize - asize;

    delete_node(fbp);
    if((rsize) < (2*DSIZE)) {
        PUT(HDRP(fbp), PACK(bsize,1));
        PUT(FTRP(fbp), PACK(bsize,1));
    }
    else if (asize >= 96) {
        PUT(HDRP(fbp), PACK(rsize,0));
        PUT(FTRP(fbp), PACK(rsize,0));
        PUT(HDRP(NEXT_BLKP(fbp)), PACK(asize,1));
        PUT(FTRP(NEXT_BLKP(fbp)), PACK(asize,1));
        insert_node(fbp, rsize);
        return NEXT_BLKP(fbp);
        // PUT(HDRP(fbp), PACK(asize,1));
        // PUT(FTRP(fbp), PACK(asize,1));
        // fbp = NEXT_BLKP(fbp);
        // PUT(HDRP(fbp), PACK(rsize,0));
        // PUT(FTRP(fbp), PACK(rsize,0));
        // insert_node(fbp, rsize);
    }
    else {
        PUT(HDRP(fbp), PACK(asize,1));
        PUT(FTRP(fbp), PACK(asize,1));
        PUT(HDRP(NEXT_BLKP(fbp)), PACK(rsize,0));
        PUT(FTRP(NEXT_BLKP(fbp)), PACK(rsize,0));
        insert_node(NEXT_BLKP(fbp), rsize);
    }
    return fbp;
}

/*
 * insert a free block into segregated free list.
 */
static void insert_node(void *fbp, size_t size)
{
    int listidx;
    void *succ_ptr = NULL;
    void *pred_ptr = NULL;

    listidx = (1<<5) - __builtin_clz(size) - 1;
    pred_ptr = segregated_free_list[listidx];

    while((pred_ptr != NULL) && (size > GET_SIZE(HDRP(pred_ptr)))) {
        succ_ptr = pred_ptr;
        pred_ptr = PRED_FB(pred_ptr);
    }

    if(pred_ptr != NULL) {
        if(succ_ptr != NULL) {
            SET_PTR(SUCC_PTR(fbp), succ_ptr);
            SET_PTR(PRED_PTR(fbp), pred_ptr);
            SET_PTR(PRED_PTR(succ_ptr), fbp);
            SET_PTR(SUCC_PTR(pred_ptr), fbp);
        }
        else {
            SET_PTR(SUCC_PTR(fbp), NULL);
            SET_PTR(PRED_PTR(fbp), pred_ptr);
            SET_PTR(SUCC_PTR(pred_ptr), fbp);
            segregated_free_list[listidx] = fbp;
        }
    }
    else {
        if(succ_ptr != NULL) {
            SET_PTR(SUCC_PTR(fbp), succ_ptr);
            SET_PTR(PRED_PTR(fbp), NULL);
            SET_PTR(PRED_PTR(succ_ptr), fbp);
        }
        else {
            SET_PTR(SUCC_PTR(fbp), NULL);
            SET_PTR(PRED_PTR(fbp), NULL);
            segregated_free_list[listidx] = fbp;
        }
    }
}

/* 
 * delete a free block from segregated free list.
 */
static void delete_node(void *fbp)
{
    int listidx;
    size_t size;
    void *succ_ptr = SUCC_FB(fbp);
    void *pred_ptr = PRED_FB(fbp);

    size = GET_SIZE(HDRP(fbp));
    listidx = (1<<5) - __builtin_clz(size) - 1;

    if(pred_ptr != NULL) {
        if(succ_ptr != NULL) {
            SET_PTR(PRED_PTR(succ_ptr), pred_ptr);
            SET_PTR(SUCC_PTR(pred_ptr), succ_ptr);
        }
        else {
            SET_PTR(SUCC_PTR(pred_ptr), NULL);
            segregated_free_list[listidx] = pred_ptr;
        }
    }
    else {
        if(succ_ptr != NULL) {
            SET_PTR(PRED_PTR(succ_ptr), NULL);
        }
        else {
            segregated_free_list[listidx] = NULL;
        }
    }
}

void *mm1_realloc(void *ptr, size_t newsize)
{
    if(ptr == NULL) {
        return mm1_malloc(newsize);
    }

    if(newsize == 0) {
        mm1_free(ptr);
        return NULL;
    }

    void *oldptr = ptr;
    void *newptr;
    size_t oldsize;
    // size_t copysize;

    oldsize = GET_SIZE(HDRP(ptr));
    newsize = ALIGN(newsize + SIZE_T_SIZE);
    if(newsize-oldsize == 0) {
        return oldptr;
    }

    if(newsize < oldsize) {
        // oldptr = place(oldptr, newsize);
        return oldptr;
    }

    if((newsize > oldsize) && !GET_ALLOC(HDRP(NEXT_BLKP(oldptr)))) {
        size_t nextsize = GET_SIZE(HDRP(NEXT_BLKP(oldptr)));
        if(oldsize+nextsize >= newsize) {
            delete_node(NEXT_BLKP(oldptr));
            PUT(HDRP(oldptr), PACK(oldsize+nextsize,1));
            PUT(FTRP(oldptr), PACK(oldsize+nextsize,1));
            // if(oldsize+nextsize-newsize >= 2*DSIZE) {
            //     PUT(HDRP(oldptr), PACK(newsize,1));
            //     PUT(FTRP(oldptr), PACK(newsize,1));
            //     PUT(HDRP(NEXT_BLKP(oldptr)), PACK(oldsize+nextsize-newsize,0));
            //     PUT(FTRP(NEXT_BLKP(oldptr)), PACK(oldsize+nextsize-newsize,0));
            //     insert_node(NEXT_BLKP(oldptr), oldsize+nextsize-newsize);
            // }
            return oldptr;
        }
    }

    newptr = mm1_malloc(newsize);
    // copysize = oldsize<newsize?oldsize:newsize;
    memcpy(newptr, oldptr, oldsize);
    mm1_free(oldptr);
    return newptr;
}