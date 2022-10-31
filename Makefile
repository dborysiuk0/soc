all: server

server: main.cpp
	g++ main.cpp -o server -pthread -O0 -ggdb3

