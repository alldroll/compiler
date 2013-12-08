#ifndef CODEGEN_H
#define CODEGEN_H
#include "parser.h"
#include <fstream>
#include <stack>

class SymTypeFunc;

typedef enum
{
	PREFIX_GLOBAL,
	PREFIX_LOCAL,
	PREFIX_PARAM,
	PREFIX_STRUCT,
	PREFIX_ENUM,
	PREFIX_LABEL,
	PREFIX_STRING_CONST,
	PREFIX_FLOAT_CONST,
	PREFIX_DOUBLE_CONST,
}PrefixT;

typedef enum
{
	ASM_RET,
	ASM_LEA,
	ASM_MOV,
	ASM_PUSH,
	ASM_POP,
	ASM_ADD,
	ASM_SUB,
	ASM_CALL,
	ASM_IMUL,
	ASM_IDIV,
	ASM_INC,
	ASM_DEC,
	ASM_NEG,
	ASM_NOT,
	ASM_TEST,
	ASM_JMP,
	ASM_JZ,
	ASM_JNZ,
	ASM_JL,
	ASM_JLE,
	ASM_JG,
	ASM_JGE,
	ASM_JE,
	ASM_JNE,
	ASM_OR,
	ASM_XOR,
	ASM_AND,
	ASM_CMP,
	ASM_SHL,
	ASM_SHR,
	ASM_SETL,
	ASM_SETLE,
	ASM_SETG,
	ASM_SETGE,
	ASM_SETE,
	ASM_SETNE,
	ASM_SETZ,
	ASM_SETB,
	ASM_SETA,
	ASM_SETAE,
	ASM_SETBE,
	ASM_CDQ,
	ASM_ASSIGN,
	ASM_NONE,

	ASM_MOVSD,
	ASM_MOVSS,
	ASM_COMISD,
	ASM_ADDSD,
	ASM_SUBSD,
	ASM_MULSD,
	ASM_DIVSD,
	ASM_CVTTSD2SI,
	ASM_CVTSI2SD,
	ASM_CVTTSD2S,
}CommandT;

typedef enum
{
	REG_EAX,
	REG_EBX,
	REG_ECX,
	REG_EDX,
	REG_ESP,
	REG_EBP,
	REG_BL,
	REG_CL,

	REG_XMM0,
	REG_XMM1,
	REG_XMM2,
	REG_XMM3,
	REG_XMM4,
	REG_XMM5,
	REG_XMM6,
}RegT;

extern string getPrefix(string var, PrefixT t), getReg(RegT r);

class AsmCode
{
public :
	AsmCode(){};
	virtual void print(ostream& s){};
	virtual bool isCommand(){return false;}
	virtual bool isLabel(){return false;}
	virtual bool isEOLN(){return false;}
};

class AsmCommand : public AsmCode
{
protected :

	CommandT com;
	string l, r;
public :

	AsmCommand(CommandT c, string _l, string _r) : com(c), l(_l), r(_r){};
	AsmCommand(CommandT c, string _l) : com(c), l(_l) {};
	AsmCommand(string _l) : com(ASM_NONE), l(_l){};
	bool isCommand(){return true;}
	CommandT& getCom(){return com;}
	string& getLeftOp(){return l;}
	string& getRightOp(){return r;}
	void print(ostream& s);
};

class AsmLocalVar
{
private :

	string l, r;
public :
	AsmLocalVar(string& _l, string& _r) : l(_l), r(_r){};
	AsmLocalVar(string& _l) : l(_l){}
	void print(ostream& s);
};

class AsmLabel : public AsmCode
{
private :
	string label;

public :
	AsmLabel(string& l) : AsmCode(), label(l){};
	AsmLabel() : AsmCode(){};
	void print(ostream& s);
	bool isLabel(){return true;}
	string& getLabelName(){return label;}
};

class Eoln : public AsmCode
{
private :

	int count;
public :
	Eoln(int c) : AsmCode(), count(c){};
	void print(ostream& s);
	bool isEOLN(){return true;}
};

class AsmFunction
{
private :

	string name;
	list<AsmCode*> code;
	list<AsmLocalVar*> localVars;
public :

	AsmFunction(string& _name) : name(_name){}
	string getName(){return name;}
	void addCode(AsmCode* com) {code.push_back(com);}
	void addLocalVars(AsmLocalVar* lvar) {localVars.push_back(lvar);}
	void print(ostream& s);
	void addPrologue(int& shift);
	void addEpilogue(int& shift);
	list<AsmCode*>& getCode(){return code;};
};

class AsmOperand
{
};

class AsmData
{
protected :
	
	string name, value;
public :

	AsmData(string _name, const string _value) : name(_name), value(_value){};
	string getValue(){return value;}
	virtual void print(ostream& s){s << name + " ";}
};

class AsmDD : public AsmData
{
private :

	size_t size;
public :

	AsmDD(string _name, const string _value) : AsmData(_name, _value), size(4){};
	AsmDD(string _name, const string _value, size_t sz) : AsmData(_name, _value), size(sz){};
	virtual void print(ostream& s);
};

class AsmDQ : public AsmData
{
private :

	size_t size;
public :

	AsmDQ(string _name, const string _value) : AsmData(_name, _value), size(8){};
	AsmDQ(string _name, const string _value, size_t sz) : AsmData(_name, _value), size(sz){};
	virtual void print(ostream& s);
};

class AsmDB : public AsmData
{
public :

	AsmDB(string& _name, const string& _value) : AsmData(_name, _value){};
	void print(ostream& s);
};

class CodeGen
{
private:

	Parser& parser;
	SymbolTable* globalTable;
	ofstream outStream;
	list<AsmData*> data;
	list<AsmFunction*> functions;
	stack< pair<string, string> > jump;
	map<string, string> doubles, strings, floats;
	int stacksLevel, lablesCount;
public:

	CodeGen(Parser& _parser, const string& out);
	void generate();
	void addDD(string name, const string value, size_t size);
	void addDQ(string name, const string value, size_t size);
	void addDB(string name, const string value);
	void addData(string name);
	void addFunc(string name);
	void addCommand(CommandT com, string l, string r);
	void addCommand(CommandT com, string l, int size = 4);
	void addCommand(CommandT com) {string f; addCommand(com, f, 4);}
	void addLocalVars(string l, string r);
	void addLabel(string label);
	string genLabel();
	void addEoln(int count);
	void printData();
	void printFunctions();
	void shiftStack(int shift){stacksLevel += shift;};
	void restoreStack();
	int getConstCount(TypeT type);
	string getLastFunction(){return (*functions.rbegin())->getName();}
	string getLastDDValue(){return (*data.rbegin())->getValue();}
	void genPrologue(int& shift);
	void genEpilogue(int& shift);
	void pushJumps(string start, string end);
	string getJumpBreak();
	string getJumpContinue();
	void popJumps();
	bool hasConst(TypeT type, string value);
	void insertConst(TypeT type, string value);
	void optimize();
	string getConst(TypeT type, string value);
	//void addElementInArray(string element, size_t elementSize);
	//void 
};

#endif


