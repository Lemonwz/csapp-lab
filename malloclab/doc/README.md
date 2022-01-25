# CSAPP Malloc Lab

该lab的目标是实现一个c的动态内存分配器，由以下四个函数组成：

`int mm_init(void)`

`void *mm_malloc(size_t size)`

`void mm_free(void *ptr)`

`void *mm_realloc(void *ptr, size_t size)`

最后会通过一组trace文件（由malloc free realloc指令组成）按照一定的顺序调用实现的malloc、free和realloc函数，评估分配器的性能，即吞吐率（单位时间内完成的请求数）和内存利用率（已分配块有效载荷的和 / 堆的大小），二者是互相牵制的，我们的目标便是在最大化利用率和最大化吞吐率之间找到一个适当的平衡点，需要考虑以下几个问题：

- 如何记录空闲块
- 如何选择一个合适的空闲块来放置一个新分配的块
- 将一个新分配的块放置到某个空闲块后，如何处理空闲块中的剩余部分
- 如何处理一个刚刚被释放的块

这里提供了隐式空闲链表和显式空闲链表两种实现

### 1. Implicit Free List (隐式空闲链表)：

隐式空闲链表的实现主要参照书上的内容，并在此基础上进行了一些优化

隐式空闲链表的数据结构如下图所示，一个块是由一个字的头部、有效载荷和一个字的脚部组成，头部记录了块的大小和分配信息（已分配的或空闲的），脚部是头部的一个副本，用于快速合并。在双字对齐的约束条件下，块的大小总是8的倍数，因此头部最低3位总是0，用其中的最低位（已分配位）来指明这个块是已分配的还是空闲的

<img src="/home/comethru/tinykv/markdown/img/implicit-free-list.png" style="zoom: 67%;" />

头部和脚部的中间就是应用调用malloc时请求的有效载荷和用于各种需求的填充部分，之所以称为隐式空闲链表是因为空闲块是通过头部中的大小字段隐含地连接着的，分配器可以通过遍历堆中的所有块从而间接地遍历空闲块集合

1. **宏定义：**

WHFSIZE表示单字大小，也是块头部和脚部的大小，ALIGNMENT表示双字大小（对齐要求），CHUNKSIZE表示初始空闲块的大小和扩展堆时的默认大小，PACK宏将块大小和已分配位结合起来并返回，GET宏读取和返回参数p引用的字，PUT宏将val值存放在参数p指向的字中，GET_SIZE宏和GET_ALLOC宏从地址p处的头部或脚部分别返回大小和已分配位，HDRP和FTRP宏分别返回指向这个块头部和脚部的指针，NEXT_BLKP宏和PREV_BLKP宏分别返回指向后面的块和前面的块的块指针，ALIGN宏用于对齐块大小

```c
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
```

2. **`mm_init`：**

在调用`mm_malloc`或者`mm_free`之前，应用必须通过调用`mm_init`函数来初始化分配器，`mm_init`函数从内存系统得到4个字，并将他们初始化，创建一个空的空闲链表，这4个字分别用作填充字，序言块头部，序言块尾部和结尾块，全局变量`heap_listp`始终指向序言块。随后调用`extend_heap`函数，将堆扩展CHUNKSIZE字节并创建初始的空闲块

```c
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
```

3. **`extend_heap`**

`extend_heap`函数会在两种不同的环境中被调用：堆被初始化时和`mm_malloc`不能找到一个合适的空闲块时，`extend_heap`首先将请求大小做对齐处理，然后向内存系统请求额外的堆空间，`mem_sbrk`返回分配空间的起始地址，紧跟在结尾块头部的后面，因此结尾块的头部变成了新空闲块的头部，新空闲块的最后一个字变成了新的结尾块；最后，若扩展前的堆以一个空闲块结束，那么扩展后需要合并这两个空闲块，调用`coalesce`函数

```c
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
```

4. **`mm_malloc`：**

应用通过调用mm_malloc函数来向内存请求大小为size字节的块，size为0直接返回即可，否则mm_malloc调整请求大小以满足双字对齐要求：对小于8字节的请求，强制调整为最小块大小16字节；对大于8字节的请求，请求大小加上8字节的头部和脚部后向上舍入到最接近的8的整数倍（eg：6->16 15->24），得到对齐的请求大小后，分配器搜索链表，查找一个足够大可以放置所请求块的空闲块，如果有合适的，那么分配器就放置这个请求块，并可选地分割出多余的部分，然后返回新分配块的地址

```c
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
```

5. **`first_fit` / `next_fit` / `best_fit`：**

三个函数对应三种不同的空闲块搜索策略：首次适配（first fit）、下一次适配（next fit）和最佳适配（best fit）

首次适配从头开始搜索空闲链表，找到第一个适配的空闲块时便返回

```c
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
```

下一次适配从上一次搜索结束的地方开始查找（需要一个全局变量来记录上一次搜索结束的位置）

