#pragma once

#include <fmt/core.h>

#include "utils.hpp"

class Instruction {
	public:
		virtual ~Instruction() = default;

		virtual void print(int indent) const  {
			(void) indent;
		}

		virtual std::string typeName() const {
			return "Instruction";
		}

		virtual void code() {
			throw_error(1, "emit called on base Instruction");
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

		~BinaryInstr() = default;
		
		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("BinaryInstr {} = {} {} {}\n", operandToStr(dest),
													  operandToStr(left),
													  binaryOpToStr(op),
													  operandToStr(right));
		}
		
		std::string typeName() const override {
			return "BinaryInstr";
		}
		
		void code() override;
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

		~UnaryInstr() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("UnaryInstr {} {}\n", unaryOpToStr(op), operandToStr(dest));
		}

		void code() override;
};


class ReturnInstr : public Instruction {
	public:
		Operand operand;

		ReturnInstr(Operand operand) {
			this->operand = operand;
		}

		~ReturnInstr() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("Return {}\n", operandToStr(operand));
		}

		std::string typeName() const override {
			return "ReturnInstr";
		}
		
		void code() override;
};

class AssignmentInstr : public Instruction {
	public:
		Operand dest;
		Operand src;

		AssignmentInstr(Operand dest, Operand src) {
			this->dest = dest;
			this->src = src;
		}

		~AssignmentInstr() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("Assign {} = {}\n", operandToStr(dest), operandToStr(src));
		}

		std::string typeName() const override {
			return "AssignmentInstr";
		}
		void code() override;
};

class Label : public Instruction {
	public:
		std::string name;

		Label(std::string name) {
			this->name = name;
		}

		~Label() = default;

		std::unique_ptr<Label> def() const {
			return std::make_unique<Label>(std::string{this->name}.append(":"));
		}

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("{}\n", name);
		}

		std::string typeName() const override {
			return fmt::format("Label");
		}

		void code() override;
};

class JumpInstr : public Instruction {
	public:
		std::shared_ptr<Label> label;

		JumpInstr(std::shared_ptr<Label> l) : label(std::move(l)) {}

		~JumpInstr() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("JumpInstr {}\n", label->name);
		}
		
		std::string typeName() const override {
			return "JumpInstr";
		}

		void code() override;
};

// TODO: combine into one JumpIfInstr
class JumpIfTrueInstr : public JumpInstr {
	public:
		Operand operand;

		JumpIfTrueInstr(std::shared_ptr<Label> label, Operand operand)
		:	JumpInstr(label),
			operand(operand) {}

		~JumpIfTrueInstr() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("JumpIfTrue {} {}\n", operandToStr(operand), label->name);
		}

		std::string typeName() const override {
			return "JumpIfTrueInstr";
		}

		void code() override;
};

class JumpIfFalseInstr : public JumpInstr {
	public:
		Operand operand;

		JumpIfFalseInstr(std::shared_ptr<Label> label, Operand operand)
		:	JumpInstr(label),
			operand(operand) {}

		~JumpIfFalseInstr() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("JumpIfFalse {} {}\n", operandToStr(operand), label->name);
		}

		std::string typeName() const override {
			return "JumpIfFalseInstr";
		}

		void code() override;
};

std::unique_ptr<Label> newLocalLabel(std::string text);

class FuncPrologueInstr : public Instruction {
	public:
		std::unique_ptr<Label> name;
		int frameSize;

		FuncPrologueInstr(std::string name) {
			this->name = std::make_unique<Label>(name);
		}

		~FuncPrologueInstr() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("FuncPrologueInstr {}\n", name->name);
		}

		std::string typeName() const override {
			return "FuncPrologueInstr";
		}
		void code() override;
};

class FuncEpilogueInstr : public Instruction {
	public:
		std::string name;

		FuncEpilogueInstr(std::string name) : name(name) {}

		~FuncEpilogueInstr() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("FuncEpilogueInstr {}\n", name);
		}

		std::string typeName() const override {
			return "FuncEpilogueInstr";
		}
		void code() override;
};

class CallInstr : public Instruction {
	public:
		std::string name;

		CallInstr(std::string name) : name(name) {}

		~CallInstr() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("CallInstr {}\n", name);
		}

		std::string typeName() const override {
			return "CallInstr";
		}
		void code() override;
};

class LoadArg : public Instruction {
	public:
		size_t index;
		Operand src;

		LoadArg(size_t index, Operand src) : index(index), src(src) {}

		~LoadArg() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("LoadArg {} = {}\n", index, operandToStr(src));
		}

		std::string typeName() const override {
			return "LoadArg";
		}

		void code() override;
};

class SaveRet : public Instruction {
	public:
		Operand dest;
		
		SaveRet(Operand dest) : dest(dest) {}

		~SaveRet() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("Save eax to {}\n", dest.offset);
		}

		std::string typeName() const override {
			return "SaveRet";
		}
		void code() override;
};

// basically identical to LoadArg
class SaveArg : public Instruction {
	public:
		size_t index;
		Operand dest;

		SaveArg(size_t index, Operand dest) : index(index), dest(dest) {}

		~SaveArg() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("SaveArg {} = arg {}\n", operandToStr(dest), index);
		}

		std::string typeName() const override {
			return "SaveArg";
		}

		void code() override;
};
