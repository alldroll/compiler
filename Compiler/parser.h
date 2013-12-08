#ifndef PARSER_H
#define PARSER_H

#include "node.h"
#include <stack>

class Parser
{
private:

	typedef enum
	{
		DIRECT_SIMPLE,
		DIRECT_ABSTRACT,
		DIRECT_BOTH,
	}DeclaratorT;

	typedef enum
	{
		LOGIC_OR_EXPR,
		LOGIC_AND_EXPR,
		INCLUSIVE_EXPR,
		EXCLUSIVE_EXPR,
		AND_EXPR,
		EQUAL_EXPR,
		RELATION_EXPR,
		SHIFT_EXPR,
		ADD_EXPR,
		MULTI_EXPR,
	}BinaryT;

	#define BINARYT_NUMBER 10

	Token look;
	Scanner &scanner;
	SymbolTableStack symTableStack;
	bool isFuncDif;
	
	void match(TokenTypeT type, bool toMove);
	void exception(string error);
	void move();
	bool maybeBinaryOp(BinaryT btype);
	void pushTable(SymbolTable* table);
	void popTable();
	Symbol* get();
	void pushSymbol(Symbol* sym);
	void binaryCheck(ExprNode* expr, const Token& op);

	ExprNode* parsePrimaryExpr();
	ExprNode* parsePostfixExpr();
	ExprNode* parseUnaryExpr();
	ExprNode* parseConditionalExpr();
	ExprNode* parseAssignmentExpr();
	ExprNode* parseExpression();
	ExprNode* constExpr();
	vector<ExprNode*> parseArgmExpr(ExprNode*);
	ExprNode* varExpr(SymbolTable* table);
	ExprNode* parseBinaryExpr(BinaryT btype);
	ExprNode* parseParExpr();
	ExprNode* parseCastExpr();

	StmtNode* parseStmt(SymType* returnType);
	StmtNode* parseCompoundStmt(SymType* returnType);
	StmtNode* parseExprStmt(SymType* returnType);
	StmtNode* parseSelectionStmt(SymType* returnType);
	StmtNode* parseIterationStmt(SymType* returnType);
	StmtNode* parseJumpStmt(SymType* returnType);
	StmtNode* parseLabeledStmt(SymType* returnType);
	StmtNode* parseEmptyStmt(SymType* returnType);

	void parseDeclList();
	void parseDeclaration();

	SymType* parseTypeSpecifier();
	void parseInitDeclList(SymType* type);
	bool parseInitDeclarator(SymType* type);
	bool findInCurrTable(const Token& name);
	SymType* parsePointer(SymType* type);
	Symbol* parseAbstractDeclarator(SymType* type);
	vector<Symbol*> parseParamList();
	Symbol* parseParameterDeclaration();
	Symbol* parseGeneralDirectDeclarator(SymType* type, DeclaratorT decl);
	Symbol* parseGeneralDeclarator(SymType* type, DeclaratorT decl);
	SymType* parseArray(SymType* type);
	SymType* parseFunc(SymType* type);
	ExprNode* parseInitializerList(SymType* type);
	ExprNode* parseInitializer(SymType* type);
	SymType* parseStruct();
	SymbolTable* parseStructDeclarationList();
	map<string, Symbol*> parseStructDeclaration();
	SymType* parseEnum();
	SymbolTable* parseEnumeratorList();
	void parseTranslationUnit();
	bool isMainDif();

public:

	Parser(Scanner &scan);
	~Parser();
	void parse();
	void printTree();
	SymbolTable* getGlobalTable();
	vector<Symbol*> getFunctions(){return symTableStack.getFunctions();}
	vector<Symbol*> getVars(){return symTableStack.getVars();}
	void initFunctionsAndVars(){symTableStack.initFunctionsAndVarsArray();}
};




#endif