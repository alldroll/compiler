#include "token.h"

using namespace std;

class SymbolTable;

class SyntaxNode
{
	friend class Parser;
	friend class AsmFunction;
protected :

	Token token;
public :

	SyntaxNode();
	SyntaxNode(Token _token);
	~SyntaxNode();
	SyntaxNode(SyntaxNode &node);

	virtual void print(string str, bool isTail);
	string& getName(){return token.text;}
	string getValue(int type);
	int getLine(){return token.line;}
	int getCol(){return token.col;}
	virtual SymbolTable* getTable(){return NULL;}
};