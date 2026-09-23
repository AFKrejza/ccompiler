#pragma once

#include <fmt/core.h>
#include <unordered_map>
#include <vector>

#include "error.hpp"
#include "operators.hpp"
#include "type.hpp"
#include "utils.hpp"

class Node {
	public:
		Type *type = nullptr;
		int line;

		Node(int line) : line(line) {}

		virtual void print(int indent) {
			printIndentLines(indent);
			fmt::print("{}\n", typeName());
		}

		virtual void printChildren(int indent) {}

		virtual void killChildren() {}

		virtual std::string typeName() {
			return "Node";
		}
};

// symbol tables
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

class StatementNode : public Node {
	public:
		std::vector<Node*> body;
		// each locally scoped symbol has a name and Attributes
		std::unordered_map<std::string, Attrs> scope;
		StatementNode* parent = nullptr;

		StatementNode(int line) : Node(line) {}

		// used in semantic analysis to verify that the variable exists in scope
		bool findSymbolScope(std::string name)
		{
			if (this->scope.count(name) == 1) return true;
			else if (this->scope.count(name) == 0 && this->parent == nullptr) {
				throw_error_line(1,
								 this->line,
								 fmt::format("Variable '{}' is not defined", name));
			}
			else if (this->scope.count(name) > 1) {
				throw_error_line(1,
								 this->line,
								 fmt::format("Compiler error: {} was declared more "
											 "than once in a given scope!!!"));
			}
			else {
				throw_error_line(1,
								 this->line,
								 fmt::format("idk what to call this one but it's not good"));
			}
			return this->parent->findSymbolScope(name);
		}

		// used in taco, variable guaranteed to exist thanks to sema
		Attrs getSymbol(std::string name)
		{
			if (this->scope.count(name))
				return this->scope.at(name);

			return this->parent->getSymbol(name);
		}

};


class GodNode : public StatementNode {
	public:
		GodNode(int line) : StatementNode(line) {}

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


class Parameter {
	public:
		Type *type;
		std::string identifier;

		Parameter(Type *type, std::string identifier) {
			this->type = type;
			this->identifier = identifier;
		}

		void print(int indent) {
			fmt::print("{} {}, ", type->typeName(), identifier);
		}
};


class FuncDefNode : public StatementNode {
	public:
		std::string name;
		Type *returnType;
		std::vector<Parameter> paramList;
		int frameSize = 0;
		Node* parent;

		FuncDefNode(int line,
					Node* parent,
					std::string name,
					Type *returnType) : StatementNode(line) {
			this->name = name;
			this->parent = parent;
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


class ReturnNode : public Node {
	public:
		Node *expression = nullptr;

		ReturnNode(int line) : Node(line) {}

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


// class VoidNode : public Node {
// 	public:
// 		VoidNode() : Node() {}
// };

class ElseNode : public StatementNode {
	public:		
		ElseNode(int line) : StatementNode(line) {}

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


class IfNode : public StatementNode {
	public:
		Node* expression;
		ElseNode* elseBranch = NULL;

		IfNode(int line, Node* expression)
		:	StatementNode(line),
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
