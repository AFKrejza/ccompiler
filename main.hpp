#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <format>

#include <fmt/core.h>

// #include "type.hpp"
#include "scope.hpp"

void throw_error(int code, std::string msg);
void throw_error_line(int code, int line, std::string msg);
void throw_warn(int code, int line, std::string msg);
void throw_invalid_identifier(int line);
void throw_invalid_identifier_start(int line);
std::string readFile(std::string filename);
void printTokens();

// like a union
using Literal = std::variant<
	std::monostate,
	int,
	std::string,
	bool
>;

enum class BinaryOp {
	ADD,
	SUB,
	MUL,
	DIV
};

struct LiteralPrintVisitor {
	std::string operator()(std::monostate v) const {
		(void) v;
		return "empty";
	}
	std::string operator()(int v) const {
		return std::to_string(v);
	}
	std::string operator()(std::string v) const {
		return v;
	}
	std::string operator()(bool v) const {
		return v ? "True" : "False";
	}
};


enum TokenType {
	SEMICOLON, PLUS, MINUS, ASSIGNMENT, OPEN_PARENTHESES, CLOSED_PARENTHESES,
	OPEN_SQUARE_BRACKET, CLOSED_SQUARE_BRACKET, OPEN_CURLY_BRACE, CLOSED_CURLY_BRACE,
	COMMA, ASTERISK,

	GREATER_THAN, LESS_THAN, EQUAL_TO, GREATER_OR_EQUAL, LESSER_OR_EQUAL,
	NOT_EQUAL,

	LOGICAL_NOT, LOGICAL_AND, LOGICAL_OR,
	BITWISE_NOT, BITWISE_AND, BITWISE_OR,

	IDENTIFIER, CHAR, SHORT, INT, LONG, STRING_LITERAL, INTEGER,

	IF, ELSE, 

	RETURN,

	END_OF_FILE,
};


class Token {
	public:
		TokenType tokenType;
		std::string lexeme; // exact source text
		Literal literal;
		int line;
		int column; // TODO: broken
		int length; // TODO: broken

		Token(TokenType tokenType, int length, std::string lexeme, Literal literal, int line, int column) {
			this->tokenType = tokenType;
			this->lexeme = lexeme;
			this->literal = literal;
			this->line = line;
			this->column = column;
			this->length = length;
		}

		// only for 0-initialization
		Token() {
			this->tokenType = END_OF_FILE;
			this->lexeme = "";
			this->line = 0;
			this->column = 0;
			this->length = 0;
		}

		void print()
		{
			fmt::print("   TokenType: {} \n", token_type_to_string(tokenType));
			fmt::print(" Lexeme: {}\n", lexeme);
			fmt::print("Literal: {}\n", std::visit(LiteralPrintVisitor{}, literal));
			fmt::print("   Line: {}\n", line);
			fmt::print(" Column: {}\n", column);
			fmt::print(" Length: {}\n", length);
		}
		
		// contains all symbols and keywords
		static const std::unordered_map<TokenType, std::string> printmap;

		static std::string token_type_to_string(TokenType type)
		{
			int int_type = (int)type;

			auto it = printmap.find(type);
			if (it != printmap.end()) {
				return it->second;
			}
			throw_error(3, "Couldn't find token to be printed. ???");
			return "";
		}
};


static void printIndentLines(int indent)
{
	std::string indentation{};
	std::string bar = "|";
	for (int i = 0; i < indent - 1; i++) {
		indentation.append(bar)
					.append("   ");
	}
	indentation.append(bar)
				.append("--");
	std::cout << indentation;
}


class Node {
	public:
		Type *type = nullptr; // TODO: consider creating a statement node without a type
		int line;

		Node(int line) : line(line) {} // TODO: use initializer list syntax in other places

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


class ProgramNode : public Node {
	public:
		std::vector<Node*> children;
		// each locally scoped symbol has a name and Attributes
		std::unordered_map<std::string, Attrs> scope;

		ProgramNode(int line) : Node(line) {}

		void printChildren(int indent) override {
			print(indent);
			for (Node *i : children) {
				i->printChildren(indent + 1);
			}
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("{}\n", typeName());
		}

		std::string typeName() override {
			return "ProgramNode";
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


class FuncDefNode : public Node {
	public:
		std::string name;
		Type *returnType;
		std::vector<Parameter> paramList;
		int frameSize = 0;
		Node* parent;

		// each locally scoped symbol has a name and Attributes
		std::unordered_map<std::string, Attrs> scope;

		// must contain at least one return for each control path if returnType isn't void.
		// should only contain statements?
		std::vector<Node*> body;

		FuncDefNode(int line, Node* parent, std::string name, Type *returnType) : Node(line) {
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

class DeclarationNode : public Node {
	public:
		std::string name;
		Type* type;
		Node* expression = nullptr;

		DeclarationNode(int line, std::string name, Type* type, Node* expression = nullptr) : Node(line) {
			this->name = name;
			this->type = type;
			this->expression = expression;
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
			expression->printChildren(indent + 1);
		}
	};

// class VoidNode : public Node {
// 	public:
// 		VoidNode() : Node() {}
// };

// TODO: This would probably make it easier to structure functions
// class StatementNode : public Node {
// 	public:

// };

// class IdentifierNode : Node {
// 	public:
// 		IdentifierNode()
// };

void lexer(std::string src);

// AST
ProgramNode *parser();

// Semantic Analysis
ProgramNode *sema(ProgramNode *ast);

class Instruction {
	public:
		virtual ~Instruction() = default;

		virtual void print(int indent) {}
};

std::string codegen(std::string fileName, std::vector<Instruction*> ir);

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

// TODO: need a derived class for lvalues ?


static std::string operandToStr(Operand operand)
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
			throw_error(1, "Operand has invalid kind. Hello??");
			exit(1);
	}
}


class BinaryInstr : public Instruction {
	public:
		Operand dest; // TODO: needs to be an lvalue, not just an operand. could simply be checked in sema?
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
};

// find the dest offset from Scope*
class DeclarationInstr : public Instruction {
	public:
		Operand dest;
		Operand src;
		std::string destName;

		DeclarationInstr(Operand dest, std::string destName, Operand src) {
			this->dest = dest;
			this->destName = destName;
			this->src = src;
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("Decl {} {} = {}\n", destName, operandToStr(dest), operandToStr(src));
		}
};


// Three Address Code IR
std::vector<Instruction*> taco(ProgramNode *program);


