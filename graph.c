#include "graph.h"



void test(double cities[MAXNUM][MAXNUM])
{

	int x=0;
	int y=0;
	
	FILE *fptr;

	fptr = fopen ("cities.dat", "r");

	if(fptr==NULL)
	{
		fprintf(stderr, "Error\n");
		exit(1);
	}
	fscanf (fptr, "%lf", &cities[x][y]);

	printf("|%lf|\n", cities[x][y] );


}


void pGraph(double cities[MAXNUM][MAXNUM])
{
	int x;
	int y;
	int index=65;
	printf("The Contents of the graph\n");
	printf(" _____|");
	for(x=0; x<MAXNUM ; x++)
	{
		printf("|%3c  |", index);
		index+=1;
	}
	printf("\n");
	index=65;
	for(y=0; y<MAXNUM ; y++)
	{
		printf("|%3c  |", index);
		for(x=0; x<MAXNUM ; x++)
		{
			{
				printf("|%5.1lf|", cities[x][y] );
			}
		}
		printf("\n");
		index+=1;
	}

}


void iGraph(double cities[MAXNUM][MAXNUM])
{
	int x;
	int y;
	
	FILE *fptr;

	fptr = fopen ("cities.dat", "r");

	if(fptr==NULL)
	{
		fprintf(stderr, "Error\n");
		exit(1);
	}

	for(x=0; x<MAXNUM; x++)
	{
		for(y=0; y<MAXNUM; y++)
		{
				if(y==x)
				{
					cities[x][y]=0;
				}
				else
				{
					fscanf (fptr, "%lf", &cities[x][y]);
				}

		}
	}
	
	fclose (fptr);
}
	
