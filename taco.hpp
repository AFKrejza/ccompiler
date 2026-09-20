#pragma once

#include <fmt/core.h>

class Instruction {
	public:
		virtual ~Instruction() = default;

		virtual void print(int indent) {}

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
	std::string name;

	// vReg number and stack offset
	static Operand Temp(int vReg, int offset) {
		return { OperandKind::Temp, vReg, offset };
	}
	// just the immediate value.
	// TODO:I should add a check to ensure that it's within 64 bit
	// (change that to a long everywhere)
	static Operand Immediate(int value) {
		return { OperandKind::Immediate, value };
	}

	static Operand Variable(std::string name, int offset) {
		return { OperandKind::Variable, 0, offset, name };
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

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("Label {}\n", name);
		}

		std::string typeName() override {
			return "Label";
		}
};

// enum class JumpCond {
// 	EQUAL,
// 	ZERO,
// 	NONZERO,
// };

// i could do the codegen similarly to emitBinaryInstr

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
			fmt::print("JumpIfTrue {} {}\n", operand.name, label->name);
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
			fmt::print("JumpIfFalse {} {}\n", operand.name, label->name);
		}

		std::string typeName() override {
			return "JumpIfFalseInstr";
		}
};

class JumpIfInstr : public JumpInstr {
	public:
		BinaryOp condition; // SORT of a BinaryOp. But semantically different here.
		// should probably translate it to some other enum. Idk bro.
		Operand operand;
		Operand comparand;

		JumpIfInstr(BinaryOp condition, Operand operand, Operand comparand, Label* label)
		:	JumpInstr(label),
			condition(condition),
			operand(operand),
			comparand(comparand) {}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("JumpIf({}({}) {} {}({})) -> {} \n",
					   operand.name,
					   operand.val,
					   binaryOpToStr(condition),
					   comparand.name,
					   comparand.val,
					   label->name);
		}

		std::string typeName() override {
			return "JumpIfInstr";
		}
};
