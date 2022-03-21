/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
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
    "ateam",
    /* First member's full name */
    "Zhang Yi",
    /* First member's email address */
    "---",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// mallocmacros from example code in CSAPP 9.9.12
/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ //line:vm:mm:beginconst
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */  //line:vm:mm:endconst 

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) //line:vm:mm:pack

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))            //line:vm:mm:get
#define PUT(p, val)  (*(unsigned int *)(p) = (val))    //line:vm:mm:put

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   //line:vm:mm:getsize
#define GET_ALLOC(p) (GET(p) & 0x1)                    //line:vm:mm:getalloc

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      //line:vm:mm:hdrp
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) //line:vm:mm:ftrp

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //line:vm:mm:nextblkp
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //line:vm:mm:prevblkp
/* $end mallocmacros */

// for Explicit Free List
/* Given block ptr bp, compute address of next point and prev pointer */
#define NEXT(bp)       ((char *) (bp))
#define PREV(bp)       ((char *) (bp) + WSIZE)

/* Global variables */
static char* heap_listp = NULL;
static char* heap_listendp = NULL;
static char* dummy_head = NULL;

/* Function prototypes for internal helper routines */
static void* extend_heap(size_t words);
static void place(void* bp, size_t asize);
static void* find_fit(size_t asize);
static void* coalesce(void* bp);
static void printblock(void* bp);
static void checkheap(int verbose);
static void checkblock(void* bp);
static void print_freelist();
static void insert_block(void* bp, int size);
void detach_block(void* bp);
/*
 * mm_init - Initialize the memory manager
 */
int mm_init(void)
{
    // Create the initial empty heap
    if ((heap_listp = mem_sbrk(14 * DSIZE)) == (void*)-1)
        return -1;

    // null for alignment
    PUT(heap_listp + (0 * WSIZE), 0);  // block size <= 16
    PUT(heap_listp + (1 * WSIZE), 0);  // block size <= 32
    PUT(heap_listp + (2 * WSIZE), 0);  // block size <= 64
    PUT(heap_listp + (3 * WSIZE), 0);  // block size <= 128
    PUT(heap_listp + (4 * WSIZE), 0);  // block size <= 256
    PUT(heap_listp + (5 * WSIZE), 0);  // block size <= 512
    PUT(heap_listp + (6 * WSIZE), 0);  // block size <= 1024
    PUT(heap_listp + (7 * WSIZE), 0);  // block size <= 2048
    PUT(heap_listp + (8 * WSIZE), 0);  // block size <= 4096
    PUT(heap_listp + (9 * WSIZE), 0);  // block size > 4096

    heap_listendp = heap_listp + (10 * WSIZE); // indicate where the list ends

    dummy_head = heap_listp + (11 * WSIZE);
    PUT(HDRP(dummy_head), PACK(DSIZE, 1));         // prologue header
    PUT(FTRP(dummy_head), PACK(DSIZE, 1));         // prologue footer

    PUT(HDRP(NEXT_BLKP(dummy_head)), PACK(0, 1));  // epilogue header

    // extend the heap to CHUNKSIZE
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;

    return 0;
}


/*
 * mm_malloc - Allocate a block with at least size bytes of payload
 */
void* mm_malloc(size_t size)
{
    // checkheap(1);
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char* bp;

    // check init status
    if (dummy_head == NULL)
        mm_init();

    // ignore spurious requests
    if (size == NULL)
        return NULL;

    // Adjust block size to include overhead and alignment reqs.
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE)+(DSIZE - 1)) / DSIZE);

    // Search the free list for a fit 
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    // No fit found. Get more memory and place the block 
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);

    return bp;
}


/*
 * mm_free - Free a block
 */
void mm_free(void* bp)
{
    // checkheap(1);
    // ignore spurious requests
    if (bp == 0)
        return;

    // get the size to be freed
    size_t size = GET_SIZE(HDRP(bp));

    // check init status
    if (dummy_head == 0)
        mm_init();

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}


/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */

static void* coalesce(void* bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            // Case 1
        // insert the current block between dummy_head and dummy_head
        insert_block(bp, size);
        return bp;
    }
    else if (prev_alloc && !next_alloc) {      // Case 2 
        // detach next block from the EFL
        char* next_block = NEXT_BLKP(bp);
        detach_block(next_block);

        // set the size of the combined block
        size += GET_SIZE(HDRP(next_block));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        insert_block(bp, size);
        return bp;
    }
    else if (!prev_alloc && next_alloc) {      // Case 3 
        // detach the previous block from the EFL
        char* prev_block = PREV_BLKP(bp);
        detach_block(prev_block);

        // set the size of the combined block
        size += GET_SIZE(HDRP(prev_block));
        PUT(HDRP(prev_block), PACK(size, 0));
        PUT(FTRP(prev_block), PACK(size, 0));

        insert_block(prev_block, size);
        return prev_block;
    }
    else {                                     // Case 4
        // detach both next and prev block from the EFL
        char* prev_block = PREV_BLKP(bp);
        char* next_block = NEXT_BLKP(bp);
        detach_block(prev_block);
        detach_block(next_block);

        // set the size of the combined block
        size += GET_SIZE(HDRP(prev_block)) + GET_SIZE(HDRP(next_block));
        PUT(HDRP(prev_block), PACK(size, 0));
        PUT(FTRP(prev_block), PACK(size, 0));

        insert_block(prev_block, size);
        return prev_block;
    }
}


