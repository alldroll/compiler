#include "symTable.h"
#include "codeGen.h"
#include "node.h"

#define EBP(offset) (offset + "[ebp]")
#define RET_LABEL(funcName) (funcName + "RetLabel")
#define RET_ADDR "func_ret"
#define DW(name) ("dword ptr " + name)

const string TypeText[11] = 
{
	"pointer",
	"struct",
	"function",
	"enum",
	"int",
	"float",
	"double",
	"string const",
	"void",
	"array",
	"undefined",
};

SymType* _void = new SymTypeVoid();
SymType* _float = new SymTypeFloat();
SymType* _int = new SymTypeInteger();
SymType* _double = new SymTypeDouble();

SymVar* _printf = new SymVarGlobal(Token(IDENTIFIER, "printf", "printf"), _int, new SyntaxNode());
SymVar* _scanf = new SymVarGlobal(Token(IDENTIFIER, "scanf", "scanf"), _int, new SyntaxNode());

SymbolTable::SymbolTable(){index = 0;}

typedef list<SymbolTable*>::iterator iter;

void SymbolTable::putSymbol(Symbol *sym)
{
	const string& name = sym->getName();
	Symbol* s = table[name];
	if (s != NULL && !sym->equal(s) || s != NULL && s->isParam())
		throw ParserException(sym->getLine(), sym->getCol(), "redefinition, different basic types");
	if (s == NULL || !s->isInit() && sym->isInit())
	{
		table[name] = sym;
		if (s == NULL)
			orderList.push_back(name);
	}
}

Symbol* SymbolTable::operator[](const string& name)
{
	return table.find(name) == table.end() ? NULL : table[name];
}

SymbolTableStack::SymbolTableStack()
{
	push(new SymbolTable());
	putSymbol(_void);
	putSymbol(_float);
	putSymbol(_double);
	putSymbol(_int);
	_printf->assignType(new SymTypeFunc(_int, vector<Symbol*>()), false);
	_scanf->assignType(new SymTypeFunc(_int, vector<Symbol*>()), false);
	putSymbol(_printf);
	putSymbol(_scanf);
};

SymbolTable* SymbolTableStack::getGlobal()
{
	return *symTableStack.begin();
}

SymbolTableStack::~SymbolTableStack(){};

void SymbolTableStack::push(SymbolTable* table)
{
	symTableStack.push_front(table);
	if (symTableStack.size() == 1)
		ptr = symTableStack.begin();
	else
		--ptr;
	table->getLevel() = symTableStack.size() - 1;
}

void SymbolTableStack::pop()
{
	symTableStack.pop_front();
	ptr = symTableStack.begin();
}

Symbol* SymbolTable::next()
{
	return index >= orderList.size() ? NULL : (*this)[orderList[index++]];	
}

Symbol* SymbolTable::first()
{
	index = 0;
	return (*this)[orderList[index++]];	
}

Symbol* SymbolTable::last()
{
	return orderList.size() == 0 ? NULL : (*this)[orderList[orderList.size() - 1]];
}

Symbol* SymbolTable::at(int i)
{
	return orderList.size() == 0  || orderList.size() <= index ? NULL : (*this)[orderList[i]];
}

void SymbolTableStack::putSymbol(Symbol* sym)
{
	(*ptr)->putSymbol(sym);
}

Symbol* SymbolTableStack::getSymbol(const Token& name)
{
	Symbol *sym = NULL;
	iter i = ptr;
	SymbolTable* t = NULL;
	for (; i != symTableStack.end() && sym == NULL; i++)
	{
		t = *i;
		sym = (*t)[name.text];
	}
	if (sym == NULL)
		throw ParserException(name.line, name.col, "identifier " + name.text + " is undefined");
	return sym;
}

bool SymbolTableStack::hasSymbolInCurrTable(const Token& name)
{
	SymbolTable* t = *ptr;
	return (*t)[name.text] != NULL && name.text[name.text.length() - 1] != '{';
}

void SymType::print(string out, bool printDecl = true)
{
	cout << out + TypeText[type];
}

void SymTypePointer::print(string out, bool printDecl = true)
{
	SymType::print(out);
	cout << " to ";
	to->print(out, printDecl);
}

void SymTypeArray::print(string out, bool printDecl = true)
{
	SymType::print(out);
	if (printDecl)
	{
		cout << " of length ";
		if (length != NULL)
		{
			cout << endl;
			length->print(out + "           ", true);
		}
		else
			cout << " : undefined" << " ";
	}
	cout << (length != NULL ? out : "") << out + (printDecl && length != NULL ? "        " : "") + " of ";
	to->print(out, printDecl);
}

