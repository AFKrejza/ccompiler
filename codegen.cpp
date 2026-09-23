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
static void emitReturn(ReturnInstr* instr);
static void emitBinaryInstr(BinaryInstr* instr);
static void emitAssignment(AssignmentInstr* instr);
static std::string operandText(const Operand& operand);
static void emitJumpIfTrueInstr(JumpIfTrueInstr* instr);
static void emitJumpIfFalseInstr(JumpIfFalseInstr* instr);
static void emitLabel(Label* instr);
static void emitJump(JumpInstr* instr);
static void emitUnaryInstr(UnaryInstr* instr);

std::ofstream output;

std::string codegen(std::string fileName, std::vector<Instruction*> ir, GodNode* ast)
{
	// TODO: have it use the user-defined output name
	(void) fileName;
	if (std::filesystem::exists("out.s"))
		system("rm out.s");
	if (std::filesystem::exists("out"))
		system("rm out");

	std::string outputFilename = "out.s";
	output.open(outputFilename);
	
	emitProgramStart();
	
	// kinda just hardcode main. Check sema.cpp
	emitni(fmt::format("main: "));
	emit("push rbp");
	emit("mov rbp, rsp");
	emit(fmt::format("sub rsp, {}", abs(static_cast<FuncDefNode*>(ast->body[0])->frameSize)));
	emit("");

	for (Instruction* i : ir)
	{
		if (auto* instr = dynamic_cast<ReturnInstr*>(i)) {
			emitReturn(instr);
		}
		else if (auto* instr = dynamic_cast<BinaryInstr*>(i)) {
			emitBinaryInstr(instr);
		}
		else if (auto* instr = dynamic_cast<UnaryInstr*>(i)) {
			emitUnaryInstr(instr);
		}
		else if (auto* instr = dynamic_cast<AssignmentInstr*>(i)) {
			emitAssignment(instr);
		}
		else if (auto* instr = dynamic_cast<JumpIfTrueInstr*>(i)) {
			emitJumpIfTrueInstr(instr);
		}
		else if (auto* instr = dynamic_cast<JumpIfFalseInstr*>(i)) {
			emitJumpIfFalseInstr(instr);
		}
		else if (auto* instr = dynamic_cast<JumpInstr*>(i)) {
			emitJump(instr);
		}
		else if (auto* instr = dynamic_cast<Label*>(i)) {
			emitLabel(instr);
		}
		else {
			throw_error(1, fmt::format("No rule for instruction type {}", i->typeName()));
		}
	}
	
	emitProgramEnd();
	output.close();

	fmt::print("\nAssembly generated in {}", outputFilename);

	// assemble & link
	int resp = system("gcc out.s");
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

static void emitReturn(ReturnInstr* instr)
{
	emit("");
	if (instr->operand.kind == OperandKind::Immediate) {
		emit(fmt::format("mov eax, {}", instr->operand.val));
	}
	else if (instr->operand.kind == OperandKind::Temp) {
		emit(fmt::format("mov eax, [rbp {}]", instr->operand.offset));
	}
	else if (instr->operand.kind == OperandKind::Variable) {
		emit(fmt::format("mov eax, [rbp {}]", instr->operand.offset));
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

static void emitBinaryInstr(BinaryInstr* instr)
{
	std::string left = operandText(instr->left);
	std::string right = operandText(instr->right);
	std::string dest = operandText(instr->dest);

	switch(instr->op)
	{
		case BinaryOp::ADD:
		case BinaryOp::SUB:
		case BinaryOp::MUL:
			emit(fmt::format("mov r10d, {}", left));
			emit(fmt::format("{} r10d, {}", binaryOpToAsm(instr->op), right));
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
			emit(fmt::format("{} r10b", binaryOpToAsm(instr->op)));
			emit(fmt::format("movzx r10d, r10b"));
			emit(fmt::format("mov {}, r10d", dest));
			break;
		default:
			throw_error(1, fmt::format("emitBinaryInstr: No rule for {}",
									   binaryOpToStr(instr->op)));
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
		default:
			throw_error(1, "Error in binaryOpToAsm: Missing op translation");
			exit(1);
	}
}

static void emitAssignment(AssignmentInstr* instr)
{
	switch (instr->src.kind)
	{
		case OperandKind::Immediate:
			emit(fmt::format("mov DWORD PTR [rbp {}], {}", instr->dest.offset, instr->src.val));
			break;

		case OperandKind::Temp:
			emit(fmt::format("mov r10d, [rbp {}]", instr->src.offset));
			emit(fmt::format("mov DWORD PTR [rbp {}], r10d", instr->dest.offset));
			break;
		
		case OperandKind::Variable:
			emit(fmt::format("mov r10d, [rbp {}]", instr->src.offset));
			emit(fmt::format("mov DWORD PTR [rbp {}], r10d", instr->dest.offset));
			break;
		
		default:
			throw_error(1, "what the helly");
	}
}

static void emitJumpIfTrueInstr(JumpIfTrueInstr* instr)
{
	emit(fmt::format("mov r10d, {}", operandText(instr->operand)));
	emit(fmt::format("cmp r10d, 0"));
	emit(fmt::format("jnz {}", instr->label->name));
}

static void emitJumpIfFalseInstr(JumpIfFalseInstr* instr)
{
	emit(fmt::format("mov r10d, {}", operandText(instr->operand)));
	emit(fmt::format("cmp r10d, 0"));
	emit(fmt::format("je {}", instr->label->name));
}

static void emitLabel(Label* instr)
{
	emit("");
	emit(instr->name.append(":"));
}

static void emitJump(JumpInstr* instr)
{
	emit(fmt::format("jmp {}", instr->label->name));
}

// The dest of a unary instruction should always be a temporary register.
static void emitUnaryInstr(UnaryInstr* instr)
{
	assert(instr->dest.kind != OperandKind::Immediate);

	switch (instr->op)
	{
		case UnaryOp::NEGATE:
			emit(fmt::format("mov r10d, {}", operandText(instr->src)));
			emit("neg r10d");
			emit(fmt::format("mov {}, r10d", operandText(instr->dest)));
			break;
		case UnaryOp::LOGICAL_NOT:
			emit(fmt::format("mov r10d, {}", operandText(instr->src)));
			emit("cmp r10d, 0");
			emit("sete r10b");
			emit("movzx r10d, r10b");
			emit(fmt::format("mov {}, r10d", operandText(instr->dest)));
			break;
		default:
			throw_error(1, "emitUnaryInstr: invalid Unary Instruction");
	}
	
}
