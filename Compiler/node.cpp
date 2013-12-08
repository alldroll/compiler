#include "node.h"
#include "codeGen.h"

#define DW(value) ("dword ptr " + value)
#define QW(value) ("qword ptr " + value)
#define OFFSET(value) ("offset " + value)
#define ADR(value) ("[" + value + "]")
#define EBP(value) (value + "[ebp]")
#define RET(name) (name + "RetLabel")
#define LEFT_CHILD(children) (children[0])
#define RIGHT_CHILD(children) (children[1])
#define ONLY_CHILD(children) (children[0])
#define TERNARY_CHILD(children) (children[2])
#define SHIFT(arr, shift) (arr.substr(0, arr.length() - 1) + " + " + shift + "]")


static void SemException(int line, int col, const string& err)
{
	throw SemanticsException(line, col, err);
}

static bool isArithmetic(ExprNode* node)
{
	return *node == FLOAT || *node == INT || *node == DOUBLE;
}

static bool isArithmetic(SymType* type)
{
	return *type == FLOAT || *type == INT || *type == DOUBLE;
}

static bool isArithmetic(SymType* type1, SymType* type2)
{
	return isArithmetic(type1) && isArithmetic(type2);
}

static bool isIncrementOperand(ExprNode* node)
{
	return node->isPointer() || isArithmetic(node);
}

static bool isIncrementOperand(SymType* t)
{
	return t->isPointer() || isArithmetic(t);
}

static bool maybeCastWithPointer(SymType* p1, SymType* p2)
{
	return *p1 == POINTER && *p2 == POINTER && *static_cast<SymTypePointer*>(p1)->dereference() == VOID;
}

static void throwAssignmentError(Token& op, string t1, string t2)
{
	SemException(op.line, op.col, "to use operator " + op.text + " for types : " + t1 + " and " + t2 + " is impossible");
}

ExprNode* tryCastInAssignment(SymType* t1, ExprNode* e, Token _token)
{
	if (*t1 == ARRAY || !(isIncrementOperand(t1) && isArithmetic(e) || t1->equal(e->getType())) && !maybeCastWithPointer(t1, e->getType()))
		throwAssignmentError(_token, t1->getName(), e->getType()->getName());
	return !t1->equal(e->getType()) ? new ExprCast(t1, e) : e;
}

static bool isLogic(Token& token)
{
	return token == OP_AND || token == OP_OR || token == OP_EQUAL || token == OP_BOR || token == OP_GREATER ||
		token == OP_GREATER_OR_EQUAL || token == OP_LESS ||  token == OP_LESS_OR_EQUAL || token == OP_XOR; 
}

SymType* getBinaryType(ExprNode* lChild, ExprNode* rChild, Token& op)
{
	if (isLogic(op))
		return _int;
	if (op == OP_SUB && lChild->isPointer() && rChild->isPointer())
		return _int;
	return rChild->isPointer() ||  *rChild == FLOAT || *rChild == DOUBLE ? rChild->getType() : lChild->getType();
}

void ExprNode::print(string str, bool isTail)
{
	SyntaxNode::print(str, isTail);
	cout << "    ";
	if (type != NULL)
		type->print("", false);
	cout << endl;
	if (children.size() >= 1)
	{
		for (int i = 0; i < children.size() - 1; i++)
			children[i]->print(str + (isTail ? "    " : "|   "), false);
		children[children.size() - 1]->print(str + (isTail ? "    " : "|   "), true);
	}
}

void ExprCast::print(string str, bool isTail)
{
	if (toCast)
	{
		type->print(str + " casted to ", false);
		cout << endl;
	}
	if (children.size() >= 1)
	{
		for (int i = 0; i < children.size() - 1; i++)
			children[i]->print(str + (isTail ? "    " : "|   "), false);
		children[children.size() - 1]->print(str + (isTail ? "    " : "|   "), true);
	}
}

void StmtNode::print(string str, bool isTail)
{
	SyntaxNode::print(str, isTail);
	cout << endl;
	string out = "   ";
	for (int i = 0; i < str.size(); i++)
		out += " ";
	tablePrint(out + "    ");
	if (expr.size() >= 1)
	{
		for (size_t i = 0; i < expr.size() - 1 && expr[i] != NULL; i++)
			expr[i]->print(str + (isTail ? "    " : "|   "), false);
		if (expr[expr.size() - 1] != NULL)
			expr[expr.size() - 1]->print(str + (isTail ? "    " : "|   "), true);
	}

	if (stmt.size() >= 1)
	{
		for (int i = 0; i < stmt.size() - 1 && stmt[i] != NULL; i++)
			stmt[i]->print(str + (isTail ? "    " : "|   "), false);
		if (stmt[stmt.size() - 1] != NULL)
			stmt[stmt.size() - 1]->print(str + (isTail ? "    " : "|   "), true);
	}
	if (isCompound())
		cout << out + "}" << endl;
}

InitList::InitList(Token _name, SymType* _type) : ExprNode(_name)
{
	if (*_type == FUNCTION || *_type == STRUCT)
		SemException(token.line, token.col, "array cannot be init by this type");
	type = _type;
	length = 0;
	shift = 0;
};

