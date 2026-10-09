#include <cassert>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <vector>

#include "ast.hpp"
#include "error.hpp"
#include "operators.hpp"
#include "taco.hpp"
#include "utils.hpp"

/*
	Convert TACO IR to assembly.

	Defines a function for translating each instruction to assembly.
*/

static std::string binaryOpToAsm(BinaryOp op);
static void emit(std::string instr);
static void emitni(std::string instr); // no indent
static void emitProgramEnd();
static void emitProgramStart();
static std::string operandText(const Operand& operand);

std::ofstream output;

std::string codegen(std::string fileName, std::vector<Instruction*> ir, GodNode* ast)
{
	// TODO: have it use the user-defined output name
	(void)ast;
	(void) fileName;
	int call = 0;

	if (std::filesystem::exists("out.s"))
		call = system("rm out.s");
	if (call != 0) throw_error(call, "'rm out.s' didn't return 0");
	
	if (std::filesystem::exists("out"))
		call = system("rm out");
	if (call != 0) throw_error(call, "'rm out' didn't return 0");

	std::string outputFilename = "out.s";
	output.open(outputFilename);
	
	emitProgramStart();
	
	for (Instruction* i : ir)
	{
		i->code();
	}
	
	emitProgramEnd();
	output.close();

	fmt::print("\nAssembly generated in {}", outputFilename);

	// assemble & link
	int resp = system("gcc -g out.s");
	if (resp != 0) throw_error(resp, "Failure in gcc assembling");
	
	return outputFilename;
}

static void emit(std::string instr)
{
	output << "    " << instr << "\n";
}

static void emitni(std::string instr)
{
	output << instr << "\n";
}

// Could be nice to print variable names
// static void emitComment(std::string comment)
// {
// 	output << "#" << comment << "\n";
// }

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

void ReturnInstr::code()
{
	emit("");
	if (this->operand.kind == OperandKind::Immediate) {
		emit(fmt::format("mov eax, {}", this->operand.val));
	}
	else if (this->operand.kind == OperandKind::Temp) {
		emit(fmt::format("mov eax, [rbp {}]", this->operand.offset));
	}
	else if (this->operand.kind == OperandKind::Variable) {
		emit(fmt::format("mov eax, [rbp {}]", this->operand.offset));
	}
	else {
		throw_error(1, "emitReturn: missing rule");
	}

	emit("mov rsp, rbp");
	emit("pop rbp");
	emit("ret");
}

static std::string operandText(const Operand& operand)
{
	switch(operand.kind)
	{
		case OperandKind::Immediate:
			return fmt::format("{}", operand.val);
		case OperandKind::Temp:
		case OperandKind::Variable:
			return fmt::format("[rbp {}]", operand.offset);
		default:
			throw_error(1, fmt::format("operandText: invalid OperandKind in {}",
						operandToStr(operand)));
			return NULL;
	}
}

void BinaryInstr::code()
{
	std::string left = operandText(this->left);
	std::string right = operandText(this->right);
	std::string dest = operandText(this->dest);

	switch(this->op)
	{
		case BinaryOp::ADD:
		case BinaryOp::SUB:
		case BinaryOp::MUL:
			emit(fmt::format("mov r10d, {}", left));
			emit(fmt::format("{} r10d, {}", binaryOpToAsm(this->op), right));
			emit(fmt::format("mov {}, r10d", dest));
			break;
		case BinaryOp::LESS_THAN:
		case BinaryOp::GREATER_THAN:
		case BinaryOp::LESSER_OR_EQUAL:
		case BinaryOp::GREATER_OR_EQUAL:
		case BinaryOp::NOT_EQUAL:
		case BinaryOp::EQUAL_TO:
			emit(fmt::format("mov r10d, {}", left));
			emit(fmt::format("cmp r10d, {}", right));
			emit(fmt::format("{} r10b", binaryOpToAsm(this->op)));
			emit(fmt::format("movzx r10d, r10b"));
			emit(fmt::format("mov {}, r10d", dest));
			break;
		case BinaryOp::LOGICAL_AND:
		case BinaryOp::LOGICAL_OR:
			break;
		case BinaryOp::DIV:
		default:
			throw_error(1, fmt::format("emitBinaryInstr: No rule for {}",
									   binaryOpToStr(this->op)));
	}
}

static std::string binaryOpToAsm(BinaryOp op)
{
	switch (op)
	{
		case BinaryOp::ADD:
			return "add";
		case BinaryOp::SUB:
			return "sub";
		case BinaryOp::MUL:
			return "imul";
		case BinaryOp::LESS_THAN:
			return "setl";
		case BinaryOp::GREATER_THAN:
			return "setg";
		case BinaryOp::LESSER_OR_EQUAL:
			return "setle";
		case BinaryOp::GREATER_OR_EQUAL:
			return "setge";
		case BinaryOp::NOT_EQUAL:
			return "setne";
		case BinaryOp::EQUAL_TO:
			return "sete";
		case BinaryOp::DIV:
		case BinaryOp::LOGICAL_AND:
		case BinaryOp::LOGICAL_OR:
		default:
			throw_error(1, "Error in binaryOpToAsm: Missing op translation");
			exit(1);
	}
}

