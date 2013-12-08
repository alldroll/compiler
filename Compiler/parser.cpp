#include "parser.h" 

stack<StmtCompound*> compound;

void static pushCompound(StmtCompound* com)
{
	compound.push(com);
}

void static popCompound()
{
	compound.pop();
}

StmtCompound* first()
{
	return compound.size() == 0 ? NULL : compound.top();
}

void static addCompound(StmtCompound* com)
{
	if (compound.size() != 0)
		first()->addCompound(com);
	pushCompound(com);	
}

const TokenTypeT BINARY_CLASS_OP[BINARYT_NUMBER][4]=
{
	{OP_OR, UNDEFINED, UNDEFINED, UNDEFINED},
	{OP_AND, UNDEFINED, UNDEFINED, UNDEFINED},
	{OP_BOR, UNDEFINED, UNDEFINED, UNDEFINED},
	{OP_XOR, UNDEFINED, UNDEFINED, UNDEFINED},
	{OP_AMP, UNDEFINED, UNDEFINED, UNDEFINED},
	{OP_EQUAL, OP_UNEQUAL, UNDEFINED, UNDEFINED},
	{OP_LESS, OP_GREATER, OP_LESS_OR_EQUAL, OP_GREATER_OR_EQUAL},
	{OP_L_SHIFT, OP_R_SHIFT, UNDEFINED, UNDEFINED},
	{OP_ADD, OP_SUB, UNDEFINED, UNDEFINED},
	{OP_ASTERISK, OP_MOD, OP_DIV, UNDEFINED},
};

const TokenTypeT UNARY_OP[] = 
{
	OP_AMP, OP_ASTERISK,
	OP_ADD, OP_SUB, 
	OP_TILDA, OP_NOT,
	UNDEFINED,
};

const TokenTypeT ASSIGNMENT_OP[] = 
{
	OP_ASSIGN,
	OP_ADD_ASSIGN,
	OP_SUB_ASSIGN,
	OP_MUL_ASSIGN,
	OP_DIV_ASSIGN,
	OP_MOD_ASSIGN,
	OP_AND_ASSIGN,
	OP_OR_ASSIGN,
	OP_XOR_ASSIGN,
	UNDEFINED,
};

const TokenTypeT DECL_SPEC[] = 
{
	KW_VOID, KW_INT, KW_FLOAT, KW_DOUBLE, KW_STRUCT, KW_ENUM, UNDEFINED
};

#define TYPESPEC_BEGIN  0
#define TYPESPEC_END  4

const bool withoutMove = false, specifierQualifier = false;

static bool maybeAssignmentOp(const Token& look)
{
	bool isAssign = false;
	for (int i = 0; ASSIGNMENT_OP[i] != UNDEFINED && !isAssign; i++)
		isAssign = look == ASSIGNMENT_OP[i];
	return isAssign;
}

static bool maybeUnaryOp(const Token& look)
{
	bool isUnary = false;
	for (int i = 0; UNARY_OP[i] != UNDEFINED && !isUnary; i++)
		isUnary = look == UNARY_OP[i];
	return isUnary;
}

bool Parser::maybeBinaryOp(BinaryT btype)
{
	bool isBinary = false;
	for (int i = 0; i < 4 && !isBinary; i++)
		isBinary = look == BINARY_CLASS_OP[btype][i];
	return isBinary;
}

static bool maybeVar(const Token& look)
{
	return look == IDENTIFIER;
}

static bool maybeConst(const Token& look)
{
	return look == STRING_CONST || look == INT_CONST || look == DOUBLE_CONST;
}

static Token getOpToken(const Token look)
{
	Token curr = look;
	switch(look)
	{
		case OP_COLON : curr.text = "?:"; break;
		case OP_R_SQUARE : curr.text = "[]"; break;
		default : curr.text = "()";
	}
	return curr;
}

static Token getIncDecOp(const Token look, bool isPost)
{
	Token curr = look;
	if (look == OP_INC || look == OP_DEC)
		curr.text = isPost ? "x" + curr.text : curr.text + "x";
	return curr;
}

static bool maybeDecl(const Token& look)
{
	bool isDecl = false;
	int i = 0;
	while (DECL_SPEC[i] != UNDEFINED && !isDecl)
		isDecl = look == DECL_SPEC[i++];
	return isDecl;
}

void Parser::pushTable(SymbolTable* table)
{
	symTableStack.push(table);
}

