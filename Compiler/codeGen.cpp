#include "codeGen.h"
#include <math.h>

#define COM(it) (static_cast<AsmCommand*>(*it))

class AdvanceError : public exception
{
public :
	AdvanceError() throw() : exception(){};
};	

#define SAFE_ADVANCE(iter, n)  for (int i = 0; i < n; i++){++iter; if (iter == code.end()) throw AdvanceError();}
#define ADVANCE(iter, n) SAFE_ADVANCE(iter, n); while(COM(iter)->isEOLN()) SAFE_ADVANCE(iter, 1);

const char* Prefix[12] =
{
	"global$_",
	"local$_",
	"param$_",
	"struct$_",
	"enum$_",
	"label$_",
	"stringConst$_",
	"floatConst$_",
	"doubleConst$_",
};

const char* RegText[15] =
{
	"eax",
	"ebx",
	"ecx",
	"edx",
	"esp", 
	"ebp",
	"bl", 
	"cl",
	"xmm0",
	"xmm1",
	"xmm2",
	"xmm3",
	"xmm4",
	"xmm5",
	"xmm6",
};

const char* AsmTextCommand[54] =
{
	"ret",
	"lea",
	"mov",
	"push",
	"pop",
	"add",
	"sub",
	"call",
	"imul",
	"idiv",
	"inc",
	"dec",
	"neg",
	"not",
	"test",
	"jmp",
	"jz",
	"jnz",
	"jl",
	"jle",
	"jg",
	"jge",
	"je",
	"jne",
	"or",
	"xor",
	"and",
	"cmp",
	"shl",
	"shr",
	"setl",
	"setle",
	"setg",
	"setge",
	"sete",
	"setne",
	"setz",
	"setb",
	"seta",
	"setae",
	"setbe",
	"cdq",
	"=",
	"none",
	"movsd",
	"movss",
	"comisd",
	"addsd",
	"subsd",
	"mulsd",
	"divsd",
	"cvttsd2si",
	"cvtsi2sd",
	"cvttsd2ss",
};

string getPrefix(string var, PrefixT t)
{
	to_string(12);
	return  Prefix[t] + var;
}

string getReg(RegT r)
{
	return RegText[r];
}

void AsmDD::print(ostream& s)
{
	s << name << "\tdd\t";
	if (size == 4)
		s << hex << value << endl;
}

void AsmDQ::print(ostream& s)
{
	s << name << "\tdq\t";
	if (size == 8)
		s << value << endl;
}

void AsmDB::print(ostream& s)
{
#define IS_WHT(ch) (ch == '\n' || ch == '\f')
	s << name << "\tdb\t";
	bool needQuote = true;
	if ((value.size() > 0 && (value[value.length() - 1] != '\"')) || (value.size() == 2))
	{
		s << (value.size() == 2 ? "0" : value) << endl;
		return;
	}
	for (int i = 0; i < value.size() - 1; i++)
	{
		switch(value[i])
		{
		case '\"' : needQuote = !(i < value.size() - 1 && IS_WHT(value[i + 1])); break;
		case '\n' : case '\f' : s << (needQuote ? "\", " : "") << to_string((int)value[i]) << (value[i + 1] != '\"' ? ", " : ""); needQuote = !IS_WHT(value[i + 1]) && value[i + 1] != '\"'; break;
		default : s << (needQuote == true ? "\"" : "") << value[i]; needQuote = IS_WHT(value[i + 1]) || value[i + 1] == '\"';
		}
	}
	s << (needQuote ? "\"" : "") << ",\t" << 0 << endl;
#undef IS_WHT
}

void AsmFunction::print(ostream& s)
{
	s <<  name + (name != "main" ? " proc" : ":") << "\n\n";
	// for (auto &x : localVars)
	// 	x->print(s);
	// s << endl;
	// for (auto &x : code)
	// 	x->print(s);
	for (list<AsmLocalVar*>::iterator it = localVars.begin(); it != localVars.end(); ++it)
		(*it)->print(s);

	s << endl;

	for (list<AsmCode*>::iterator it = code.begin(); it != code.end(); ++it)
		(*it)->print(s);

	s << endl << (name != "main" ?  name +  " endp" : "end main") << endl << endl;
}

