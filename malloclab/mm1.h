#include <stdio.h>

extern int mm1_init (void);
extern void *mm1_malloc (size_t size);
extern void mm1_free (void *ptr);
extern void *mm1_realloc(void *ptr, size_t newsize);


/* 
 * Students work in teams of one or two.  Teams enter their team name, 
 * personal names and login IDs in a struct of this
 * type in their bits.c file.
 */
// typedef struct {
//     char *teamname; /* ID1+ID2 or ID1 */
//     char *name1;    /* full name of first member */
//     char *id1;      /* login ID of first member */
//     char *name2;    /* full name of second member (if any) */
//     char *id2;      /* login ID of second member */
// } team_t;

// extern team_t team1;