void InitList::add(ExprNode* arg)
{
	if (!static_cast<SymTypeArray*>(type)->dereference()->equal(arg->getType()))
		if (!isArithmetic(static_cast<SymTypeArray*>(type)->dereference(), arg->getType()))
			SemException(token.line, token.col, "different basic types");
		//else
			//arg = //new ExprCast(static_cast<SymTypeArray*>(type)->dereference(), arg);
	++length;
	children.push_back(arg);
}

ExprVar::ExprVar(Token _token, Symbol* s) : ExprNode(_token)
{
	if (*s != FUNCTION)
		isLvalue = true;
	sym = s;
	type = sym->getType();
}

UnaryNode::UnaryNode(Token _token, ExprNode* _child) : ExprNode(_token)
{
	type = _child->getType();
	switch(_token)
	{
	case OP_AMP : case OP_DEC : case OP_INC :
		{
			if (!_child->lvalue())
				SemException(_token.line, _token.col, "expression must be a modifiable lvalue");
			if (_token == OP_AMP)
				type = new SymTypePointer(type);
			else
				if (!isIncrementOperand(_child))
					SemException(_token.line, _token.col, "lvalue required as a increment operand");
		}
		break;

	case OP_ASTERISK :
		{
			if (!type->isPointer())
				SemException(_token.line, _token.col, "operand of '*' must be a pointer");
			if (*static_cast<SymTypePointer*>(type)->dereference() == VOID)
				SemException(_token.line, _token.col, "expression must be a pointer to a complete object type");
			type = static_cast<SymTypePointer*>(type)->dereference();
		}break;

	case OP_ADD : case OP_SUB :
		if (!isArithmetic(_child))
			SemException(_token.line, _token.col, "operand of '+' must have float or int types");
	}
	if (*type != FUNCTION)
		isLvalue = true;
	children.push_back(_child);
}

BinaryNode::BinaryNode(Token _token, ExprNode *_lChild, ExprNode *_rChild) : ExprNode(_token)
{
	switch(_token)
	{
	case OP_ASTERISK :
	case OP_DIV :
		if (!isArithmetic(_lChild) || !isArithmetic(_rChild))
			SemException(_token.line, _token.col, "invalid operands for binary " + _token.text + "(" + _lChild->getName() + ", " + _rChild->getName() + ")");
		break;

	case OP_ADD :
	case OP_SUB :
		{
			if (_lChild->isPointer() && _rChild->isPointer() && _token == OP_ADD)
				SemException(_token.line, _token.col, "pointer + pointer is not allowed");

			if (!isIncrementOperand(_lChild) && !isIncrementOperand(_rChild))
				SemException(_token.line, _token.col, "invalid operands for binary " + _token.text + "(" + _lChild->getName() + ", " + _rChild->getName() + ")");
		}
		break;

	case OP_AND :
	case OP_OR :
		{
			if (!isIncrementOperand(_lChild->getType()) ||  !isIncrementOperand(_rChild->getType()))
				SemException(_token.line, _token.col, "requared scalar type for binary " + _token.text);
			if (*_lChild != INT)
				_lChild = new ExprCast(_int, _lChild);
			if (*_rChild != INT)
				_rChild = new ExprCast(_int, _rChild);			
		}
	}
	if (isLogic(token))
	{
		SymType* t = *_rChild == FLOAT || *_rChild == DOUBLE ? _rChild->getType() : _lChild->getType();
		if (!_lChild->getType()->equal(t))
				_lChild = new ExprCast(t, _lChild);
		if (!_rChild->getType()->equal(t))
			_rChild = new ExprCast(t, _rChild);
	}

	type = getBinaryType(_lChild, _rChild, _token);
	
	if (*_lChild != POINTER && *_rChild != POINTER && !isLogic(token))
	{
		
		if (!_lChild->getType()->equal(type))
			_lChild = new ExprCast(type, _lChild);
		if (!_rChild->getType()->equal(type))
			_rChild = new ExprCast(type, _rChild);
	}
	children.push_back(_lChild);
	children.push_back(_rChild);
}

ExprFuncCall::ExprFuncCall(Token _token, vector<ExprNode*> arg, SymTypeFunc* _type) : ExprNode(_token)
{
	vector<Symbol*>& params = _type->getParams();
	type = _type->dereference();
	children = arg;

	if (_type->getVarName() == "printf" || _type->getVarName() == "scanf")
	{
		if (!arg[1]->isStringConst())
			SemException(arg[0]->getLine(), arg[0]->getCol(), "expected const char* but argument is type of " + arg[1]->getType()->getName());
		return;
	}

	if (children.size() - 1 != params.size())
		SemException(_type->getLine(), _type->getCol(), "there are different number of arguments in a function definition");
	for (int i = 0; i < params.size(); i++)
	{
		if (!children[i + 1]->getType()->equal(params[i]->getType()))
		{
			if (!isArithmetic(children[i + 1]->getType(), params[i]->getType()) && !maybeCastWithPointer(params[i]->getType(), children[i + 1]->getType()))
				SemException(_type->getLine(), _type->getCol(), "conflict types of function arguments :" + children[i + 1]->getType()->getName() + " instead of " + params[i]->getType()->getName());
			children[i + 1] = new ExprCast(params[i]->getType(), children[i + 1]);
		}
	}
}

