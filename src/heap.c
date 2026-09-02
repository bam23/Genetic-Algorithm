#include "heap.h"

static size_t parent(size_t index)
{
    return index / 2U;
}

static size_t left(size_t index)
{
    return index * 2U;
}

static size_t right(size_t index)
{
    return (index * 2U) + 1U;
}

static bool candidate_is_less(const tsp_candidate *left_candidate,
                              const tsp_candidate *right_candidate)
{
    if (left_candidate->cost < right_candidate->cost) {
        return true;
    }
    if (left_candidate->cost > right_candidate->cost) {
        return false;
    }

    /* A deterministic tie-breaker makes fixed-seed runs reproducible. */
    const int count = left_candidate->city_count < right_candidate->city_count
        ? left_candidate->city_count
        : right_candidate->city_count;
    for (int i = 0; i < count; ++i) {
        if (left_candidate->tour[i] < right_candidate->tour[i]) {
            return true;
        }
        if (left_candidate->tour[i] > right_candidate->tour[i]) {
            return false;
        }
    }
    return left_candidate->city_count < right_candidate->city_count;
}

static void percolate_up(tsp_min_heap *heap, size_t index)
{
    while (index > 1U &&
           candidate_is_less(&heap->items[index],
                             &heap->items[parent(index)])) {
        const tsp_candidate temporary = heap->items[index];
        heap->items[index] = heap->items[parent(index)];
        heap->items[parent(index)] = temporary;
        index = parent(index);
    }
}

static void percolate_down(tsp_min_heap *heap, size_t index)
{
    while (true) {
        const size_t left_child = left(index);
        const size_t right_child = right(index);
        size_t smallest = index;

        if (left_child <= heap->size &&
            candidate_is_less(&heap->items[left_child],
                              &heap->items[smallest])) {
            smallest = left_child;
        }
        if (right_child <= heap->size &&
            candidate_is_less(&heap->items[right_child],
                              &heap->items[smallest])) {
            smallest = right_child;
        }
        if (smallest == index) {
            break;
        }

        const tsp_candidate temporary = heap->items[index];
        heap->items[index] = heap->items[smallest];
        heap->items[smallest] = temporary;
        index = smallest;
    }
}

void tsp_heap_init(tsp_min_heap *heap, size_t capacity)
{
    if (heap == NULL) {
        return;
    }

    heap->size = 0U;
    heap->capacity = capacity <= TSP_MAX_POPULATION
        ? capacity
        : TSP_MAX_POPULATION;
}

bool tsp_heap_insert(tsp_min_heap *heap, const tsp_candidate *candidate)
{
    if (heap == NULL || candidate == NULL || heap->size >= heap->capacity) {
        return false;
    }

    ++heap->size;
    heap->items[heap->size] = *candidate;
    percolate_up(heap, heap->size);
    return true;
}

bool tsp_heap_delete_min(tsp_min_heap *heap, tsp_candidate *minimum)
{
    if (heap == NULL || minimum == NULL || heap->size == 0U) {
        return false;
    }

    *minimum = heap->items[1];
    heap->items[1] = heap->items[heap->size];
    --heap->size;
    if (heap->size > 0U) {
        percolate_down(heap, 1U);
    }
    return true;
}

const tsp_candidate *tsp_heap_peek_min(const tsp_min_heap *heap)
{
    if (heap == NULL || heap->size == 0U) {
        return NULL;
    }
    return &heap->items[1];
}