void AsmCommand::print(ostream& s)
{
	s << AsmTextCommand[com] << " " + l + (r == "" ?  r : ", " + r) << endl;
}

void AsmLocalVar::print(ostream& s)
{
	s << l + " = " + r << endl;
}

CodeGen::CodeGen(Parser& _parser, const string& out) : parser(_parser), outStream(out.c_str()), stacksLevel(0), lablesCount(0)
{
	parser.parse();
	globalTable = parser.getGlobalTable();
}

void CodeGen::printData()
{
	// for (auto &x : data)
	// 	x->print(outStream);
	for (list<AsmData*>::iterator it = data.begin(); it != data.end(); ++it)
		(*it)->print(outStream);
	outStream << ".code\n\n";
}

void CodeGen::printFunctions()
{
	// for (auto &x : functions)
	// 	x->print(outStream);
	for (list<AsmFunction*>::iterator it = functions.begin(); it != functions.end(); ++it)
		(*it)->print(outStream);
}

void CodeGen::generate()
{
	outStream << 
	".686\n"
	".model flat,stdcall\n"
	"option casemap:none\n\n"
	"include C:\\masm32\\include\\msvcrt.inc\n"
	"includelib C:\\masm32\\lib\\msvcrt.lib\n\n"
	".xmm\n\n"
	".data\n\n";
	Symbol* main = NULL;
	bool hasCodeLabel = false;
	parser.initFunctionsAndVars();
	vector<Symbol*> vars = parser.getVars(), functions = parser.getFunctions();

	for (int i = 0; i < vars.size(); i++)
		vars[i]->gen(*this);
	
	double one = 1, mOne = -1;
	insertConst(DOUBLE, to_string(*((long long*)&(one))));
	addDQ(getPrefix(to_string(1), PREFIX_DOUBLE_CONST), to_string(*((long long*)&(one))), 8);
	for (int i = 0; i < functions.size(); i++)
	{
		if (functions[i]->getName() == "main")
		{
			main = functions[i];
			continue;
		}
		functions[i]->gen(*this);
	}

	main->gen(*this);
	optimize();
	printData();
	printFunctions();
}

static bool isEspInCommand(string command)
{
	return command.find("esp") != string::npos;
}

static void optimizeCmd1(list<AsmCode*>::iterator& it, list<AsmCode*>& code)
{
	//add/sub, 0 to nothing
	if (COM(it)->getCom() == ASM_ADD || COM(it)->getCom() == ASM_SUB)
		if (COM(it)->getRightOp() == "0")
			it = code.erase(it);
	//mov something, 0 to xor some, some
	if (COM(it)->getCom() == ASM_MOV && COM(it)->getRightOp() == "0")
	{
		code.insert(it, new AsmCommand(ASM_XOR, COM(it)->getLeftOp(), COM(it)->getLeftOp()));
		it = code.erase(it);
	}
}

static void optimizeLabel(list<AsmCode*>::iterator& it, list<AsmCode*>& code)
{
	/*
		jump label
		label :
		to 
		label :
	*/
#define IS_JUMP(com) (com == ASM_JMP || com == ASM_JE || com == ASM_JZ || com == ASM_JNE || com == ASM_JG ||  com == ASM_JL || com == ASM_JLE || com == ASM_JGE)
	list<AsmCode*>::iterator it2 = it;
	try
	{
		ADVANCE(it2, 1);
		if (IS_JUMP(COM(it)->getCom()) && (*it2)->isLabel() &&  COM(it)->getLeftOp() == static_cast<AsmLabel*>(*it2)->getLabelName().c_str())
			it = code.erase(it);
	}
	catch(AdvanceError& e){};
#undef IS_JUMP
}