ExprNode::ExprNode(Token _token, ExprNode* lChild, ExprNode* rChild) : SyntaxNode(_token) 
{
	type = NULL; 
	isLvalue = false; 
	children.push_back(lChild); 
	children.push_back(rChild);
}

ExprCast::ExprCast(SymType* t, ExprNode* child)
{
	children.push_back(child);
	type = t;
	toCast = !child->getType()->equal(t);
}

ExprCast::ExprCast(SymType* t, ExprNode* child, bool _toCast)
{
	children.push_back(child);
	type = t;
	toCast = _toCast;
}

TernaryNode::TernaryNode(Token _token, ExprNode *_fChild, ExprNode *_sChild, ExprNode *_tChild) : ExprNode(_token)
{
	if (!_sChild->getType()->equal(_tChild->getType()))
		SemException(_tChild->getLine(), _tChild->getCol(), "expected " + _sChild->getType()->getName() + " but " + _tChild->getType()->getName() + " was founded");
 	children.push_back(_fChild);
	children.push_back(_sChild);
	children.push_back(_tChild);
	type = _sChild->getType();
}

ExprFieldSelect::ExprFieldSelect(Token _token, ExprNode *_lChild, ExprNode *_rChild) : ExprNode(_token)
{
	type = _rChild->getType();
	children.push_back(_lChild);
	children.push_back(_rChild);
	if (*type != FUNCTION)
		isLvalue = true;
}

ExprIndexing::ExprIndexing(Token _token, ExprNode *_lChild, ExprNode *_rChild) : ExprNode(_token)
{
	if (!(_lChild->lvalue() && _lChild->isPointer()))
		SemException(_token.line, _token.col, "expression must have pointer-to-object type");
	children.push_back(_lChild);
	children.push_back(_rChild);
	type = static_cast<SymTypePointer*>(_lChild->getType())->dereference();
	isLvalue = true;
}

ExprAssignment::ExprAssignment(Token _token, ExprNode *_lChild, ExprNode *_rChild, bool toCast) : ExprNode(_token)
{
	type = _lChild->getType();
	if (!_lChild->lvalue())
		SemException(_token.line, _token.col, "expression must be a modifiable lvalue");
	if (toCast)
		_rChild = tryCastInAssignment(_lChild->getType(), _rChild, _token);
	switch(_token)
	{		
	case OP_ADD_ASSIGN :
	case OP_MUL_ASSIGN :
	case OP_MOD_ASSIGN :
	case OP_SUB_ASSIGN :
		if (!isIncrementOperand(_lChild))
			throwAssignmentError(_token, _lChild->getType()->getName(), _rChild->getType()->getName());
	}
	children.push_back(_lChild);
	children.push_back(_rChild);
}

PostfixUnaryNode::PostfixUnaryNode(Token _token, ExprNode* _child) : ExprNode(_token)
{
	if (!_child->lvalue() || !isIncrementOperand(_child))
		SemException(_token.line, _token.col, "lvalue required as a increment operand");
	type = _child->getType();
	children.push_back(_child);
}

StmtCompound::StmtCompound(Token _token, SymbolTable* _table, SymType* funcType) : StmtNode(_token)
{
	table = _table;
}

StmtJump::StmtJump(Token _token, StmtNode* _stmt, SymType* retType) : StmtNode(_token)
{	
	if (token == KW_RETURN)
	{
		if (!_stmt->getType()->equal(retType) && !isArithmetic(_stmt->getType(), retType))
			SemException(_token.line, _token.col, "return : cannot convert from \'float *\' to \'int *\'");
		if (isArithmetic(_stmt->getType(), retType) && !_stmt->getType()->equal(retType))
			_stmt->setExpr(new ExprCast(retType, _stmt->getExpr()));		
	}
	stmt.push_back(_stmt);
}

StmtExpr::StmtExpr(Token _token, ExprNode* _expr) : StmtNode(_token)
{
	expr.push_back(_expr);
}

StmtSelection::StmtSelection(Token _token, ExprNode* _expr, StmtNode *ifBody, StmtNode* elseBody) : StmtNode(_token)
{
	expr.push_back(_expr);
	stmt.push_back(ifBody);
	stmt.push_back(elseBody);
}

StmtFor::StmtFor(Token _token,  StmtNode *e1, StmtNode *e2, ExprNode* e, StmtNode* _stmt) : StmtNode(_token)
{
	stmt.push_back(e1);
	stmt.push_back(e2);
	expr.push_back(e);
	stmt.push_back(_stmt);
}

StmtWhile::StmtWhile(Token _token, ExprNode *e, StmtNode* _stmt) : StmtNode(_token)
{
	expr.push_back(e);
	stmt.push_back(_stmt);
}

StmtDo::StmtDo(Token _token, StmtNode *stmt1, StmtNode *stmt2) : StmtNode(_token)
{
	stmt.push_back(stmt1);
	stmt.push_back(stmt2);
}

StmtLabeled::StmtLabeled(Token _token, StmtNode *_stmt, ExprNode* e) : StmtNode(_token)
{
	stmt.push_back(_stmt);
	expr.push_back(e);
}

