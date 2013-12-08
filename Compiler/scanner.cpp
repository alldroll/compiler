#include "scanner.h"
#include <stdlib.h>

map<string, Token> operators, keywords, separators;

const char* TOKEN_TYPE_NAMES[75] = 
{
#define TOKEN(name, text) #name,
#include "tokenTypes.def"
#undef TOKEN
};

const char* TOKEN_TEXT[75] =
{
#define TOKEN(name, text) text,
#include "tokenTypes.def"
#undef TOKEN
}; 

#define OP_END OP_COLON + 1 
#define KW_END KW_WHILE + 1
#define SEP_END SEMICOLON + 1
#define TOK_BEGIN L_BRACE 
#define TOK_END KW_END

typedef enum
{
	LINE,
	BEGIN,
	END,
	IS_NOT_COMMENT,
}CommentTypeT;

static void initTables()
{
	for (int t = TOK_BEGIN; t < TOK_END + 1; t++)
	{	
		if (t < SEP_END)
		{
			separators[TOKEN_TEXT[t]] = Token((TokenTypeT)t, TOKEN_TYPE_NAMES[t], TOKEN_TEXT[t]);
			continue;
		}
		if (t < OP_END)
		{
			operators[TOKEN_TEXT[t]] = Token((TokenTypeT)t, TOKEN_TYPE_NAMES[t], TOKEN_TEXT[t]);
			continue;
		}
		if (t < KW_END)
				keywords[TOKEN_TEXT[t]] = Token((TokenTypeT)t, TOKEN_TYPE_NAMES[t], TOKEN_TEXT[t]);
	}
}

Scanner::Scanner(FILE* stream) : line(1), col(0), colTokenBegin(0), lineTokenBegin(1), wasPrev(false) 
{
	buf_stream = new Buffered_stream(stream, line, col);
	initTables();
};

Scanner::~Scanner() {delete buf_stream;};

static CommentTypeT getCommentType(string tokenText)
{
	return tokenText == "//" ? LINE : tokenText == "/*" ? BEGIN : tokenText == "*/" ? END : IS_NOT_COMMENT;
}

static __inline bool isWhiteSpace(char ch)
{
	return ch == '\n' || ch == '\t' || ch == '\v' || ch == ' ';
}

static __inline  bool isEOF(char ch)
{
	return ch == EOF;
}

static __inline  bool isEOLN(char ch)
{
	return ch == '\n';
}

static __inline  bool isHex(char ch)
{
	return ch >= 'a' && ch <= 'f' || ch >= 'A' && ch <= 'F'; 
}

static __inline bool isOct(char ch)
{
	return ch >= '0' && ch <= '7';
}

static int getDigitValue(char ch)
{
	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 0xA;
	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 0xA;
	return ch - '0';
}

static bool isDigitBase(char ch, int base)
{
	switch(base)
	{
		case 8 : return isOct(ch);
		case 10 : return isdigit(ch);
		default : return isdigit(ch) || isHex(ch);
	}
	return 0;
}

extern string getTypeText(TokenTypeT type)
{
	return TOKEN_TEXT[type];
}

int Scanner::getLine() const
{
	return line;
}

int Scanner::getCol() const
{
	return col;
}

char Scanner::getChar(ReadT type = WITH_ADDITIONALBUF)
{
	if (type == WITH_ADDITIONALBUF)
		tryUseAdditionalBuf();
	return ch = buf_stream->get();;
}

bool Scanner::trySkipComments()
{
	if (isEOF(ch))
		return false;
	tokenText.push_back(ch);
	tokenText.push_back(buf_stream->get());
	bool isComment = true;
	switch (getCommentType(tokenText))
	{
		case LINE : while (!isEOLN(ch) && !isEOF(ch)) getChar(WITHOUT_ADDITIONALBUF);break;
		case BEGIN : 
			{
				bool endfounded = false;
				while (!isEOF(ch) && !endfounded)
				{
					if (getChar(WITHOUT_ADDITIONALBUF) == '*')
						if (!(endfounded = (!isEOF(ch) && (getChar(WITHOUT_ADDITIONALBUF) == '/'))))
							buf_stream->back(1);
				}
				if (isEOF(ch))
					throw ScannerException(line, col, "expected */");
				getChar(WITHOUT_ADDITIONALBUF);
			}break;
		case END : throw ScannerException(line, col, "expected /*");
		default : { buf_stream->back(1); isComment = false; };
	}
	tokenText.clear();
	return isComment;
}

bool Scanner::trySkipWhiteSpaces()
{
	if (!isWhiteSpace(ch))
		return false;
	while (isWhiteSpace(ch))
		getChar(WITHOUT_ADDITIONALBUF);
	return !isEOF(ch);	
}

