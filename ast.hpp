#pragma once

#include <fmt/core.h>
#include <memory>
#include <unordered_map>
#include <vector>

#include "error.hpp"
#include "operators.hpp"
#include "taco.hpp"
#include "type.hpp"
#include "utils.hpp"

class Parameter {
	public:
		int line;
		std::string name;
		std::unique_ptr<Type> type;
		int offset; // to save to stack to prevent clobbering

		Parameter(int line, std::string name, std::unique_ptr<Type> type)
		:	line(line),
			name(name) {
				fmt::print("param created\n");
				this->type = std::move(type);
			}
		
		~Parameter() = default;

		Parameter(Parameter&&) = default; //  move 
		Parameter& operator=(Parameter&&) = default; // copy

		void print(int indent) const {
			(void) indent;
			fmt::print("{} {}", type->typeName(), name);
		}

		void printChildren(int indent) const {
			print(indent);
		}
};

class ScopeNode;

class Node {
	public:
		int line;
		std::unique_ptr<Type> type = nullptr; // yeah it's not right but like man whatever

		Node(int line) : line(line) {}

		virtual ~Node() = default;

		virtual void print(int indent) const {
			printIndentLines(indent);
			fmt::print("{}\n", typeName());
		}

		virtual void printChildren(int indent) const {
			(void) indent;
		}

		virtual std::string typeName() const {
			return "Node";
		}

		virtual void eval(ScopeNode* parent)
		{
			(void)parent;
			throw_error(1, "eval called on base Node");
		}
		virtual void gen(ScopeNode* parent) 
		{
			(void)parent;
			throw_error(1, "gen called on base Node");
		}
		
};

class VoidNode : public Node {
	public:
		VoidNode(int line) : Node(line) { }

		~VoidNode() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("{}\n", typeName());
		}

		void printChildren(int indent) const {
			(void)indent;
		}

		std::string typeName() const {
			return "VoidNode";
		}

		void gen(ScopeNode* parent) override { (void)parent; }
};

class FuncNode;

// for symbol tables
struct Attrs {
	Type* type; // points to its Node's type. Should be okay since it has the same lifetime.
	int offset;
	int line; // declaration line
	FuncNode* func = nullptr; // same reasoning as Type

	Attrs(Type* type, int offset, int line, FuncNode* func = nullptr)
	:	type(type),
		offset(offset),
		line(line),
		func(func) {}
};

class GodNode;

class StatementNode : public Node {
	public:
		std::vector<std::unique_ptr<Node>> body;
		ScopeNode* parent = nullptr;

		StatementNode(int line, ScopeNode* parent) : Node(line), parent(parent) {}

		~StatementNode() = default;
};

class ScopeNode : public StatementNode {
	public:
		// each locally scoped symbol has a name and Attributes
		std::unordered_map<std::string, Attrs> scope;

		ScopeNode(int line, ScopeNode* parent) : StatementNode(line, parent) {}

		// returns the symbol OR throws an error with line argument
		static Attrs getSymbol(std::string name, int line, ScopeNode* scope)
		{
			size_t count = scope->scope.count(name);

			if (count == 1) {
				return scope->scope.at(name);
			}
			else if (count > 1) {
				throw_error_line(1, line, fmt::format("Compiler error: {} was declared more "
									"than once in a given scope. I messed up somewhere", name));
			}
			else if (!scope->scope.count(name) && scope->parent == nullptr) {
				throw_error_line(1, line, fmt::format("Use of uninitialized variable {}", name));
			}

			return scope->parent->getSymbol(name, line, scope->parent);
		}

		virtual void print(int indent) {
			(void)indent;
			throw_error(1, "Never should have come here (ScopeNode->print())");
		}
		virtual void printChildren(int indent) const {
			(void)indent;
			throw_error(1, ":sobbingemoji: (ScopeNode->printChildren())");
		}
		virtual std::string typeName() const {
			return "ScopeNode";
		}

		// for changing parent function
		virtual int changeFrameSize(int size);
};

class GodNode : public ScopeNode {
	public:
		GodNode(int line = 0, ScopeNode* parent = nullptr) : ScopeNode(line, parent) {}

		~GodNode() = default;

		void printChildren(int indent) const override {
			print(indent);
			for (const std::unique_ptr<Node>& i : body) {
				i->printChildren(indent + 1);
			}
		}

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("{}\n", typeName());
		}

		std::string typeName() const override {
			return "GodNode";
		}

		int changeFrameSize(int size) override {
			(void)parent; (void)size;
			throw_error(1, fmt::format("Attempt to change frame size of {}", this->typeName()));
			exit(1);
		}
};

