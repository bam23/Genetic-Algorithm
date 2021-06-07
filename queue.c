#include "queue.h"


void tester()
{
	int x, i, n, R;

	printf("How many #'s are you inputting\n");

	do	
	{
		scanf("%d",&n);
	}while (n>MAXSIZE || n<0);

	for (i=0; i < n; i ++)
	{	
		printf("Enter Number\n");
		scanf("%d", &x);
		insert(x);
	}

	print();
	R=deleteMin( );
	print();
	printf("The number %d was removed\n", R);

}

void insert(int x)
{
	theSize+=1;
//inserts at the end of the heap
	PQ[theSize].cost=x;
 memcpy(PQ[theSize].tour,s,TOURNUM);

//percolatesUP the heap to satisfy condition
percolateUp( );

}



//prints the PQ
void print()
{
	printf("PQ contents\n");

	if(theSize!=0)
	{
		int i;
		for(i=1; i <=theSize; i++)
		{
			printf("|%.2f",PQ[i].cost);
		}
		printf("|\n");
	}
	else
	printf("The PQ is empty\n");


}


void percolateUp( )
{	//sets slot to end of heap
    int slot = theSize;

    //heap condition
    while(PQ[ slot ].cost < PQ[ slot / 2 ].cost) 
    { 
    	//temp swap holder
    	PQ[0].cost= PQ[ slot / 2 ].cost;
 memcpy(PQ[0].tour, PQ[ slot / 2 ].tour,TOURNUM);

    	//swaps variables
     	PQ[ slot / 2 ].cost = PQ[ slot ].cost;
 memcpy(PQ[ slot / 2 ].tour,  PQ[ slot ].tour,TOURNUM);

     	PQ[ slot ].cost = PQ[0].cost;
 memcpy(PQ[ slot ].tour,  PQ[0].tour,TOURNUM);
     	//print();

     		//changes index (next parent up)
		   slot /= 2;
    }
}

void percolateDown(int slot)
{
	int child;
  	int tmp = PQ[slot].cost;  
  	int tour[TOURNUM];

  	memcpy(tour,PQ[slot].tour,TOURNUM);
  	

	while(slot * 2 <= theSize)
	{
		child = slot * 2;
		if( child != theSize && PQ[ child + 1 ].cost < PQ[ child ].cost )
		{
			child++;
					 //printf("check3\n");
		}

		if( PQ[ child ].cost < tmp )
		{
			PQ[slot].cost = PQ[ child ].cost;
		 memcpy(PQ[slot].tour,  PQ[ child ].tour,TOURNUM);
			//printf("check4\n");
		}

		else
		{	
			break;
		}

	slot = child;

	}

         PQ[slot].cost = tmp;
  memcpy(PQ[slot].tour,tour,TOURNUM);
  
}


float deleteMin( )
{
  float tmp = PQ[ 1 ].cost;
  	PQ[ 1 ].cost = PQ[ theSize ].cost;
 memcpy(PQ[ 1 ].tour,  PQ[ theSize-- ].tour,TOURNUM);
 // printf("check2\n");
  
    percolateDown( 1 );
    PQ[0].cost=tmp;
    return tmp;
}






//Functions for store Queue
void insertStore( )
{
	storeSize+=1;
	//inserts at the end of the heap
	store[storeSize].cost=store[1].cost;
 memcpy(store[storeSize].tour,store[1].tour,TOURNUM);

	//percolatesUP the heap to satisfy condition
	percolateUpStore( );

}


void percolateUpStore( )
{	//sets slot to end of heap
    int slot = storeSize;

    //heap condition
    while(store[ slot ].cost < store[ slot / 2 ].cost) 
    { 
    	//temp swap holder
    	store[0].cost = store[ slot / 2 ].cost;
 memcpy(store[0].tour,  store[ slot / 2 ].tour,TOURNUM);

    	//swaps variables
     	store[ slot / 2 ].cost = store[ slot ].cost;
 memcpy(store[ slot / 2 ].tour,  store[ slot ].tour,TOURNUM);
     	store[ slot ].cost = store[0].cost;
 memcpy(store[ slot ].tour,  store[0].tour,TOURNUM);
     	//print();

     		//changes index (next parent up)
		   slot /= 2;
    }
}

void percolateDownStore(int slot)
{
	int child;
  	int tmp = store[slot].cost;  

	while(slot * 2 <= theSize)
	{
		child = slot * 2;
		if( child != theSize && store[ child + 1 ].cost < store[ child ].cost )
		{
			child++;
					 //printf("check3\n");
		}

		if( store[ child ].cost < tmp )
		{
			store[slot].cost = store[ child ].cost;
		 memcpy(store[slot].tour,  store[ child].tour,TOURNUM);
			//printf("check4\n");
		}

		else
		{	
			break;
		}

	slot = child;

	}

  store[slot].cost = tmp;
}


float deleteMinStore( )
{
  float tmp = store[ 1 ].cost;
  	store[ 1 ].cost = store[ storeSize ].cost;
 memcpy(store[ 1 ].tour,  store[storeSize--].tour,TOURNUM);
 // printf("check2\n");
  
    percolateDownStore( 1 );
    store[0].cost=tmp;
    return tmp;
}






