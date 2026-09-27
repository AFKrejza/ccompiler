#pragma once

#include <fmt/core.h>
#include <unordered_map>
#include <vector>

#include "error.hpp"
#include "operators.hpp"
#include "taco.hpp"
#include "type.hpp"
#include "utils.hpp"

class Parameter {
	public:
		Type *type;
		std::string identifier;

		Parameter(Type *type, std::string identifier) {
			this->type = type;
			this->identifier = identifier;
		}

		void print(int indent) {
			(void) indent;
			fmt::print("{} {}, ", type->typeName(), identifier);
		}
};

class Node {
	public:
		int line;
		Type *type = nullptr; // yeah it's not right but like man whatever

		Node(int line) : line(line) {}

		virtual void print(int indent) {
			printIndentLines(indent);
			fmt::print("{}\n", typeName());
		}

		virtual void printChildren(int indent) {
			(void) indent;
		}

		virtual void killChildren() {}

		virtual std::string typeName() {
			return "Node";
		}
		
		// TODO: add destructors to all classes
		virtual ~Node() = default;
};

// might be better in case i forget to set something? idk.
class VoidNode : public Node {
	public:
		VoidNode(int line) : Node(line) { }

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("{}\n", typeName());
		}
		void printChildren(int indent) {
			(void)indent;
		}
		void killChildren() override {
			// TODO: add kill self to all
		}
		std::string typeName() {
			return "VoidNode";
		}
};

// for symbol tables
struct Attrs {
	Type* type;
	int line; // declaration line
	int offset;

	Attrs(Type* type, int offset, int line) {
		this->type = type;
		this->offset = offset;
		this->line = line;
	}
};

class ScopeNode;
class FuncDefNode;
class GodNode;

class StatementNode : public Node {
	public:
		std::vector<Node*> body;
		ScopeNode* parent = nullptr;

		StatementNode(int line, ScopeNode* parent) : Node(line) {
			this->parent = parent;
		}

};

class ScopeNode : public StatementNode {
	public:
		// each locally scoped symbol has a name and Attributes
		std::unordered_map<std::string, Attrs> scope;

		ScopeNode(int line, ScopeNode* parent) : StatementNode(line, parent) {}

		// returns the symbol OR throws an error
		static Attrs getSymbol(std::string name, int line, ScopeNode* parent)
		{
			size_t count = parent->scope.count(name);

			if (count == 1) {
				return parent->scope.at(name);
			}
			else if (count > 1) {
				throw_error_line(1,
								 line,
								 fmt::format("Compiler error: {} was declared more "
											 "than once in a given scope. I messed up somewhere"));
			}
			else if (!parent->scope.count(name) && parent->parent == nullptr) {
				throw_error_line(1, line, fmt::format("Use of uninitialized variable {}", name));
			}

			return parent->parent->getSymbol(name, line, parent->parent);
		}

		virtual void print(int indent) {
			(void)indent;
			throw_error(1, "Never should have come here (ScopeNode->print())");
		}
		virtual void printChildren(int indent) {
			(void)indent;
			throw_error(1, ":sobbingemoji: (ScopeNode->printChildren())");
		}
		virtual std::string typeName() {
			return "ScopeNode";
		}

		// for changing parent function
		int changeFrameSize(ScopeNode* parent, int size);
};

class GodNode : public ScopeNode {
	public:
		GodNode(int line, ScopeNode* parent = nullptr) : ScopeNode(line, parent) {}

		void printChildren(int indent) override {
			print(indent);
			for (Node *i : body) {
				i->printChildren(indent + 1);
			}
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("{}\n", typeName());
		}

		std::string typeName() override {
			return "GodNode";
		}
};

class FuncDefNode : public ScopeNode {
	public:
		std::string name;
		Type *returnType;
		std::vector<Parameter> paramList;
		int frameSize = 0;

		FuncDefNode(int line,
					ScopeNode* parent,
					std::string name,
					Type *returnType) : ScopeNode(line, parent) {
			this->name = name;
			this->returnType = returnType;
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("{} {} {} (", typeName(), returnType->typeName(), name);
			for (Parameter i : paramList) {
				i.print(indent + 1);
			}
			fmt::print(")\n");
		}

		void printChildren(int indent) override {
			print(indent);
			for (Node *i : body) {
				i->printChildren(indent + 1);
			}
		}