class FuncNode : public ScopeNode {
	public:
		std::string name;
		std::unique_ptr<Type> returnType;
		std::vector<Parameter> paramList;
		bool isDef;
		bool isExtern;
		std::unique_ptr<Type> type = nullptr;

		FuncNode(int line,
				 ScopeNode* parent,
				 std::string name,
				 std::unique_ptr<Type> returnType,
				 std::vector<Parameter> paramList,
				 bool isDef,
				 bool isExtern)
			: ScopeNode(line, parent),
			name(name),
			returnType(std::move(returnType)),
			paramList(std::move(paramList)),
			isDef(isDef),
			isExtern(isExtern)
			{
				this->type = std::make_unique<FuncType>();
			}

		~FuncNode() = default;
		FuncNode(FuncNode&&) = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("{} {} {} (", typeName(), returnType->typeName(), name);
			for (size_t i = 0; i < paramList.size(); ++i)
			{
				paramList[i].print(indent + 1);
				if (i + 2 <= paramList.size()) fmt::print(", ");
			}
			fmt::print(")\n");
		}

		void printChildren(int indent) const override {
			print(indent);
			for (const std::unique_ptr<Node>& i : body) {
				i->printChildren(indent + 1);
			}
		}

		std::string typeName() const override {
			return "FuncNode";
		}

		void gen(ScopeNode* parent) override;

		int getFrameSize() const { return this->frameSize; }

		int changeFrameSize(int size) override {
			if (size < 0)
				throw_error(1, fmt::format("Size cannot be negative. "
											  "In function starting on line {}", this->line));
			return -(this->frameSize += size);
		}

		void eval(ScopeNode* parent) override;

	private:
		int frameSize = 0;
};

inline int ScopeNode::changeFrameSize(int size)
{
	return this->parent->changeFrameSize(size);
}

BinaryOp TokenTypeToBinaryOp(TokenType op);
std::string binaryOpToStr(BinaryOp op);

class BinaryOpNode : public Node {
	public:
		std::unique_ptr<Node> left;
		std::unique_ptr<Node> right;
		BinaryOp op;

		BinaryOpNode(int line, TokenType op) : Node(line) {
			this->op = TokenTypeToBinaryOp(op);
		}

		~BinaryOpNode() = default;

		void printChildren(int indent) const override {
			print(indent);
			if (left) left->printChildren(indent + 1);
			if (right) right->printChildren(indent + 1);
		}

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("{} {}\n", typeName(), binaryOpToStr(op));
		}

		std::string typeName() const override {
			return "BinaryOpNode";
		}

		void eval(ScopeNode* parent) override;
		void gen(ScopeNode* parent) override;
};

UnaryOp TokenTypeToUnaryOp(TokenType op);
std::string unaryOpToStr(UnaryOp op);

class UnaryOpNode : public Node {
	public:
		UnaryOp op;
		std::unique_ptr<Node> expression;

		UnaryOpNode(int line, TokenType op, std::unique_ptr<Node> expression) :
			Node(line),
			op(TokenTypeToUnaryOp(op)),
			expression(std::move(expression)) {}

		~UnaryOpNode() = default;
		
		void printChildren(int indent) const override {
			print(indent);
			expression->printChildren(indent + 1);
		}

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("{} {}\n", this->typeName(), unaryOpToStr(op));
		}

		std::string typeName() const override {
			return "UnaryOpNode";
		}

		void eval(ScopeNode* parent) override;
		void gen(ScopeNode* parent) override;
};

class ImmediateNode : public Node {
	public:
		int value;

		ImmediateNode(int line, int value) : Node(line) {
			this->value = value;
		}

		ImmediateNode(ImmediateNode&&) = default;

		~ImmediateNode() = default;

		std::string typeName() const override {
			return "ImmediateNode";
		}

		void printChildren(int indent) const {
			print(indent);
		}

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("ImmediateNode {}\n", value);
		}

		void eval(ScopeNode* parent) override;
		void gen(ScopeNode* parent) override;
};

class VariableNode : public Node {
	public:
		std::string name;
		std::unique_ptr<Type> type;

		VariableNode(int line, std::string name) : Node(line) {
			this->name = name;
		}

		~VariableNode() = default;

		std::string typeName() const override {
			return "VariableNode";
		}

		void printChildren(int indent) const {
			print(indent);
		}

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("VariableNode {}\n", name);
		}

		void eval(ScopeNode* parent) override;
		void gen(ScopeNode* parent) override;
};