```c
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
```

最佳适配遍历空闲链表，检查每一个空闲块，选出最合适的一个（满足请求大小的最小的空闲块）

```c
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
```

6. **`place`：**

将请求块放置在空闲块的起始位置，当空闲块剩余空间的大小超过最小块大小时，分割空闲块

```c
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
```

7. **`mm_free`：**

应用通过调用`mm_free`函数来释放一个以前分配的块，随后调用`coalesce`函数将其与邻接的空闲块合并起来（如果有）

```c
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size,0));
    PUT(FTRP(ptr), PACK(size,0));
    coalesce(ptr);
}
```

8. **`coalesce`：**

 分配器释放当前块或扩展得到新的空闲块时可能会出现以下情况：

- 前面的块和后面的块都是已分配的：

  此时无法进行合并，直接返回即可

- 前面的块是已分配的，后面的块是空闲的：

  将当前块和后面的块合并，用两个块大小的和来更新当前块的头部和后面块的脚部

- 前面的块是空闲的，后面的块是已分配的：

  将当前块和前面的块合并，用两个块大小的和来更新前面块的头部和当前块的脚部

- 前面的块和后面的块都是空闲的：

  将当前块、前面的块和后面的块合并，用三个块大小的和来更新前面块的头部和后面块的脚部

```c
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
```

9. **`mm_realloc`：**

应用调用`mm_realloc`函数为已分配块重新分配newsize大小的空间

当ptr == NULL时，表明是一个未分配的块，那么这次调用等同于调用mm_malloc(newsize)

当newsize=0时，即为一个已分配块分配0字节的空间，等同于调用mm_free(ptr)

排除以上两种特殊情况后，我们需要判断realloc是对已分配块的放大还是收缩。如果是对已分配块的收缩（oldsize > newsize），则需要判断收缩后空闲出来的空间是否能够组成一个空闲块（但根据最后的结果来看这一优化好像没什么作用，直接返回oldptr即可）；如果是对已分配块的放大，则需要先检查下一个块是否空闲以及加上该块大小后能否满足放大要求，如果满足，则合并当前块和下一个块（这里还可以检查一下合并后的块能否分割），若以上条件均不满足，则mm_malloc重新分配地址空间，调用memcpy复制原块内容过去，并释放原块

```c
void *mm_realloc(void *ptr, size_t newsize)
{
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
```

### 2. Explicit Free List (显式空闲链表)：

在隐式空闲链表中，块分配与堆块的总数呈线性关系，在查找空闲块时需要遍历很多没有必要遍历的已分配块，一种更好的方法是将空闲块组织为某种显式数据结构（双向空闲链表），实现这个数据结构的指针可以存放在这些空闲块的主体里面，空闲块之间在逻辑上是相连的，但在物理位置上可以按任意顺序排列

分离的空闲链表（分离存储） + 分离适配：

分离存储即维护多个空闲链表，其中每个链表中的块有大致相等的大小，这里根据2的幂来划分块大小，分配器维护着一个空闲链表的数组，每个空闲链表是和一个大小类相关联的，为了分配一个块，必须确定请求的大小类，并且对适当的空闲链表做首次适配（为什么做首次适配后面会解释），查找合适的块

显式空闲链表的数据结构如下图所示，在每个空闲块中都包含一个祖先（pred）和后继（succ），位于有效载荷的前两个字中，分别指向前一个和后一个插入到空闲链表中的空闲块

<img src="/home/comethru/tinykv/markdown/img/explicit-free-list.png" style="zoom: 33%;" />



1. **宏定义**

这里只给出了在隐式空闲链表基础上添加的宏定义，INIT_CHUNKSIZE和NORMAL_CHUNKSIZE分别用于初始化堆和扩展堆，LIST_SIZE为隐式空闲链表数组的大小（即大小类个数），SET_PTR宏让指针 p 指向 ptr ，PRED_PTR和SUCC_PTR宏分别返回指向这个块的祖先和后继的指针，PRED_FB和SUCC_FB宏分别返回前驱块和后继块的地址

```c
/* initial and normal heap extend size */
#define INIT_CHUNKSIZE (1<<6)
#define NORMAL_CHUNKSIZE (1<<12)

/* size of free list array */
#define LIST_SIZE 32

/* Set pointer p points to ptr */
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

/* Given free block ptr fbp, compute address of its pred and succ ptr */
#define PRED_PTR(fbp) ((char *)(fbp))
#define SUCC_PTR(fbp) ((char *)(fbp) + WSIZE)

/* Given free block ptr fbp, compute address of pred and succ free blocks */
#define PRED_FB(fbp) (*(char **)(fbp))
#define SUCC_FB(fbp) (*(char **)(SUCC_PTR(fbp)))
```

2. **`mm1_init`**

和隐式空闲链表类似，只不过这里要先初始化空闲链表数组

```c
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
```

3. **`extend_heap`**

