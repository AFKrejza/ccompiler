#pragma once

#include <fmt/core.h>

#include "utils.hpp"

class Instruction {
	public:
		virtual ~Instruction() = default;

		virtual void print(int indent) {
			(void) indent;
		}

		virtual std::string typeName() {
			return "Instruction";
		}
};

enum class OperandKind {
	Temp,
	Immediate,
	Variable
};

// Created in TACO and used in codegen
struct Operand {
	OperandKind kind;
	// TODO: perhaps set a limit of i64 for any one value. Add a validation phase somewhere
	// for individual values. Overflow should be allowed.
	int val;
	int offset = 0;
	std::string name{};
	int size;

	// vReg number and stack offset
	static Operand Temp(int vReg, int offset) {
		return { OperandKind::Temp, vReg, offset, "", 4 };
	}
	// just the immediate value.
	// TODO:I should add a check to ensure that it's within 64 bit
	// (change that to a long everywhere... or an unsigned long int??)
	static Operand Immediate(int value) {
		return { OperandKind::Immediate, value, 0, "", 4 };
	}

	static Operand Variable(std::string name, int offset, int size) {
		return { OperandKind::Variable, 0, offset, name, size };
	}
};

std::string unaryOpToStr(UnaryOp op);
std::string binaryOpToStr(BinaryOp op);
std::string operandToStr(Operand operand);

// TODO: need a derived class for lvalues ?

class BinaryInstr : public Instruction {
	public:
		Operand dest; // needs to be an lvalue
		BinaryOp op;
		Operand left;
		Operand right;

		BinaryInstr(Operand dest, BinaryOp op, Operand left, Operand right) {
			this->dest = dest;
			this->op = op;
			this->left = left;
			this->right = right;
		}
		
		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("BinaryInstr {} = {} {} {}\n", operandToStr(dest),
													  operandToStr(left),
													  binaryOpToStr(op),
													  operandToStr(right));
		}
		
		std::string typeName() override {
			return "BinaryInstr";
		}
};

class UnaryInstr : public Instruction {
	public:
		Operand dest;
		Operand src;
		UnaryOp op;

		UnaryInstr(Operand dest, Operand src, UnaryOp op) {
			this->dest = dest;
			this->src = src;
			this->op = op;
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("UnaryInstr {} {}\n", unaryOpToStr(op), operandToStr(dest));
		}
};


class ReturnInstr : public Instruction {
	public:
		Operand operand;

		ReturnInstr(Operand operand) {
			this->operand = operand;
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("Return {}\n", operandToStr(operand));
		}

		std::string typeName() override {
			return "ReturnInstr";
		}
};

class AssignmentInstr : public Instruction {
	public:
		Operand dest;
		Operand src;

		AssignmentInstr(Operand dest, Operand src) {
			this->dest = dest;
			this->src = src;
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("Assign {} = {}\n", operandToStr(dest), operandToStr(src));
		}

		std::string typeName() override {
			return "AssignmentInstr";
		}
};

class Label : public Instruction {
	public:
		std::string name;

		Label(std::string name) {
			this->name = name;
		}

		Label* def() {
			return new Label(std::string{this->name}.append(":"));
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("{}\n", name);
		}

		std::string typeName() override {
			return fmt::format("Label");
		}
};

class JumpInstr : public Instruction {
	public:
		Label* label;

		JumpInstr(Label* label) : label(label) {}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("JumpInstr {}\n", label->name);
		}
		
		std::string typeName() override {
			return "JumpInstr";
		}
};

class JumpIfTrueInstr : public JumpInstr {
	public:
		Operand operand;

		JumpIfTrueInstr(Label* label, Operand operand)
		:	JumpInstr(label),
			operand(operand) {}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("JumpIfTrue {} {}\n", operandToStr(operand), label->name);
		}

		std::string typeName() override {
			return "JumpIfTrueInstr";
		}
};

class JumpIfFalseInstr : public JumpInstr {
	public:
		Operand operand;

		JumpIfFalseInstr(Label* label, Operand operand)
		:	JumpInstr(label),
			operand(operand) {}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("JumpIfFalse {} {}\n", operandToStr(operand), label->name);
		}

		std::string typeName() override {
			return "JumpIfFalseInstr";
		}
};

Label* newLocalLabel(std::string text);

class FuncPrologueInstr : public Instruction {
	public:
		Label* name;
		int frameSize;

		FuncPrologueInstr(std::string name) {
			this->name = new Label(name);
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("FuncPrologueInstr {}\n", name->name);
		}

		std::string typeName() override {
			return "FuncPrologueInstr";
		}
};

class FuncEpilogueInstr : public Instruction {
	public:
		std::string name;

		FuncEpilogueInstr(std::string name) : name(name) {}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("FuncEpilogueInstr {}\n", name);
		}

		std::string typeName() override {
			return "FuncEpilogueInstr";
		}
};

class CallInstr : public Instruction {
	public:
		std::string name;

		CallInstr(std::string name) : name(name) {}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("CallInstr {}\n", name);
		}

		std::string typeName() override {
			return "CallInstr";
		}
};

class LoadArg : public Instruction {
	public:
		size_t index;
		Operand src;

		LoadArg(size_t index, Operand src) : index(index), src(src) {}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("LoadArg {} = {}\n", index, operandToStr(src));
		}

		std::string typeName() override {
			return "LoadArg";
		}
};

class SaveRet : public Instruction {
	public:
		Operand dest;
		
		SaveRet(Operand dest) : dest(dest) {}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("Save eax to {}\n", dest.offset);
		}

		std::string typeName() override {
			return "SaveRet";
		}
};

class SaveArg : public Instruction {
	public:
		size_t index;
		Operand dest;

		SaveArg(size_t index, Operand dest) : index(index), dest(dest) {}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("SaveArg {} = arg {}\n", operandToStr(dest), index);
		}

		std::string typeName() override {
			return "SaveArg";
		}
};