static void compareOpDoubleGen(CodeGen& gen, CommandT opType)
{
	gen.addCommand(ASM_XOR, getReg(REG_ECX), getReg(REG_ECX));
	gen.addCommand(ASM_COMISD, getReg(REG_XMM0), getReg(REG_XMM1));
	gen.addCommand(opType, getReg(REG_CL));
	gen.addCommand(ASM_PUSH, getReg(REG_ECX));
}

static void pushDouble(CodeGen& gen, string name)
{
	gen.shiftStack(-8);
	gen.addCommand(ASM_SUB, getReg(REG_ESP), to_string(8));
	gen.addCommand(ASM_MOVSD, getReg(REG_XMM0), name);
	gen.addCommand(ASM_MOVSD, QW(ADR(getReg(REG_ESP))), getReg(REG_XMM0)); 
}

static void popDouble(CodeGen& gen)
{
	gen.shiftStack(8);
	gen.addCommand(ASM_MOVSD, getReg(REG_XMM0), QW(ADR(getReg(REG_ESP))));
	gen.addCommand(ASM_ADD, getReg(REG_ESP), to_string(8));
}
static void simpleOpIntGen(CodeGen& gen, CommandT opType)
{
	gen.addCommand(opType, getReg(REG_EAX), getReg(REG_EBX));
	gen.addCommand(ASM_PUSH, getReg(REG_EAX));
}

static void divisionOpIntGen(CodeGen& gen, bool isMod = false)
{
	gen.addCommand(ASM_CDQ);
	gen.addCommand(ASM_IDIV, getReg(REG_EBX));
	gen.addCommand(ASM_PUSH, getReg(isMod ? REG_EDX : REG_EAX));
}

static void compareOpIntGen(CodeGen& gen, CommandT opType)
{
	gen.addCommand(ASM_XOR, getReg(REG_ECX), getReg(REG_ECX));
	gen.addCommand(ASM_CMP, getReg(REG_EAX), getReg(REG_EBX));
	gen.addCommand(opType, getReg(REG_CL));
	gen.addCommand(ASM_PUSH, getReg(REG_ECX));
}

static void logIntGen(CodeGen& gen, CommandT cond, string label, RegT r)
{
	gen.addCommand(ASM_TEST, getReg(r), getReg(r));
	gen.addCommand(cond, label);
}

static void logAndOrIntGen(CodeGen& gen, bool isAnd = true)
{
	string finishL = gen.genLabel(), falseL = gen.genLabel();
	logIntGen(gen, (isAnd ? ASM_JZ : ASM_JNZ), falseL, REG_EAX);
	logIntGen(gen, (isAnd ? ASM_JZ : ASM_JNZ), falseL, REG_EBX);

	gen.addCommand(ASM_PUSH, string(isAnd ? "1" : "0") );
	gen.addCommand(ASM_JMP, string(finishL));
	gen.addLabel(string(falseL));
	gen.addCommand(ASM_PUSH, string(isAnd ? "0" : "1"));
	gen.addLabel(string(finishL));
}

void static simpleAssignment(CodeGen& gen)
{
	gen.addCommand(ASM_POP, getReg(REG_EBX));
	gen.addCommand(ASM_POP, getReg(REG_EAX));
	gen.addCommand(ASM_MOV, DW(ADR(getReg(REG_EAX))), getReg(REG_EBX));
	gen.addCommand(ASM_PUSH, DW(ADR(getReg(REG_EAX))));
}

static void simpleOpDoubleGen(CodeGen& gen, CommandT opType)
{
	gen.addCommand(opType, getReg(REG_XMM0), getReg(REG_XMM1));
	pushDouble(gen, getReg(REG_XMM0));
}

void UnaryNode::gen(CodeGen& gen)
{
	switch(token)
	{

	case OP_ASTERISK :
		{
			ONLY_CHILD(children)->gen(gen);
			gen.addCommand(ASM_POP, getReg(REG_EAX));

			if (*type == DOUBLE || *type == FLOAT)
				pushDouble(gen, QW(ADR(getReg(REG_EAX))));
			else
				gen.addCommand(ASM_PUSH, DW(ADR(getReg(REG_EAX))));
		}break;
	

	case OP_AMP : ONLY_CHILD(children)->genLvalue(gen); break;

	case OP_INC : case OP_DEC :
		{
			ONLY_CHILD(children)->genLvalue(gen);
			gen.addCommand(ASM_POP, getReg(REG_EAX));
			if (*type == INT)
			{
				gen.addCommand(token == OP_INC ? ASM_INC : ASM_DEC, DW(ADR(getReg(REG_EAX))));
				gen.addCommand(ASM_PUSH, getReg(REG_EAX));
			}
			else
			{
				gen.addCommand(ASM_MOVSD,  getReg(REG_XMM0), QW(ADR(getReg(REG_EAX))));
				gen.addCommand(token == OP_INC ? ASM_ADDSD : ASM_SUBSD, getReg(REG_XMM0), getPrefix(to_string(1), PREFIX_DOUBLE_CONST));
				gen.addCommand(ASM_MOVSD,  QW(ADR(getReg(REG_EAX))), getReg(REG_XMM0));
				pushDouble(gen, getReg(REG_XMM0));
			}
		}break;

	case OP_NOT :
		{
			ONLY_CHILD(children)->gen(gen);
			gen.addCommand(ASM_PUSH, to_string(0));
			if (*ONLY_CHILD(children) == DOUBLE)
			{
				popDouble(gen);
				compareOpDoubleGen(gen, ASM_SETE);
			}
			else	
			{	
				gen.addCommand(ASM_POP, getReg(REG_EAX));
				compareOpIntGen(gen, ASM_SETE);
			}
		}break;
	case OP_ADD : ONLY_CHILD(children)->gen(gen);break;
	case OP_SUB : 
		{
			if (*ONLY_CHILD(children) == DOUBLE)
			{
				pushDouble(gen, getPrefix(to_string(2), PREFIX_DOUBLE_CONST));
				ONLY_CHILD(children)->gen(gen);
				popDouble(gen);
				gen.addCommand(ASM_MOVSD, getReg(REG_XMM1), getReg(REG_XMM0));
				popDouble(gen);
				simpleOpDoubleGen(gen, ASM_SUBSD);
			}
			else
			{
				gen.addCommand(ASM_POP, getReg(REG_EAX));
				gen.addCommand(ASM_NEG, getReg(REG_EAX));
				gen.addCommand(ASM_PUSH, getReg(REG_EAX));
			}
		}
	}
}

