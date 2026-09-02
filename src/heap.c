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

static bool node_is_less(const node *left_node, const node *right_node)
{
    if (left_node->cost < right_node->cost) {
        return true;
    }
    if (left_node->cost > right_node->cost) {
        return false;
    }

    /* A deterministic tie-breaker makes fixed-seed runs reproducible. */
    const int count = left_node->city_count < right_node->city_count
        ? left_node->city_count
        : right_node->city_count;
    for (int i = 0; i < count; ++i) {
        if (left_node->tour[i] < right_node->tour[i]) {
            return true;
        }
        if (left_node->tour[i] > right_node->tour[i]) {
            return false;
        }
    }
    return left_node->city_count < right_node->city_count;
}

static void percolate_up(min_heap *heap, size_t index)
{
    while (index > 1U &&
           node_is_less(&heap->items[index], &heap->items[parent(index)])) {
        const node temporary = heap->items[index];
        heap->items[index] = heap->items[parent(index)];
        heap->items[parent(index)] = temporary;
        index = parent(index);
    }
}

static void percolate_down(min_heap *heap, size_t index)
{
    while (true) {
        const size_t left_child = left(index);
        const size_t right_child = right(index);
        size_t smallest = index;

        if (left_child <= heap->size &&
            node_is_less(&heap->items[left_child], &heap->items[smallest])) {
            smallest = left_child;
        }
        if (right_child <= heap->size &&
            node_is_less(&heap->items[right_child], &heap->items[smallest])) {
            smallest = right_child;
        }
        if (smallest == index) {
            break;
        }

        const node temporary = heap->items[index];
        heap->items[index] = heap->items[smallest];
        heap->items[smallest] = temporary;
        index = smallest;
    }
}

void heap_init(min_heap *heap, size_t capacity)
{
    if (heap == NULL) {
        return;
    }

    heap->size = 0U;
    heap->capacity = capacity <= MAX_POPULATION ? capacity : MAX_POPULATION;
}

bool heap_insert(min_heap *heap, const node *candidate)
{
    if (heap == NULL || candidate == NULL || heap->size >= heap->capacity) {
        return false;
    }

    ++heap->size;
    heap->items[heap->size] = *candidate;
    percolate_up(heap, heap->size);
    return true;
}

bool heap_delete_min(min_heap *heap, node *minimum)
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

const node *heap_peek_min(const min_heap *heap)
{
    if (heap == NULL || heap->size == 0U) {
        return NULL;
    }
    return &heap->items[1];
}
