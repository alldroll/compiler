#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H
#include <string>
#include <sstream>

using namespace std;

class CompilerException : public exception
{
	private:
	int _line, _col;
	string _err;
public:
	
	CompilerException(const int line, const int col, const string& error) throw():
        exception(), _line(line), _col(col), _err(error) {}
    ~CompilerException() throw() {}
	
	virtual string text() const throw();
};

class ScannerException : public CompilerException
{
public:

	ScannerException(const int line, const int col, const string& error) throw():
        CompilerException(line, col, error) {}
	string text() const throw();
};

class ParserException : public CompilerException
{
public:

	ParserException(const int line, const int col, const string& error) throw():
        CompilerException(line, col, error) {}
	string text() const throw();
};

class SemanticsException : public CompilerException
{
public :

	SemanticsException(const int line, const int col, const string& error) throw():
        CompilerException(line, col, error) {}
	string text() const throw();
};



#endif