void binaryIntGen(CodeGen& gen, Token& op)
{
	gen.addCommand(ASM_POP, getReg(REG_EBX));
	gen.addCommand(ASM_POP, getReg(REG_EAX));

	switch(op)
	{
	case OP_ADD : simpleOpIntGen(gen, ASM_ADD) ;break;
	case OP_SUB : simpleOpIntGen(gen, ASM_SUB); break;
	case OP_ASTERISK : simpleOpIntGen(gen, ASM_IMUL); break;

	case OP_AMP : simpleOpIntGen(gen, ASM_AND); break;
	case OP_BOR : simpleOpIntGen(gen, ASM_OR); break;
	case OP_XOR : simpleOpIntGen(gen, ASM_XOR); break;

	case OP_EQUAL : compareOpIntGen(gen, ASM_SETE); break;
	case OP_UNEQUAL : compareOpIntGen(gen, ASM_SETNE); break;
	case OP_LESS_OR_EQUAL : compareOpIntGen(gen, ASM_SETLE); break;
	case OP_GREATER_OR_EQUAL : compareOpIntGen(gen, ASM_SETGE); break;
	case OP_LESS : compareOpIntGen(gen, ASM_SETL); break;
	case OP_GREATER : compareOpIntGen(gen, ASM_SETG); break;

	case OP_DIV : divisionOpIntGen(gen); break;
	case OP_MOD : divisionOpIntGen(gen, true); break;

	case OP_AND : logAndOrIntGen(gen); break;
	case OP_OR : logAndOrIntGen(gen, false);break;
	}
}

void binaryDoubleGen(CodeGen& gen, Token& op)
{
	popDouble(gen);
	gen.addCommand(ASM_MOVSD, getReg(REG_XMM1), getReg(REG_XMM0));
	popDouble(gen);

	switch(op)
	{
	case OP_ADD : simpleOpDoubleGen(gen, ASM_ADDSD) ;break;
	case OP_SUB : simpleOpDoubleGen(gen, ASM_SUBSD); break;
	case OP_ASTERISK : simpleOpDoubleGen(gen, ASM_MULSD); break;
	case OP_DIV : simpleOpDoubleGen(gen, ASM_DIVSD); break;

	case OP_AMP : simpleOpDoubleGen(gen, ASM_AND); break;
	case OP_BOR : simpleOpDoubleGen(gen, ASM_OR); break;
	case OP_XOR : simpleOpDoubleGen(gen, ASM_XOR); break;

	case OP_EQUAL : compareOpDoubleGen(gen, ASM_SETE); break;
	case OP_UNEQUAL : compareOpDoubleGen(gen, ASM_SETNE); break;
	case OP_LESS_OR_EQUAL : compareOpDoubleGen(gen, ASM_SETBE); break;
	case OP_GREATER_OR_EQUAL : compareOpDoubleGen(gen, ASM_SETAE); break;
	case OP_LESS : compareOpDoubleGen(gen, ASM_SETB); break;
	case OP_GREATER : compareOpDoubleGen(gen, ASM_SETA); break;
	}
}

void static binaryPointerGen(CodeGen& gen, Token op, vector<ExprNode*>& children, bool isPointerLeft)
{
	if (op == OP_ADD)
	{
		ExprNode* pointer = isPointerLeft ? LEFT_CHILD(children) : RIGHT_CHILD(children);
		SymType* pType = static_cast<SymTypePointer*>(pointer->getType())->dereference();
		RegT left = isPointerLeft ? REG_EBX : REG_EAX, right = !isPointerLeft ? REG_EBX : REG_EAX;
		gen.addCommand(ASM_POP, getReg(left));
		gen.addCommand(ASM_POP, getReg(right));
		gen.addCommand(ASM_IMUL, getReg(REG_EBX), to_string(pType->getSize()));
		gen.addCommand(ASM_ADD, getReg(REG_EAX), getReg(REG_EBX));	
	}
	if (op == OP_SUB)
	{
		gen.addCommand(ASM_POP, getReg(REG_EBX));
		gen.addCommand(ASM_POP, getReg(REG_EAX));
		gen.addCommand(ASM_SUB, getReg(REG_EAX), getReg(REG_EBX));
	}
	gen.addCommand(ASM_PUSH, getReg(REG_EAX));
}