static void optimizeStackShift(list<AsmCode*>::iterator& it, list<AsmCode*>& code)
{
	list<AsmCode*>::iterator it2 = it;
	try
	{	
		/*
		push something
		add esp, sizeof(something)
		to
		nothing

		push some
		pop some
		to
		nothing
		*/
		if (COM(it)->getCom() == ASM_PUSH)
		{
			ADVANCE(it2, 1);
			if (COM(it2)->getCom() == ASM_ADD && isEspInCommand(COM(it2)->getLeftOp()) || COM(it2)->getCom() == ASM_POP)
			{
				if (COM(it2)->getCom() == ASM_POP && COM(it2)->getLeftOp() != COM(it)->getLeftOp())
				{
					string op1 = static_cast<AsmCommand*>((*it2))->getLeftOp(), op2 = static_cast<AsmCommand*>((*it))->getLeftOp();
					code.insert(it, new AsmCommand(ASM_MOV, op1, op2));
				}
				it2 = code.erase(it2);
				it = code.erase(it);
				it2 = --it;
			}

		}
		/*
		sub esp, 8
		movsd xmm0,  ...
		add esp, 8
		to 
		nothing
		*/
		if (COM(it)->getCom() == ASM_SUB && COM(it)->getLeftOp() == "esp" && COM(it)->getRightOp() == "8")
		{
			it2 = it;
			ADVANCE(it2, 1);
			if (!(COM(it2)->getCom() == ASM_MOVSD &&  COM(it2)->getLeftOp() == "xmm0"))
				return;
			ADVANCE(it2, 2);
			if (!(COM(it2)->getCom() == ASM_ADD && COM(it2)->getLeftOp() == "esp" && COM(it2)->getRightOp() == "8"))
				return;
			ADVANCE(it2, 1);
			it = code.erase(it, it2);
		}
	}
	catch(AdvanceError& e){};
}

void optimizeCmd2(list<AsmCode*>::iterator& it, list<AsmCode*>& code)
{
	//add/sub some, arg1
	//add/sub some, arg2
	//....
	//to add/sub some, uArg
	if (COM(it)->getCom() != ASM_ADD && COM(it)->getCom() != ASM_SUB)
		return;
	string left = COM(it)->getLeftOp();
	int value = COM(it)->getCom() != ASM_ADD ? -atoi(COM(it)->getRightOp().c_str()) : atoi(COM(it)->getRightOp().c_str());
	list<AsmCode*>::iterator it2 = it;
	int i = 0;
	try
	{
		while (++i)
		{
			ADVANCE(it2, 1);
			if (!((COM(it2)->getCom() == ASM_ADD || COM(it2)->getCom() == ASM_SUB) && isdigit(COM(it2)->getRightOp()[0]) && COM(it2)->getLeftOp() == left))
				break;
			value += COM(it2)->getCom() != ASM_ADD ? -atoi(COM(it2)->getRightOp().c_str()) : atoi(COM(it2)->getRightOp().c_str());
			it2 = code.erase(it2);	
		}
		if (i > 1)
		{
			COM(it)->getCom() = value > 0 ? ASM_ADD : ASM_SUB;
			COM(it)->getRightOp() = to_string(abs(value));
		}
	}
	catch(AdvanceError& e){};
}

void CodeGen::optimize()
{
	//for (auto &func : functions)
	for (list<AsmFunction*>::iterator it = functions.begin(); it != functions.end(); ++it)
	{
		AsmFunction* func = *it;
		list<AsmCode*>& code = func->getCode();
		for (list<AsmCode*>::iterator it = code.begin(); it != code.end(); ++it)
		{
			if (!(*it)->isCommand())
				continue;
			
			optimizeStackShift(it, code);
			optimizeCmd1(it, code);
			optimizeLabel(it, code);
			optimizeCmd2(it, code);
		}
	}
}

void CodeGen::addDD(string name, const string value, size_t size = 4)
{
	data.push_back(new AsmDD(name, value, size));
}

void CodeGen::addDB(string name, const string value)
{
	data.push_back(new AsmDB(name, value));
}

void CodeGen::addDQ(string name, const string value, size_t size = 8)
{
	data.push_back(new AsmDQ(name, value, size));
}

void CodeGen::addData(string name)
{
	data.push_back(new AsmData(name, string()));
}

void CodeGen::addFunc(string name)
{
	functions.push_back(new AsmFunction(name));
}