void Parser::popTable()
{
	symTableStack.pop();
}

Symbol* Parser::get()
{
	return symTableStack.getSymbol(look);
}

void Parser::pushSymbol(Symbol* sym)
{
	symTableStack.putSymbol(sym);
}

bool Parser::findInCurrTable(const Token& name)
{
	return symTableStack.hasSymbolInCurrTable(name);
}

Parser::Parser(Scanner &scan) : scanner(scan)
{
	isFuncDif = false;
};

SymbolTable* Parser::getGlobalTable()
{
	return symTableStack.getGlobal();
}

Parser::~Parser(){};

void Parser::match(TokenTypeT type, bool toMove = true)
{
	string error = "expected ";
	if (look != type)
		exception(error  + getTypeText(type));
	if (toMove)
		move();
}

void Parser::exception(string error)
{
	throw ParserException(look.line, look.col, error);
}

void Parser::move()
{
	look = scanner.next();
}

bool Parser::isMainDif()
{
	return symTableStack.getSymbol(Token(IDENTIFIER, "main", "main"))->isInit();
}

void Parser::parse()
{
	move();
	parseTranslationUnit();
}

void Parser::printTree()
{
	if (!symTableStack.hasSymbolInCurrTable(Token(IDENTIFIER, "main", "main")) || !isMainDif())
		exception("expected function main definition");
	symTableStack.print();
}

ExprNode* Parser::constExpr()
{
	Token tok = look;
	move();
	if (tok == INT_CONST)
		return new IntegerConst(tok);
	if (tok == DOUBLE_CONST)
		return new DoubleConst(tok);
	return new StringConst(tok);
}

ExprNode* Parser::varExpr(SymbolTable* table = NULL)
{
	Token tok = look;
	Symbol *sym = (table == NULL) ? get() : (*table)[tok.text];
	if (sym == NULL)
		exception(tok.text + " identifier is undefined");
	move();
	return new ExprVar(tok, sym);
}

ExprNode* Parser::parsePrimaryExpr()
{
	ExprNode* prim = NULL;
	if (maybeConst(look))
		prim = constExpr();
	if (maybeVar(look))
		prim = varExpr();
	
	if (prim == NULL && look == L_PARENTHESIS)
		prim = parseParExpr();
	
	if (prim == NULL)
		exception("expected expression");
	return prim;
}

ExprNode* Parser::parsePostfixExpr()
{
	ExprNode *post = NULL;
	Token op = look;
	post = parsePrimaryExpr();
	bool isPostfixExpr = true;
	while (isPostfixExpr)
	{
		switch(look)
		{
			case OP_L_SQUARE :
				{	
					move();	
					ExprNode* rExpr = parseExpression();
					Token op = look;
					match(OP_R_SQUARE);
					post = new ExprIndexing(getOpToken(op), post, rExpr);
				}
			break; 

			case L_PARENTHESIS :
				{
					if (!(*post == FUNCTION || (*post == POINTER && *static_cast<SymTypePointer*>(post->getType())->dereference() == FUNCTION)))
						exception("expression preceding parentheses of apparent call must have (pointer-to-) function type");
					move();
					SymType* fType = (*post == FUNCTION ? static_cast<SymTypeFunc*>(post->getType()) : static_cast<SymTypePointer*>(post->getType())->dereference());
					post = new ExprFuncCall(getOpToken(look), parseArgmExpr(new ExprCast(fType, post)), static_cast<SymTypeFunc*>(fType));
					move();
				}
			break;

			case OP_STRUCT_REFERENCE :
			case OP_STRUCT_DEREFERENCE :
				{
					Token op = look;
					move();
					match(IDENTIFIER, withoutMove);
					string err = "expression must have ";
					
					switch(op)
					{
						case OP_STRUCT_REFERENCE :
							if (*post != STRUCT)
								throw SemanticsException(op.line, op.col, err + "struct type");
							break;
						default :
							{
							if (*post != POINTER || *static_cast<SymTypePointer*>(post->getType())->dereference() != STRUCT)
								throw SemanticsException(op.line, op.col, err + "pointer type");
							post = new ExprCast(static_cast<SymTypePointer*>(post->getType())->dereference(), post);
							}
					}

					SymbolTable* table = post->getType()->getTable();//*post == POINTER ? static_cast<SymTypePointer*>(post->getType())->dereference()->getTable() : post->getTable();
					post = new ExprFieldSelect(op, post, varExpr(table));
				}
			break;

			case OP_INC :
			case OP_DEC :
				{
					
					post = new PostfixUnaryNode(getIncDecOp(look, true), post);
					move();
				}
			break;

		default :
			isPostfixExpr = false;
		}
	}
	return post;
}