void BinaryNode::gen(CodeGen& gen)
{

	LEFT_CHILD(children)->gen(gen);
	RIGHT_CHILD(children)->gen(gen);
	
#define isType(type) *LEFT_CHILD(children) == type || *RIGHT_CHILD(children) == type

	if (isType(POINTER))
	{
		binaryPointerGen(gen, token, children, *LEFT_CHILD(children) == POINTER);
		return;
	}
	if (isType(DOUBLE) && !isLogic(token))
		binaryDoubleGen(gen, token);
	if (isType(INT))
		binaryIntGen(gen, token);
	
#undef isType
}

void TernaryNode::gen(CodeGen& gen)
{
	string end = gen.genLabel(), rightCond = gen.genLabel();

	LEFT_CHILD(children)->gen(gen);
	gen.addCommand(ASM_POP, getReg(REG_ECX));
	gen.addCommand(ASM_JZ, rightCond);
	RIGHT_CHILD(children)->gen(gen);
	gen.addCommand(ASM_JMP, end);
	gen.addLabel(rightCond);
	TERNARY_CHILD(children)->gen(gen);
	gen.addLabel(end);
}

void PostfixUnaryNode::gen(CodeGen& gen)
{
	ONLY_CHILD(children)->genLvalue(gen);
	gen.addCommand(ASM_POP, getReg(REG_EAX));
	if (*type == INT)
	{
		gen.addCommand(ASM_MOV, getReg(REG_EBX), DW(ADR(getReg(REG_EAX)))); 
		gen.addCommand(token == OP_INC ? ASM_INC : ASM_DEC, DW(ADR(getReg(REG_EAX))));
		gen.addCommand(ASM_PUSH, getReg(REG_EBX));
	}
	else
	{
		gen.addCommand(ASM_MOVSD, getReg(REG_XMM1), QW(ADR(getReg(REG_EAX)))); 
		gen.addCommand(ASM_MOVSD,  getReg(REG_XMM0), QW(ADR(getReg(REG_EAX))));
		gen.addCommand(token == OP_INC ? ASM_ADDSD : ASM_SUBSD, getReg(REG_XMM0), getPrefix(to_string(1), PREFIX_DOUBLE_CONST));
		gen.addCommand(ASM_MOVSD,  QW(ADR(getReg(REG_EAX))), getReg(REG_XMM0));
		pushDouble(gen, getReg(REG_XMM1));
	}
}

void ExprConst::gen(CodeGen& gen)
{
	gen.addCommand(ASM_PUSH, getValue(0));
}

void FloatConst::gen(CodeGen& gen)
{
	//gen.addCommand(ASM_PUSH, getValue(0));
}

void DoubleConst::gen(CodeGen& gen)
{
	string name;
	if (!gen.hasConst(DOUBLE, getValue(2)))
	{
		gen.insertConst(DOUBLE, getValue(2));
		name = getPrefix(to_string(gen.getConstCount(DOUBLE)), PREFIX_DOUBLE_CONST);
		gen.addDQ(name, getValue(2), 8);
	}
	else
		name = gen.getConst(DOUBLE, getValue(2));
	pushDouble(gen, QW(name));
}

void StringConst::gen(CodeGen& gen)
{
	string name;
	if (!gen.hasConst(STRING, token.val.strValue))
	{
		gen.insertConst(STRING, token.val.strValue);
		name = getPrefix(to_string(gen.getConstCount(STRING)), PREFIX_STRING_CONST);
		gen.addDB(name, token.val.strValue);
	}
	else
		name = gen.getConst(STRING, token.val.strValue);
	gen.addCommand(ASM_PUSH, OFFSET(name));
}

void InitList::gen(CodeGen& gen)
{
	int prev = 0;
	vector<ExprNode*> v = getChildren();
	for (size_t i = 0; i < v.size(); i++)
	{
		ExprNode* x = v[i];
		if (prev != 0)
			shift += prev;
		prev = x->getType()->getSize();
		if (*x == ARRAY)
		{
			static_cast<InitList*>(x)->initShift(shift);
			x->gen(gen);
		}
		else
		{
			if(static_cast<SymTypeArray*>(getType())->isGlobal())
			{
				if (static_cast<SymTypeArray*>(getType())->dereference()->getSize() == 4)
					gen.addDD(string(), x->getValue(0), 4);
				else
					gen.addDQ(string(), x->getValue(2), 8);
			}
			else
			{
				if (x->getType()->getSize() == 4)
					gen.addCommand(ASM_MOV, DW(SHIFT(static_cast<SymTypeArray*>(getType())->getVarAsmName(), to_string(shift))), x->getValue(0));
				else
				{
					gen.addCommand(ASM_MOVSD, getReg(REG_XMM0), QW(x->getValue(2)));
					gen.addCommand(ASM_MOVSD, QW(SHIFT(static_cast<SymTypeArray*>(getType())->getVarAsmName(), to_string(shift))), getReg(REG_XMM0));
				}

			}
		}
	}
}

void ExprFieldSelect::gen(CodeGen& gen)
{
}


