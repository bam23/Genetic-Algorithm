#include "graph.h"

/* New functions required by the tester */
void iGraph(double cities[MAXNUM][MAXNUM]) {
    FILE *fptr = fopen("cities.dat", "r");
    if (fptr == NULL) {
        fprintf(stderr, "Error: could not open cities.dat\n");
        exit(1);
    }
    for (int i = 0; i < MAXNUM; i++) {
        for (int j = 0; j < MAXNUM; j++) {
            if (i == j) {
                cities[i][j] = 0.0;
            } else if (fscanf(fptr, "%lf", &cities[i][j]) != 1) {
                fprintf(stderr, "Error: malformed cities.dat at (%d,%d)\n", i, j);
                fclose(fptr);
                exit(1);
            }
        }
    }
    fclose(fptr);
}

void pGraph(double cities[MAXNUM][MAXNUM]) {
    for (int i = 0; i < MAXNUM; i++) {
        for (int j = 0; j < MAXNUM; j++) {
            printf("%8.2f ", cities[i][j]);
        }
        printf("\n");
    }
}

/* Original helper kept for compatibility */
void test(double cities[MAXNUM][MAXNUM]) {
    FILE *fptr = fopen("cities.dat", "r");
    if (fptr == NULL) {
        fprintf(stderr, "Error: could not open cities.dat\n");
        exit(1);
    }
    for (int i = 0; i < MAXNUM; i++) {
        for (int j = 0; j < MAXNUM; j++) {
            if (i == j) cities[i][j] = 0.0;
            else if (fscanf(fptr, "%lf", &cities[i][j]) != 1) {
                fprintf(stderr, "Error: malformed cities.dat at (%d,%d)\n", i, j);
                fclose(fptr);
                exit(1);
            }
        }
    }
    fclose(fptr);
}