tsp : graph.o tester.c function.o queue.o
	gcc -o tsp graph.o tester.c function.o queue.o

queue.o : queue.c queue.h function.h
	gcc -c -Wall queue.c

graph.o : graph.c graph.h
	gcc -c -Wall graph.c
	
function.o: function.c function.h graph.h queue.h
	gcc -c -Wall function.c

clean : 
	rm tsp *.o  
