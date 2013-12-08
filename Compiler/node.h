#ifndef SYNTAXNODE_H
#define SYNTAXNODE_H
#include <string>
#include <iostream>
#include <vector>
#include "symTable.h"

using namespace std;

class CodeGen;
class ExprNode;

extern ExprNode* tryCastInAssignment(SymType* t1, ExprNode* e, Token _token);

class ExprNode : public SyntaxNode
{
protected:

	vector<ExprNode*> children;
	SymType* type;
	bool isLvalue;
public:

	virtual void print(string str, bool isTail);
	ExprNode() : SyntaxNode() {type = NULL; isLvalue = false;};
	ExprNode(Token _token) : SyntaxNode(_token){ type = NULL; isLvalue = false;};
	ExprNode(Token _token, ExprNode* lChild, ExprNode* rChild);
	SymType* getType() {return type;}
	void setType(SymType* t){type = t;}
	bool& lvalue() {return isLvalue;}
	virtual bool isStringConst(){return false;}
	bool operator==(TypeT t){return *type == t;}
	bool operator!=(TypeT t){return !(*type == t);}
	bool isPointer(){return type->isPointer();}
	SymbolTable* getTable(){return type->getTable();}
	virtual bool isConst(){return false;}
	virtual void gen(CodeGen&){};
	virtual void genLvalue(CodeGen&){};
	vector<ExprNode*> getChildren() {return children;}
};

class ExprVar : public ExprNode
{

protected:

	Symbol* sym;
public:

	ExprVar(Token _token, Symbol* s);
	void gen(CodeGen&);
	void genLvalue(CodeGen&);
};

class ExprConst : public ExprNode
{
public:

	ExprConst(Token _token) : ExprNode(_token){}
	virtual bool isConst(){return true;}
	virtual void gen(CodeGen&);
};

class IntegerConst : public ExprConst
{
public:

	IntegerConst(Token _token) : ExprConst(_token){type = _int;}
};

class FloatConst : public ExprConst
{
public:

	FloatConst(Token _token) : ExprConst(_token){type = _float;}
	void gen(CodeGen&);
};

class DoubleConst : public ExprConst
{
public:

	DoubleConst(Token _token) : ExprConst(_token){type = _double;}
	void gen(CodeGen&);
};

class StringConst : public ExprConst
{
public:

	StringConst(Token _token) : ExprConst(_token) {type = new SymTypePointer(_int);};
	bool isStringConst(){return true;}
	void gen(CodeGen&);
};

class UnaryNode : public ExprNode
{
public:
	
	UnaryNode(Token _token, ExprNode* _child);
	void gen(CodeGen&);
	void genLvalue(CodeGen&);
};

class PostfixUnaryNode : public ExprNode
{
public:

	PostfixUnaryNode(Token _token, ExprNode* _child);
	void gen(CodeGen&);
	void genLvalue(CodeGen&);
};

class ExprCast : public ExprNode
{
private:

    bool toCast;
public:

	ExprCast(SymType* t, ExprNode* _child);
	ExprCast(SymType* t, ExprNode* _child, bool _toCast);
	void gen(CodeGen&);
	void print(string str, bool isTail);
};

class BinaryNode : public ExprNode
{
public:

	BinaryNode(Token _token, ExprNode *_lChild, ExprNode *_rChild);
	void gen(CodeGen&);
	void genLvalue(CodeGen&);
};

class TernaryNode : public ExprNode
{
public:

	TernaryNode (Token _token, ExprNode *_fChild, ExprNode *_sChild, ExprNode *_tChild);
	void gen(CodeGen&);
};

class ExprFuncCall : public ExprNode
{
public:

	ExprFuncCall(Token _token, vector<ExprNode*> arg, SymTypeFunc* _type);
	void gen(CodeGen&);
};

class ExprAssignment : public ExprNode
{
public:

