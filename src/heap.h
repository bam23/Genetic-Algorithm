#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stddef.h>

#include "graph.h"

/* Preserve the coursework's deliberately small population limit. */
#define MAX_POPULATION 12

typedef struct node {
    int tour[MAXNUM];
    int city_count;
    double cost;
} node;

/*
 * One-based, array-backed binary min-heap. The representation and
 * percolation operations are the same data-structures concepts used in the
 * original project, but the state now belongs to a heap instance instead of
 * global arrays.
 */
typedef struct min_heap {
    node items[MAX_POPULATION + 1];
    size_t size;
    size_t capacity;
} min_heap;

void heap_init(min_heap *heap, size_t capacity);
bool heap_insert(min_heap *heap, const node *candidate);
bool heap_delete_min(min_heap *heap, node *minimum);
const node *heap_peek_min(const min_heap *heap);

#endif