void SymVar::print(string out, bool printDecl = true)
{	
	cout << out + name.text << " : ";
	type->print("", printDecl && *type != STRUCT);
	if (printDecl && initializer != NULL)
	{
		cout << endl << out + "initializer :" << endl;
		initializer->print(out, printDecl);
	}
}

void SymTypeFunc::print(string out, bool printDecl = true)
{
	SymType::print(out, printDecl);
	if (printDecl)
	{
		cout << " {param list : (";
		if (paramList.size() == 0)
			cout << "none)}";
	
		for (size_t i = 0; i < paramList.size(); i++)
		{
			paramList[i]->print("", false);
			cout << (i < paramList.size() - 1 ?  ", " : ")}");
		}
	}
	cout << " returns ";
	to->print(out, printDecl);
}

void SymbolTable::print(string out, bool printDecl = true)
{
	int i = 0;
	bool p = printDecl;
	while (i != table.size())
	{
		printDecl = p;
		if (out.size() == 0 && i < 6)
		{
			cout << "<base> type : ";
			printDecl = false;
		}
		table[orderList[i++]]->print(out, printDecl);
		cout << endl;
	}
}

void SymbolTableStack::print()
{
	(*ptr)->print("");
}

void SymTypePointer::assign(SymType* t, bool e) 
{
	SymTypePointer *root = this;
	if (e && (*root->to == ARRAY || *root->to == FUNCTION) || !e && root->to->isPointer())
		root->to->assign(t, *root->to == POINTER);
	else
		root->to = t;
}

void SymVar::assignType(SymType* t, bool isP) 
{
	if (*t == FUNCTION || *t == ARRAY)
		static_cast<SymTypePointer*>(t)->assignVar(this);

	if (!isP && type->isPointer() && *type != POINTER || isP && type->isPointer())
		type->assign(t, *type == POINTER);
	else
		type = t;
}

void SymTypeEnumConst::print(string out, bool printDecl = true)
{
	cout << "enum const " ;
	SymVar::print(out, printDecl);
	cout << "= " << index;
}

void SymTypeRecord::print(string out, bool printDeclaration = true)
{
	cout << out + name.text + " ";
	if (table == NULL)
		throw ParserException(name.line, name.col, name.text + " is undefined record(struct or enum)");
	if (printDeclaration)
	{
		cout << endl << out + " {" << endl;
		table->print(out + "    ", false);
		cout << out + " }" << endl;
	}
}

void SymTypeStruct::print(string out, bool printDeclaration = true)
{
	SymTypeRecord::print(out, printDeclaration);
}

void SymTypeEnum::print(string out, bool printDecl = true)
{
	SymTypeRecord::print(out, printDecl);
}

size_t SymbolTable::size() const
{
	return table.size();
}

bool SymType::equal(Symbol* a)
{
	return a != NULL && *static_cast<SymType*>(a) == type;
}

bool static isEqualFunctions(SymTypeFunc* f1, SymTypeFunc* f2)
{
	return f2 != NULL && f2->isPointer() &&  f1->dereference()->equal(f2->dereference());
}

bool static isEqualFunctionDis(SymTypePointer* p, SymTypeFunc* f)
{
	SymType* d = p->dereference();
	return *d == FUNCTION && isEqualFunctions(static_cast<SymTypeFunc*>(d), f);
}

bool SymTypePointer::equal(Symbol* a)
{
	if (a != NULL && *a == FUNCTION)
		return isEqualFunctionDis(this, static_cast<SymTypeFunc*>(a));
	return a != NULL && a->isPointer() &&  to->equal(static_cast<SymTypePointer*>(a)->to);
}

void SymVar::init(void* initList)
{
	Token assignment = Token(getLine(), getCol());
	assignment.setTag(OP_ASSIGN, "=", "=");
	initializer = (*type == FUNCTION || *type == ARRAY) ? static_cast<ExprNode*>(initList) : tryCastInAssignment(getType(), static_cast<ExprNode*>(initList), assignment);
}

void SymTypeRecord::init(void* t)
{
	table = static_cast<SymbolTable*>(t);	
}

void SymbolTableStack::init(Symbol* var, void* init)
{
	Symbol* s = NULL;
	Token name = var->getTokenName();
	if (hasSymbolInCurrTable(name))
		s = getSymbol(name);
	if (s != NULL && s->isInit())
		throw ParserException(name.line, name.col, "redefinition; multiple initialization");
	var->init(init);
	putSymbol(var);
}

bool SymTypeFunc::equal(Symbol* a)
{
	if (a == NULL || !(*a == FUNCTION))
		return false;
	SymTypeFunc* func = static_cast<SymTypeFunc*>(a);

	if (func->getParams().size() != paramList.size())
		throw ParserException(getLine(), getCol(), "too few arguments");

	bool isParamsGood = true;

	for (int i = 0; i < int(paramList.size()) && isParamsGood; i++)
		isParamsGood = paramList[i]->equal(func->getParams()[i]);

	if (!isParamsGood)
		throw ParserException(getLine(), getCol(), "conflict types of function arguments");
	return isEqualFunctions(this, func);
}