void static doubleAssignment(CodeGen& gen)
{
	popDouble(gen);
	gen.addCommand(ASM_POP, getReg(REG_EAX));
	gen.addCommand(ASM_MOVSD, QW(ADR(getReg(REG_EAX))), getReg(REG_XMM0));
	pushDouble(gen, getReg(REG_XMM0));
}

void ExprAssignment::gen(CodeGen& gen)
{
	if (*LEFT_CHILD(children) != ARRAY)
		LEFT_CHILD(children)->genLvalue(gen);
	RIGHT_CHILD(children)->gen(gen);

	if (*LEFT_CHILD(children) == STRUCT)
	{
		//lala topola
	}
			
	if (*LEFT_CHILD(children) == DOUBLE)
		doubleAssignment(gen);
	if (*LEFT_CHILD(children) == INT || *LEFT_CHILD(children) == POINTER)
		simpleAssignment(gen);
}

void ExprCast::gen(CodeGen& gen)
{
	ONLY_CHILD(children)->gen(gen);
	if (!toCast)
		return;
	if (*type == INT)
	{
		popDouble(gen);
		gen.addCommand(ASM_CVTTSD2SI, getReg(REG_EAX), getReg(REG_XMM0));
		gen.addCommand(ASM_PUSH, getReg(REG_EAX));
	}
	if (*type == DOUBLE)
	{
		if(*ONLY_CHILD(children) == INT)
		{
			gen.addCommand(ASM_POP, getReg(REG_EAX));
			gen.addCommand(ASM_CVTSI2SD, getReg(REG_XMM0), getReg(REG_EAX));
			pushDouble(gen, getReg(REG_XMM0));
		}
	}
}

void ExprFuncCall::gen(CodeGen& gen)
{
	SymTypeFunc* func = static_cast<SymTypeFunc*>(LEFT_CHILD(children)->getType());

	size_t size = 0, rshift = func->dereference()->getSize();
	gen.addCommand(ASM_SUB, getReg(REG_ESP), to_string(rshift));
	gen.shiftStack(-rshift);

	for (int i = children.size() - 1; i > 0; i--)
	{
		children[i]->gen(gen);
		size += children[i]->getType()->getSize();
	}
	
	gen.addCommand(ASM_CALL, func->getVarAsmName());
	gen.shiftStack(size);
	gen.addCommand(ASM_ADD, getReg(REG_ESP), to_string(size));
}

void generateLvalue(CodeGen& gen, string name)
{
	gen.addCommand(ASM_LEA, getReg(REG_EAX), name);
	gen.addCommand(ASM_PUSH, getReg(REG_EAX));
}

void ExprVar::gen(CodeGen& gen)
{
	if (*this == INT || *this == FLOAT)
		gen.addCommand(ASM_PUSH, DW(static_cast<SymVar*>(sym)->getAsmName()));

	else 
		if (*this == DOUBLE)
		pushDouble(gen, QW(static_cast<SymVar*>(sym)->getAsmName()));

	else
		if (*this == POINTER)
		{
			gen.addCommand(ASM_MOV, getReg(REG_EAX), DW(static_cast<SymVar*>(sym)->getAsmName()));//tmp for int
			gen.addCommand(ASM_PUSH, getReg(REG_EAX));
		}
	else
		genLvalue(gen);
}

void ExprVar::genLvalue(CodeGen& gen)
{
	if (sym->isGlobal())
		gen.addCommand(ASM_PUSH, OFFSET(static_cast<SymVar*>(sym)->getAsmName()));
	else
		generateLvalue(gen, static_cast<SymVar*>(sym)->getAsmName());
}

void ExprIndexing::gen(CodeGen& gen)
{
	LEFT_CHILD(children)->gen(gen);
	RIGHT_CHILD(children)->gen(gen);

	binaryPointerGen(gen, Token(OP_ADD, "+", "+"), children, true);
	gen.addCommand(ASM_POP, getReg(REG_EAX));

	if (*type == DOUBLE || *type == FLOAT)
		pushDouble(gen, QW(ADR(getReg(REG_EAX))));
	else
	if (*type == INT || *type == POINTER)
		gen.addCommand(ASM_PUSH, DW(ADR(getReg(REG_EAX))));
	else
		gen.addCommand(ASM_PUSH, getReg(REG_EAX));
}

void StmtNode::gen(CodeGen& gen)
{
	gen.restoreStack();
	gen.addEoln(1);
}

void StmtLabeled::gen(CodeGen& gen)
{
}

void StmtSelection::gen(CodeGen& gen)
{
	string lElse = gen.genLabel() + "else", lEnd = gen.genLabel()  + "end";
	LEFT_CHILD(expr)->gen(gen);
	gen.addCommand(ASM_POP, getReg(REG_ECX));
	gen.addCommand(ASM_CMP, getReg(REG_ECX), to_string(0));
	gen.addCommand(ASM_JZ,  lElse);
	LEFT_CHILD(stmt)->gen(gen);
	gen.addCommand(ASM_JMP, lEnd);
	gen.addLabel(lElse);
	if (RIGHT_CHILD(stmt) != NULL)
		RIGHT_CHILD(stmt)->gen(gen);
	gen.addLabel(lEnd);
}

