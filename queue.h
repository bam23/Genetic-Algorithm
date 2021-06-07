#ifndef _queue_h
#define _queue_h
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <unistd.h>
#include "function.h"

//physical size of the dataspace
#define MAXSIZE 479001600
#define TOURNUM 21



struct node
{

int tour[TOURNUM];

float cost;

};

typedef struct node node;

node PQ[MAXSIZE+1];
node store[MAXSIZE+1];

//the logical size
int theSize;
int storeSize;

//moves items to the top
void percolateUp();

//delets top of heap
float deleteMin( );

//prints heap
void print();

//insertion to the bottom of heap
void insert(int );

//moves items to bottom
void percolateDown(int );

//tester program to demonstrate heap
void tester();

void insertStore();
void percolateUpStore( );
void percolateDownStore(int slot);
float deleteMinStore( );









#endif
