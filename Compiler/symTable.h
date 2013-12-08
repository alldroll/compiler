#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <map>
#include "scanner.h"
#include <list>
#include "syntaxNode.h"

typedef enum  
{
	POINTER,
	STRUCT,
	FUNCTION,
	ENUM,
	INT,
	FLOAT,
	DOUBLE,
	STRING,
	VOID,
	ARRAY,
	UNDEF,
}TypeT;

extern const string TypeText[11];

class SymbolTable;
class SymVar;
class SymType;
class CodeGen;

extern SymType* _void;
extern SymType* _float;
extern SymType* _double;
extern SymType* _int;

extern SymVar* _printf;
extern SymVar* _scanf;

class Symbol
{
	friend class SyntaxNode;
private:

	Token name;
public:

	Symbol(){};
	virtual void print(string out, bool printDecl){};
	virtual void gen(CodeGen&){};
	virtual string getName(){return string();};
	virtual bool isInit(){return false;};
	virtual bool equal (Symbol* a){return false;};
	virtual bool isPointer(){return false;};
	virtual Token& getTokenName(){return name;};
	virtual void init(void*){};
	virtual bool operator==(TypeT t){return false;};
	virtual bool operator!=(TypeT t){return false;}
	virtual SymType* getType(){return NULL;};
	virtual int getLine(){return 0;}
	virtual int getCol(){return 0;}
	virtual SymbolTable* getTable(){return NULL;}
	virtual bool isVar(){return false;}
	virtual bool isLocal(){return false;}
	virtual bool isParam(){return false;}
	virtual bool isGlobal(){return false;}
};


class SymType : public Symbol
{
protected :
	TypeT type;

public :

	SymType(TypeT t) {type = t;}
	virtual void print(string out, bool printDecl);
	virtual bool equal (Symbol* a);
	bool operator==(TypeT t){return type == t;}
	bool operator!=(TypeT t){return !(type == t);}
	virtual string getName(){return TypeText[type];}
	virtual void assign(SymType* a, bool e){};
	virtual size_t getSize(){return 0;}
};

class SymVar : public Symbol
{
protected :
	Token name;
	SymType* type;
	SyntaxNode* initializer;
	string asmName;
public :

	SymVar(const Token _name, SymType *t, SyntaxNode* i) : type(t), name(_name), initializer(i){asmName = name.text;}
	virtual SymType* getType() {return type;}
	virtual void print(string out, bool printDecl);
	virtual void assignType(SymType* type, bool isP);
	virtual void init(void* init);
	virtual bool isInit(){return initializer != NULL;}
	virtual bool equal (Symbol* a);
	virtual string getName(){return name.text;}
	virtual Token& getTokenName(){return name;}
	virtual int getLine(){return name.line;}
	virtual int getCol(){return name.col;}
	bool operator==(TypeT t){return *type == t;};
	bool operator!=(TypeT t){return !(*type == t);}
	bool isVar(){return true;}
	string& getAsmName(){return asmName;}
	SyntaxNode* getInitializer(){return initializer;}
	virtual void gen(CodeGen&);
};


class SymTypeScalar : public SymType
{
public :

	SymTypeScalar(TypeT t): SymType(t){}
	virtual size_t getSize(){return 4;}
};

class SymTypeRecord : public SymType
{
protected :

	SymbolTable* table;
	Token name;
	size_t size;
public :

	SymTypeRecord(Token _name) : name(_name), SymType(UNDEF){table = NULL; size = 0;}
	virtual void print(string out, bool printDecl);
	bool isInit() {return table != NULL;}
	SymbolTable* getTable(){return table;}
	string getName(){return name.text;}
	bool equal(Symbol* a);
	virtual void init(void* init);
	Token& getTokenName(){return name;} 
	virtual size_t getSize();
};

class SymTypeStruct : public SymTypeRecord
{
public :

	SymTypeStruct(Token _name) : SymTypeRecord(_name){type = STRUCT;};
	void print(string out, bool printDecl);
	void gen(CodeGen&);
};

class SymTypeEnum : public SymTypeRecord
{
public :

	SymTypeEnum(Token _name) : SymTypeRecord(_name){type = ENUM;};
	void print(string out, bool printDecl);
	void gen(CodeGen&){};
};

class SymTypeVoid : public SymType
{
public :

	SymTypeVoid() : SymType(VOID){}
	void gen(){};
};

class SymTypeInteger : public SymTypeScalar
{
public :

	SymTypeInteger() : SymTypeScalar(INT){}
	void gen(CodeGen&){};
};

class SymTypeFloat : public SymTypeScalar
{
public :

	SymTypeFloat() : SymTypeScalar(FLOAT){}
	void gen(CodeGen&){};
};

