#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <format>

#include <fmt/core.h>

void throw_error(int code, std::string msg);
void throw_error_line(int code, int line, std::string msg);
void throw_warn(int code, int line, std::string msg);
void throw_invalid_identifier(int line);
void throw_invalid_identifier_start(int line);
std::string readFile(std::string filename);

// like a union
using Literal = std::variant<
	std::monostate,
	int,
	std::string,
	bool
>;


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
	SEMICOLON, PLUS, MINUS, MULT, ASSIGNMENT, OPEN_PARENTHESES, CLOSED_PARENTHESES,
	OPEN_SQUARE_BRACKET, CLOSED_SQUARE_BRACKET, OPEN_CURLY_BRACE, CLOSED_CURLY_BRACE,
	COMMA,

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
		TokenType type;
		std::string lexeme; // exact source text
		Literal literal;
		int line;
		int column; // TODO: broken
		int length; // TODO: broken

		Token(TokenType type, int length, std::string lexeme, Literal literal, int line, int column) {
			this->type = type;
			this->lexeme = lexeme;
			this->literal = literal;
			this->line = line;
			this->column = column;
			this->length = length;
		}

		// only for 0-initialization
		Token() {
			this->type = END_OF_FILE; // TODO: switch to NULL
			this->lexeme = "";
			this->line = 0;
			this->column = 0;
			this->length = 0;
		}

		void print()
		{
			fmt::print("   Type: {} \n", token_type_to_string(type));
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
		Node *parent; // TODO: this is never set. Is this a mistake?

		Node(Token *token) : token(token) {} // TODO: learn initializer list syntax properly and use it in other places

		virtual void print(int indent) {
			printIndentLines(indent);
			fmt::print("{}\n", typeName());
		}

		// TODO: this is dumb. This design is strange.
		virtual void printChildren(int indent) {
			// assert(false);
		}

		virtual void killChildren() {

		}

		virtual std::string typeName() {
			return "Node";
		}

		// TODO: these shouldn't exist for a lot of nodes. Reconsider storing the Token pointer as well.
		// TODO: just store line or smth. Only what's necessary for ALL nodes.
		TokenType getType() const {
			return this->token->type;
		}
		Literal getLiteral() const {
			return this->token->literal;
		}
		int getLine() const {
			return this->token->line;
		}
		std::string getLexeme() const {
			return this->token->lexeme;
		}
		
		private:
			Token *token;
};


class ProgramNode : public Node {
	public:
		std::vector<Node*> children;

		ProgramNode(Token *token) : Node(token) {}

		void printChildren(int indent) override {
			print(indent);
			for (Node *i : children) {
				i->printChildren(indent + 1);
			}
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("{}\n", "ProgramNode");
		}
};


class BinaryOpNode : public Node {
	public:
		Node *left;
		Node *right;
		TokenType op;

		BinaryOpNode(Token *token) : Node(token) {
			this->op = token->type;
		}

		void printChildren(int indent) override {
			print(indent);
			if (left) left->printChildren(indent + 1);
			if (right) right->printChildren(indent + 1);
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("{} {}\n", typeName(), Token::token_type_to_string(op));
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


class IntegerNode : public Node {
	public:
		int value;

		IntegerNode(Token *token) : Node(token) {
			this->value = std::get<int>(token->literal);
		}

		std::string typeName() override {
			return "IntegerNode";
		}

		void printChildren(int indent) {
			print(indent);
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("IntegerNode {}\n", value);
		}
};


class Parameter {
	public:
		TokenType type;
		std::string identifier;

		Parameter(TokenType type, std::string identifier) {
			this->type = type;
			this->identifier = identifier;
		}

		void print(int indent) {
			fmt::print("{} {}, ", Token::token_type_to_string(type), identifier);
		}
};


class FuncDefNode : public Node {
	public:
		std::string identifier;
		TokenType returnType;
		std::vector<Parameter> paramList;

		// must contain at least one return for each control path if returnType isn't void.
		// should only contain statements?
		std::vector<Node*> body;

		FuncDefNode(Token *identifierToken, TokenType returnType) : Node(identifierToken) {
			this->identifier = identifierToken->lexeme;
			this->returnType = returnType;
		}

		void print(int indent) override {
			printIndentLines(indent);
			fmt::print("{} {} {} (", typeName(), Token::token_type_to_string(returnType), identifier);
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
		Node *expression; // can be almost anything, including nothing.

		ReturnNode(Token *token) : Node(token) {

		}

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

void codegen(std::string fileName, ProgramNode *program);
