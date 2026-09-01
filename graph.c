#include "graph.h"

#include <stdio.h>

bool graph_load(const char *path, double cities[MAXNUM][MAXNUM])
{
    if (path == NULL || cities == NULL) {
        return false;
    }

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: could not open %s\n", path);
        return false;
    }

    for (int row = 0; row < MAXNUM; ++row) {
        for (int column = 0; column < MAXNUM; ++column) {
            if (row == column) {
                cities[row][column] = 0.0;
            } else if (fscanf(file, "%lf", &cities[row][column]) != 1) {
                fprintf(stderr,
                        "Error: malformed %s at matrix position (%d,%d)\n",
                        path,
                        row,
                        column);
                fclose(file);
                return false;
            }
        }
    }

    double extra_value = 0.0;
    if (fscanf(file, "%lf", &extra_value) == 1) {
        fprintf(stderr, "Error: %s contains more than 380 distance values\n", path);
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}

void graph_print(const double cities[MAXNUM][MAXNUM], int city_count)
{
    if (cities == NULL || city_count < 1 || city_count > MAXNUM) {
        return;
    }

    for (int row = 0; row < city_count; ++row) {
        for (int column = 0; column < city_count; ++column) {
            printf("%8.2f ", cities[row][column]);
        }
        putchar('\n');
    }
}
