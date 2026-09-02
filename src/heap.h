#ifndef TSP_HEAP_H
#define TSP_HEAP_H

#include <stdbool.h>
#include <stddef.h>

#include <tsp/tsp.h>

typedef struct tsp_candidate {
    int tour[TSP_MAX_CITIES];
    int city_count;
    double cost;
} tsp_candidate;

/* Preserve the coursework's one-based, array-backed binary min-heap. */
typedef struct tsp_min_heap {
    tsp_candidate items[TSP_MAX_POPULATION + 1];
    size_t size;
    size_t capacity;
} tsp_min_heap;

void tsp_heap_init(tsp_min_heap *heap, size_t capacity);
bool tsp_heap_insert(tsp_min_heap *heap, const tsp_candidate *candidate);
bool tsp_heap_delete_min(tsp_min_heap *heap, tsp_candidate *minimum);
const tsp_candidate *tsp_heap_peek_min(const tsp_min_heap *heap);

#endif
