#ifndef TOKEN_H
#define TOKEN_H
#include <iostream>
#include <string>

enum TokenTypeT
{
#define TOKEN(name, text) name,
#include "tokenTypes.def"
#undef TOKEN
};

typedef struct
{
	std::string strValue;
	int iValue;
	double fValue;
}ValueT;

class Token
{
public:
	enum TokenTypeT type;
	std::string text, strType;
	int line, col;
	ValueT val;

	Token();
	Token(TokenTypeT _type, std::string _strType, std::string _text);
	Token(int line, int col);
	~Token();
	friend std::ostream& operator<< (std::ostream &s, Token tok);
	operator TokenTypeT() const;
	bool isKeyWord();
	void setTag(TokenTypeT t, std::string _strType, std::string _text);
	bool operator==(const Token& t);
};


#endif