class SymTypeDouble : public SymTypeScalar
{
public :

	SymTypeDouble() : SymTypeScalar(DOUBLE){}
	void gen(CodeGen&){};
	size_t getSize(){return 8;}
};

class SymTypePointer : public SymType
{
protected :

	SymType* to;
public :

	SymTypePointer(SymType* _to) : SymType(POINTER){to = _to;}
	virtual void print(string out, bool printDecl);
	virtual void assign(SymType* t, bool e);
	virtual SymType* dereference(){return to;}
	virtual bool equal (Symbol* a);
	virtual void gen(CodeGen&){};
	virtual void assignVar(SymVar* var){};
	bool isPointer(){return true;}
	size_t getSize(){return 4;}
};

class SymTypeArray : public SymTypePointer
{
private :

	SyntaxNode* length;
	SymVar* var;
	int arraySize;
public :

	SymTypeArray(SyntaxNode* len, SymType* _to) : SymTypePointer(_to){type = ARRAY; length = len; var = NULL; arraySize = -1;}
	void print(string out, bool printDecl);
	bool isInit(){return length != NULL;}
	bool equal (Symbol* a);
	void gen(CodeGen&);
	void assignVar(SymVar* _var){var = _var;}
	size_t getSize();
	size_t getArraySize();
	SyntaxNode* getInitList(){return isInit() && var != NULL ? var->getInitializer() : NULL;}
	string getVarAsmName(){return var->getAsmName();}
	bool isGlobal(){return var->isGlobal();}
};

class SymTypeFunc : public SymTypePointer
{

	friend class AsmFunction;
private :

	SymVar* var;
	vector<Symbol*> paramList;
public :

	SymTypeFunc(SymType* _ret, vector<Symbol*> arg) : SymTypePointer(_ret){type = FUNCTION; var = NULL; paramList = arg;};
	void put(SymVar* arg) {paramList.push_back(arg);}
	void print(string out, bool printDecl);
	vector<Symbol*>& getParams(){return paramList;}
	void assignVar(SymVar* _var){var = _var;}
	bool equal (Symbol* a);
	void gen(CodeGen&);
    int getLine(){return var->getLine();}
	int getCol(){return var->getCol();}
	SyntaxNode* getBody(){return var->getInitializer();}
	string getVarName(){return var->getName();}
	string getVarAsmName(){return var->getAsmName();}
};

class SymVarParam : public SymVar
{
public :

	SymVarParam(const Token _name, SymType *type, SyntaxNode* i) : SymVar(_name, type, i){};
	void gen(CodeGen&);
	bool isInit(){return true;}
	bool isParam(){return true;}
};

class SymTypeEnumConst : public SymVar
{
private :

	int index;
	SymTypeEnum* enumType;
public :

	SymTypeEnumConst(Token name, int idx, SymTypeEnum* t) : SymVar(name, t, NULL), index(idx){};//IntegerNode()
	void print(string out, bool printDecl);
	virtual bool isInit() {return true;}
	void gen(CodeGen&){};
};

class SymVarLocal : public SymVar
{
public :

	SymVarLocal(Token _name, SymType *type, SyntaxNode* i) : SymVar(_name, type, i){};
	void gen(CodeGen&);
	bool isLocal(){return true;}
};

class SymVarGlobal : public SymVar
{
public :

	SymVarGlobal(Token _name, SymType *type, SyntaxNode* i) : SymVar(_name, type, i){};
	void gen(CodeGen&);
	bool isGlobal(){return true;}
};

class SymbolTable
{
typedef map<string, Symbol*>::iterator iter;

private :

	map<string, Symbol*> table;
	vector<string> orderList;
	int index, level;
public :

	SymbolTable();
	void putSymbol(Symbol *sym);
	Symbol* operator[](const string& name);
	void print(string out, bool printDecl);
	size_t size() const;
	Symbol* next();
	Symbol* first();
	Symbol* last();
	Symbol* at(int index);
	int& getLevel(){return level;}
};

class SymbolTableStack
{
typedef list<SymbolTable*>::iterator iterator;

private :

	list<SymbolTable*> symTableStack;
	iterator ptr;
	vector<Symbol*> functions, vars;
public :

	SymbolTableStack();
	~SymbolTableStack();
	void push(SymbolTable* symTable);
	void pop();
	void putSymbol(Symbol* sym);
	Symbol* getSymbol(const Token& name);
	bool hasSymbolInCurrTable(const Token& name);
	void print();
	void init(Symbol* var, void* init);
	SymbolTable* getGlobal();
	bool isGlobal(){return ptr == --symTableStack.end();}
	void initFunctionsAndVarsArray();
	vector<Symbol*> getFunctions(){return functions;}
	vector<Symbol*> getVars(){return vars;}
};

#endif