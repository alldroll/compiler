#include "syntaxNode.h"
#include <sstream>																																																																																																			
#include <iostream>

template<typename T> 
string to_string(const T& v)
{
    ostringstream stm;
    return ( stm << v ) ? stm.str() : "0";
}

SyntaxNode::SyntaxNode() {};

SyntaxNode::SyntaxNode(Token _token) : token(_token) {};

SyntaxNode::~SyntaxNode() {};

void SyntaxNode::print(string str, bool isTail)
{
	string out = (str.size() == 0 ? "-->" : "|__");
	cout << str + out << token.text << " ";
}

string SyntaxNode::getValue(int type)
{
	switch(type)
	{
	case 0 : return to_string(token.val.iValue);
	case 1 : {float f = (float)token.val.fValue; return to_string(*(int*)&token.val.fValue);}
	case 2 : return to_string(*((long long*)&token.val.fValue));
	}																
}