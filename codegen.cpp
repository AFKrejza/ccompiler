#include "main.hpp"

#include <filesystem>

/*
	nasm -felf64 output.asm && ld output.o for non-gcc linking

	converting AST nodes to assembly. Each one will have a specific rule.

	Rules:

	program = Program(functionDefinition)
	functionDefinition = Function(Identifier name, Instruction* instructions)
	instruction = Mov(operand dst, operand src) | Ret
	operand = Imm(int) | Register


*/

static void emit(std::string code);
static void emitni(std::string code); // no indent
static void emitProgramEnd();
static void emitProgramStart();
static void emitProgram(ProgramNode *node);
static void emitFunction(FuncDefNode* node);
static void emitReturn(ReturnNode *node);

std::ofstream output;

void codegen(std::string fileName, ProgramNode *program)
{
	fmt::print("codegen\n");

	// TODO: have it use the user-defined output name
	(void) fileName;
	if (std::filesystem::exists("out.s"))
		system("rm out.s");
	if (std::filesystem::exists("out"))
		system("rm out");

	std::string outputFilename = "out.s";
	output.open(outputFilename);
	
	emitProgramStart();

	// only supports the main function
	FuncDefNode *main = dynamic_cast<FuncDefNode*>(program->children[0]);
	assert(main->typeName() == "FuncDefNode" && main->getLexeme() == "main");
	emitFunction(main);
	
	emitProgramEnd();
	output.close();

	// std::string result = readFile(outputFilename);
	// std::cout << result;

	// assemble & link
	int resp = system("gcc out.s -o out");
	if (resp != 0) throw_error(resp, "Failure in gcc assembling");
}

static void emit(std::string code)
{
	output << "    " << code << "\n";
}

static void emitni(std::string code)
{
	output << code << "\n";
}

static void emitProgramStart()
{
	std::string str = ".intel_syntax noprefix\n.global main\n\n.text";
	emitni(str);
}

static void emitProgramEnd()
{
	std::string str = "\n.section .note.GNU-stack,\"\",@progbits";
	emitni(str);
}

// could be cleaner
static void emitProgram(ProgramNode *node)
{

}

//
static void emitFunction(FuncDefNode *node)
{
	emitni(fmt::format("{}: ", node->name));

	// prologue
	emit("push rbp");
	emit("mov rbp, rsp");

	ReturnNode *ret = dynamic_cast<ReturnNode*>(node->body[0]);
	assert(ret->typeName() == "ReturnNode");
	emitReturn(ret); 	

	// epilogue
	emit("mov rsp, rbp");
	emit("pop rbp");
	emit("ret");
}


static void emitReturn(ReturnNode *node)
{
	assert(node->expression->getType() == INTEGER);

	IntegerNode *intNode = dynamic_cast<IntegerNode*>(node->expression);

	int val = intNode->value;

	emit(fmt::format("mov eax, {}", val));
}

