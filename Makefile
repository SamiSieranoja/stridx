all: demo

demo: *.hpp *.cpp Makefile
	#g++  -Wall -O0 -g -lstdc++ demo.cpp -o demo
	g++  -Wall -O3 -lstdc++ demo.cpp -o demo
	
clean: 
	rm demo