bool Scanner::isOperator()
{
	return operators.find(tokenText) != operators.end();
}

void Scanner::readOperator()
{
	if (state == TOKEN_IS_INIT)
		return;
	bool isRead = false;
	while (isOperator() || (!isRead && ch == '|'))
	{
		isRead = true;
		tokenText.push_back(getChar(WITHOUT_ADDITIONALBUF));
		if (isEOLN(ch) || !isOperator())
			buf_stream->back(1);	
	}
	if (state = StateT(isRead))
		currToken = operators[tokenText.substr(0, tokenText.size() - 1)];
}

bool Scanner::isSeparator()
{
	return separators.find(tokenText) != separators.end();
}

void Scanner::readSeparator()
{
	if (state == TOKEN_IS_INIT || !isSeparator())
		return;
	currToken = separators.find(tokenText)->second;
	state = TOKEN_IS_INIT;
}

void Scanner::readFloatPart()
{
	if (ch == '.')
	{
		getChar();
		while(isDigitBase(ch, 10))
			getChar();
	}
	if (ch == 'e' || ch == 'E')
	{
		getChar();
		if (ch == '+' || ch == '-')
			getChar();
		int i = 0;
		for (; isDigitBase(ch, 10); i++)
			getChar();
		if (i == 0)
			throw ScannerException(lineTokenBegin, colTokenBegin, "invalid float constant");
	}
	if (ch == '.' || ch == 'e' || ch == 'E')
		throw ScannerException(lineTokenBegin, colTokenBegin, "invalid float constant");
	currToken = Token(DOUBLE_CONST, TOKEN_TYPE_NAMES[DOUBLE_CONST], tokenText);
}

void Scanner::readNum()
{
	if (state == TOKEN_IS_INIT || !isdigit(ch))
		return;
	int base = 10, intValue = 0, digit = 0;
	bool isHexOct = false, isFirstDigitNull = false;
	for (; isDigitBase(ch, base) || isFirstDigitNull; digit++)
	{
		if (digit == 0)
			isHexOct = isFirstDigitNull = (ch == '0');
		intValue = intValue * base + getDigitValue(ch);
		getChar();
		if (isFirstDigitNull)
		{
			base = (ch == 'x' || ch == 'X') ? 16 : 8;
			if (ch == '.')
				base = 10;
			isFirstDigitNull = false;
		}
		if (base == 8 && isdigit(ch) && !isOct(ch))
			base = 10;
		if (digit == 0 && base == 16)
			getChar(WITHOUT_ADDITIONALBUF);
	}
	if (isHexOct)
	{
		if (base == 16 && (digit == 1 || ch == '.'))
			throw ScannerException(lineTokenBegin, colTokenBegin, "invalid hex constant");
		if (base == 10 && ch != '.' && ch != 'e' && ch != 'E')
			throw ScannerException(lineTokenBegin, colTokenBegin, "invalid oct constant");
	}
	currToken = Token(INT_CONST, TOKEN_TYPE_NAMES[INT_CONST], tokenText);
	currToken.val.iValue = intValue;
	if (base == 10 && ch == '.' || ch == 'e' || ch == 'E')
		readFloatPart();
	initTokenText();

	//if (currToken.type != INT_CONST)
		currToken.val.fValue = atof(tokenText.c_str());
	currToken.text = tokenText;
	state = TOKEN_IS_INIT;
}

void Scanner::tryUseAdditionalBuf()
{
	if (tokenBegin < 0)
	{
		additional_buf.push(tokenText[0]);
		tokenBegin = 0;
	}
	if (!buf_stream->hasNext())
	{
		buf_stream->fillQueueFrom(additional_buf, tokenBegin);
		tokenBegin = 0;
	}
}

void Scanner::initTokenText()
{
	tokenText.clear();

	int tokenEnd = buf_stream->getpos() < 0 ? 0 : buf_stream->getpos(), valSize = additional_buf.size() + tokenEnd - tokenBegin;
	if (valSize >= tokenText.max_size())
		throw ScannerException(lineTokenBegin, colTokenBegin, "too long name");
		
	while (!additional_buf.empty())
	{
		tokenText.push_back(additional_buf.front());
		additional_buf.pop();
	}
	buf_stream->back(tokenEnd - tokenBegin + 1);
	
	for(int i = tokenBegin; i < tokenEnd; i++)
		tokenText.push_back(buf_stream->get());
}