ExprNode* Parser::parseCastExpr()
{
	if (look != L_PARENTHESIS)
		return parseUnaryExpr();
	move();
	if (!maybeDecl(look))
	{
		look = scanner.prev();
		return parseUnaryExpr();
	}
	SymType* type = parsePointer(parseTypeSpecifier());
	match(R_PARENTHESIS);
	ExprNode* n = new ExprCast(type, parseCastExpr());
	n->setType(type);
	return n;
}

ExprNode* Parser::parseBinaryExpr(BinaryT btype)
{
	ExprNode *lExpr = btype == MULTI_EXPR ? parseCastExpr() : parseBinaryExpr(BinaryT(btype + 1));
	Token op = look;
	while (maybeBinaryOp(btype))
	{
		move();
		lExpr = new BinaryNode(op, lExpr, (btype == MULTI_EXPR ? parseCastExpr() : parseBinaryExpr(BinaryT(btype + 1))));
		op = look;
	}
	return lExpr;
}

ExprNode* Parser::parseUnaryExpr()
{
	Token op = look;
	if (maybeUnaryOp(look)|| look == OP_INC || look == OP_DEC)
	{
		move();
		ExprNode *node = (look == OP_DEC || look == OP_INC) ? parseUnaryExpr (): parseCastExpr();//UnaryExpr();
		return new UnaryNode(getIncDecOp(op, false), node);
	}
	return parsePostfixExpr();
}

ExprNode* Parser::parseConditionalExpr()
{
	BinaryNode *lExpr = static_cast<BinaryNode*>(parseBinaryExpr(LOGIC_OR_EXPR));
	if (look != OP_QUEST)
		return lExpr;
	move();
	ExprNode* rExpr = parseExpression();
	Token op = getOpToken(look);
	match(OP_COLON);
	return new TernaryNode(op, lExpr, rExpr, parseConditionalExpr());
}

ExprNode* Parser::parseAssignmentExpr()
{
	ExprNode *lExpr = parseConditionalExpr();
	if (maybeAssignmentOp(look))
	{
		Token op = look;
		move();
		lExpr = new ExprAssignment(op, lExpr, parseAssignmentExpr());
	}
	return lExpr;
}

vector<ExprNode*> Parser::parseArgmExpr(ExprNode* post)
{
	vector<ExprNode*> argm;
	argm.push_back(post);
	if (look == R_PARENTHESIS)
		return argm;
	argm.push_back(parseAssignmentExpr());
	while (look == OP_COMMA)
	{
		move();
		argm.push_back(parseAssignmentExpr());
	}
	match(R_PARENTHESIS, withoutMove);
	return argm;
}

ExprNode* Parser::parseExpression()
{
	ExprNode *lExpr = parseAssignmentExpr();
	while (look == OP_COMMA)
	{
		Token op = look;
		move();
		lExpr = new ExprNode(op, lExpr, parseAssignmentExpr());
	}
	return lExpr;
}

ExprNode* Parser::parseParExpr()
{
	match(L_PARENTHESIS);
	ExprNode* expr = parseExpression();
	match(R_PARENTHESIS);
	return expr;
}

StmtNode* Parser::parseStmt(SymType* retType = NULL)
{
	switch(look)
	{
		case KW_FOR : case KW_WHILE :case KW_DO  : return parseIterationStmt(retType);
		case KW_BREAK : case KW_CONTINUE : case KW_RETURN : return parseJumpStmt(retType != NULL ? static_cast<SymTypeFunc*>(retType)->dereference() : NULL);
		case KW_IF : case KW_SWITCH : return parseSelectionStmt(retType);
		case KW_CASE : case KW_DEFAULT : return parseLabeledStmt(retType);
		case L_BRACE : return parseCompoundStmt(retType);
		default : return parseExprStmt(retType);
	}
}

StmtNode* Parser::parseExprStmt(SymType* returnType = NULL)
{
	ExprNode* expr = NULL;
	Token op = look;
	if (op != SEMICOLON)
	{
		expr = parseExpression();
		op = look;
		match(SEMICOLON, withoutMove);
	}
	move();
	return new StmtExpr(op, expr); 
}