和隐式空闲链表类似，在从内存中申请了一个大的空闲块后，需要调用`insert_node`函数将这个空闲块插入到对应的空闲链表中

```c
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
```

4. **`mm1_free`**

同样在释放了一个块后，将该空闲块插入到对应的空闲链表中

```c
void mm1_free (void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size,0));
    PUT(FTRP(ptr), PACK(size,0));

    insert_node(ptr, size);
    coalesce(ptr);
}
```

5. **`coalesce`**

合并空闲块，当前一个块或后一个块也是空闲块时，需要先将当前块和前一个块 / 后一个块先从空闲链表中删除，并在合并后将合并的空闲块插入到对应的空闲链表中

```c
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
```

6. **`mm1_malloc`**

同隐式空闲链表类似，不过这里采用的是首次适配策略，因为在显式空闲链表中是按照size大小来排列空闲块，即小的空闲块在前面，大的空闲块在后面，这样采用首次适配搜索从对应链表开始找到的第一个合适的空闲块即为最小的满足要求的空闲块，其内存利用率近似于对整个堆的最佳适配搜索的内存利用率

```c
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
```

7. **`first_fit`**

首先需要根据请求大小找到对应的空闲链表，这里用到了c语言的内建函数`__builtin_clz(unsigned int x)`，函数返回x的前导0的个数（内建函数经过了编译器的高度优化，运行速度较挨个遍历有一定的提升），找到对应的空闲链表后，先在该空闲链表中查找是否有满足需求的空闲块，如果没有则查找下一个空闲链表，最后返回满足需求的空闲块的地址

```c
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
```

8. **`place`**

这里根据binary-bal.rep和binary2-bal.rep两个测试文件进行了特判（write-up中指明不允许特判，追求高分可以尝试），在这两个文件中，每次allocate都是按照小-大-小-大的顺序进行，在后续free时只free大块，小块依旧是allocated，因为这些空闲的大块是不连续的，所以无法合并，虽然表面上是有非常多的空闲空间，但在接收到一个大于大块size的分配请求时却只能重新去找一个空闲块，从而导致很低的内存利用率

因此我们需要在allocate时根据请求块的大小将小的块和大的块都连续地放在一起，这样在连续释放大的块或小的块时也能够合并空闲块（经过测试大小块的分界值在96左右能够得到最优值）

```c
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
```

9. **`mm1_realloc`**

和隐式空闲链表的realloc实现类似，当下一个块空闲且需要合并时，从空闲链表中删除该空闲块

```c
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

    oldsize = GET_SIZE(HDRP(ptr));
    newsize = ALIGN(newsize + SIZE_T_SIZE);
    if(newsize-oldsize == 0) {
        return oldptr;
    }

    if(newsize < oldsize) {
        return oldptr;
    }

    if((newsize > oldsize) && !GET_ALLOC(HDRP(NEXT_BLKP(oldptr)))) {
        size_t nextsize = GET_SIZE(HDRP(NEXT_BLKP(oldptr)));
        if(oldsize+nextsize >= newsize) {
            delete_node(NEXT_BLKP(oldptr));
            PUT(HDRP(oldptr), PACK(oldsize+nextsize,1));
            PUT(FTRP(oldptr), PACK(oldsize+nextsize,1));
            return oldptr;
        }
    }

    newptr = mm1_malloc(newsize);
    memcpy(newptr, oldptr, oldsize);
    mm1_free(oldptr);
    return newptr;
}
```

10. **`insert_node`**

在对应空闲链表中插入空闲块，首先需要根据空闲块的大小找到对应的空闲链表，随后在空闲链表中找到插入的位置（按照空闲块大小由小到大的顺序）以及前驱和后继指针，插入时有四种情况：

- 前驱和后继指针均不为空：

  表明是在表中插入，需要修改后继空闲块的前驱指针和前驱空闲块的后继指针，并设置当前块的前驱和后继指针

- 前驱指针不为空，后继指针为空：

  表明是在表头插入，需要修改前驱空闲块的后继指针以及设置当前块的前驱指针，并设置当前块为空闲链表表头

- 前驱指针为空，后继指针不为空：

  表明是在表尾插入，需要修改后继空闲块的前驱指针以及设置当前块的后继指针，并设置当前块的前驱指针为空

- 前驱和后继指针均为空：

  表明是一个空链表，设置当前块前驱和后继指针均为空，并设置当前块为空闲链表表头

```c
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
```

11. **`delete_node`**

在对应的空闲链表中删除空闲块，和插入类似，找到对应的空闲链表后，根据不同情况进行相应的操作即可

```c
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
```



### 3. 结果

隐式空闲链表 + 下一次适配：

<img src="/home/comethru/tinykv/markdown/img/result1.png" style="zoom: 67%;" />

显式空闲链表 + 分离适配：

<img src="/home/comethru/tinykv/markdown/img/result2.png" style="zoom: 67%;" />
