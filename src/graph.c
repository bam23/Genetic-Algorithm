#include <tsp/tsp.h>

#include <stdio.h>

tsp_status tsp_graph_load(tsp_graph *out_graph, const char *path)
{
    if (out_graph == NULL || path == NULL) {
        return TSP_STATUS_INVALID_ARGUMENT;
    }

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return TSP_STATUS_IO_ERROR;
    }

    tsp_graph graph = { .costs = { { 0.0 } }, .city_count = TSP_MAX_CITIES };
    for (int row = 0; row < TSP_MAX_CITIES; ++row) {
        for (int column = 0; column < TSP_MAX_CITIES; ++column) {
            if (row == column) {
                graph.costs[row][column] = 0.0;
            } else if (fscanf(file, "%lf", &graph.costs[row][column]) != 1) {
                fclose(file);
                return TSP_STATUS_INVALID_DATA;
            }
        }
    }

    double extra_value = 0.0;
    if (fscanf(file, "%lf", &extra_value) == 1) {
        fclose(file);
        return TSP_STATUS_INVALID_DATA;
    }

    fclose(file);
    *out_graph = graph;
    return TSP_STATUS_OK;
}