bool SymVar::equal(Symbol* a)
{
	return a != NULL && type->equal(a->getType());
}

bool SymTypeArray::equal(Symbol* a)
{
	if (a == NULL || !(*a == ARRAY))
		return false;
	return SymTypePointer::equal(a);
}

bool SymTypeRecord::equal(Symbol *a)
{
	return a != NULL && *a == STRUCT && name.text == a->getName();
}

void SymbolTableStack::initFunctionsAndVarsArray()
{
	SymbolTable* gl = getGlobal();
	Symbol* currSym = NULL;
	while ((currSym = gl->next()) != NULL)
	{
		if (*currSym == FUNCTION)
		{
			functions.push_back(currSym);
			continue;
		}
		if (currSym->isVar())
			vars.push_back(currSym);
	}
}

void SymVar::gen(CodeGen& gen)
{
	if (!isInit() || *this == FUNCTION || *this == ARRAY)
		return;
	if (!static_cast<ExprNode*>(initializer)->isConst())
		throw SemanticsException(getLine(), getCol(), "initializer must be a const");
}

void SymVarLocal::gen(CodeGen& gen)
{
	asmName = getPrefix(asmName, PREFIX_LOCAL);
}

void SymVarGlobal::gen(CodeGen& gen)
{
	if (asmName == "printf" || asmName == "scanf")
	{
		asmName = "_imp__" + asmName;
		return;
	}

	SymVar::gen(gen);
			
	if (*this == INT || *this == FLOAT || *this == POINTER)
		gen.addDD(asmName = getPrefix(asmName, PREFIX_GLOBAL), *this != POINTER  && isInit() ? initializer->getValue(*this != INT) : to_string(0), 4);
	if (*this == DOUBLE)
		gen.addDQ(asmName = getPrefix(asmName, PREFIX_GLOBAL), isInit() ? initializer->getValue(2) : to_string(0), 8);
	if (*this == FUNCTION || *this == ARRAY || *this == STRUCT)
	{
		if (*this != FUNCTION)
			gen.addData(asmName = getPrefix(asmName, PREFIX_GLOBAL));
		getType()->gen(gen);
	}
}

void SymVarParam::gen(CodeGen& gen)
{
	asmName = getPrefix(asmName, PREFIX_PARAM);
}

void SymTypeFunc::gen(CodeGen& gen)
{
	if (var->getName() == "printf" || var->getName() == "scanf")
		return;
		
	gen.addFunc(var->getName());

	SyntaxNode* body = getBody();

	int level = 0, retShift = 4, pShift = static_cast<StmtCompound*>(body)->genLocal(gen, 0, level, retShift);
	if (*dereference() != VOID)
		gen.addLocalVars(string(RET_ADDR), to_string(retShift + dereference()->getSize()));

	static_cast<StmtCompound*>(body)->gen(gen);

	gen.genPrologue(pShift);
	gen.addLabel(RET_LABEL(var->getName()));
	gen.genEpilogue(pShift);
}

size_t SymTypeRecord::getSize()
{
	Symbol* sym;
	for(size != 0; (sym = table->next()) != NULL;)
		size += sym->getType()->getSize();
	return size;
}

void SymTypeStruct::gen(CodeGen& gen)
{
	gen.addDB(string(), to_string(getSize()) + " dup(?)");
}

void SymTypeArray::gen(CodeGen& gen)
{
	InitList* list = static_cast<InitList*>(var->getInitializer());
	if (list == NULL)
	{
		SymType* type = dereference();
		while(type->isPointer())
			type = static_cast<SymTypePointer*>(type)->dereference();
		string value = to_string(getArraySize()) + " dup(?)";
		switch(type->getSize())
		{
		case 1 : gen.addDB(string(), value);break;
		case 4 : gen.addDD(string(), value, 4);break;
		case 8 : gen.addDB(string(), value);break;
		}
		return;
	}
	list->gen(gen);
}

size_t SymTypeArray::getArraySize()
{
	if (!isInit())
		return 0;
	if (arraySize != - 1)
		return arraySize;
	arraySize = atoi(length->getName().c_str());
	SymType* type = dereference();
	while(*type == ARRAY)
	{
		arraySize *= static_cast<SymTypeArray*>(type)->getArraySize();
		type = static_cast<SymTypePointer*>(type)->dereference();
	}
	return arraySize;
}

size_t SymTypeArray::getSize()
{
	SymType* type = dereference();
	while(*type == ARRAY)
		type = static_cast<SymTypePointer*>(type)->dereference();
	return getArraySize() * type->getSize();
}