class ReturnNode : public StatementNode {
	public:
		std::unique_ptr<Node> expression = nullptr;

		ReturnNode(int line, ScopeNode* parent) : StatementNode(line, parent) {}

		~ReturnNode() = default;

		std::string typeName() const override {
			return "ReturnNode";
		}

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("{} \n", typeName());
		}

		void printChildren(int indent) const override {
			print(indent);
			expression->printChildren(indent + 1);
		}

		FuncNode* findParentFunction(ScopeNode* parent)
		{
			if (parent == nullptr)
				return nullptr;
			else if (auto* funky = dynamic_cast<FuncNode*>(parent))
				return funky;

			return findParentFunction(parent->parent);
		}

		void eval(ScopeNode* parent) override;
		void gen(ScopeNode* parent) override;
};

class AssignmentNode : public Node {
	public:
		std::string name;
		std::unique_ptr<Node> lhs = nullptr;
		std::unique_ptr<Node> expression = nullptr;

		AssignmentNode(int line)
		:	Node(line) {}

		~AssignmentNode() = default;

		std::string typeName() const override {
			return "AssignmentNode";
		}

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("{} {} \n", typeName(), name);
		}

		void printChildren(int indent) const override {
			print(indent);
			expression->printChildren(indent + 1);
		}

		void eval(ScopeNode* parent) override;
		void gen(ScopeNode* parent) override;
};

class DeclarationNode : public Node {
	public:
		std::string name;
		std::unique_ptr<Type> type;
		std::unique_ptr<Node> expression = nullptr;

		DeclarationNode(int line,
						std::string name,
						std::unique_ptr<Type> type,
						Node* expression = nullptr) : 
			Node(line), name(name),
			type(std::move(type)),
			expression(std::move(expression)) {}

		~DeclarationNode() = default;

		std::string typeName() const override {
			return "DeclarationNode";
		}

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("{} {} \n", typeName(), name);
		}

		void printChildren(int indent) const override {
			print(indent);
			if (expression != nullptr)
				expression->printChildren(indent + 1);
		}

		void eval(ScopeNode* parent) override;
		void gen(ScopeNode* parent) override;
};

class ElseNode : public ScopeNode {
	public:		
		ElseNode(int line, ScopeNode* parent) : ScopeNode(line, parent) {}

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("Else\n");
			printIndentLines(indent + 1);
			fmt::print(":>\n");
		}

		~ElseNode() = default;

		void printChildren(int indent) const override {
			print(indent);
			for (const std::unique_ptr<Node>& i : body) {
				i->printChildren(indent + 2);
			}
		}

		std::string typeName() const override {
			return "ElseNode";
		}

		void eval(ScopeNode* parent) override;
		void gen(ScopeNode* parent) override;
};

class IfNode : public ScopeNode {
	public:
		std::unique_ptr<Node> expression;
		std::unique_ptr<ElseNode> elseBranch = NULL;

		IfNode(int line, ScopeNode* parent, std::unique_ptr<Node> expression)
		:	ScopeNode(line, parent),
			expression(std::move(expression)) {}

		~IfNode() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("If\n");
			expression->printChildren(indent + 1);
			printIndentLines(indent + 1);
			fmt::print(":>\n");
		}

		void printChildren(int indent) const override {
			print(indent);
			for (const std::unique_ptr<Node>& i : body) {
				i->printChildren(indent + 2);
			}
			if (elseBranch)
				elseBranch->printChildren(indent);
		}

		std::string typeName() const override {
			return "IfNode";
		}

		void eval(ScopeNode* parent) override;
		void gen(ScopeNode* parent) override;
};

class LoopNode : public ScopeNode {
	public:
		std::unique_ptr<Node> expression;
		std::shared_ptr<Label> endLabel;
		std::shared_ptr<Label> startLabel;

		LoopNode(int line, ScopeNode* parent, std::unique_ptr<Node> expression, std::string loop)
		:	ScopeNode(line, parent),
			expression(std::move(expression)) {
				startLabel = newLocalLabel(loop);
				endLabel = newLocalLabel(std::string{loop}.append("_end"));
			}

		void print(int indent) const override {
			(void)indent;
			throw_error(1, "Tryna print() LoopNode");
		}

		void printChildren(int indent) const override {
			(void)indent;
			throw_error(1, "Tryna printChildren() LoopNode");
		}

		static void printLoopChildren(const LoopNode* loop, int indent) {
			loop->print(indent);
			for (const std::unique_ptr<Node>& i : loop->body) {
				i->printChildren(indent + 2);
			}
		}

