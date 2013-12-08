#ifndef SCANNER_H
#define SCANNER_H
#include "token.h"
#include <map>
#include "buffer.h"
#include "exceptions.h"

extern string getTypeText(TokenTypeT type);

class Scanner
{
private:

	enum StateT
	{
		TOKEN_IS_NOT_INIT,
		TOKEN_IS_INIT,
	};

	enum ReadT
	{
		WITHOUT_ADDITIONALBUF,
		WITH_ADDITIONALBUF,
	};

	queue<char> additional_buf;
	Buffered_stream *buf_stream;
	string tokenText;
	char ch;
	int line, col, colTokenBegin, lineTokenBegin, tokenBegin;
	StateT state; 
	bool wasPrev;
	
	void readIdentAndKeyword();
	void readChar();
	void readStr();
	void readOperator();
	void readFloatPart();
	void readIntPart();
	void readNum();
	void readSeparator();
	bool isOperator();
	bool isSeparator();
	bool trySkipComments();
	bool trySkipWhiteSpaces();
	void initTokenText();
	void tryUseAdditionalBuf();
	void processEscapeSequences();

	char getChar(ReadT type);
public:
	Scanner(FILE* stream);
	Scanner(Scanner &scan);
	~Scanner();
	Token currToken, prevToken;
	Token& get();
	Token& next();
	Token& prev();
	int getLine() const;
	int getCol() const;
};


#endif