void Scanner::processEscapeSequences()
{
	if (ch != '\\')
		return;
	char next = buf_stream->get();
	switch(next)
	{
        case '\'': ch = '\''; break;
        case '\"': ch = '\"'; break;
        case '\\': ch = '\\'; break;
        case '?': ch = '\?'; break;
        case 'a': ch = '\a'; break;
        case 'b': ch = '\b'; break;
        case 'f': ch = '\f'; break;
        case 'n': ch = '\n'; break;
        case 'r': ch = '\r'; break;
        case 't': ch = '\t'; break;
        case 'v': ch = '\v'; break;
		default : buf_stream->back(1);
	}
}

void Scanner::readIdentAndKeyword()
{
	if (state == TOKEN_IS_INIT || (ch != '_' && !isalpha(ch)))
		return;
	while (isalpha(ch) || ch == '_' || isdigit(ch))
		getChar();
	initTokenText();
	currToken = keywords.find(tokenText) != keywords.end() ? keywords[tokenText] : Token(IDENTIFIER, TOKEN_TYPE_NAMES[IDENTIFIER], tokenText);
	state = TOKEN_IS_INIT;
}

void Scanner::readStr()
{
	if (state == TOKEN_IS_INIT || ch != '"')
		return;
	do
		getChar();
	while (ch != '"' && !isEOF(ch) && !isEOLN(ch));

	if (isEOF(ch) || isEOLN(ch))
		throw ScannerException(line, col, "expected end of the string");
		
	initTokenText();
	tokenText.push_back(ch = buf_stream->get());
	string tmp;
	int i = 0;
	for (; i < tokenText.size(); i++)
	{
		if (tokenText[i] != '\\')
		{
			tmp.push_back(tokenText[i]);
			continue;
		}
		char& next = tokenText[++i];
		switch(next)
		{
			case '\'': tmp.push_back('\''); break;
			case '\"': tmp.push_back('\"'); break;
			case '\\': tmp.push_back('\\'); break;
			case '?': tmp.push_back('\?'); break;
			case 'a': tmp.push_back('\a'); break;
			case 'b': tmp.push_back('\b'); break;
			case 'f': tmp.push_back('\f'); break;
			case 'n': tmp.push_back('\n'); break;
			case 'r': tmp.push_back('\r'); break;
			case 't': tmp.push_back('\t'); break;
			case 'v': tmp.push_back('\v'); break;
		}
	}
	currToken = Token(STRING_CONST, TOKEN_TYPE_NAMES[STRING_CONST], tokenText);
	currToken.val.strValue = tmp;
	state = TOKEN_IS_INIT;
}

void Scanner::readChar()
{
	if (state == TOKEN_IS_INIT || ch != '\'')
		return;
	getChar(WITHOUT_ADDITIONALBUF);
	while (ch != '\'' && !isEOF(ch) && !isEOLN(ch))
	{
		processEscapeSequences();
		tokenText.push_back(ch);
		getChar(WITHOUT_ADDITIONALBUF);
	}
	
	if (isEOF(ch) || isEOLN(ch))
		throw ScannerException(line, col, "invalid char const");
	tokenText.push_back(ch);
	currToken = Token(CHAR_CONST, TOKEN_TYPE_NAMES[CHAR_CONST], tokenText);
	if (tokenText.length() == 2 ||  tokenText.length() > 3)
		throw ScannerException(line, col, "improper char's length");
	currToken.val.strValue = tokenText.substr(1, tokenText.size() - 2); 
	state = TOKEN_IS_INIT;
}

Token& Scanner::next()
{
	if (wasPrev)
	{
		wasPrev = false;
		return currToken;
	}
	prevToken = currToken;
	tokenText.clear();
	getChar(WITHOUT_ADDITIONALBUF);
	state = TOKEN_IS_NOT_INIT;

	while (trySkipComments() || trySkipWhiteSpaces());
	
	tokenText.push_back(ch);
	lineTokenBegin = line;
	colTokenBegin = col;
	tokenBegin = buf_stream->getpos();

	readOperator();
	readSeparator();
	readStr();
	readNum();
	readIdentAndKeyword();
	readChar();
	
	if (state == TOKEN_IS_NOT_INIT && isEOF(ch))
	{
		state = TOKEN_IS_INIT;
		currToken = Token(EOF_TOKEN, TOKEN_TYPE_NAMES[EOF_TOKEN], TOKEN_TEXT[EOF_TOKEN]);
	}

	currToken.line = lineTokenBegin;
	currToken.col = colTokenBegin;

	if (state == TOKEN_IS_NOT_INIT)
		throw ScannerException(line, col, "undefined");
	return currToken;
}

Token& Scanner::get()
{
	return currToken;
}

Token& Scanner::prev()
{
	wasPrev = true;
	return prevToken;
}