StmtNode* Parser::parseJumpStmt(SymType* retType = NULL)
{
	Token op = look;
	move();
	if (op == KW_BREAK || op == KW_CONTINUE)
		match(SEMICOLON, withoutMove);
	return new StmtJump(op, parseExprStmt(), retType);
}

StmtNode* Parser::parseEmptyStmt(SymType* returnType = NULL)
{
	Token op = look;
	match(SEMICOLON);
	return new StmtNode(op);
}

StmtNode* Parser::parseIterationStmt(SymType* returnType = NULL)
{
	StmtNode *eStmt1 = NULL, *eStmt2 = NULL, *stmt;
	ExprNode* expr = NULL;
	Token op = look;
	move();
	switch(op)
	{
		case KW_FOR :
			{
				match(L_PARENTHESIS);
				eStmt1 = parseExprStmt();
				eStmt2 = parseExprStmt();
				if (look != R_PARENTHESIS)
					expr = parseExpression();
				match(R_PARENTHESIS);
				stmt = new StmtFor(op, eStmt1, eStmt2, expr, parseStmt());
			}break;

		case KW_WHILE :
			{
				ExprNode* expr = parseParExpr();
				stmt = new StmtWhile(op, expr, parseStmt());
			}break;

		default :
			{
				eStmt1 = parseStmt(returnType);
				Token wh = look;
				match(KW_WHILE);
				ExprNode* expr = parseParExpr();
				match(SEMICOLON);
				stmt = new StmtDo(op, eStmt1, new StmtExpr(wh, expr));
			}
	}
	return stmt;
}

StmtNode* Parser::parseSelectionStmt(SymType* returnType = NULL)
{
	StmtNode *eStmt1 = NULL, *eStmt2 = NULL;
	ExprNode* expr = NULL;
	Token op = look;
	move();
	expr = parseParExpr();
	eStmt1 = parseStmt(returnType);
	if (look == KW_ELSE && op == KW_IF)
	{
		move();
		eStmt2 = parseStmt(returnType);
	}
	return new StmtSelection(op, expr, eStmt1, eStmt2);
}

StmtNode* Parser::parseCompoundStmt(SymType* funcType = NULL)
{
	Token op = look;
	move();
	SymbolTable* table = new SymbolTable();
	pushTable(table);
	if (table->getLevel() == 1)
	{
		vector<Symbol*> params = static_cast<SymTypeFunc*>(funcType)->getParams();
		for(int i = 0; i < params.size(); i++)
		{
			Symbol* param = params[i];
			if (param->getName() == "undefined")
				exception("expected formal parameter list, not a type list");
			pushSymbol(param);
		}
	}
	vector<StmtNode*> compounds;
	StmtCompound* c = new StmtCompound(op, table, funcType);
	addCompound(c);
	while (look != R_BRACE && look != EOF_TOKEN)
        {
		if (maybeDecl(look))
			{
				size_t beforeSize = table->size();
				parseDeclaration();

				for (int i = beforeSize; i < table->size(); i++)
				{
					Symbol* var = table->at(i);
					if (!var->isInit() || !var->isVar())
						continue;
					ExprNode* assignment = new ExprAssignment(Token(OP_ASSIGN, "=", "="), new ExprVar(var->getTokenName(), var), static_cast<ExprNode*>(static_cast<SymVar*>(var)->getInitializer()), *var->getType() != ARRAY);
					c->addStmt(new StmtExpr(Token(SEMICOLON, ";", ";"), assignment));
				}
				continue;
			}
		if (look != R_BRACE)
			c->addStmt(parseStmt(funcType));
	}
	match(R_BRACE);
	popTable();
	popCompound();
	return c;
}

StmtNode* Parser::parseLabeledStmt(SymType* returnType = NULL)
{
	ExprNode* expr = NULL;
	Token op = look;
	move();
	if (op == KW_CASE)
	{
		expr = parseExpression();
		if (!expr->isConst())
			exception("case label must be constant-expression");
	}
	match(OP_COLON);
	return new StmtLabeled(op, parseStmt(), expr);
}

void Parser::parseDeclList()
{
	while (maybeDecl(look))
		parseDeclaration();
}

