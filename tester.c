/*************************************************************** 
Tester File for the project
***************************************************************/

#include "graph.h"
#include "function.h"

int main()
{

	double cities[MAXNUM][MAXNUM];

	iGraph(cities);
	pGraph(cities);
	brute(cities);

	genetic(cities);


	
return 0;
}