/*
 * mm_realloc - Naive implementation of realloc
 */
void* mm_realloc(void* ptr, size_t size)
{
    size_t oldsize;
    void* newptr;

    // If size == 0 then this is just free, and we return NULL.
    if (size == 0) {
        mm_free(ptr);
        return 0;
    }

    // If oldptr is NULL, then this is just malloc.
    if (ptr == NULL) {
        return mm_malloc(size);
    }

    newptr = mm_malloc(size);

    // If realloc() fails the original block is left untouched 
    if (!newptr) {
        return 0;
    }

    // Copy the old data.
    oldsize = GET_SIZE(HDRP(ptr));
    if (size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);

    // Free the old block.
    mm_free(ptr);

    return newptr;
}

/*
 * mm_checkheap - Check the heap for correctness
 */
void mm_checkheap(int verbose)
{
    checkheap(verbose);
}

/*
 * The remaining routines are internal helper routines
 */

 /*
  * extend_heap - Extend heap with free block and return its block pointer
  */
static void* extend_heap(size_t words)
{
    char* bp;
    size_t size;

    // Allocate an even number of words to maintain alignment 
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    // set the block size
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    insert_block(bp, size);

    // Coalesce if the previous block was free 
    return coalesce(bp);
}


/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 */
static void place(void* bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    // detach the whole block
    detach_block(bp);

    if ((csize - asize) >= (2 * DSIZE)) {
        // set the used block as allocated
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        // set the remaining block as free
        bp = NEXT_BLKP(bp);
        int remain_size = csize - asize;
        PUT(HDRP(bp), PACK(remain_size, 0));
        PUT(FTRP(bp), PACK(remain_size, 0));

        // insert it in the right place
        insert_block(bp, remain_size);
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * find_fit - Find a fit for a block with asize bytes
 */
static void* find_fit(size_t asize)
{
    // First-fit search 
    char* head;
    for (head = get_head(asize); head != dummy_head; head = head + WSIZE) {
        for (char* bp = GET(head); bp != NULL; bp = GET(NEXT(bp))) {
            if (GET_SIZE(HDRP(bp)) >= asize)
                return bp;
        }
    }

    return NULL; // No fit
}

static void printblock(void* bp)
{
    size_t hsize, halloc, fsize, falloc;

    // checkheap(0);
    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));

    if (hsize == 0) {
        printf("%p: EOL\n", bp);
        return;
    }

    printf("%p: next:[%p] prev:[%p] header: [%u:%c] footer: [%u:%c]\n", bp,
        GET(NEXT(bp)), GET(PREV(bp)),
        hsize, (halloc ? 'a' : 'f'),
        fsize, (falloc ? 'a' : 'f'));
}

static void checkblock(void* bp)
{
    if ((size_t)bp % 8)
        printf("Error: %p is not doubleword aligned\n", bp);
    if (GET(HDRP(bp)) != GET(FTRP(bp)))
        printf("Error: header does not match footer\n");
    if (GET_ALLOC(bp))
        printf("Error: block in the free list is not free\n");
}

/*
 * checkheap - Minimal check of the heap for consistency
 */
void checkheap(int verbose)
{
    if (verbose)
        printf("Heap (%p):\n", dummy_head);

    checkblock(dummy_head);
    if (verbose) {
        printblock(dummy_head);
    }

    if (dummy_head != GET(NEXT(dummy_head)))
        printf("dummy_head != get(next(dummy_dead))\n");

    for (char* bp = dummy_head; bp != dummy_head; bp = GET(NEXT(bp))) {
        if (verbose)
            printblock(bp);
        checkblock(bp);
    }
}

void print_freelist() {
    char* iter = dummy_head;
    do {
        printf("%o->", NEXT(iter));
        iter = GET(NEXT(iter));
    } while (iter != dummy_head);
    printf("\n");
}


void* get_head(int size) {
    assert(size >= 8 && size % 8 == 0);
    int n = 0;
    size = size >> 4;
    while (size != 0) {
        n += 1;
        size = size >> 1;
    }
    return heap_listp + n * WSIZE;
}

void insert_block(void* bp, int size) {
    assert(bp != NULL && size >= 8 && size % 8 == 0);
    void* head = get_head(size);
    PUT(NEXT(bp), GET(head));
    PUT(PREV(bp), NULL);
    PUT(head, bp);
}

void detach_block(void* bp) {
    char* prev = GET(PREV(bp));
    char* next = GET(NEXT(bp));
    PUT(NEXT(prev), next);
    if (next != NULL)
        PUT(PREV(next), prev);
}