SymType* Parser::parseTypeSpecifier()
{
	Token t = look;
	move();
	switch(t)
	{
		case KW_VOID : return _void;
		case KW_INT : return _int;
		case KW_FLOAT : return _float;
		case KW_DOUBLE : return _double;
		case KW_STRUCT : return parseStruct();
		case KW_ENUM : return parseEnum();
	}
	exception("expected type specifier"); 
	return NULL;
}

void Parser::parseDeclaration()
{
	if (look != SEMICOLON)
	{
		if (!maybeDecl(look))
			exception("expected declaration");
		SymType* symType = parseTypeSpecifier();
		if (look != SEMICOLON)
			parseInitDeclList(symType);
	}
	if (!isFuncDif || look == SEMICOLON)
		match(SEMICOLON);
}

void Parser::parseInitDeclList(SymType* type)
{  
	while (parseInitDeclarator(type))
	{
		if (look != OP_COMMA)
			return;
		move();
	}
}

bool Parser::parseInitDeclarator(SymType* type)
{
	isFuncDif = false;
	Symbol* sym = parseGeneralDeclarator(type, DIRECT_SIMPLE);
	if (sym == NULL)
		exception("expected direct declarator");
	pushSymbol(sym);
	if (look == OP_ASSIGN || look == L_BRACE)
	{
		if (*sym == FUNCTION && look == OP_ASSIGN || *sym != FUNCTION && look == L_BRACE)
			exception("expected ;");
		if (*sym == FUNCTION)
		{
			symTableStack.init(sym, parseCompoundStmt(static_cast<SymTypeFunc*>(sym->getType())));
			isFuncDif = true;
		}
		else
		{
			move();
			ExprNode* init = parseInitializer(sym->getType());
			symTableStack.init(sym, init);
		}
	}
	return !isFuncDif;
}

SymType* Parser::parsePointer(SymType* type)
{
	SymType* pointer = type;
	while (look == OP_ASTERISK)
	{
		move();
		pointer = new SymTypePointer(pointer);
	}
	return pointer;
}

vector<Symbol*> Parser::parseParamList()
{
	vector<Symbol*> paramList;
	while (maybeDecl(look))
	{
		paramList.push_back(parseParameterDeclaration());
		if (look != OP_COMMA)
			break;
		move();
		}
	if (look != R_PARENTHESIS)
		exception("expected type specifier");
	return paramList;
}

Symbol* Parser::parseParameterDeclaration()
{
	SymType* type = parseTypeSpecifier();
	Symbol *decl = parseGeneralDeclarator(type, DIRECT_BOTH);
	SymVarParam* param = NULL;
	if (decl != NULL)
		param = new SymVarParam(*static_cast<SymVarParam*>(decl));
	if (param == NULL)
		param = new SymVarParam(Token(), type, NULL);
	return param;
}

Symbol* Parser::parseGeneralDeclarator(SymType* type, DeclaratorT decl)
{
	SymType *t = parsePointer(type);
	if (look == R_PARENTHESIS && decl == DIRECT_BOTH)
	{
		decl = DIRECT_ABSTRACT;
		return new SymVar(Token(), t, NULL);
	}
	return parseGeneralDirectDeclarator(t, decl);
}

Symbol* Parser::parseGeneralDirectDeclarator(SymType* type, DeclaratorT decl)
{
	bool isP = false;
	Symbol* sym = NULL;
	switch(look)
	{
		case IDENTIFIER : 
		{
			if (decl == DIRECT_BOTH)
				decl = DIRECT_SIMPLE;
			if (symTableStack.isGlobal())
				sym = new SymVarGlobal(look, type, NULL);
			else
				sym = new SymVarLocal(look, type, NULL);
			move();
		}
		break;

		case L_PARENTHESIS : 
		{
			isP = true;
			move();
			sym = parseGeneralDeclarator(type, decl);
			if (sym == NULL)
			{
				if (decl == DIRECT_SIMPLE)
					exception("expected direct declarator");
				sym = new SymVarParam(Token(look.line, look.col), parseFunc(type), NULL);
				return sym;
			}
			match(R_PARENTHESIS);
		}
	}
	while(look == OP_L_SQUARE)
	{
		move();
		if (sym == NULL)
		{
			sym = new SymVarParam(Token(look.line, look.col), parseArray(type), NULL);
			continue;
		}
		static_cast<SymVar*>(sym)->assignType(parseArray(type), isP);
	}
	if (look == L_PARENTHESIS && sym != NULL)
	{
		move();
		static_cast<SymVar*>(sym)->assignType(parseFunc(type), isP);
	}
	return sym;
}

