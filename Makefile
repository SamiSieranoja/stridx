all: demo

demo: *.hpp *.cpp Makefile
	g++  -Wall -O3 -lstdc++ demo.cpp -o demo
	
clean: 
	rm demo


