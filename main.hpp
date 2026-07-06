#include <iostream>
#include <fstream>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

void throw_error(int code, std::string msg);
void throw_warn(int code, std::string msg);
void throw_invalid_identifier();
void throw_invalid_identifier_start();

// like a union
using Literal = std::variant<
	std::monostate,
	int,
	std::string,
	bool
>;

struct LiteralVisitor {
	std::string operator()(std::monostate v) const {
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
	SEMICOLON, PLUS, MINUS, MULT, ASSIGNMENT, OPEN_PARENTHESES, CLOSED_PARENTHESES, OPEN_SQUARE_BRACKET, CLOSED_SQUARE_BRACKET, OPEN_CURLY_BRACE, CLOSED_CURLY_BRACE,

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
			this->type = END_OF_FILE;
			this->lexeme = "";
			this->line = 0;
			this->column = 0;
			this->length = 0;
		}

		void print()
		{
			std::cout << "   Type: " << token_type_to_string(type) << std::endl;
			std::cout << " Lexeme: " << lexeme << std::endl;
			std::cout << "Literal: " << std::visit(LiteralVisitor{}, literal) << std::endl;
			std::cout << "   Line: " << line << std::endl;
			std::cout << " Column: " << column << std::endl;
			std::cout << " Length: " << length << "\n" << std::endl;
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

// AST parser
void parser();