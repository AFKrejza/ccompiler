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

static void emit(std::string instr);
static void emitni(std::string instr); // no indent
static void emitProgramEnd();
static void emitProgramStart();

std::ofstream output;

std::string codegen(std::string fileName,
					std::vector<std::unique_ptr<Instruction>> ir,
					GodNode* ast)
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
	
	for (const auto& i : ir)
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

	// optimization: remove, but only possible if FuncPrologueInstr does it as well.
	emit("mov rsp, rbp");
	emit("pop rbp");

	emit("ret");
}

std::string Operand::code(const Operand& operand)
{
	switch(operand.kind)
	{
		case OperandKind::Immediate:
			return fmt::format("{}", operand.val);
		case OperandKind::Temp:
		case OperandKind::Variable:
			return fmt::format("[rbp {}]", operand.offset);
		default:
			throw_error(1, fmt::format("Operand::code: invalid OperandKind in {}",
						Operand::toStr(operand)));
			return NULL;
	}
}

void BinaryInstr::code()
{
	std::string left = Operand::code(this->left);
	std::string right = Operand::code(this->right);
	std::string dest = Operand::code(this->dest);

	switch(this->op)
	{
		case BinOp::en::ADD:
		case BinOp::en::SUB:
		case BinOp::en::MUL:
			emit(fmt::format("mov r10d, {}", left));
			emit(fmt::format("{} r10d, {}", BinOp::toAsm(this->op), right));
			emit(fmt::format("mov {}, r10d", dest));
			break;
		case BinOp::en::LESS_THAN:
		case BinOp::en::GREATER_THAN:
		case BinOp::en::LESSER_OR_EQUAL:
		case BinOp::en::GREATER_OR_EQUAL:
		case BinOp::en::NOT_EQUAL:
		case BinOp::en::EQUAL_TO:
			emit(fmt::format("mov r10d, {}", left));
			emit(fmt::format("cmp r10d, {}", right));
			emit(fmt::format("{} r10b", BinOp::toAsm(this->op)));
			emit(fmt::format("movzx r10d, r10b"));
			emit(fmt::format("mov {}, r10d", dest));
			break;
		case BinOp::en::LOGICAL_AND:
		case BinOp::en::LOGICAL_OR:
			throw_error(1, "Logical AND/OR don't exist at the assembly level");
			break;
		case BinOp::en::DIV:
		default:
			throw_error(1, fmt::format("emitBinaryInstr: No rule for {}",
									   BinOp::toStr(this->op)));
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

void JumpIfInstr::code()
{
	emit(fmt::format("mov r10d, {}", Operand::code(this->operand)));
	emit(fmt::format("cmp r10d, 0"));

	std::string jmp = condition == true ? "jnz" : "je";

	emit(fmt::format("{} {}", jmp, this->label->name));
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
			throw_error(1, fmt::format("getArgRegister: no rule for arg of size {}", size));
	}
	
	return reg;
}

// The dest of a unary instruction must be a Temp or Variable
void UnaryInstr::code()
{
	assert(this->dest.kind != OperandKind::Immediate);

	switch (this->op)
	{
		case UnOp::en::NEGATE:
			emit(fmt::format("mov r10d, {}", Operand::code(this->src)));
			emit("neg r10d");
			emit(fmt::format("mov {}, r10d", Operand::code(this->dest)));
			break;
		case UnOp::en::LOGICAL_NOT:
			emit(fmt::format("mov r10d, {}", Operand::code(this->src)));
			emit("cmp r10d, 0");
			emit("sete r10b");
			emit("movzx r10d, r10b");
			emit(fmt::format("mov {}, r10d", Operand::code(this->dest)));
			break;
		case UnOp::en:: PREINC:
			emit(fmt::format("inc DWORD PTR {}", Operand::code(this->src)));
			break;
		case UnOp::en:: PREDEC:
			emit(fmt::format("dec DWORD PTR {}", Operand::code(this->src)));
			break;
		case UnOp::en::POSTINC:
			emit(fmt::format("mov r10d, {}", Operand::code(this->src)));
			emit(fmt::format("mov {}, r10d", Operand::code(this->dest)));
			emit(fmt::format("inc DWORD PTR {}", Operand::code(this->src)));
			break;
		case UnOp::en::POSTDEC:
			emit(fmt::format("mov r10d, {}", Operand::code(this->src)));
			emit(fmt::format("mov {}, r10d", Operand::code(this->dest)));
			emit(fmt::format("dec DWORD PTR {}", Operand::code(this->src)));
			break;
		default:
			throw_error(1, "emitUnaryInstr: invalid Unary Instruction");
	}
	
}

void FuncPrologueInstr::code()
{
	emitni(this->name->def()->name);

	// optimization: could remove stack frame creation if frameSize == 0. ReturnInstr sync
	emit("push rbp");
	emit("mov rbp, rsp");
	emit(fmt::format("sub rsp, {}\n", this->frameSize));

	size_t len = this->args.size();
	for (size_t i{}; i < len; ++i)
	{
		emit(fmt::format("mov {}, {}", Operand::code(this->args[i]),
									   getArgRegister(i, this->args[i].size)));
	}
}

void CallInstr::code()
{
	for (size_t i{}; i < this->args.size(); ++i)
	{
		emit(fmt::format("mov {}, {}", getArgRegister(i, this->args[i].size),
									   Operand::code(this->args[i])));
	}

	emit(fmt::format("call {}", this->name));

	if (this->saveReturn) {
		emit(fmt::format("mov {}, eax", Operand::code(this->dest)));
	}
}

void CondJump::code()
{
	// must contain a relational operator

	emit(fmt::format("mov r10d, {}", Operand::code(this->left)));
	emit(fmt::format("cmp r10d, {}", Operand::code(this->right)));

	emit(fmt::format("{} {}", BinOp::relationalToAsm(this->op, true), this->ifTrue->name));
	emit(fmt::format("{} {}", BinOp::relationalToAsm(this->op, false), this->ifFalse->name));
	
	return;
}
