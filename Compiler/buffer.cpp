#include "buffer.h"

FILE  *stream;
bool isEOF = false, isEOLN = false, is;

Buffered_stream::Buffered_stream(FILE* f, int& _line, int& _col) : pos(0), size(0), line(_line), col(_col) {stream = f;};

char Buffered_stream::get()
{
	++col;
	if (isEOF)
		return EOF;
	if (!hasNext())
	{
		pos = 0;
		size = fread(buffer, sizeof(char), BUFFER_SIZE, stream);
		isEOF = !size;
	}
	if (isEOLN)
	{
		isEOLN = false;
		++line;
		col = 1;
	}
	char ch = isEOF ? EOF : buffer[pos++];
	isEOLN = ch == '\n';
	return ch;
}

void Buffered_stream::fillQueueFrom(queue<char> &_queue, int index)
{
	if (!size || index > pos)
		return;
	for (; index < pos; index++) 
		_queue.push(buffer[index]);
}

int Buffered_stream::getpos()
{
	return pos - 1;
}

void Buffered_stream::back(int count)
{
	col -= count;
	if (isEOF)
		return;
	if (pos - count >= 0)
		pos -= count;
	if (isEOLN)
		isEOLN = false;
}

bool Buffered_stream::hasNext()
{
	return pos != size;
}
