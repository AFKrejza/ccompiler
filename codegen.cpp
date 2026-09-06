#include "main.hpp"

#include <filesystem>

/*
	Convert TACO IR to assembly.

	Defines a function for translating each instruction to assembly.
*/

std::string binaryOpToAsm(BinaryOp op);
static void emit(std::string code);
static void emitni(std::string code); // no indent
static void emitProgramEnd();
static void emitProgramStart();
static void emitReturn(ReturnInstr* instr);
static void emitBinaryInstr(BinaryInstr* instr);
static void emitAssignment(AssignmentInstr* instr);

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
	emit(fmt::format("sub rsp, {}", -1 * static_cast<FuncDefNode*>(ast->body[0])->frameSize));
	emit("");

	for (Instruction* i : ir)
	{
		if (auto* instr = dynamic_cast<ReturnInstr*>(i)) {
			emitReturn(instr);
		}
		else if (auto* instr = dynamic_cast<BinaryInstr*>(i)) {
			emitBinaryInstr(instr);
		}
		else if (auto* instr = dynamic_cast<AssignmentInstr*>(i)) {
			emitAssignment(instr);
		}
	}
	
	emitProgramEnd();
	output.close();

	fmt::print("Assembly generated in {}\n", outputFilename);

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

// contains each vreg and its offset
// static std::unordered_map<int, int> vregMap;

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

	emit("mov rsp, rbp");
	emit("pop rbp");
	emit("ret");
}

static void emitBinaryInstr(BinaryInstr* instr)
{
	std::string op = binaryOpToAsm(instr->op);

	if (instr->left.kind == OperandKind::Immediate)
	{
		if (instr->right.kind == OperandKind::Immediate)
		{
			if (instr->op == BinaryOp::ADD || instr->op == BinaryOp::SUB)
			{
				emit(fmt::format("mov DWORD PTR [rbp {}], {}",
					 instr->dest.offset,
					 instr->left.val));
				emit(fmt::format("{} DWORD PTR [rbp {}], {}",
					 op, 
					 instr->dest.offset, 
					 instr->right.val));
			}
			else if (instr->op == BinaryOp::MUL)
			{
				emit(fmt::format("mov r10d, {}", instr->right.val));
				emit(fmt::format("imul r10d, {}", instr->left.val));
				emit(fmt::format("mov [rbp {}], r10d", instr->dest.offset));
			}
			else throw_error(1, "emitBinaryInstr fail");
		}
		else if (instr->right.kind == OperandKind::Temp ||
		     	 instr->right.kind == OperandKind::Variable)
		{
			if (instr->op == BinaryOp::ADD || instr->op == BinaryOp::SUB) {
				emit(fmt::format("mov r10d, {}", instr->left.val));
				emit(fmt::format("{} r10d, [rbp {}]", op, instr->right.offset));
				emit(fmt::format("mov DWORD PTR [rbp {}], r10d", instr->dest.offset));
			}
			else if (instr->op == BinaryOp::MUL) {
				emit(fmt::format("mov r10d, {}", instr->left.val));
				emit(fmt::format("imul r10d, [rbp {}]", instr->right.offset));
				emit(fmt::format("mov [rbp {}], r10d", instr->dest.offset));
			}
			else throw_error(1, "emitBinaryInstr fail");			
		}
			else throw_error(1, "emitBinaryInstr fail");
	}
	else if (instr->left.kind == OperandKind::Temp ||
				instr->left.kind == OperandKind::Variable)
	{
		if (instr->right.kind == OperandKind::Immediate)
		{
			if (instr->op == BinaryOp::ADD || instr->op == BinaryOp::SUB) {
				emit(fmt::format("mov r10d, [rbp {}]", instr->left.offset));
				emit(fmt::format("{} r10d, {}", op, instr->right.val));
				emit(fmt::format("mov [rbp {}], r10d", instr->dest.offset));
			}
			else if (instr->op == BinaryOp::MUL) {
				emit(fmt::format("mov r10d, [rbp {}]", instr->left.offset));
				emit(fmt::format("imul r10d, {}", instr->right.val));
				emit(fmt::format("mov [rbp {}], r10d", instr->dest.offset));
			}
			else throw_error(1, "emitBinaryInstr fail");
		}
		else if (instr->right.kind == OperandKind::Temp ||
					instr->right.kind == OperandKind::Variable)
		{
			if (instr->op == BinaryOp::ADD || instr->op == BinaryOp::SUB) {
				emit(fmt::format("mov r10d, [rbp {}]", instr->left.offset));
				emit(fmt::format("{} r10d, [rbp {}]", op, instr->right.offset));
				emit(fmt::format("mov DWORD PTR [rbp {}], r10d", instr->dest.offset));
			}
			else if (instr->op == BinaryOp::MUL) {
				emit(fmt::format("mov r10d, [rbp {}]", instr->left.offset));
				emit(fmt::format("imul r10d, [rbp {}]", instr->right.offset));
				emit(fmt::format("mov [rbp {}], r10d", instr->dest.offset));
			}
			else throw_error(1, "emitBinaryInstr fail");
		}
		else throw_error(1, "emitBinaryInstr fail");
	}
	else throw_error(1, "emitBinaryInstr fail");
}

std::string binaryOpToAsm(BinaryOp op)
{
	switch (op)
	{
		case BinaryOp::ADD:
			return "add";
		case BinaryOp::SUB:
			return "sub";
		case BinaryOp::MUL:
			return "imul";
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