		void killChildren() override {
			for (Node *i : body) {
				i->killChildren();
			}
			delete this;
		}

		std::string typeName() override {
			return "FuncDefNode";
		}
};

inline int ScopeNode::changeFrameSize(ScopeNode* parent, int size)
{
	if (auto* func = dynamic_cast<FuncDefNode*>(parent))
	{
		return func->frameSize -= size;
	}
	else if (dynamic_cast<GodNode*>(parent)) {
		throw_error(1, "Global variables aren't supported yet");
	}
	return this->changeFrameSize(parent->parent, size);
}

BinaryOp TokenTypeToBinaryOp(TokenType op);
std::string binaryOpToStr(BinaryOp op);

class BinaryOpNode : public Node {
	public:
		Node *left;
		Node *right;
		BinaryOp op;

		BinaryOpNode(int line, TokenType op) : Node(line) {
			this->op = TokenTypeToBinaryOp(op);
		}

		void printChildren(int indent) override {
			print(indent);
			if (left) left->printChildren(indent + 1);
			if (right) right->printChildren(indent + 1);
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("{} {}\n", typeName(), binaryOpToStr(op));
		}

		std::string typeName() override {
			return "BinaryOpNode";
		}

		void killChildren() override {
			left->killChildren();
			right->killChildren();
			delete this;
		}
};

UnaryOp TokenTypeToUnaryOp(TokenType op);
std::string unaryOpToStr(UnaryOp op);

class UnaryOpNode : public Node {
	public:
		UnaryOp op;
		Node* expression;

		UnaryOpNode(int line, TokenType op, Node* expression) : Node(line) {
			this->op = TokenTypeToUnaryOp(op);
			this->expression = expression;
		}
		
		void printChildren(int indent) override {
			print(indent);
			expression->printChildren(indent + 1);
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("{} {}\n", typeName(), unaryOpToStr(op));
		}

		std::string typeName() override {
			return "UnaryOpNode";
		}

		void killChildren() override {
			expression->killChildren();
			delete this;
		}
};

class ImmediateNode : public Node {
	public:
		int value;

		ImmediateNode(int line, int value) : Node(line) {
			this->value = value;
		}

		std::string typeName() override {
			return "ImmediateNode";
		}

		void printChildren(int indent) {
			print(indent);
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("ImmediateNode {}\n", value);
		}
};

class VariableNode : public Node {
	public:
		std::string name;
		Type *type;

		VariableNode(int line, std::string name) : Node(line) {
			this->name = name;
		}

		std::string typeName() override {
			return "VariableNode";
		}

		void printChildren(int indent) {
			print(indent);
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("VariableNode {}\n", name);
		}
};

class ReturnNode : public StatementNode {
	public:
		Node *expression = nullptr;

		ReturnNode(int line, ScopeNode* parent) : StatementNode(line, parent) {}

		std::string typeName() override {
			return "ReturnNode";
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("{} \n", typeName());
		}

		void printChildren(int indent) override {
			print(indent);
			expression->printChildren(indent + 1);
		}

		void killChildren() override {
			delete expression;
			delete this;
		}

		FuncDefNode* findParentFunction(ScopeNode* parent)
		{
			if (parent == nullptr)
				return nullptr;
			else if (auto* funky = dynamic_cast<FuncDefNode*>(parent))
				return funky;

			return findParentFunction(parent->parent);
		}
};

class AssignmentNode : public Node {
	public:
		std::string name;

		Node* expression = nullptr;

		AssignmentNode(int line, std::string name, Node* expression = nullptr) : Node(line) {
			this->name = name;
			this->expression = expression;
		}

		std::string typeName() override {
			return "AssignmentNode";
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("{} {} \n", typeName(), name);
		}

		void printChildren(int indent) override {
			print(indent);
			expression->printChildren(indent + 1);
		}
};

class DeclarationNode : public Node {
	public:
		std::string name;
		Type* type;
		AssignmentNode* assignment = nullptr;

		DeclarationNode(int line,
						std::string name,
						Type* type,
						AssignmentNode* assignment = nullptr) : Node(line)
		{
			this->name = name;
			this->type = type;
			this->assignment = assignment;
		}

		std::string typeName() override {
			return "DeclarationNode";
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("{} {} \n", typeName(), name);
		}

