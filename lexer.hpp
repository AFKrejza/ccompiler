#pragma once

#include <fmt/core.h>
#include <unordered_map>
#include <variant>

#include "error.hpp"
#include "operators.hpp"

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

class Token {
	public:
		enum en {
			SEMICOLON,
			PLUS,
			MINUS,
			ASSIGNMENT,
			OPEN_PARENTHESES,
			CLOSED_PARENTHESES,
			OPEN_SQUARE_BRACKET,
			CLOSED_SQUARE_BRACKET, 
			OPEN_CURLY_BRACE, 
			CLOSED_CURLY_BRACE,
			COMMA, 
			ASTERISK,
			GREATER_THAN, 
			LESS_THAN, 
			EQUAL_TO, 
			GREATER_OR_EQUAL, 
			LESSER_OR_EQUAL,
			NOT_EQUAL,
			LOGICAL_NOT, 
			LOGICAL_AND, 
			LOGICAL_OR,
			BITWISE_NOT, 
			BITWISE_AND, 
			BITWISE_OR,
			IDENTIFIER, 
			CHAR, 
			SHORT, 
			INT, 
			LONG, 
			STRING_LITERAL, 
			INTEGER,
			IF,
			ELSE,
			RETURN,
			WHILE,
			DO,
			FOR,
			BREAK,
			CONTINUE,
			END_OF_FILE,
			INC, DEC,
			PREINC,
			POSTINC,
			PREDEC,
			POSTDEC,
			EXTERN
		};



		Token::en type;
		std::string lexeme; // exact source text
		Literal literal;
		int line;
		int column; // TODO: broken
		int length; // TODO: broken

		Token(Token::en tokenType,
			  int length,
			  std::string lexeme,
			  Literal literal,
			  int line,
			  int column) : type(tokenType),
			  				lexeme(lexeme),
							literal(literal),
							line(line),
			  				column(column),
							length(length) {}

		// only for 0-initialization
		Token() : type(Token::en::END_OF_FILE), lexeme(""), line(0), column(0), length(0) {}

		void print()
		{
			fmt::print("   Token::en: {} \n", Token::toStr(this->type));
			fmt::print(" Lexeme: {}\n", lexeme);
			fmt::print("Literal: {}\n", std::visit(LiteralPrintVisitor{}, literal));
			fmt::print("   Line: {}\n", line);
			// fmt::print(" Column: {}\n", column);
			// fmt::print(" Length: {}\n", length);
		}
		
		// contains all symbols and keywords
		static const std::unordered_map<Token::en, std::string> printmap;

		static std::string toStr(Token::en type)
		{
			auto it = printmap.find(type);
			if (it != printmap.end()) {
				return it->second;
			}
			throw_error(3, "Couldn't find token to be printed. ???");
			return "";
		}

		static BinOp::en toBinaryOp(Token::en op) {
			switch (op)
			{
				case Token::en::PLUS:
					return BinOp::en::ADD;
				case Token::en::MINUS:
					return BinOp::en::SUB;
				case Token::en::ASTERISK:
					return BinOp::en::MUL;
				case Token::en::LOGICAL_AND:
					return BinOp::en::LOGICAL_AND;
				case Token::en::LOGICAL_OR:
					return BinOp::en::LOGICAL_OR;
				case Token::en::EQUAL_TO:
					return BinOp::en::EQUAL_TO;
				case Token::en::NOT_EQUAL:
					return BinOp::en::NOT_EQUAL;
				case Token::en::LESS_THAN:
					return BinOp::en::LESS_THAN;
				case Token::en::LESSER_OR_EQUAL:
					return BinOp::en::LESSER_OR_EQUAL;
				case Token::en::GREATER_THAN:
					return BinOp::en::GREATER_THAN;
				case Token::en::GREATER_OR_EQUAL:
					return BinOp::en::GREATER_OR_EQUAL;
				default:
					throw_error(1, fmt::format(
						"Invalid Token::en to BinOp::en conversion: type {}", Token::toStr(op)));
					exit(1);
			}
		}

		static UnOp::en toUnaryOp(Token::en type) {
			UnOp::en op;
			switch (type)
			{
				case Token::en::MINUS:
					op = UnOp::en::NEGATE;
					break;
				case Token::en::LOGICAL_NOT:
					op = UnOp::en::LOGICAL_NOT;
					break;
				case Token::en::PREINC:
					op = UnOp::en::PREINC;
					break;
				case Token::en::POSTINC:
					op = UnOp::en::POSTINC;
					break;
				case Token::en::PREDEC:
					op = UnOp::en::PREDEC;
					break;
				case Token::en::POSTDEC:
					op = UnOp::en::POSTDEC;
					break;
				default:
					throw_error(1, fmt::format("Token::toUnaryOp: no rule for token {}",
						Token::toStr(type)));
					exit(1);
			}
			return op;
		}
};
