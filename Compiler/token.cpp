#include "token.h"

Token::Token() : line(0), col(0), type(UNDEFINED){val.strValue = ""; text = "undefined";};

Token::Token(TokenTypeT _type, std::string _strType, std::string _text) : type(_type), text(_text), line(0), col(0) 
{
	strType = _strType; 
	val.strValue = _type == STRING_CONST ? '"' + text + '"' : "nan";
};

Token::~Token() {};

std::ostream& operator<< (std::ostream &s, Token tok)
{
	s << "(" << tok.line << "," << tok.col << ") " << "\ttype\t" + tok.strType << "\tvalue\t";
	if  (tok.type == INT_CONST)
		s << tok.val.iValue;
	else
		if (tok.type == DOUBLE_CONST)
			s << tok.val.fValue;
	else
		s << tok.val.strValue;
	return s << "\ttext\t" + tok.text; 
}

Token::operator TokenTypeT() const
{
	return type;
}

bool Token::isKeyWord()
{
	return strType.substr(0, 2) == "KW";
}

void Token::setTag(TokenTypeT t, std::string _strType, std::string _text)
{
	type = t;
	strType = _strType;
	text = _text;
}

bool Token::operator==(const Token& t)
{
	return type == t.type && strType == t.strType && text == t.text;
}

Token::Token(int l, int c)
{
	type = UNDEFINED;
	text = "undefined";
	line = l;
	col = c;
}