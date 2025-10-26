#include "queue.h"

/* The current tour is provided by function.c */
extern int s[TOURNUM];

/* Global heaps and sizes */
int theSize = 0;
int storeSize = 0;
node PQ[MAXSIZE + 1];
node store[MAXSIZE + 1];

static inline int parent(int i) { return i >> 1; }
static inline int left(int i)   { return i << 1; }
static inline int right(int i)  { return (i << 1) | 1; }

/* ---------- Primary heap: PQ ---------- */
void percolateUp(void) {
    int i = theSize;
    while (i > 1 && PQ[i].cost < PQ[parent(i)].cost) {
        node tmp = PQ[i];
        PQ[i] = PQ[parent(i)];
        PQ[parent(i)] = tmp;
        i = parent(i);
    }
}

void percolateDown(int i) {
    while (1) {
        int l = left(i), r = right(i), smallest = i;
        if (l <= theSize && PQ[l].cost < PQ[smallest].cost) smallest = l;
        if (r <= theSize && PQ[r].cost < PQ[smallest].cost) smallest = r;
        if (smallest == i) break;
        node tmp = PQ[i];
        PQ[i] = PQ[smallest];
        PQ[smallest] = tmp;
        i = smallest;
    }
}

void insert(float x) {
    if (theSize + 1 > MAXSIZE) {
        fprintf(stderr, "PQ overflow, MAXSIZE=%d\n", MAXSIZE);
        return;
    }
    theSize++;
    PQ[theSize].cost = x;
    for (int i = 0; i < TOURNUM; i++) {
        PQ[theSize].tour[i] = s[i];
    }
    percolateUp();
}

float deleteMin(void) {
    if (theSize == 0) {
        return 0.0f;
    }
    float minCost = PQ[1].cost;
    /* Expose the best tour to callers via global s */
    for (int i = 0; i < TOURNUM; i++) {
        s[i] = PQ[1].tour[i];
    }
    PQ[1] = PQ[theSize];
    theSize--;
    if (theSize > 0) percolateDown(1);
    return minCost;
}

void print(void) {
    for (int i = 1; i <= theSize; i++) {
        printf("[%d] cost=%0.3f tour:", i, PQ[i].cost);
        for (int j = 0; j < TOURNUM; j++) printf(" %d", PQ[i].tour[j]);
        printf("\n");
    }
}

/* ---------- Secondary heap: store ---------- */
static inline void percolateUpStoreInternal(void) {
    int i = storeSize;
    while (i > 1 && store[i].cost < store[parent(i)].cost) {
        node tmp = store[i];
        store[i] = store[parent(i)];
        store[parent(i)] = tmp;
        i = parent(i);
    }
}

void percolateUpStore(void) { percolateUpStoreInternal(); }

void percolateDownStore(int i) {
    while (1) {
        int l = left(i), r = right(i), smallest = i;
        if (l <= storeSize && store[l].cost < store[smallest].cost) smallest = l;
        if (r <= storeSize && store[r].cost < store[smallest].cost) smallest = r;
        if (smallest == i) break;
        node tmp = store[i];
        store[i] = store[smallest];
        store[smallest] = tmp;
        i = smallest;
    }
}

void insertStore(void) {
    if (storeSize + 1 > MAXSIZE) {
        fprintf(stderr, "store overflow, MAXSIZE=%d\n", MAXSIZE);
        return;
    }
    storeSize++;
    /* copy the current best from PQ root */
    if (theSize >= 1) {
        store[storeSize] = PQ[1];
    } else {
        /* if PQ is empty, copy from s with cost 0 */
        store[storeSize].cost = 0.0f;
        for (int i = 0; i < TOURNUM; i++) store[storeSize].tour[i] = s[i];
    }
    percolateUpStoreInternal();
}

float deleteMinStore(void) {
    if (storeSize == 0) {
        return 0.0f;
    }
    float minCost = store[1].cost;
    store[1] = store[storeSize];
    storeSize--;
    if (storeSize > 0) percolateDownStore(1);
    return minCost;
}