	ExprAssignment(Token _token, ExprNode *_lChild, ExprNode *_rChild, bool toCast = true);
	void gen(CodeGen&);
	void genLvalue(CodeGen&);
};

class ExprIndexing : public ExprNode
{
public:

	ExprIndexing(Token _token, ExprNode *_lChild, ExprNode *_rChild);
	void gen(CodeGen&);
	void genLvalue(CodeGen&);
};

class ExprFieldSelect : public ExprNode
{
public:

	ExprFieldSelect(Token _token, ExprNode *_lChild, ExprNode *_rChild);
	void gen(CodeGen&);
	void genLvalue(CodeGen&);
};

class InitList : public ExprNode
{
private:

	int length, shift;
public:

	InitList(Token _name, SymType* _type);
	void add(ExprNode* arg);
	void gen(CodeGen&);
	int getLength() {return length;}
	void initShift(int s){shift = s;}
};

class StmtNode : public SyntaxNode
{
protected:

	vector<ExprNode*> expr;
	vector<StmtNode*> stmt;
public:

	StmtNode(Token _token) : SyntaxNode(_token){};
	virtual void print(string str, bool isTail);
	virtual bool isCompound(){return false;}
	virtual void tablePrint(string out){};
	virtual SymType* getType(){return NULL;}
	virtual void setType(SymType* t){};
	virtual void gen(CodeGen&);
	ExprNode* getExpr(int i, int j) {return stmt[i]->expr[j];}
	ExprNode* getExpr() {return *expr.rbegin();}
	void setExpr(ExprNode* _expr){if (expr.size() > 0) expr[0] = _expr;}
};

class StmtJump : public StmtNode
{
public:

	StmtJump(Token _token, StmtNode* _stmt, SymType* retType);
	StmtJump(Token _token) : StmtNode(_token){};
	SymType* getType(){return stmt[0]->getType() != NULL ? stmt[0]->getType() : _void;}
	void setType(SymType* t) {stmt[0]->setType(t);}
	void gen(CodeGen&);
};

class StmtCompound : public StmtNode
{
private :
	SymbolTable* table;
	vector<StmtNode*> compounds;

public:

	virtual bool isCompound(){return true;}
	StmtCompound(Token _token, SymbolTable* _table, SymType* returnType);
	virtual void tablePrint(string out){table->print(out, true);};
	SymbolTable* getTable(){return table;}
	size_t genLocal(CodeGen&, int, int&, int&);
	void gen(CodeGen&);
	void addCompound(StmtCompound* com) {compounds.push_back(com);}
	void addStmt(StmtNode* _stmt){stmt.push_back(_stmt);}
};

class StmtExpr : public StmtNode
{
public:

	StmtExpr(Token _token, ExprNode* _expr);
	SymType* getType(){return expr[0] != NULL ? expr[0]->getType() : NULL;}
	void setType(SymType* t){ if (expr[0] != NULL) expr[0]->setType(t);}
	void gen(CodeGen&);
};

class StmtSelection : public StmtNode
{
public:

	StmtSelection(Token _token, ExprNode* _expr, StmtNode *ifBody, StmtNode* elseBody);
	void gen(CodeGen&);
};

class StmtFor : public StmtNode
{
public:

	StmtFor(Token _token,  StmtNode *e1, StmtNode *e2, ExprNode* e, StmtNode* _stmt);
	void gen(CodeGen&);
};

class StmtWhile : public StmtNode
{
public:

	StmtWhile(Token _token, ExprNode *e, StmtNode* _stmt);
	void gen(CodeGen&);
};

class StmtDo : public StmtNode
{
public:

	StmtDo(Token _token, StmtNode *stmt1, StmtNode *stmt2);
	void gen(CodeGen&);
};

class StmtLabeled : public StmtNode
{
public:

	StmtLabeled(Token _token, StmtNode *_stmt, ExprNode* e);
	void gen(CodeGen&);
};


#endif