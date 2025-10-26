#include "function.h"
#include "graph.h"
#include "queue.h"
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

double optimal = 999999999999.0;
int flag = 0;
int NUMCITIES = 0;

/* s[] is declared in headers. It represents the current tour. */

static void shuffle_current_tour(void) {
    for (int i = NUMCITIES - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        int tmp = s[i];
        s[i] = s[j];
        s[j] = tmp;
    }
}

void printS(double cities[MAXNUM][MAXNUM])
{
    int x;
    double c;
    for (x = 0; x < NUMCITIES; x++) {
        printf("|%d|", s[x]);
    }
    printf("|%d|", s[0]);
    printf("\n");

    c = cost(cities);

    if (flag == 1) {
        insert((float)c);
    }
    printf("Cost equals %lf\n", c);
}

void swap(int a, int b)
{
    int temp;
    temp = s[a];
    s[a] = s[b];
    s[b] = temp;
}

void reset()
{
    /* heap contents do not matter if we reset sizes */
    theSize = 0;
}

void resetS()
{
    storeSize = 0;
}

void permute(int nfact, double cities[MAXNUM][MAXNUM])
{
    int m, k, p, q, i;
    for (i = 1; i < nfact; i++) {
        m = NUMCITIES - 2;
        while (m > 0 && s[m] > s[m + 1]) {
            m = m - 1;
        }

        k = NUMCITIES - 1;
        while (s[m] > s[k]) {
            k = k - 1;
        }
        swap(m, k);

        p = m + 1;
        q = NUMCITIES - 1;

        while (p < q) {
            swap(p, q);
            p++;
            q--;
        }
        printS(cities);
    }
}

int factorial(int n)
{
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

double cost(double cities[MAXNUM][MAXNUM])
{
    double sum = 0.0;
    for (int i = 0; i < NUMCITIES; i++) {
        int j = (i + 1) % NUMCITIES;
        sum += cities[s[i]][s[j]];
    }
    if (sum < optimal) {
        optimal = sum;
    }
    return sum;
}

/* mutate top of PQ by swapping two random nonzero indices */
void mutate(void)
{
    if (NUMCITIES <= 2 || theSize == 0) return;
    int i = 1 + rand() % (NUMCITIES - 1);
    int j = 1 + rand() % (NUMCITIES - 1);
    if (i == j) j = (j % (NUMCITIES - 1)) + 1;
    int temp = PQ[1].tour[i];
    PQ[1].tour[i] = PQ[1].tour[j];
    PQ[1].tour[j] = temp;
}

void brute(double cities[MAXNUM][MAXNUM])
{
    int i;
    int nfact;
    printf("Brute Force\n");
    printf("Please enter the number of cities to permute\n");
    scanf("%d", &i);

    nfact = factorial(i - 1);
    NUMCITIES = i;

    struct timeval t;
    int timeb;
    int timea;

    gettimeofday(&t, NULL);
    timeb = t.tv_sec;

    for (int x = 0; x < NUMCITIES; x++) {
        s[x] = x;
    }
    printS(cities);
    permute(nfact, cities);

    gettimeofday(&t, NULL);
    timea = t.tv_sec;

    printf("Optimal cost %lf\n", optimal);
    printf("It took %d Secounds\n", timea - timeb);
}

void genetic(double cities[MAXNUM][MAXNUM])
{
    static int seeded = 0;
    if (!seeded) { srand((unsigned)time(NULL)); seeded = 1; }

    int gnum, pop, mut, city, count;
    int elitePercent;
    count = 0;

    optimal = 1e300;

    printf("\n\nGenetic\n");
    printf("How many generation to run?\n");
    scanf("%d", &gnum);

    printf("Population size? (0-12)\n");
    scanf("%d", &pop);
    while (pop > 12) {
        printf("Please select a number (0-12)\n");
        scanf("%d", &pop);
    }

    printf("How many cities are you visiting (0-19)\n");
    scanf("%d", &city);
    while (city > 19 || city < 0) {
        printf("Please re-enter (0-19) cities \n");
        scanf("%d", &city);
    }
    NUMCITIES = city;

    printf("Percentage of population to be kept as elites?\n");
    scanf("%d", &elitePercent);
    int elite = (int)(pop * (elitePercent / 100.0));
    if (elite < 1) elite = 1;
    if (elite > pop) elite = pop;

    printf("How many cities to swap on mutations?\n");
    scanf("%d", &mut);
    if (mut < 1) mut = 1;

    struct timeval t;
    int timeb;
    int timea;

    gettimeofday(&t, NULL);
    timeb = t.tv_sec;

    for (int x = 0; x < city; x++) {
        s[x] = x;
    }
    shuffle_current_tour();

    printf("Generation %d\n", ++count);
    flag = 1;
    printS(cities);
    permute(pop, cities);

    int track = 0;
    int rem = pop - (mut + elite);
    for (int gen = 0; gen < gnum; gen++) {
        for (track = 0; track < elite; track++) {
            insertStore();
            deleteMin();
        }

        for (track = 0; track < mut; track++) {
            mutate();
            insertStore();
            deleteMin();
        }

        reset();

        for (int i = 1; i <= storeSize; i++) {
            memcpy(s, store[i].tour, TOURNUM * sizeof(int));
            insert(store[i].cost);
        }

        resetS();
        printf("Generation %d\n", ++count);
        if ((mut + elite) < pop) {
            permute(rem, cities);
        }
    }
    gettimeofday(&t, NULL);
    timea = t.tv_sec;

    printf("\n");
    printf("Optimal cost %lf\n", optimal);
    printf("It took %d Secounds\n", timea - timeb);
    flag = 0;
}