void StmtWhile::gen(CodeGen& gen)
{
	string lWhile = gen.genLabel() + "while", lendWhile = gen.genLabel()  + "endWhile";
	gen.pushJumps(lWhile, lendWhile);
	gen.addLabel(lWhile);
	LEFT_CHILD(expr)->gen(gen);
	gen.addCommand(ASM_POP, getReg(REG_ECX));
	gen.addCommand(ASM_CMP, getReg(REG_ECX), to_string(0));
	gen.addCommand(ASM_JZ,  lendWhile);
	LEFT_CHILD(stmt)->gen(gen);
	gen.addCommand(ASM_JMP, lWhile);
	gen.addLabel(lendWhile);
	gen.popJumps();
}

void StmtDo::gen(CodeGen& gen)
{
	string lDo = gen.genLabel() + "do", lEnd = gen.genLabel() + "endDo";
	gen.pushJumps(lDo, lEnd);
	gen.addLabel(lDo);
	LEFT_CHILD(stmt)->gen(gen);
	RIGHT_CHILD(stmt)->getExpr()->gen(gen);
	gen.addCommand(ASM_POP, getReg(REG_ECX));
	gen.addCommand(ASM_CMP, getReg(REG_ECX), to_string(0));
	gen.addCommand(ASM_JNZ,  lDo);
	gen.addLabel(lEnd);
	gen.popJumps();
}

void StmtExpr::gen(CodeGen& gen)
{
	for (int i = 0; i < expr.size() && expr[i] != NULL; i++)
		expr[i]->gen(gen);
	StmtNode::gen(gen);
}

void StmtFor::gen(CodeGen& gen)
{
	string cond = gen.genLabel() + "cond", end = gen.genLabel() + "end", start = gen.genLabel() + "start";
	gen.pushJumps(start, end);
	LEFT_CHILD(stmt)->gen(gen);
	gen.addCommand(ASM_JMP, cond);
	gen.addLabel(start);
	LEFT_CHILD(expr)->gen(gen);
	gen.restoreStack();

	gen.addLabel(cond);
	RIGHT_CHILD(stmt)->getExpr()->gen(gen);
	gen.addCommand(ASM_POP, getReg(REG_ECX));
	gen.addCommand(ASM_CMP, getReg(REG_ECX), to_string(0));
	gen.addCommand(ASM_JZ, end);

	TERNARY_CHILD(stmt)->gen(gen);
	gen.addCommand(ASM_JMP, start);
	gen.addLabel(end);
	gen.popJumps();
}

void StmtJump::gen(CodeGen& gen)
{
	switch(token)
	{
	case KW_RETURN :
		{
			if (*this->getType() != VOID)
			{
				gen.addCommand(ASM_LEA, getReg(REG_EAX), string("func_ret[ebp]"));
				gen.addCommand(ASM_PUSH, getReg(REG_EAX));
				getExpr(0, 0)->gen(gen);
				if (*this->getType() == INT)
					simpleAssignment(gen);

				if (*this->getType() == DOUBLE)
					doubleAssignment(gen);
			}			
			StmtNode::gen(gen);
			gen.addCommand(ASM_JMP, RET(gen.getLastFunction()));
		}break;

	case KW_BREAK : gen.addCommand(ASM_JMP, gen.getJumpBreak()); break;

	default : gen.addCommand(ASM_JMP, gen.getJumpContinue()); break;

	}
	StmtNode::gen(gen);
}

void StmtCompound::gen(CodeGen& gen)
{
	for (int i = 0; i < stmt.size(); i++)
		stmt[i]->gen(gen);
}

size_t StmtCompound::genLocal(CodeGen& gen, int beforeSize, int& level, int& paramShift)
{
	Symbol* var = NULL;
	size_t lShift = beforeSize;
	bool isParamList = true;
	int i = 0;
	while ((var = table->next() )!= NULL)
	{
		if (!var->isVar())
			continue;
		if (isParamList)
		{
			if (var->isLocal())
				isParamList = false;
			else
			{
				paramShift += var->getType()->getSize();
				if (i++ == 0)
					paramShift = 8;
			}
		}
		if (!isParamList)
			lShift += var->getType()->getSize();
		var->gen(gen);
		string& asmName = static_cast<SymVar*>(var)->getAsmName();
		gen.addLocalVars(asmName += to_string(level), to_string(var->isLocal() ? -((int)lShift) : (int)paramShift));
		asmName = EBP(asmName);
	}

	for (int i = 0; i < compounds.size(); i++)
		lShift = static_cast<StmtCompound*>(compounds[i])->genLocal(gen, lShift, ++level, paramShift);
	return lShift;
}

void PostfixUnaryNode::genLvalue(CodeGen& gen)
{
	//ONLY_CHILD(children)->gen(gen);
}

void UnaryNode::genLvalue(CodeGen& gen)
{
	ONLY_CHILD(children)->gen(gen);	
}

void ExprIndexing::genLvalue(CodeGen& gen)
{
	LEFT_CHILD(children)->gen(gen);
	RIGHT_CHILD(children)->gen(gen);
	binaryPointerGen(gen, Token(OP_ADD, "+", "+"), children, true);
}

void ExprFieldSelect::genLvalue(CodeGen& gen)
{

}

void ExprAssignment::genLvalue(CodeGen& gen){}

void BinaryNode::genLvalue(CodeGen& gen){}