#ifndef _FUNCTION_H_
#define _FUNCTION_H_
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "graph.h"
#include "queue.h"

//restriction case. If over this number you might as well as did brute force
#define maxperm 12
#define maxtour 20
#define TOURNUM 21
#define max

int s[TOURNUM];

void printS(double cities[MAXNUM][MAXNUM]);
void swap(int , int );
void permute(int nfact, double cities[MAXNUM][MAXNUM]);
void brute(double cities[MAXNUM][MAXNUM]);
void genetic(double cities[MAXNUM][MAXNUM]);
int factorial(int);
double cost(double cities[MAXNUM][MAXNUM] );
void mutate();


#endif

