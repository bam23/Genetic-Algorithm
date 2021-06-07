#include "function.h"
#include "graph.h"

int NUMCITIES=0;

double optimal = 999999999999;
int flag=0;

void printS(double cities[MAXNUM][MAXNUM])
{
	int x; 
	double c;
	//printf("Your Tour\n");
	for (x = 0; x < NUMCITIES; x++)
	{ 
		printf("|%d|", s[x]);
	}
	printf("|%d|", s[0]);
	printf("\n");
	
	c = cost(cities);

	if(flag==1)
	{
		insert(c);
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
	memset(PQ, 0,MAXSIZE+1);
	theSize = 0;
}

void resetS()
{
	memset(store, 0,MAXSIZE+1);
	storeSize = 0;
}


void permute(int nfact, double cities[MAXNUM][MAXNUM]) 
{
	int m, k, p , q, i;
	//m = NUMCITIES - 2;

	for(i = 1; i < nfact; i++)
	{	
		m = NUMCITIES - 2;
		while(s[m]>s[m+1])
		{
			m = m - 1;
		}
		
		k = NUMCITIES-1;

		while(s[m] > s[k])
		{
			k = k - 1;
		}

		swap(m,k);

		p = m + 1;
		q = NUMCITIES-1;
		
		while( p < q)
		{
			swap(p,q);
			p++;	
			q--;
		}
	
		printS(cities);
	}
	
} 


void brute(double cities[MAXNUM][MAXNUM])
{

	int i;
	
	int nfact;
	printf("Brute Force\n");
	printf("Please enter the number of cities to permute\n");
	scanf("%d", &i);

	nfact=factorial(i-1);
	//printf("Factorial equals %d\n", nfact);
	
	NUMCITIES = i;

	struct timeval * t;
  
  	int timeb;
  	int timea;

 	 t = (struct timeval *)malloc(sizeof(struct timeval));

 	gettimeofday(t,NULL);
  	timeb = t->tv_sec;

	int x;
	
	for (x=0; x<NUMCITIES; x++)
	{
		s[x]=x;

	}


	printS(cities);

	
	permute(nfact, cities);

	gettimeofday(t,NULL);
  	timea = t->tv_sec;
  	
	printf("Optimal cost %lf\n", optimal);
	printf("It took %d Secounds\n",timea-timeb); 
	
}


int factorial( int n )
{
    if ( n <= 1 )
        return 1;
    else
        return  n * factorial( n-1 );
}

double cost(double cities[MAXNUM][MAXNUM])
{
	double sum;
	int x;
	//price of going the path
	for(x=0; x<NUMCITIES; x++)
	{
		sum+= cities[ s[x] ] [ s[x+1] ] ;
	}

	//returning back to the source
	sum += cities[ s[0] ] [ s[x-1] ];

	if(sum<optimal)
	{
		optimal = sum;
	}
	
	return sum;
}

void mutate()
{
	int temp;

	temp = PQ[1].tour[1];

	PQ[1].tour[1] = PQ[1].tour[2];
	PQ[1].tour[2] = temp;

}


void genetic(double cities[MAXNUM][MAXNUM])
{
	int gnum, pop, elite, mut, city, count;
	float temp;

	count=0;

	printf("\n");
	printf("\n");
		printf("Genetic\n");
	//how many sets of permutations/routes to do
	printf("How many generation to run?\n");
	scanf("%d", &gnum);

	//how many Permutations/# of routes in a population
	printf("Population size? (0-12)\n");
	scanf("%d", &pop);

	while (pop>12)
	{
		printf("Please select a number (0-12)\n");
		scanf("%d", &pop);
	}

	//number is used to calculate route
	printf("How many cities are you visiting (0-19)\n");
	scanf("%d", &city);

	while(city>19 || city < 0)
	{
		printf("Please re-enter (0-19) cities \n");
		scanf("%d", &city);
	}
	
	NUMCITIES=city;
	
	//how many paths to keep to re populate next generation
	printf("Percentage of population to be kept as elites?\n");
	scanf("%d", &elite);

	temp = elite/100;
	elite=elite*temp;
	//printf("Elite equals %d\n", elite);
	if(elite==0)
	{
		elite = 1;
	}
	//printf("Elite equals %d\n", elite);

	//how many mutations to make/permuations
	printf("How many cities to swap on mutations?\n");
	scanf("%d", &mut);

	//while (mut>(pop-elite))
	//{
	//	print("There is not enough of the population left");
	//	printf("How many cities to swap on mutations?\n");
	//	scanf("%d", &mut);

	//}

	struct timeval * t;
  
  	int timeb;
  	int timea;

 	 t = (struct timeval *)malloc(sizeof(struct timeval));

 	gettimeofday(t,NULL);
  	timeb = t->tv_sec;


		//int nfact = factorial(mut-1);
		int x;
		for (x=0; x<city; x++)
		{
			s[x]=x;
		}

		printf("Generation %d\n",++count);
		flag=1;
		printS(cities);
		permute(pop, cities);


	int track=0; 
	int rem=pop-(mut+elite);
	for(x=0; x<gnum; x++)
	{	

		for(track=0; track < elite; track++)
		{
			insertStore();
			deleteMin();
		}

		for(track=0; track < mut; track++)
		{	
			mutate();
			insertStore();
			deleteMin();
		}

		reset();

		for(track=0; track<storeSize; track++)
		{
			memcpy(s,store[1].tour,TOURNUM);
			insert(store[1].cost);
		}

		resetS();
		printf("Generation %d\n",++count);
		if( (mut+elite) < pop )
		{
			permute(rem, cities);

		}
	}	
		gettimeofday(t,NULL);
  		timea = t->tv_sec;

		printf("\n");
		printf("Optimal cost %lf\n", optimal);
		printf("It took %d Secounds\n",timea-timeb); 
		//print();
		flag=0;
}



