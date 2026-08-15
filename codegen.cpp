#include "main.hpp"

#include <filesystem>

/*
	Convert TACO IR to assembly.

	for each function: I need to track:
	- how much stack is needed,
	- how large each register must be (i'll just do 32 bit for now)

	Defines a function for translating each instruction to assembly.

	also note that this is truly the last stage. There won't be any
	more processing after this. All optimizations shall be done
	in the previous stages.
*/

std::string binaryOpToAsm(BinaryOp op);
static void emit(std::string code);
static void emitni(std::string code); // no indent
static void emitProgramEnd();
static void emitProgramStart();
static void emitReturn(ReturnInstr* instr, int* offset);
static void emitBinaryOp(BinaryInstr* instr, int* offset);

std::ofstream output;

std::string codegen(std::string fileName, std::vector<Instruction*> ir)
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
	
	// kinda just hardcode main. Check sema.cpp
	emitni(fmt::format("main: "));
	emit("push rbp");
	emit("mov rbp, rsp\n");


	int offset = 0; // stack pointer offset

	for (Instruction* i : ir)
	{
		if (auto* instr = dynamic_cast<ReturnInstr*>(i)) {
			emitReturn(instr, &offset);
		}
		else if (auto* instr = dynamic_cast<BinaryInstr*>(i)) {
			emitBinaryOp(instr, &offset);
		}
	}
	
	emitProgramEnd();
	output.close();

	// assemble & link
	int resp = system("gcc out.s -o out");
	if (resp != 0) throw_error(resp, "Failure in gcc assembling");

	return outputFilename;
}

// pass one instruction at a time to it for readability
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
static std::unordered_map<int, int> vregMap;

static void emitReturn(ReturnInstr* instr, int* offset)
{
	emit("");
	if (instr->operand.kind == OperandKind::Immediate) {
		emit(fmt::format("mov eax, {}", instr->operand.val));
	}
	else if (instr->operand.kind == OperandKind::Temp) {
		emit(fmt::format("mov eax, [rbp {}]", vregMap.at(instr->operand.val)));
	}

	emit("mov rsp, rbp");
	emit("pop rbp");
	emit("ret");
}

// TODO: add a size field to Operand! easy solution.

static void emitBinaryOp(BinaryInstr* instr, int* offset)
{
	vregMap.insert({instr->dest.val, *offset -= 4});

	std::string op = binaryOpToAsm(instr->op);

	// these could be merged
	if (instr->left.kind == OperandKind::Immediate)
	{
		if (instr->right.kind == OperandKind::Immediate)
		{
			emit(fmt::format("mov DWORD PTR [rbp {}], {}", vregMap.at(instr->dest.val), instr->left.val));
			emit(fmt::format("{} DWORD PTR [rbp {}], {}", op, vregMap.at(instr->dest.val), instr->right.val));
		}
		else if (instr->right.kind == OperandKind::Temp)
		{
			emit(fmt::format("mov r10d, {}", instr->left.val));
			emit(fmt::format("{} r10d, [rbp {}]", op, vregMap.at(instr->right.val)));
			emit(fmt::format("mov DWORD PTR [rbp {}], r10d", vregMap.at(instr->dest.val)));
		}
	}
	else if (instr->left.kind == OperandKind::Temp)
	{
		if (instr->right.kind == OperandKind::Immediate)
		{
			emit(fmt::format("mov r10d, [rbp {}]", vregMap.at(instr->left.val)));
			emit(fmt::format("{} r10d, {}", op, instr->right.val));
			emit(fmt::format("mov DWORD PTR [rbp {}], r10d", vregMap.at(instr->dest.val)));
		}
		else if (instr->right.kind == OperandKind::Temp)
		{
			emit(fmt::format("mov r10d, [rbp {}]", vregMap.at(instr->left.val)));
			emit(fmt::format("{} r10d, [rbp {}]", op, vregMap.at(instr->right.val)));
			emit(fmt::format("mov DWORD PTR [rbp {}], r10d", vregMap.at(instr->dest.val)));
		}		
	}
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