void AssignmentInstr::code()
{
	switch (this->src.kind)
	{
		case OperandKind::Immediate:
			emit(fmt::format("mov DWORD PTR [rbp {}], {}", this->dest.offset, this->src.val));
			break;

		case OperandKind::Temp:
			emit(fmt::format("mov r10d, [rbp {}]", this->src.offset));
			emit(fmt::format("mov DWORD PTR [rbp {}], r10d", this->dest.offset));
			break;
		
		case OperandKind::Variable:
			emit(fmt::format("mov r10d, [rbp {}]", this->src.offset));
			emit(fmt::format("mov DWORD PTR [rbp {}], r10d", this->dest.offset));
			break;
		
		default:
			throw_error(1, "what the helly");
	}
}

void JumpIfTrueInstr::code()
{
	emit(fmt::format("mov r10d, {}", operandText(this->operand)));
	emit(fmt::format("cmp r10d, 0"));
	emit(fmt::format("jnz {}", this->label->name));
}

void JumpIfFalseInstr::code()
{
	emit(fmt::format("mov r10d, {}", operandText(this->operand)));
	emit(fmt::format("cmp r10d, 0"));
	emit(fmt::format("je {}", this->label->name));
}

void Label::code()
{
	emit("");
	emit(this->name);
}

void JumpInstr::code()
{
	emit(fmt::format("jmp {}", this->label->name));
}

// The dest of a unary instruction should always be a temporary register.
void UnaryInstr::code()
{
	assert(this->dest.kind != OperandKind::Immediate);

	switch (this->op)
	{
		case UnaryOp::NEGATE:
			emit(fmt::format("mov r10d, {}", operandText(this->src)));
			emit("neg r10d");
			emit(fmt::format("mov {}, r10d", operandText(this->dest)));
			break;
		case UnaryOp::LOGICAL_NOT:
			emit(fmt::format("mov r10d, {}", operandText(this->src)));
			emit("cmp r10d, 0");
			emit("sete r10b");
			emit("movzx r10d, r10b");
			emit(fmt::format("mov {}, r10d", operandText(this->dest)));
			break;
		case UnaryOp:: PREINC:
			emit(fmt::format("inc DWORD PTR {}", operandText(this->src)));
			break;
		case UnaryOp:: PREDEC:
			emit(fmt::format("dec DWORD PTR {}", operandText(this->src)));
			break;
		case UnaryOp::POSTINC:
			emit(fmt::format("mov r10d, {}", operandText(this->src)));
			emit(fmt::format("mov {}, r10d", operandText(this->dest)));
			emit(fmt::format("inc DWORD PTR {}", operandText(this->src)));
			break;
		case UnaryOp::POSTDEC:
			emit(fmt::format("mov r10d, {}", operandText(this->src)));
			emit(fmt::format("mov {}, r10d", operandText(this->dest)));
			emit(fmt::format("dec DWORD PTR {}", operandText(this->src)));
			break;
		default:
			throw_error(1, "emitUnaryInstr: invalid Unary Instruction");
	}
	
}

void FuncPrologueInstr::code()
{
	emitni(this->name->def()->name);
	emit("push rbp");
	emit("mov rbp, rsp");
	emit(fmt::format("sub rsp, {}", this->frameSize));
}

void FuncEpilogueInstr::code()
{
	emit("");
}

void CallInstr::code()
{
	emit(fmt::format("call {}", this->name));
}

void SaveRet::code()
{
	emit(fmt::format("mov {}, eax", operandText(this->dest)));
}

static std::string getArgRegister(size_t index, int size)
{
	std::string reg;
	switch (size)
	{
		case 4:
			switch (index)
			{
				case 0:
					reg = "edi";
					break;
				case 1:
					reg = "esi";
					break;
				case 2:
					reg = "edx";
					break;
				case 3:
					reg = "ecx";
					break;
				case 4:
					reg = "r8d";
					break;
				case 5:
					reg = "r9d";
					break;
				default:
					throw_error(1, "Only six arguments are supported for now");
			}
			break;
		default:
			throw_error(1, "Invalid argument size");
	}
	
	return reg;
}

void LoadArg::code()
{
	emit(fmt::format("mov {}, {}", getArgRegister(this->index, this->src.size),
								   operandText(this->src)));
}

void SaveArg::code()
{
	emit(fmt::format("mov {}, {}", operandText(this->dest),
								   getArgRegister(this->index, this->dest.size)));
}
