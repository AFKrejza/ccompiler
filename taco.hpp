#pragma once

#include <optional>

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
	Variable,
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
	bool exists = false;

	// vReg number and stack offset
	static Operand Temp(int vReg, int offset) {
		return { OperandKind::Temp, vReg, offset, "", 4, true };
	}
	// just the immediate value.
	// TODO:I should add a check to ensure that it's within 64 bit
	// (change that to a long everywhere... or an unsigned long int??)
	static Operand Immediate(int value) {
		return { OperandKind::Immediate, value, 0, "", 4, true };
	}

	static Operand Variable(std::string name, int offset, int size) {
		return { OperandKind::Variable, 0, offset, name, size, true };
	}

	static std::string toStr(Operand operand)
	{
		switch (operand.kind)
		{
			case OperandKind::Immediate:
				return fmt::format("Immediate({})", operand.val);
			case OperandKind::Temp:
				return fmt::format("Temp({}, {})", operand.val, operand.offset);
			case OperandKind::Variable:
				return fmt::format("Variable({}, {})", operand.name, operand.offset);
			default:
				fmt::print("\nOperand: {}\n", operand.name,
											  operand.offset,
											  operand.size,
											  operand.val);
				throw_error(1, "Operand has invalid kind. Hello??");
				exit(1);
		}
	}

	static std::string code(const Operand& operand);
};

std::string unaryOpToStr(UnaryOp op);
std::string binaryOpToStr(BinaryOp op);

class BinaryInstr : public Instruction {
	public:
		Operand dest;
		BinaryOp op;
		Operand left;
		Operand right;

		BinaryInstr(Operand dest, BinaryOp op, Operand left, Operand right)
		{
			if (dest.kind != OperandKind::Temp && dest.kind != OperandKind::Variable) {
				throw_error(1, fmt::format(
					"Destination Operand {} must be a Temp or a Variable, got {} instead",
					dest.name, Operand::toStr(dest)));
			}

			if (op == BinaryOp::LOGICAL_AND || op == BinaryOp::LOGICAL_OR) {
				throw_error(1, fmt::format("BinaryInstr cannot contain Logical AND/OR"));
			}

			this->dest = dest;
			this->op = op;
			this->left = left;
			this->right = right;
		}

		~BinaryInstr() = default;
		
		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("BinaryInstr {} = {} {} {}\n", Operand::toStr(dest),
													  Operand::toStr(left),
													  binaryOpToStr(op),
													  Operand::toStr(right));
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

		UnaryInstr(Operand dest, Operand src, UnaryOp op) : dest(dest), src(src), op(op) {}

		~UnaryInstr() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("UnaryInstr {} {}\n", unaryOpToStr(op), Operand::toStr(dest));
		}

		void code() override;
};

class ReturnInstr : public Instruction {
	public:
		Operand operand;

		ReturnInstr(Operand operand) : operand(operand) {}

		~ReturnInstr() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("Return {}\n", Operand::toStr(operand));
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

		AssignmentInstr(Operand dest, Operand src) : dest(dest), src(src) {}

		~AssignmentInstr() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("Assign {} = {}\n", Operand::toStr(dest), Operand::toStr(src));
		}

		std::string typeName() const override {
			return "AssignmentInstr";
		}
		void code() override;
};

class Label : public Instruction {
	public:
		std::string name;

		Label(std::string name) : name(name) {}

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

class JumpIfInstr : public JumpInstr {
	public:
		Operand operand;
		bool condition;

		JumpIfInstr(bool condition, std::shared_ptr<Label> label, Operand operand)
		:	JumpInstr(label),
			operand(operand),
			condition(condition) {}

		~JumpIfInstr() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("JumpIf{} {} {}\n", condition ? "True" : "False",
										   Operand::toStr(operand), label->name);
		}

		std::string typeName() const override {
			return "JumpIfInstr";
		}

		void code() override;
};

class CondJump : public Instruction {
	public:
		Operand left;
		Operand right;
		std::shared_ptr<Label> ifTrue;
		std::shared_ptr<Label> ifFalse;
		BinaryOp op; // can only be a relational operator

		CondJump(BinaryOp op,
				 Operand left,
				 Operand right, 
				 std::optional<std::shared_ptr<Label>> ifTrue,
				 std::optional<std::shared_ptr<Label>> ifFalse) 
			: left(left), right(right)
		{
			if (!isbinOpRelationalOperator(op)) {
				std::string str {};
				for (auto i : binOpRelationals) {
					str.append(fmt::format("{}, ", binaryOpToStr(i)));
				}
				str.erase(str.back()).erase(str.back());
				throw_error(1, fmt::format("CondJump: Expected a relational operator, "
					"got '{}'. Valid boolean operators: {}", binaryOpToStr(op), str));
			}
			
			this->op = op;
			this->left = left;
			this->right = right;
			// to prevent needless jump if it should execute the next instruction anyway
			if (ifTrue.has_value()) this->ifTrue = std::move(ifTrue.value());
				else this->ifTrue = nullptr;
			if (ifFalse.has_value()) this->ifFalse = std::move(ifFalse.value());
				else this->ifFalse = nullptr;
		}

		~CondJump() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("CondJump {}, {}, {}, T: {}, F: {}\n", binaryOpToStr(op),
															Operand::toStr(left),
															Operand::toStr(right),
															(ifTrue ? ifTrue->name : ""),
															ifFalse->name);
		}

		std::string typeName() const override {
			return "CondJump";
		}

		void code() override;

	private:
};

std::unique_ptr<Label> newLocalLabel(std::string text);

class FuncPrologueInstr : public Instruction {
	public:
		std::unique_ptr<Label> name;
		int frameSize;
		std::vector<Operand> args;

		FuncPrologueInstr(std::string name) : name(std::make_unique<Label>(name)) {}

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

class CallInstr : public Instruction {
	public:
		std::string name;
		bool saveReturn;
		Operand dest; // must be Temp or Variable
		std::vector<Operand> args;

		CallInstr(std::string name, bool saveRet, Operand dest) :
			name(name), saveReturn(saveRet), dest(dest) {}

		~CallInstr() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("CallInstr {}, argc({}), saveret: {}\n",
				name, args.size(), Operand::toStr(dest));
		}

		std::string typeName() const override {
			return "CallInstr";
		}
		void code() override;
};