		std::string typeName() const override {
			return "LoopNode";
		}
};

class WhileNode : public LoopNode {
	public:
		WhileNode(int line, ScopeNode* parent, std::unique_ptr<Node> expression)
		:	LoopNode(line, parent, std::move(expression), "while") {}

		~WhileNode() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("While\n");
			expression->printChildren(indent + 1);
			printIndentLines(indent + 1);
			fmt::print(":>\n");
		}

		void printChildren(int indent) const override {
			printLoopChildren(this, indent + 2);
		}

		std::string typeName() const override {
			return "WhileNode";
		}

		void eval(ScopeNode* parent) override;
		void gen(ScopeNode* parent) override;
};

class DoWhileNode : public LoopNode {
	public:
		DoWhileNode(int line, ScopeNode* parent, std::unique_ptr<Node> expression = nullptr)
		:	LoopNode(line, parent, std::move(expression), "while") {}

		~DoWhileNode() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("DoWhile\n");
			expression->printChildren(indent + 1);
			printIndentLines(indent + 1);
			fmt::print(":>\n");
		}

		void printChildren(int indent) const override {
			printLoopChildren(this, indent + 2);
		}

		std::string typeName() const override {
			return "DoWhileNode";
		}

		void eval(ScopeNode* parent) override;
		void gen(ScopeNode* parent) override;
};

class ForNode : public LoopNode {
	public:
		std::shared_ptr<Node> prologue;
		std::shared_ptr<Node> epilogue;
		std::shared_ptr<Label> epilogueLabel;

		ForNode(int line, 
				ScopeNode* parent, 
				std::unique_ptr<Node> expression,
				std::unique_ptr<Node> prologue, 
				std::unique_ptr<Node> epilogue)
		:
			LoopNode(line, parent, std::move(expression), "for"),
			prologue(std::move(prologue)), epilogue(std::move(epilogue))
		{
			epilogueLabel = newLocalLabel("for_epilogue");
		}

		~ForNode() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("For\n");
			printIndentLines(indent);
			fmt::print("Prologue:\n");
			prologue->printChildren(indent + 1);
			printIndentLines(indent);
			fmt::print("Expr:\n");
			expression->printChildren(indent + 1);
			printIndentLines(indent + 1);
			fmt::print(":>\n");
		}

		void printChildren(int indent) const override {
			printLoopChildren(this, indent + 2);
		}

		std::string typeName() const override {
			return "ForNode";
		}

		void eval(ScopeNode* parent) override;
		void gen(ScopeNode* parent) override;
};

class BreakNode : public StatementNode {
	public:
		BreakNode(int line, ScopeNode* parent) : StatementNode(line, parent) {}

		~BreakNode() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("Break\n");
		}

		void printChildren(int indent) const override {
			print(indent);
		}

		std::string typeName() const override {
			return "BreakNode";
		}
		
		LoopNode* findParentLoop(ScopeNode* parent)
		{
			if (parent == nullptr)
				return nullptr;
			else if (auto* loopy = dynamic_cast<LoopNode*>(parent))
				return loopy;
			else
				return this->findParentLoop(parent->parent);
		}

		void eval(ScopeNode* parent) override;
		void gen(ScopeNode* parent) override;
};

// basically a copy of BreakNode, should combine em
class ContinueNode : public StatementNode {
	public:
		ContinueNode(int line, ScopeNode* parent) : StatementNode(line, parent) {}

		~ContinueNode() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("Continue\n");
		}

		void printChildren(int indent) const override {
			print(indent);
		}

		std::string typeName() const override {
			return "ContinueNode";
		}
		
		LoopNode* findParentLoop(ScopeNode* parent) const {
			if (parent == nullptr)
				return nullptr;
			else if (auto* loopy = dynamic_cast<LoopNode*>(parent))
				return loopy;
			else
				return this->findParentLoop(parent->parent);
		}

		void eval(ScopeNode* parent) override;
		void gen(ScopeNode* parent) override;
};

class CallNode : public StatementNode {
	public:
		std::string name;
		std::vector<std::unique_ptr<Node>> args;

		CallNode(int line, ScopeNode* parent, std::string name)
		:	StatementNode(line, parent),
			name(name) {}

		~CallNode() = default;

		void print(int indent) const override {
			printIndentLines(indent);
			fmt::print("Call {}\n", name);
		}

		void printChildren(int indent) const override {
			print(indent);
		}

		std::string typeName() const override {
			return "CallNode";
		}

		void eval(ScopeNode* parent) override;
		void gen(ScopeNode* parent) override;
};
