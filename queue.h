#ifndef _queue_h
#define _queue_h
#include <stdio.h>
#include <stdlib.h>

/* Capacity for global heaps */
#ifndef MAXSIZE
#define MAXSIZE 200000
#endif

/* Tour length used throughout the project */
#ifndef TOURNUM
#define TOURNUM 21
#endif

typedef struct node {
    int   tour[TOURNUM];
    float cost;
} node;

/* Global min-heaps */
extern node PQ[MAXSIZE + 1];
extern node store[MAXSIZE + 1];
extern int theSize;
extern int storeSize;

/* Priority queue API */
void  insert(float x);
float deleteMin(void);
void  percolateUp(void);
void  percolateDown(int slot);
void  print(void);

/* Secondary heap for storing elites or results */
void  insertStore(void);
void  percolateUpStore(void);
void  percolateDownStore(int slot);
float deleteMinStore(void);

#endif