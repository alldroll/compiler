#ifndef BUFFER_H
#define BUFFER_H
#define BUFFER_SIZE 4096
#include <stdio.h>
#include <queue>
#include <sstream>
#include <stdlib.h>
#include <iostream>

using namespace std;

class Buffered_stream
{
private:

	char buffer[BUFFER_SIZE];
	int pos, size, &line, &col;

public:

	Buffered_stream(FILE* f, int& _line, int& _col);
	char get();
	int getpos();
	bool hasNext();
	void fillQueueFrom(queue<char> &_queue, int index); 
	void back(int count);
};


template<typename T> 
extern string to_string(const T& v)
{
    ostringstream stm;
    return ( stm << v ) ? stm.str() : "0";
}

#endif