		void printChildren(int indent) override {
			print(indent);
			if (assignment != nullptr)
				assignment->expression->printChildren(indent + 1);
		}
};

class ElseNode : public ScopeNode {
	public:		
		ElseNode(int line, ScopeNode* parent) : ScopeNode(line, parent) {}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("Else\n");
			printIndentLines(indent + 1);
			fmt::print(":>\n");
		}

		void printChildren(int indent) override {
			print(indent);
			for (Node* node : body)
				node->printChildren(indent + 2);
		}

		std::string typeName() override {
			return "ElseNode";
		}
};

class IfNode : public ScopeNode {
	public:
		Node* expression;
		ElseNode* elseBranch = NULL;

		IfNode(int line, ScopeNode* parent, Node* expression)
		:	ScopeNode(line, parent),
			expression(expression) {}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("If\n");
			expression->printChildren(indent + 1);
			printIndentLines(indent + 1);
			fmt::print(":>\n");
		}

		void printChildren(int indent) override {
			print(indent);
			for (Node* node : body)
				node->printChildren(indent + 2);
			if (elseBranch)
				elseBranch->printChildren(indent);
		}

		std::string typeName() override {
			return "IfNode";
		}
};

class LoopNode : public ScopeNode {
	public:
		Node* expression;
		Label* endLabel;
		Label* startLabel;

		LoopNode(int line, ScopeNode* parent, Node* expression, std::string loop)
		:	ScopeNode(line, parent),
			expression(expression) {
				startLabel = newLocalLabel(loop);
				endLabel = newLocalLabel(std::string{loop}.append("_end"));
			}

		void print(int indent) override {
			(void)indent;
			throw_error(1, "Tryna print() LoopNode");
		}

		void printChildren(int indent) override {
			(void)indent;
			throw_error(1, "Tryna printChildren() LoopNode");
		}

		static void printLoopChildren(LoopNode* loop, int indent) {
			loop->print(indent);
			for (Node* node : loop->body)
				node->printChildren(indent + 2);
		}

		std::string typeName() override {
			return "LoopNode";
		}
};

class WhileNode : public LoopNode {
	public:
		WhileNode(int line, ScopeNode* parent, Node* expression)
		:	LoopNode(line, parent, expression, "while") {

		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("While\n");
			expression->printChildren(indent + 1);
			printIndentLines(indent + 1);
			fmt::print(":>\n");
		}

		void printChildren(int indent) override {
			printLoopChildren(this, indent + 2);
		}

		std::string typeName() override {
			return "WhileNode";
		}
};

class DoWhileNode : public LoopNode {
	public:
		DoWhileNode(int line, ScopeNode* parent, Node* expression = nullptr)
		:	LoopNode(line, parent, expression, "while") {}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("DoWhile\n");
			expression->printChildren(indent + 1);
			printIndentLines(indent + 1);
			fmt::print(":>\n");
		}

		void printChildren(int indent) override {
			printLoopChildren(this, indent + 2);
		}

		std::string typeName() override {
			return "DoWhileNode";
		}
};

class ForNode : public LoopNode {
	public:
		Node* prologue;
		Node* epilogue;
		Label* epilogueLabel;

		ForNode(int line, 
				ScopeNode* parent, 
				Node* expression = nullptr, 
				Node* prologue = nullptr, 
				Node* epilogue = nullptr)
		:
			LoopNode(line, parent, expression, "for"),
			prologue(prologue), epilogue(epilogue)
		{
			epilogueLabel = newLocalLabel("for_epilogue");
		}

		void print(int indent) override {
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

		void printChildren(int indent) override {
			printLoopChildren(this, indent + 2);
		}

		std::string typeName() override {
			return "ForNode";
		}
};

class BreakNode : public StatementNode {
	public:
		BreakNode(int line, ScopeNode* parent) : StatementNode(line, parent) {}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("Break\n");
		}

		void printChildren(int indent) override {
			print(indent);
		}

		std::string typeName() override {
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

};

// basically a copy of BreakNode, should combine em
class ContinueNode : public StatementNode {
	public:
		ContinueNode(int line, ScopeNode* parent) : StatementNode(line, parent) {}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("Continue\n");
		}

		void printChildren(int indent) override {
			print(indent);
		}

		std::string typeName() override {
			return "ContinueNode";
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

};
