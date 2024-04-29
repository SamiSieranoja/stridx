all: demo

demo: 
	g++  -Wall -O3 -fopenmp -lstdc++ demo.cpp -o demo
	
clean: 
	rm demo


