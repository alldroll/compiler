#include "exceptions.h"

string CompilerException::text() const throw()
{
	stringstream ss;
	ss << "(" << _line << ", " << _col << ") " << _err;
	return ss.str();
}

string ScannerException::text() const throw()
{
	string e("Lexical Error: ");
	e += CompilerException::text();
	return e;
}

string ParserException::text() const throw()
{
	string e("Syntax Error: ");
	e += CompilerException::text();
	return e;
}

string SemanticsException::text() const throw()
{
	string e("Syntax Error: ");
	e += CompilerException::text();
	return e;
}