void CodeGen::addCommand(CommandT com, string l, string r)
{
	(*functions.rbegin())->addCode(new AsmCommand(com, l, r));
}

void CodeGen::addLocalVars(string l, string r)
{
	(*functions.rbegin())->addLocalVars(new AsmLocalVar(l, r));
}

void CodeGen::addCommand(CommandT com, string l, int size)
{
	switch(com)
	{
		case ASM_PUSH : shiftStack(-size);break;
		case ASM_POP : shiftStack(size);break;
	}
	(*functions.rbegin())->addCode(new AsmCommand(com, l));
}

void CodeGen::addLabel(string label)
{
	(*functions.rbegin())->addCode(new AsmLabel(label));
}

string CodeGen::genLabel()
{
	return (Prefix[PREFIX_LABEL] + to_string(lablesCount++));
}

void AsmLabel::print(ostream& s)
{
	s << endl << label  + ":\n\n";
}

void CodeGen::restoreStack()
{
	if (stacksLevel == 0)
		return;
	addCommand(ASM_ADD, getReg(REG_ESP), to_string(abs(stacksLevel)));
	stacksLevel = 0;
}

void CodeGen::addEoln(int count)
{
	(*functions.rbegin())->addCode(new Eoln(count));
}

void Eoln::print(ostream& s)
{
	for (int i = 0; i < count; i++)
		s << "\n";
}

void AsmFunction::addPrologue(int& shift)
{
	code.push_front(new Eoln(1));
	code.push_front(new AsmCommand(ASM_SUB, getReg(REG_ESP), to_string(shift)));
	code.push_front(new AsmCommand(ASM_MOV, getReg(REG_EBP), getReg(REG_ESP)));
	code.push_front(new AsmCommand(ASM_PUSH, getReg(REG_EBP)));
}

void CodeGen::genPrologue(int& shift)
{
	(*functions.rbegin())->addPrologue(shift);
}

void CodeGen::genEpilogue(int& shift)
{
	(*functions.rbegin())->addEpilogue(shift);
}

void AsmFunction::addEpilogue(int& shift)
{
	code.push_back(new AsmCommand(ASM_ADD, getReg(REG_ESP), to_string(shift)));
	code.push_back(new AsmCommand(ASM_MOV, getReg(REG_ESP), getReg(REG_EBP)));
	code.push_back(new AsmCommand(ASM_POP, getReg(REG_EBP)));
	code.push_back(new AsmCommand(ASM_RET, string()));
}

void CodeGen::pushJumps(string start, string end)
{
	jump.push(pair<string, string>(start, end));
}

void CodeGen::popJumps()
{
	jump.pop();
}

string CodeGen::getJumpBreak()
{
	return jump.top().second;
}

string CodeGen::getJumpContinue()
{
	return jump.top().first;
}

int CodeGen::getConstCount(TypeT type)
{
	switch(type)
	{
	case DOUBLE : return doubles.size();
	case STRING : return strings.size();
	case FLOAT  : return floats.size();
	default : return 0;
	}
}

void CodeGen::insertConst(TypeT type, string value)
{
#define PR(map) (to_string(map.size() + 1))
	switch(type)
	{
	case DOUBLE : doubles[value] = getPrefix(PR(doubles), PREFIX_DOUBLE_CONST);break;
	case STRING : strings[value] = getPrefix(PR(strings), PREFIX_STRING_CONST);break;
	case FLOAT  : floats[value] = getPrefix(PR(floats), PREFIX_FLOAT_CONST);
	}
#undef PR
}

bool CodeGen::hasConst(TypeT type, string value)
{
#define HAS(map, value) map.find(value) != map.end()

	switch(type)
	{
	case DOUBLE : return HAS(doubles, value);
	case STRING : return HAS(strings, value);
	case FLOAT  : return HAS(floats, value);
	default : return false;
	}

#undef HAS
}

string CodeGen::getConst(TypeT type, string value)
{
    switch(type)
	{
	case DOUBLE : return doubles[value];
	case STRING : return strings[value];
	case FLOAT  : return floats[value];
	}
}