SymType* Parser::parseArray(SymType* type)
{
	SyntaxNode* length = NULL;
	if (look != OP_R_SQUARE)
		length = parseConditionalExpr();
	match(OP_R_SQUARE);
	return new SymTypeArray(length, type);
}

SymType* Parser::parseFunc(SymType* type)
{
	vector<Symbol*> argList;
	argList = parseParamList();
	match(R_PARENTHESIS);
	return new SymTypeFunc(type, argList);
}

ExprNode* Parser::parseInitializer(SymType* type)
{
	if (look != L_BRACE)
		return parseAssignmentExpr();
    move();
	ExprNode* node = parseInitializerList(type);
	match(R_BRACE);
    return node;
}

ExprNode* Parser::parseInitializerList(SymType* type)
{
	ExprNode* i = parseInitializer(static_cast<SymTypeArray*>(type)->dereference());
    InitList* node = new InitList(look, type);
    node->add(i);
    while(look == OP_COMMA)
    {
        move();
        if(look == R_BRACE)
            return node;
		node->add(parseInitializer(static_cast<SymTypeArray*>(type)->dereference()));
    }
    return node;
}

void Parser::parseTranslationUnit()
{
	while(look != EOF_TOKEN)
		parseDeclaration();
}

map<string, Symbol*> Parser::parseStructDeclaration()
{
	map<string, Symbol*> declaration;
	SymType* symType = parseTypeSpecifier();
	Symbol* sym = NULL;
	while (look != SEMICOLON)
	{
		sym = parseGeneralDeclarator(symType, DIRECT_SIMPLE);
		if (sym == NULL)
			exception("expected declarator");
		if (*sym->getType() == FUNCTION)
			exception("function cannot be member of struct");
		if (declaration[sym->getName()] != NULL)
			exception("struct member redefinition");
		declaration[sym->getName()] = sym;
		if (look == OP_COMMA)
			move();
	}
	match(SEMICOLON);
	return declaration;
}

SymbolTable* Parser::parseStructDeclarationList()
{
	SymbolTable* declList = new SymbolTable();
	while (look != R_BRACE)
	{
		map<string ,Symbol*> decl = parseStructDeclaration();
		if (decl.size() == 0)
			exception("expected }");
		for (map<string ,Symbol*>::iterator i = decl.begin(); i != decl.end(); i++)
		{
			if ((*declList)[i->second->getName()] != NULL)
				exception("struct member redefinition");
			declList->putSymbol(i->second);
		}
	}
	if (declList->size() == 0)
		exception("C requires that a struct has at least one member");
	return declList;
}

SymType* Parser::parseStruct()
{
	Token name = look;
	name.text = "struct " + name.text;
	SymTypeStruct *_struct = new SymTypeStruct(name);
	if (look == IDENTIFIER)
	{
		pushSymbol(_struct);
		_struct = static_cast<SymTypeStruct*>(symTableStack.getSymbol(_struct->getTokenName()));
		move();
	}
	if (look == L_BRACE)
	{
		move();
		symTableStack.init(_struct, parseStructDeclarationList());
		match(R_BRACE);
	}
	return _struct;
}

SymbolTable* Parser::parseEnumeratorList()
{
	SymbolTable* l = new SymbolTable();
	int index = 0;
	while (look != R_BRACE)
	{
		Token name = look;
		match(IDENTIFIER);
		Symbol* s = new SymTypeEnumConst(name, index++, NULL);
		pushSymbol(s);
		l->putSymbol(s);
		name = look;
		move();
		if (name != OP_COMMA && look != R_BRACE)
			exception("expected }");
	}
	if (index == 0)
		exception("C requires that a enum has at least one member");
	return l;
}

SymType* Parser::parseEnum()
{
	Token name = look;
	name.text = "enum " + name.text;
	SymTypeEnum *_enum = new SymTypeEnum(name);
	if (look == IDENTIFIER)
	{
		pushSymbol(_enum);
		_enum = static_cast<SymTypeEnum*>(symTableStack.getSymbol(_enum->getTokenName()));
		move();
	}
	if (look == L_BRACE)
	{
		move();
		_enum->init(parseEnumeratorList());
		match(R_BRACE);
	}
	return _enum;
}
